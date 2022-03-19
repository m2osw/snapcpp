
<p align="center">
<img alt="sitter" title="Sitter -- a daemon making sure that things are running."
src="https://snapwebsites.org/sites/snapwebsites.org/files/images/sitter-logo.png" width="300" height="150"/>
</p>

Introduction
============

The `sitter` daemon runs in the background and check the health
of your system once every minute. The results are saved in files
for later review.

The data includes status such as CPU and memory usage by the computer as
a whole. It also supports checking such data on a per process basis.


Statistics
==========

The `sitter` deamon gathers statistics about the following elements, once
a minute:

* APT
* CPU
* Disk
* Firewall
* Flags
* Log
* Memory
* Network
* Packages
* Processes
* Shell Scripts

Other packages may offer additional `sitter` compatible plugins.

In general, the statistics about processes is to check to know whether
they are running or not running as expected in the current situation.

The Log plugin verifies the ownerhship and size of each log file.

The CPU, Disk, Firewall, Memory, Network plugins just gather statistics
(how much of each is being used). The statistics are saved in the data
directory. 7 days worth of data is kept then it gets overwritten. You
can find this data under:

    /var/lib/sitter/data/...

Whenever it looks like something is out of certain bounds (using too much
memory, not connected, etc.) then an error is raised and if the priority
of the error is high enough, an email is sent to the administrator.


Internal and External Plugins & Scripts
=======================================

While the `sitter` daemon wakes up, it runs a set of plugins and scripts.
By default, the scripts are found under:

    /usr/share/sitter/scripts/...

The scripts are expected to test whether things are running as expected.
This includes testing whether certain daemons are running or not. For
example, if `snapcommunicator` is not running, it will be reported.

The scripts are also used to detect bad things that are at times happening
on a server. For example, `fail2ban` version 0.9.3-1 has a process named
`fail2ban-client` which at times does not exit. Somehow it runs in a tight
loop using 100% of the CPU. We have a script in the `snapfirewall` project
that detects whether the `fail2ban-client` runs for 3 minutes in a row. If
that happens, then it generates an error and sends an email.

The scripts create files under the following directory:

    /var/lib/sitter/script-files/...

This includes files that the scripts use to remember things (i.e. to know
how long a process has been running so far, for example.)

We save the scripts stdout and stderr streams to a "log" file under:

    /var/log/sitter/scripts.log

Note that the content of those logs is also what we send to the
administrator's mailbox.

We use a separate directory to be able to safely run a `find` command
to delete old files.


## Script: `watch_firewall_fail2ban_client` (`fail2ban-client`)

This script is installed by the `snapfirewall` package and it is used to
make sure that `fail2ban-client` stops once it starts running and
using 100% of the CPU for too long. At this time it just sends an eamil
to the administrator who is expected to go on the server and make sure
that there is indeed a problem.

This is an issue in fail2ban version 0.9.3-1 (Ubuntu 16.04). Newer version
should not be affected anymore.

It seems that this happens if the `logrotate` tool renames a file that the
`fail2ban-client` tool is working on. However, it is nearly impossible
to reproduce in a consistent way so there is no better resolution at this
point (i.e. according to the author, this version already has a fixed
`fail2ban-client` script.)


Bugs
====

Submit bug reports and patches on
[github](https://github.com/m2osw/sitter/issues).


_This file is part of the [snapcpp project](https://snapwebsites.org/)._
