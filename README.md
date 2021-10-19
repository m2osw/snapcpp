
<p align="center">
<img alt="Snap! Websites" src="https://snapwebsites.org/sites/all/modules/mo_m2osw/snap-medium-icon.png" width="70" height="70"/>
</p>

# Introduction

Snap! C++ is a website backend project mainly written in C++ (there is a
little bit of C and it uses many C libraries...)

The project includes many libraries, many daemons/servers, it supports
plugins for third party extensions, and it uses Cassandra as its database
of choice (especially, a NoSQL, although Cassandra now has CQL...)

Click on this link to go to the [official home page](https://snapwebsites.org/)
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

* 4 frontend computers to run Apache2, `snap.cgi` and `snapserver` (1Gb)
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

Note: ScyllaDB should work just fine too. We are planning in testing
that theory soon.

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
somewhat slower, but it is bearable. Of course, as a developer with a
single node, you may lose the data in that node, but I would imagine it
would only be test data...

## A Snap! Websites Cluster

The following images shows you an installation example. The cloud represents
clients accessing your Snap! environment. The `Apache + snap.cgi` represents
access points (you can have as many as you want to load balance the incoming
hits). All the other computers can be 100% private (i.e. no direct access to
the Internet). The `snapserver` computers represent the frontend of Snap!
(middle-end as far as the cluster is concerned.) At some point `snap.cgi`
will be smart enough to auto-load balance your stack, right now we just use
a simple round robin DNS definition. The Cassandra computers each run one
node. Those must have at least 4Gb of RAM in a valid live system. The
snapbackend run CRON like tasks. However, those are dynamic and receive a
PING message from the `snapserver` whenever a client makes a change (on a
POST). So really it does work in the background but instantly once data to
crunch is available.

![Snap! Websites Cluster Setup](snapwebsites/libsnapwebsites/doc/snapwebsites-cluster-setup.png)


## The Snap! Websites Stack

We have
[another picture and more details on this page](https://snapwebsites.org/implementation/snap-websites-processes)
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

At that point, the `snap.cgi` choose a `snapserver` that's not too loaded, and
sends the access data to it and wait for the reply which is then sent to
Apache2 and finally back to the client.

The `snapserver` will access the database, Cassandra, through the
`snapdbproxy` daemon (which allows us to have the system go really fast
because we connect to it on localhost instead of directly to Cassandra,
which can be much slower, especially if through an SSL connection. We also
gather the meta data only once and cache it.)


# Getting Started

If you are an administrator who just wants to install Snap! on your systems,
then you want to look at the Launchpad section below.

If you are a developer and want to work on the source code, check 
We are using the PPA environment again at this time... If you want
a pre-compiled version, that's the way to go!


## Basics about the Snap! C++ Projects

We now make use of Launchpad, also called the PPA repository of Ubuntu,
to compile and generate all our public Snap! packages.

We currently have three main sections:

* cmake
* contribs, and
* snapwebsites

The cmake folder includes various cmake scripts used to handle the Snap!
C++ development environment.

The contribs are independent libraries, many of which we authored especially
for Snap! C++ but that can be used in other projects with very minor help.
A couple (log4cplus and zipios) are actually third party libraries which
we compile specifically because Ubuntu does not offer that latest greatest
version yet. We intend to stop supporting those on the edge versions once
we can switch to the latest version available as is in the Ubuntu repository.

The snapwebsites project is the Snap! C++ environment. Although it is
composed of many sub-projects, these would be difficult to use on their
own. We still have one or more packages per project. Often we break down
a project by library, tools, development, and documentation.
Along the way, we break-up various parts as contrib libraries instead
(for example, the libaddr and libexcept constribs are two libraries
that were created from the libsnapwebsites code that we wanted to reuse
in other contribs.)


### Getting Packages from Launchpad

We offer a [PPA](https://launchpad.net/~snapcpp/+archive/ubuntu/ppa)
with all the projects found in Snap! C++.

To get that to work on your system, you want to install the PPA in your
apt-get environment. The following are the two lines you can add to your
apt-get source files to make things work.

    deb http://ppa.launchpad.net/snapcpp/ppa/ubuntu xenial main 
    deb-src http://ppa.launchpad.net/snapcpp/ppa/ubuntu xenial main 

You can also use the `apt-add-repository` command as follow:

    sudo apt-add-repository ppa:snapcpp/ppa

This way, you automatically get the keys installed so you don't have
to do that manually.

Once you've upgraded the PPA, you can see a new file under
`/etc/apt/sources/list.d`. It should be named something like this:

    snapcpp-ubuntu-ppa-xenial.list

If you had other snapcpp list files, you probably want to remove them.

TODO: We currently don't offer the Cassandra C++ Driver as a third party
was offering it. We are now thinking it would be easier for us to include
another contrib entry with that driver so it works with whatever version
of Ubuntu we want to support.

The first installation requires the snapmanagercgi package:

    sudo apt-get update
    sudo apt-get install snapmanagercgi

The computer after that command should be ready to accept your installations
from the `snapmanager.cgi` browser interface, although you may need to tweak
a few things to get Apache and `snapmanager.cgi` to work together. It works
as is for us on DigitalOcean and our VPS using VirtualBox on our development
computers.

To upgrade later, run an `update` and `dist-upgrade` (the `snapmanagerdaemon`
does it for you, but if you like to see the output, that's the easiest way,
although with `snapmanager.cgi` it runs an upgrade on **all** your
computers at once:

    sudo apt-get update
    sudo apt-get dist-upgrade

Note that the Upgrade in the `snapmanager.cgi` interface works the same
way as these last two commands. So if you add the Snap! C++ repository to
all your nodes you can then use the `snapmanager.cgi` interface to run an
upgrade on your entire cluster.

Once ready, the `snapmanager.cgi` gives you access to a list of _bundles_.
These are used to install a part to your system. For example, the Cassandra
nodes don't get the `snapbackend` bundle installed. However, attempting to
install Snap! C++ by hand is complicated. There are small setups to do which
if you miss them prevents the system from working correctly. I'm the author
and frankly, there are just way too many things to remember about to not be
using `snapmanager.cgi` to proceed with installations.

### Installing a Website

Once you installed the `snapserver` bundle, you may start installing a
website. At the moment this is complicated because we do not have a
`snapmanager.cgi` interface quite yet. You may use the GUI, but when
installing a remote version of Snap! C++, it's complicated (as it requires
an ssh tunnel.)

First we want to prepare the database context. Note that this may take a
little time as the process waits for the creation to be complete before
returning.

    snapcreatecontext

Right now, the snapdbproxy won't detect that the new context was created.
It needs to be restarted so we can create the tables:

    sudo systemctl restart snapdbproxy

At some point we are thinking to get the `snapdbproxy` to do the work
of creating the context and tables. Then later to understand requests
to install a new website in the database. That way, you won't have to
worry about having to do that yourself. The `snapdbproxy` can easily
detect whether the context exists and check the tables before accepting
connections from other parts of the system.

Next we want to install the tables in the context. The tables are declared
in XML files so the tool can read those and do the necessary work.

    snapcreatetables

Tables are assigned a name and a model. The model defines many parameters
so Cassandra has a better chance to handle that table properly.

Once the tables were created, it is possible to install the domain name,
website name, and then the actual website data. This is currently all
done in one go with the `snapinstallwebsite` tool, although with that
tool we currently limit the installation to a standard/default setup.
Later it will be possible to edit or provide more advanced declarations
for your domains and websites so you can manage much more complex
parameter gathering from your domain name and website path.

    snapinstallwebsite --protocol HTTP --domain www.snap.website --port 80

At this time we support port 80 and 443.

The domain name may or may not include a sub-domain name.

The installation of the domain and website is extremely fast (very small
parameters to save to the database.) What followe, however, requires more
time.

By default, the tool is verbose and informs you of the current step in the
installation process. The last step says:

> Status: Initialization succeeded.

Unless you encounter a problem while initializing. You may re-run the
process multiple times until you get a postive outcome. However, some
errors won't be corrected with additional run. They will instead have
been skipped.

Now the first website is ready for viewing in your browser. You should be
able to visit the front page with:

    http://www.snap.website/

The information may not be available to the existing instance. To make
sure that the newest everything (tables, data, etc.) is ready for use,
you may want to reboot your cluster once. This will reset all memory
caches and make sure all the daemons are running as expected. Then
try again to access your website.



## From Sources (For Advanced Developers)

### Introduction

The whole environment is based on `cmake` and also matches `pbuilder` so we
can create Ubuntu packages with ease (`cmake` even makes use of the control
files to generate the inter project dependencies!) You can also find some
of the compiled packages on
[launchpad.net](https://launchpad.net/~snapcpp/+archive/ubuntu/ppa/+packages)
Note that we do not actively maintain all the versions. We try to have valid
long term versions compilable, at least.

### Compiling Everything

Here are the few commands that generally get the code from the repository
and compile it all:

    apt-get install git
    mkdir snapwebsites
    cd snapwebsites
    git clone --recursive https://github.com/m2osw/snapcpp.git snapcpp
       or
    git clone --recursive git@github.com:m2osw/snapcpp.git
    sudo snapcpp/bin/snap-ubuntu-packages --optional
    snapcpp/bin/snap-build

The first time you try to build on your system, we suggest you run the
`bin/snap-ubuntu-packages` script to get all the dependencies installed.
Then the `snap-build` script does most everything else.

**NOTE:** The `git clone` command is going to download all the contribs
which are in separate git repositories. This causes an issue: the scheme
will change to `git@github.com:` under your feet. So that means you need
to have an account or do not use the recursive and manually get each
project with `https:`.

**IMPORTANT:** to run the `snap-build` script, you must be right outside of
the snapcpp environment.

### `git` Fails with Permission Errors

We have setup our sub-modules to use SSH. It's much more practical for us,
but that means you need to have a key setup with github for the cloning
to complete 100%. Without a key, you'll get errors for all the submodules.

To fix that issue, you have a few solutions:

1. Create a Key

    See the [github SSH key documentation](https://docs.github.com/en/free-pro-team@latest/github/authenticating-to-github/connecting-to-github-with-ssh)
    for information on how to do that.

    This is probably your best bet because many project are likely to do it
    the same as us. It will also allow you to fork and offer patches (if
    you're able to do so).

2. Use a Redirection

    This can be tricky long term. That is, a redirect is likely to get in
    your way. You may want to look into getting a key instead. Now, if you
    remember to remove the redirection once done, you should be fine. As far
    as I know, once the clone is done, you won't need the redirect anymore.

    To add a redirect, edit your `~/.gitconfig` file and add this rule at
    the end:

        [url "https://github.com/"]
           insteadOf = ssh://git@github.com/

    This tells `git` to use `https://...` instead of `ssh://...`.

3. Edit the `.git/config` File

    We do not recommend this solution, but it's possible to edit the
    configuration file and edit the URLs there too. Do the clone of
    the `snapcpp` without the `--recursive` command line option. Then
    edit the `.git/config` file:

        # Change:
        url = git@github.com:m2osw/<project>.git
        
        # To:
        url = https://github.com/m2osw/<project>.git

    Then you should be able to get the `submodules` with a command such
    as the following (not tested):

        git submodule foreach git pull origin main

    You may instead need the following (again, not tested):

        git submodule update --recursive
        # or
        git submodule update --init --recursive

    The edit can be problematic long term to maintain the project source
    tree, but if you just want to checkout the code and compile a given
    version, that's probably enough for you.


### Installing the Dependencies

The `snap-ubuntu-packages` command installs many packages that the build
requires. This is done automatically in the build system using the control
file information. For a developer, that's not automatic. So the easiest
is to run that command. Although we try to keep it up to date, if something
is missing, `cmake` should tell you. Worst case scenario, the compiler stops
with an error. We recommend the `--optional` packages so you get full
documentation and some extras for tests, however, if you do not want to
install the X11 desktop, do NOT use that option.

### Read/Write Repository (if you have write access)

For a clone that ends up being a read/write version, you want to use a
slightly different URL for the purpose:

    git clone --recursive git@github.com:m2osw/snapcpp.git snapcpp


### The `BUILD` Directory

After a while, you'll have all the built objects under a `BUILD` directory
in your `snapcpp` directory. The debug distribution being under the
`BUILD/Debug/dist` directory (**warning:** executables under the
distribution directory are stripped from their `RPATH` which means you
cannot run them without some magic; namely changing your `PATH` and
`LD_LIBRARY_PATH` environment variables).

Similarly, we build the `Release` and `Sanitized` versions.

We support a few variables, although in most cases you will not have to
setup anything to get started. You can find the main variables in our
`bin/build-snap` script which you can use to create a developer environment.
Here is an example of what you can do to generate the build environment.
The build type can either be Debug or Release.

    cmake \
        -DCMAKE_BUILD_TYPE=Release \
        -DSNAP_COMMON_DIR:PATH="`pwd`/BUILD/dist/share/snapwebsites" \
        ..

### 'HEAD Detached' Message

**WARNING:** When you clone everything with `--recursive` the leaves
(a.k.a. contribs) are going to be in "HEAD detached" mode. To fix this,
assuming you think about it before you make changes, just checkout the
master:

    cd snapcpp/contrib/as2js
    git checkout master

If you already made changes, the simplest is to create a branch and then
merge that in the master branch like so:

    cd snapcpp/contrib/as2js
    ...changes happen...
    git checkout -b iforgot
    git checkout master
    git merge iforgot
    git branch -d iforget
    ...

After that maneuver you can use your git as normal.

**BUT WHY?** -- this happens because `git` uses the latest available in
the master git info and not the newer version in the repository. When we
use modules, the module that gets checked out is the one defined in the
snapcpp git and that's not always the latest.

### C++ Cassandra Driver

To compile everything in older version, you had one more external dependency
which was the C++ Cassandra Driver.

You can obtain the precompiled for Ubuntu 16.04 code with the following PPA:

    sudo add-apt-repository ppa:tcpcloud/extra
    sudo apt-get update
    sudo apt-get install cassandra-cpp-driver-dev

Newer version make sue of the driver inside the `snapcpp/contrib` folder
(i.e. we compile our own version).

### Building Launchpad Packages

To rebuild all the packages on Launchpad, we have all the necessary scripts
in our makefiles. Once you ran `cmake` successfully, say under a directory
named `BUILD`, then you can run the following command:

    make -C BUILD dput

This will prepare all the source packages and upload them to the Launchpad
repository. The files are not going to be uploaded if their version did not
change. To change the version of a project, edit its `debian/changelog` file
and add an entry at the beginning of the file. In most cases, this is about
what changed in your last commit(s). If you did not really change anything
but still need a rebuild (i.e. a dependency changed but the PPA was not
smart enough to rebuild another package), then increase the 4th number and
put a comment about the fact this is just to kickstart a build of that
project.


### Making sure packages will build on Launchpad

Whenever you are ready to build on the PPA, you may want to first check
whether everything builds on your system. If you used the `bin/build-snap`
scripts, you have three folders under `BUILD`:

* `BUILD/Debug` -- the debug version.
* `BUILD/Release` -- the release version which has no debug code but is fully
optimized (in case you need to test the speed of the resulting software).
* `BUILD/Sanitize` -- a version with the sanitize options turned on, helpful
to detect memory leaks and invalid accesses (access after free, for example).

The `BUILD/Debug` folder is generally the one you use on your development
system since it's the debug version. The debug version is the default so
you do not need to specify the `-g` flag on the `mk` command line:

    ./mk

If you have memory issues, you can also use the `BULID/Sanitize` version
which is likely to let you know as soon as you misuse memory. This version
is accessed using the `-s` command line option as in:

    ./mk -s

The `BUILD/Release` is rarely built on your development system except when
you are ready to build packages on Launchpad. This ensures it compiles with
all the optimizations turned on. The release is build with the `-r` command
line option as in:

    ./mk -r -i

If the release build fails, then the PPA will sure fail. Note that I also put
the `-i` option since you want to test instaling the software to the
distribution folder (`BUILD/Release/dist/...`). On errors, make sure you have
the latest version of everything, check where the error occurs, try
compiling and fixing just that one project until it works, and push all your
changes if any, then run the PPA update. Note that you can rebuild one project
at a time on the PPA. The `snapbuilder` tool is expected to be used for that
purpose (it's not yet complete as of Oct 2021, but it's getting there--it is
also specific to our Launchpad environment at the moment).

While working, you can run the tests with the `-t` option like so:

    ./mk -t [<test-name> ...]

You can also include test names to limit the run to only those tests. The
`-t` can be used with the `-g`, `-s` and `-r` options.


## Setting up a development environment

TODO: need to complete this one...

Once you compiled everything, you will have an environment that can be
used to run Snap! Only, it won't work as is. You first have to setup a
certain number of parts: `/etc/snapwebsites`, `/var/lib/snapwebistes`,
`/var/log/snapwebsites`, `/var/cache/snapwebsites`, and make sure
`snapmanagerdaemon` starts as root.

Although these things do get installed automatically when you install
the Snap! packages, doing so with the packages will install these files
as `snapwebsites:snapwebsites` (user/group) and that is not practical
when trying to debug Snap!, since you'll want to quickly edit these
files.


## Creating packages

To build Ubuntu packages, you want to run the following commands,
although this is currently incomplete! We will try to ameliorate that
info with time. It currently takes 1h30 to rebuild everything as packages.
(Note that it can be parallelized and run a lot faster that way, like
10 to 15 minutes.)

    # get some extra development tools if you don't have them yet
    apt-get install ubuntu-dev-tools eatmydata debhelper

    # create the build environment (a chroot env.)
    pbuilder-dist `lsb_release --codename --short` create

    # Prepare source packages
    make debuild

    # Create packages
    make pbuilder


## Linux Only

At this point we only have a Linux version of the project. We have no
plans in updating the project to work on a different platform, although
changes are welcome if you would like to do so. However, as it stands,
the project is not yet considered complete, so it would be quite premature
to attempt to convert it.

Note that the software makes heavy use of the `fork()` instruction. That
means it will be prohibitive to use under earlier versions of MS-Windows
(i.e. MS-Windows 10 and newer offer the `fork()` instruction now.)


# Dependency Tree

This tree changes from time to time, but it gives you an idea of all
the projects that we are managing inside the Snap! C++ environment.

<p align="center">
<img alt="advgetopt" title="CMake Module Extensions to Build Snap! C++."
src="https://snapwebsites.org/sites/snapwebsites.org/files/images/clean-dependencies.svg"/>
</p>



# Bugs

Submit bug reports and patches on
[github](https://github.com/m2osw/snapcpp/issues).


vim: ts=4 sw=4 et

_This file is part of the [snapcpp project](https://snapwebsites.org/)._
