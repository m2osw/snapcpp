
# Compile Driver with C++ Snap

In order to get this driver to compile, you have to install the following:

    sudo apt-get install libuv1-snap-dev

The libuv1-snap-dev is our own version of the libuv1 library that we compile
so we get the correct version of libuv1 on Ubuntu 16.04. It may not be required
in newer versions, although so far what I've seen in 18.04 we still need
our own version.

