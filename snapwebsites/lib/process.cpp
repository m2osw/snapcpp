// Snap Websites Server -- C++ object to run advance processes
// Copyright (C) 2011-2013  Made to Order Software Corp.
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

namespace snap
{


/** \class process
 * \brief A process class to run a process and get information about the results.
 *
 * This class is used to run processes. Especially, it can run with in and
 * out capabilities (i.e. piping) although this is generally not recommanded
 * because piping can block (if you do not send enough data, or do not read
 * enough data, then the pipes can get stuck.) We use a thread to read the
 * results. We do not currently expect that the use of this class will require
 * the input read to be necessary to know what needs to be written (i.e. in
 * most cases all we want is to convert a file [input] from one format to
 * another [output] avoiding reading/writing on disk.)
 */

/** \brief Initialize the process object.
 *
 * This function saves the name of the process. The name is generally a
 * static string and it is used to distinguish between processes when
 * managing several at once. The function makes a copy of the name.
 *
 * \param[in] name  The name of the process.
 */
process::process(const QString& name)
    : f_name(name)
    //, f_input("") -- auto-init
    //, f_output("") -- auto-init
    //, f_forced_environment("") -- auto-init
    //, f_output_callback(NULL) -- auto-init
{
}

/** \brief Retrieve the name of this process object.
 *
 * This process object is given a name on creation. In most cases this is
 * a static name that is used to determine which process is which.
 *
 * \return The name of the process.
 */
const QString& process::get_name() const
{
    return f_name;
}

/** \brief Set the management mode.
 *
 * This function defines the mode that the process is going to use when
 * running. It cannot be changed once the process is started (the run()
 * function was called.)
 *
 * The available modes are:
 *
 * \li PROCESS_MODE_COMMAND
 *
 * Run a simple command (i.e. very much like system() would.)
 *
 * \li PROCESS_MODE_INPUT
 *
 * Run a process that wants some input. We write data to its input. It
 * does not generate output (i.e. sendmail).
 *
 * \li PROCESS_MODE_OUTPUT
 *
 * Run a process that generates output. We read the output.
 *
 * \li PROCESS_MODE_INOUT
 *
 * Run the process in a way so we can write input to it, and read its
 * output from it. This is one of the most useful mode. This mode does
 * not give you any interaction capabilities (i.e. what comes in the
 * output cannot be used to intervene with what is sent to the input.)
 *
 * This is extermely useful to run filter commands (i.e. html2text).
 *
 * \li PROCESS_MODE_INOUT_INTERACTIVE
 *
 * Run the process interactively, meaning that its output is going to be
 * read and interpreted to determine what the input is going to be. This
 * is a very complicated mode and it should be avoided if possible because
 * it is not unlikely that the process will end up blocking. To be on the
 * safe side, look into whether it would be possible to transform that
 * process in a server and connect to it instead.
 *
 * Otherwise this mode is similar to the in/out mode only the output is
 * used to know to further feed in the input.
 *
 * \param[in] mode  The mode of the process.
 */
void process::set_mode(mode_t mode)
{
    f_mode = mode;
}

/** \brief Set how the environment variables are defined in the process.
 *
 * By default all the environment variables from the current process are
 * passed to the child process. If the child process is not 100% trustworthy,
 * however, it may be preferable to only pass a specific set of environment
 * variable (as added by the add_environment() function) to the child process.
 * This function, when called with true (the default) does just that.
 *
 * \param[in] forced  Whether the environment will be forced.
 */
void process::set_forced_environment(bool forced)
{
    f_forced_environment = forced;
}

/** \brief Define the command to run.
 *
 * The command name may be a full path or just the command filename.
 * (i.e. the execvp() function makes use of the PATH variable to find
 * the command on disk unless the \p command parameter includes a
 * slash character.)
 *
 * If the process cannot be found an error is generated at the time you
 * call the run() function.
 *
 * \param[in] command  The command to start the new process.
 */
void process::set_command(const QString& command)
{
    f_command = command;
}

/** \brief Add an argument to the command line.
 *
 * This function adds one individual arguement to the command line.
 * You have to add all the arguments in the right order.
 *
 * \param[in] arg  The argument to be added.
 */
void process::add_argument(const QString& arg)
{
    f_arguments.push_back(arg);
}

/** \brief Add an environment to the command line.
 *
 * This function adds a new environment variable for the child process to
 * use. In most cases this function doesn't get used.
 *
 * By default all the parent process (this current process) environment
 * variables are passed down to the child process. To avoid this behavior,
 * call the set_forced_environment() function before the run() function.
 *
 * An environment variable is defined as a name, an equal sign, and a value
 * as in:
 *
 * \code
 * add_environ("HOME=/home/snap");
 * \endcode
 *
 * \param[in] env  An environment variable.
 */
void process::add_environ(const QString& env)
{
    f_environment.push_back(env);
}

/** \brief Run the process and return once done.
 *
 * This function creates all the necessary things that the process requires
 * and run the command and then return the result.
 */
int process::run()
{
    // if the user imposes environment restrictions we cannot use system()
    // or popen(). In that case just use the more complex case anyway.
    if(!f_forced_environment && f_environment.isEmpty())
    {
        switch(f_mode)
        {
        case PROCESS_MODE_COMMAND:
            {
                QString command(f_command);
                if(!f_arguments.isEmpty())
                {
                    command += f_arguments.join(" ");
                }
                return system(command.toUtf8().data());
            }

        case PROCESS_MODE_INPUT:
            {
                QString command(f_command);
                if(!f_arguments.isEmpty())
                {
                    command += f_arguments.join(" ");
                }
                f = popen(command.toUtf8().data(), "w");
                if(f == NULL)
                {
                    return -1;
                }
                QByteArray data(f_input.toUtf8());
                if(write(data.data(), data.size(), 1, f) != 1)
                {
                    pclose(f);
                    return -1;
                }
                return pclose(f);
            }

        case PROCESS_MODE_OUTPUT:
            {
                QString command(f_command);
                if(!f_arguments.isEmpty())
                {
                    command += f_arguments.join(" ");
                }
                f = popen(command.toUtf8().data(), "r");
                if(f == NULL)
                {
                    return -1;
                }
                QByteArray data;
                while(!feof(f) && !ferror(f))
                {
                    char buf[4096];
                    size_t l(fread(buf, 1, sizeof(buf), f))
                    data.append(buf, static_cast<int>(l));
                }
                if(ferror(f))
                {
                    pclose(f);
                    return -1;
                }
                return pclose(f);
            }

        default:
            // In/Out modes require the more complex case
            break;

        }
    }

    // in this case we want to create a pipe(), fork(), execvp() the
    // command and have a thread to handle the output separately
    // from the input
}

/** \brief The input to be sent to stdin.
 *
 * Add the input data to be written to the stdin pipe. Note that the input
 * cannot be modified once the run() command was called unless the mode
 * is PROCESS_MODE_INOUT_INTERACTIVE.
 *
 * Note that in case the mode is interactive, calling this function adds
 * more data to the input. It does not erase what was added before.
 * The thread may eat some of the input in which case it gets removed
 * from the internal variable.
 *
 * \param[in] input  The input of the process (stdin).
 */
void process::set_input(const QString& input)
{
    // this is additive!
    f_input += input;
}

/** \brief Read the output of the command.
 *
 * This function reads the output of the process.
 *
 * \param[in] reset  Whether the output so far should be cleared.
 */
QString process::get_output(bool reset) const
{
    QString output(f_output);
    if(reset)
    {
        f_output = "";
    }
    return output;
}

/** \brief Setup a callback to receive the output as it comes in.
 *
 * This function is used to setup a callback. That callback is expected
 * to be called each time data arrives in our input pipe (i.e. stdout
 * or the output pipe of the child process.)
 *
 * \param[in] callback  The callback class that is called on output arrival.
 */
void process::set_output_callback(process_output_callback *callback)
{
    f_output_callback = callback;
}

} // namespace snap

// vim: ts=4 sw=4 et
