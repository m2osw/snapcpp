
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


# Details About Snap! Websites

## Cluster Requirement

Please make sure to understand that Snap! Websites are not for a one
quick computer install a la Wordpress or Drupal. Snap! Websites only
runs well on a cluster. It is built to run any number of websites
within one cluster. Making it easy to share resources of large powerful
websites with tiny websites. Both benefit from the other!

Your cluster should have at least:

* 4 frontend computers to run Apache2, snap.cgi and snapserver (1Gb)
* 4 Cassandra nodes (at least 2 CPU and 4Gb of RAM)
* 1 snapbackend computer (for the CRON like tasks) (1Gb)
* 1 other services (mail, DNS, etc.) (1Gb)

Less than those 10 computers and you are likely to have some slowness
unless you have a monster computer with 14 processors, really fast
hard drives (i.e. SSD), and a lot of RAM (32Gb+). But even that won't
work right because you want separate computers for the Cassandra nodes.
Otherwise loss of data is very high (only the Cassandra cluster retains
website data, other computers may have various caches, but they do not
retain the official website data.)

## Cassandra Requirement

The reason for having at least 4 Cassandra nodes is because you want
a minimum of 3 as the replication factor and with only 3 computers,
you cannot do an upgrade without stopping everything. With 4 computers,
you can stop one Cassandra node, upgrade that computer, restart the
node. Proceed to the next node.

## Horizontal Growth

Note that the number of frontend computers, middle-end if you separate
the `snapserver` entries, and the Cassandra nodes can be much larger.
At this time we think that the current implementation can support up
to 300 computers in all, without too much trouble. In other words,
the system is built to grow horizontally. If all your Cassandra nodes
are always very busy (75%+) or all your frontend computers are always
very busy, then you can just add more and the load will automatically
be shared between the additional computers. (It is not currently
dynamic, but we may at some point offer such a capability too!)

## Backend Growth, also horizontal

Similarly, some of the backends can be duplicated to share the load,
although there are some limits on this one: one backend process can
run against one website; another backend instance on another computer
can run against another website. So in other words, we do not allow
multiple backend run against the same website in parallel.

## Developement and Testing

To test or as a developer, you can test on a single machine, but of
course that won't prove the cluster mechanisms work as expected. For
such a test, you should at least have 8Gb or have another machine to run
a Cassandra node.

In other words, you do not need to have a cluster to work on Snap! Websites
as a developer. You can even have a single Cassandra node. It will be
somwhat slower, but it is bearable. Of course, as a developer with a
single node, you may lose the data in that node, but I would imagine it
would only be test data...

## A Snap! Websites Cluster

The following images shows you an installation example. The cloud represents
clients accessing your Snap! environment. The Apache + snap.cgi represents
access points (you can have as many as you want to load balance the incoming
hits). All the other computers can be 100% private (i.e. no direct access to
the Internet). The snapserver computers represent the frontend of Snap!
(middle-end as far as the cluster is concerned.) At some point snap.cgi
will be smart enough to auto-load balance your stack, right now we just use
a simple round robin DNS definition. The Cassandra computers each run one
node. Those must have at least 4Gb of RAM in a valid live system. The
snapbackend run CRON like tasks. However, those are dynamic and receive a
PING message from the snapserver whenever a client makes a change (on a
POST). So really it does work in the background but instantly once data to
crunch is available.

![Snap! Websites Cluster Setup](snapwebsites/libsnapwebsites/doc/snapwebsites-cluster-setup.png)

## The Snap! Websites Stack

We have
[another picture and more details on this page](http://snapwebsites.org/implementation/snap-websites-processes)
showing how the processes are linked together. It does not show all
the details, such as the fact that `snapcommunicator` is used as the
inter-process and inter-computer hub (i.e. `snapserver` sends the
`PING` message to `snapcommunicator`, which knows where the `snapbackends`
are and thus forwards the message to the correct computer automatically.)

We see that a connection has to first be accepted by the firewall which
is dynamically updated by our anti-hacker/spam processes. If it does go
through, it gets sent to Apache2. If Apache2 is happy (you may put all sorts
of protections in Apache2 such as `mod_security`), then it loads `snap.cgi`
and runs it with a set of variables representing the header we just
received and a packet with POST data if any was attached.

At that point, the snap.cgi choose a snapserver that's not too loaded, and
sends the access data to it and wait for the reply which is then sent to
Apache2 and finally back to the client.

The snapserver will access the database, Cassandra, through the
`snapdbproxy` daemon (which allows us to have the system go really fast
because we connect to it on localhost instead of directly to Cassandra,
which can be much slower, especially if through an SSL connection. We also
gather the meta data only once and cache it.)


# Getting Started

TODO: We are not using the PPA environment at this time...
I suggest you first run bin/snap-ubuntu-packages to get all
dependencies installed. Then do cmake + make.

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
    git clone --recursive https://github.com/m2osw/snapcpp.git snapcpp
    snapcpp/bin/snap-ubuntu-packages --optional
    snapcpp/bin/snap-build

For a clone that ends up being a read/write version, then you want to
use a slightly different URL for the purpose:

    git clone --recursive git@github.com:m2osw/snapcpp.git snapcpp

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
