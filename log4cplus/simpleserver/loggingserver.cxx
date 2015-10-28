// Module:  LOG4CPLUS
// File:    loggingserver.cxx
// Created: 5/2003
// Author:  Tad E. Smith
//
//
// Copyright 2003-2015 Tad E. Smith
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <cstdlib>
#include <iostream>
#include <log4cplus/configurator.h>
#include <log4cplus/socketappender.h>
#include <log4cplus/version.h>
#include <log4cplus/helpers/socket.h>
#include <log4cplus/thread/threads.h>
#include <log4cplus/spi/loggingevent.h>


namespace loggingserver
{

log4cplus::thread::Mutex access_mutex;

log4cplus::helpers::ServerSocket * serverSocket;

bool quit = false;


class ClientThread : public log4cplus::thread::AbstractThread
{
public:
    ClientThread(log4cplus::helpers::Socket accepted_clientsock)
        : clientsock(accepted_clientsock)
    {
        //std::cout << "Received a client connection!!!!" << std::endl;
    }

    ~ClientThread()
    {
        //std::cout << "Client connection closed." << std::endl;
    }

    virtual void run();

    void shutdownSocket()
    {
        clientsock.shutdown();
    }

private:
    log4cplus::helpers::Socket clientsock;
};


class ThreadManager
{
public:
    ThreadManager()
        : thread_ptr(NULL)
    {
    }

    // transfer rhs to this; we have to have a copy operator for the
    // ThreadManager to be used in an std::vector<>
    ThreadManager(ThreadManager const & rhs)
        : thread_ptr(const_cast<ThreadManager &>(rhs).thread_ptr.release())
    {
    }

    ~ThreadManager()
    {
        if(thread_ptr.get())
        {
            thread_ptr->shutdownSocket();
            thread_ptr->join();
        }
    }

    void createThread(log4cplus::helpers::Socket accepted_clientsock)
    {
        assert(thread_ptr.get() == NULL);
        thread_ptr.reset(new loggingserver::ClientThread(accepted_clientsock));
        thread_ptr->start();
    }

    bool isRunning() const
    {
        if(thread_ptr.get()) {
            if(thread_ptr->isRunning()) {
                return true;
            }
            thread_ptr->join();
            const_cast<ThreadManager *>(this)->thread_ptr.reset();
        }
        return false;
    }

private:
    std::auto_ptr<ClientThread> thread_ptr;
};


class ThreadPool
{
public:
    ThreadPool()
    {
        threads.reserve(100);
    }

    void addThread(log4cplus::helpers::Socket accepted_clientsock)
    {
        if(accepted_clientsock.isOpen())
        {
            std::vector<ThreadManager>::iterator it(std::find_if(threads.begin(), threads.end(), threadStopped));
            if(it == threads.end())
            {
                // create a new thread manager, since all existing (if any) are
                // currently busy
                ThreadManager manager;
                threads.push_back(manager);
                it = threads.end() - 1;
            }

            // create the thread and start it
            it->createThread(accepted_clientsock);
        }
    }

private:
    static bool threadStopped(ThreadManager const & manager)
    {
        return !manager.isRunning();
    }

    std::vector<ThreadManager> threads;
};


} // namespace loggingserver




