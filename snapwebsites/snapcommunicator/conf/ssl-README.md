
SSL Repository
==============

This directory is used to create various types of key pairs.

This directory is created when you install the `snapcommunicator` package.
It also automatically generates a key and certificate so snapcommunicator
will actually work as expected and securely.

You may remove the encryption between computers running `snapcommunicator`
by editing the `/etc/snapwebsites/snapwebsites.d/snapcommunicator.conf`
file and setting the `ssl_certificate` and `ssl_private_key` parameters
to empty (or comment them out.)


_This file is part of the [snapcpp project](http://snapwebsites.org/)._
