# Introduction

The libQtCassandra library is a Qt extension to support accessing
Cassandra clusters.

It is implemented with C++ in mind rather than the way the Cassandra
cluster can be accessed by default. We offer objects that are neatly
organizied in a tree: context, table, row, cell, value. And in many
cases you can access the data using the C++ array syntax:

    value = my_table["table-name"]["row-name"][QString("cell-name")];

You can find additional information about that in the [reference
manual](http://snapwebsites.org/project/references) of the library.
The main page is found in file src/QCassandra.cpp.


# Compiling

In order to compile the library, you need the cmake extensions from
the Snap! C++ environment. We provide that file in SourceForge as
a tarball. Just extract along the library.

You also need to have the thrift interface to communicate with the
library. I have a chapter below in that regard. In order to generate
the necessary connectors, you have to compile thrift.

If you want to get the documentation extracted, you have to have
Doxygen installed before you run the cmake command.

Once you extracted the .tar.gz files you do something like this:

    mkdir BUILD
    cd BUILD
    cmake ../libQtCassandra
    make
    make install

The cmake step may require some additional options depending on
where you have thrift, etc.


# What's Next?

We are planning in offering two main things next:

1. We need to switch to CQL. I looked into it and at our level we
   will not have to convert commands into strings to then have
   them interpreted on the other side. Instead, commands are numbers
   saved in a buffer and we send that buffer over. This will
   therefore not be much different from what we currently support.
   For sure, for clients it will be transparent.

2. The current implementation only offers a direct connection to
   one node. We want to offer a driver that connects to the
   cluster instead. This means the driver will be connected to
   a few nodes and send commands to any one of them depending on
   how busy they say they are. This will also allow you to stop
   a Cassandra node to do some work on it without losing connectivity
   for your front end application. We will offer a tool to tell
   each driver that a node is to go down. The tool will let us
   know how many of the drivers are still connected until it
   reaches zero and thus let us know that we can stop the node.


# Thrift (communication between libQtCassandra and Cassandra)

The thrift-gencpp-cassandra folder is generated using thrift.

It is a good idea to regenerate the folder if your version is
different than what is offered in the libQtCassandra package.

The command line is:

    thrift -gen cpp cassandra.thrift

The cassandra.thrift file comes with Cassandra and is found in
the interface folder. (apache-cassandra-0.8.0/interface)

I had no problems with thrift 0.7.0 that I can remember. (It was
a bit difficult to find out all the headers and proper order to
include them, but no major problems.)

With version 0.8.0, I had to fix the CassandraProcessor interface
because it made use of

    apache::thrift::protocol

instead of

    ::apache::thrift::protocol

(:: missing at the start.) Also, the netinet/in.h header was not
included. I fixed that by including that header in
src/QCassandraPrivate.h before including thrift.
Note that for the two templates you will need a space:

    ... boost::shared_ptr< ::apache::...> ...

since "<:" is viewed as a special character by g++.

