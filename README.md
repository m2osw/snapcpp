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
depending on the project. Most of the projects use the GNU GPL or GNU LGPL.


# Installation

The whole environment is based on cmake and also matches pbuilder so we
can create Ubuntu packages with ease. We do not yet release the Ubuntu
packages publicly. We have a launchpad.net environment, but unfortunately,
it is complicated to use when you manage a large project that includes
many sub-projects.

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


## Linux

At this point we only have a Linux version of the project. We have no
plans in updating the project to work on a different platform, although
changes are welcome if you would like to do so. However, as it stands,
the project is not yet considered complete, so it would be quite premature
to attempt to convert it.

Note that the software makes heavy use of the fork() instruction. That means
it will be prohibitive to use under MS-Windows unless, as I think I heard,
they now do support the fork() functionality.


# Bugs

Submit bug reports and patches on
[github](https://github.com/m2osw/snapcpp/issues)
