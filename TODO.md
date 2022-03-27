
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


# eventdispatcher

* Create an object capable of opening any number of ports and Unix sockets
  (maybe even FIFO sockets?) to listen on.
* Create an object capable of connecting to any number of addr:port.
* Look at having templates to handle the higher level buffer/message support
  (that would be instead of the virtual functions).


# snapcommunicator

* Implement with new proper graph support.
* Open several connections: Unix socket (plain), TCP plain, TCP with TLS, UDP.


# eventdispatcher-http

* Implement the HTTP 1, 1.1, 2, 3 client/server objects.
  (I'll look into using whatever NodeJS and/or Golang uses since they have
  that and it works)


# fluid settings

* Implement (Depends: snapcommunicator).


# libutf8

* Implement UTF-8 normalization.
* With the normalization, we also get upper/lower and similar capabilities.


# snapbuilder

* Information from Launchpad: still missing .deb in repo event.
* Review the dependency tree of the list (or keep alphabetized but have clicks
  work in the graph!).
* Implement the "one click" build process.
* Look at having a local build server as well.


# snapdb

* Implement our own datastore with indexes that work best for our environment.


# snaprfs

* Implement (Depends: snapcommunicator).


# snapwebsite

* Replace old code (cppthread, snaplogger, eventdispatcher, etc.)
* Move the flag(s) implementation to a library so all our services can use it.
* snapdatabse -- look into finishing that up...


# IPv6 Support

* Make sure everything works with IPv6.
  * eventdispatcher (probably done? but not tested)
  * firewall

  We already have the low level handled (eventdispatcher will be done soon),
  we nearly just need to use the correct settings everywhere and especially
  get the IPv6 firewall setup, then test and see that we can run Snap! 100%
  in IPv6 instead of IPv4 (except for the local network, although that one
  should in part make use of Unix sockets instead...).