int
main(int argc, char** argv)
{
    if(argc == 4) {
        if(std::string(argv[1]) == "--stop") {
            // send a STOP command to the server so it stops ASAP
            // (useful when shutting down your computer)
            std::string const address(argv[2]);
            int const port = std::atoi(argv[3]);
            log4cplus::helpers::Socket socket(address, port);
            if (!socket.isOpen()) {
                std::cerr << "error: could not open connection to server, maybe the server at "
                    << address << ":" << port << " is already down." << std::endl;
                return 2;
            }
            log4cplus::helpers::SocketBuffer buffer(sizeof(unsigned int));
            unsigned int shutdown(static_cast<unsigned int>(-1));
            buffer.appendInt(shutdown);        // -1 is the shutdown
            if(!socket.write(buffer)) {
                std::cerr << "error: could not write to the server.";
                return 2;
            }
            return 0;
        }
        if(std::string(argv[1]) == "--version") {
            // send a VERSION command to the server to get its version
            std::string const address(argv[2]);
            int const port = std::atoi(argv[3]);
            log4cplus::helpers::Socket socket(address, port);
            if (!socket.isOpen()) {
                std::cerr << "error: could not open connection to server, maybe the server at "
                    << address << ":" << port << " is down." << std::endl;
                return 2;
            }
            log4cplus::helpers::SocketBuffer buffer(sizeof(unsigned int));
            unsigned int version_request(static_cast<unsigned int>(-2));
            buffer.appendInt(version_request);        // -2 is the version request
            if(!socket.write(buffer)) {
                std::cerr << "error: could not write to the server."
                          << std::endl;
                return 2;
            }
            // read string size
            log4cplus::helpers::SocketBuffer versionSizeBuffer(sizeof(unsigned int));
            if(!socket.read(versionSizeBuffer)) {
                std::cerr << "error: could not read from the server."
                          << std::endl;
                return 2;
            }
            unsigned int const versionSize(versionSizeBuffer.readInt());
            log4cplus::helpers::SocketBuffer version(versionSize);
            if(!socket.read(version)) {
                std::cerr << "error: could not read from the server."
                          << std::endl;
                return 2;
            }
            std::cout << std::string(version.getBuffer(), versionSize) << std::endl;
            return 0;
        }
    }

    if(argc != 3 || argv[1][0] == '-') {
        std::cout << "Usage: loggingserver port config_file" << std::endl;
        std::cout << "   or: loggingserver --stop address port" << std::endl;
        std::cout << "   or: loggingserver --version address port" << std::endl;
        return 1;
    }
    int const port = std::atoi(argv[1]);
    const log4cplus::tstring configFile = LOG4CPLUS_C_STR_TO_TSTRING(argv[2]);

    log4cplus::PropertyConfigurator config(configFile);
    config.configure();

    loggingserver::serverSocket = new log4cplus::helpers::ServerSocket(port);
    if (!loggingserver::serverSocket->isOpen()) {
        std::cerr << "Could not open server socket, maybe port "
            << port << " is already in use." << std::endl;
        return 2;
    }

    {
        loggingserver::ThreadPool threadpool;
        for(;;) {
            {
                // quit must be guarded
                log4cplus::thread::MutexGuard guard (loggingserver::access_mutex);
                if(loggingserver::quit) {
                    break;
                }
            }
            threadpool.addThread(loggingserver::serverSocket->accept());
        }
    }

    delete loggingserver::serverSocket;

    return 0;
}


////////////////////////////////////////////////////////////////////////////////
// loggingserver::ClientThread implementation
////////////////////////////////////////////////////////////////////////////////


void
loggingserver::ClientThread::run()
{
    while(1) {
        {
            // quit accesses must be guarded
            log4cplus::thread::MutexGuard guard (loggingserver::access_mutex);
            if(loggingserver::quit) {
                return;
            }
        }
        if(!clientsock.isOpen()) {
            return;
        }
        log4cplus::helpers::SocketBuffer msgSizeBuffer(sizeof(unsigned int));
        if(!clientsock.read(msgSizeBuffer)) {
            return;
        }

        unsigned int const msgSize = msgSizeBuffer.readInt();
        if(msgSize == static_cast<unsigned int>(-1)) {
            // client requested a quit from the whole server
            // WARNING: this is absolutely not secure...
            {
                // quit accesses must be guarded
                log4cplus::thread::MutexGuard guard (loggingserver::access_mutex);
                loggingserver::quit = true;
            }
            serverSocket->interruptAccept();
            return;
        }

        if(msgSize == static_cast<unsigned int>(-2)) {
            // client requested the server version as a null terminated ASCII string
            // we return version of the library to which the server is linked
            // (rather than the version of the server itself)
            std::string const version(log4cplus::versionStr);
            log4cplus::helpers::SocketBuffer versionSizeBuffer(sizeof(unsigned int));
            unsigned int const versionSize(version.size());
            versionSizeBuffer.appendInt(versionSize);
            clientsock.write(versionSizeBuffer);
            clientsock.write(version);
        }
        else {
            // client sent a log message

            log4cplus::helpers::SocketBuffer buffer(msgSize);
            if(!clientsock.read(buffer)) {
                return;
            }

            log4cplus::spi::InternalLoggingEvent event
                = log4cplus::helpers::readFromBuffer(buffer);
            log4cplus::Logger logger
                = log4cplus::Logger::getInstance(event.getLoggerName());
            logger.callAppenders(event);
        }
    }
}

// vim: ts=4 sw=4 et
