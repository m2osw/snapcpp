# Introduction

The libcasswrapper library is a light wrapper around the Cassandra CPP Driver.

The library is implemented with C++ in mind. The Cassandra CPP Driver sadly is
really NOT C++, but straight C. All of the resources handles returned are not
managed with constructors/destructors to deal with object lifetime. The
developer is responsible to make sure resources are properly destroyed once
he/she is finished with them. This opens up the door to memory and resource
leaks.

In order to avoid those pitfalls, we've written a thin C++ "wrapper" around the
driver API itself. The resource handles are preserved in std smart pointers,
which automatically manage the lifetimes and prevent resource leaks.  This makes
the library a lot easier to use in an object oriented situation, consistent with
the tried and true design principle of RAII.


# Compiling

In order to compile the library, you need the cmake extensions from
the Snap! C++ environment. We provide that file in GitHub as
a tarball. Just extract along with the library.

You also need to have the Cassandra CPP Driver C-API in order to communicate with the
library. The setup of this driver is outside the scope of this document, but if
you run under Centos, Ubuntu or Windows, there are binary drivers here:
http://downloads.datastax.com/cpp-driver/

See documentation here:
https://github.com/datastax/cpp-driver

libcasswrapper uses Doxygen markup to provide developer documentation. If you
wish extract the docs, you have to have Doxygen installed before you run the
cmake command.

Once you extracted the .tar.gz files you do something like this:

    mkdir BUILD
    cd BUILD
    cmake ../libcasswrapper
    make
    make install


# What's next?

Although we have wrapped most of the core functionality of the cpp driver, there
is still some API which is not wrapped. As time goes on, we will add more and
more support. Still, you should have enough to handle the basic tasks of
interfacing with the Cassandra database.

