
# zipios

* Look at whether we can implement a ZipFile() constructor with ostream.


# as2js

* Remove the advgetopt dependency to allow for JSON inside advgetopt.
* Rework the test to use SnapCatch2.


# advgetopt

* Add support for the JSON format.
* Add support for the XML format.
* Add support for the yml format.


# iplock (snapfirewall)

* Move snapfirewall to iplock.
* Use a system of setup allowing other apps to grow the firewall seamlessly.


# snapcommunicator

* Implement with new proper graph support.
* Open several connections: Unix socket (plain), TCP plain, TCP with TLS, UDP.


# snaplogger daemon/network (remote logging)

* Finish up the snaplogger daemon/network (mainly testing making sure it works).


# eventdispatcher-http

* Implement the HTTP 1, 1.1, 2, 3 client/server objects.
  (I'll look into using whatever NodeJS uses since they have that and it works)


# fluid settings

* Implement (Depends: snapcommunicator).


# libutf8

* Implement UTF-8 normalization.
* With the normalization, we also get upper/lower and similar capabilities.


# snapbuilder

* Implement gathering of information from Launchpad.
* Review the dependency tree (also it did not appear last time I tested).
* Implement the "one click" build process.
* Look at having a local build server as well.


# snapcatch2-3

* Test to see whether we can easily switch to this version.


# snapdb

* Implement our own datastore with indexes that work best for our environment.


# snaprfs

* Implement (Depends: snapcommunicator).


# snapwebsite

* Replace old code (cppthread, snaplogger, eventdispatcher, etc.)
* snapdatabse -- look into finishing that up...


# IPv6 Support

* Make sure everything works with IPv6.

  We already have the low level handled (eventdispatcher will be done soon),
  we nearly just need to use the correct settings everywhere and especially
  get the IPv6 firewall setup, then test and see that we can run Snap! 100%
  in IPv6 instead of IPv4 (except for the local network).


