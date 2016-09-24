
# Introduction

Snap! C++ is a website backend project mainly written in C++ (there is a
little bit of C and it uses many C libraries...)

The project includes many libraries, many daemons/servers, it supports
plugins for third party extensions, and it uses Cassandra as its database
of choice (especially, a NoSQL, although Cassandra now has CQL...)

Click on this link to go to the [official home page](http://snapwebsites.org/)
for the development of Snap! C++. The website includes documentation about
the core plugins and their more or less current status. It also includes
many pages about the entire environment. How things work, etc.


# Licenses

Each project has an Open Source license, however, it changes slightly
depending on the project. Most of the projects use the GNU GPL or GNU LGPL
version 2. You will also find public domain code and MIT or FreeBSD
licenses.


# Getting Started

The whole environment is based on cmake and also matches pbuilder so we
can create Ubuntu packages with ease (cmake even makes use of the control
files to generate the inter project dependencies!) We do not yet release
the Ubuntu packages publicly. We have a launchpad.net environment, but
unfortunately, it is too complicated to use when you manage a large
project that includes many sub-projects.

To compile everything you have one dependency on the C++ Cassandra Driver.
This can be obtained from the following PPA:

    sudo add-apt-repository ppa:tcpcloud/extra
    sudo apt-get update
    sudo apt-get install cassandra-cpp-driver-dev

To get started quickly, create a directory, clone the source, then run
the build-snap script. (You may want to check it out once first to make
sure it is satisfactory to you.) By default it build snaps in Debug mode.

The command `snap-ubuntu-packages` installs many packages that the build
requires. This is done automatically in the build system using the control
file information. For a developer, that's not automatic. So the easiest
is to run that command. Although we try to keep it up to date, if something
is missing, cmake should tell you. Worst case scenario, the compiler stops
with an error. We recommend the --optional packages so you get full
documentation and some extras for tests.

    apt-get install git
    mkdir snapwebsites
    cd snapwebsites
    git clone https://github.com/m2osw/snapcpp.git snapcpp
    snapcpp/bin/snap-ubuntu-packages --optional
    snapcpp/bin/snap-build

After a while, you'll have all the built objects under a BUILD directory
in your snapwebsites directory. The distribution being under the BUILD/dist
directory (warning: executables under the distribuation will be stripped
from their `RPATH` which means you cannot run them without some magic;
namely changing your `PATH` and `LD_LIBRARY_PATH`)

We support a few variables, although in most cases you will not have to
setup anything to get started. You can find the main variables in our
bin/build-snap script which you can use to create a developer environment.
There is an example of what you can do to generate the build environment.
The build type can either be Debug or Release.

    cmake \
        -DCMAKE_BUILD_TYPE=Release \
        -DDTD_SOURCE_PATH:PATH="`pwd`/BUILD/dist/share/snapwebsites/dtd" \
        -DXSD_SOURCE_PATH:PATH="`pwd`/BUILD/dist/share/snapwebsites/xsd" \
        ..


## Setting up a development environment

TODO: need to complete this one...

Once you compiled everything, you will have an environment that can be
used to run Snap! Only, it won't work as is. You first have to setup a
certain number of parts: /etc/snapwebsites, /var/lib/snapwebistes,
/var/log/snapwebsites, /var/cache/snapwebsites, and make sure
snapmanagerdaemon starts as root.

Although these things do get installed automatically when you install
the Snap! packages, doing so with the packages will install these files
as `snapwebsites:snapwebsites` (user/group) and that is not practical
when trying to debug Snap!, since you'll want to quickly edit these
files.


## Creating packages

To build Ubuntu packages, you want to run the following commands,
althouh this is currently incomplete! We will try to ameliorate that
info with time. It currently takes 1h30 to rebuild everything as packages.

    # get some extra development tools if you don't have them yet
    apt-get install ubuntu-dev-tools eatmydata debhelper

    # create the build environment (a chroot env.)
    pbuilder-dist `lsb_release --codename --short` create

    # Prepare source packages
    make debuild

    # Create packges
    make pbuilder


## Linux

At this point we only have a Linux version of the project. We have no
plans in updating the project to work on a different platform, although
changes are welcome if you would like to do so. However, as it stands,
the project is not yet considered complete, so it would be quite premature
to attempt to convert it.

Note that the software makes heavy use of the `fork()` instruction. That
means it will be prohibitive to use under MS-Windows unless, as I think I
heard, they now do support the `fork()` functionality.


# Bugs

Submit bug reports and patches on
[github](https://github.com/m2osw/snapcpp/issues).


_This file is part of the [snapcpp project](http://snapwebsites.org/)._
