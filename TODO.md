
# zipios

* Look at whether we can implement a ZipFile() constructor with ostream.


# as2js

* Rework the tests to use SnapCatch2 (started).
* Make it possible to compute expressions (like the snapexpr.cpp/h would do).
* Remove the advgetopt dependency to allow for JSON inside advgetopt.
* Move sub-classes outside of their classes (to avoid one namespace).
* Move private sub-classes as details declared & implemented in our .cpp.


# advgetopt

* Add support for the JSON format.
* Add support for the XML format.
* Add support for the yml format.
* Note: extending the advgetopt is now much less of a priority since we have
        the fluid-settings which replace that in our services.


# iplock (snapfirewall)

* Move snapfirewall to iplock.
* Use a setting system allowing other packages to grow the firewall seamlessly.


# fastjournal

* Implement.


# eventdispatcher

* Create an object capable of opening any number of ports and Unix sockets
  (maybe even FIFO sockets?) to listen on.
* Create an object capable of connecting to any number of addr:port.
* Look at having templates to handle the higher level buffer/message support
  (that would be instead of the virtual functions).


# snapcommunicator

* Implement with new proper graph support.
* Open several connections: Unix socket (plain), TCP plain, TCP with TLS, UDP.


# edhttp

* Implement the HTTP 1, 1.1, 2, 3 client/server objects.
  (I'll look into using whatever NodeJS and/or Golang uses since they have
  that and it works)


# fluid-settings

* Implemented tested with the sitter.


# libutf8

* Implement UTF-8 normalization.
* With the normalization, we also get upper/lower and similar capabilities.


# snapbuilder

* Information from Launchpad: still missing .deb in repo event.
* Review the dependency tree of the list (or keep alphabetized but have clicks
  work in the graph!).
* Implement the "one click" build process.


# snapdb / snapdatabase

* Implement our own datastore with indexes that work best for our environment.


# snaprfs

* Implement (Depends: snapcommunicator).


# snapwebsite

* Replace old code (cppthread, snaplogger, eventdispatcher, etc.)
* snapdatabase -- see snapdb above


# Local Build Server

* Test that it works with Ubuntu 20.04 & 22.04.
* Look into running the tests in this case.


# IPv6 Support

* Make sure everything works with IPv6.
  * eventdispatcher (probably done? but not tested)
  * firewall

  We already have the low level handled (eventdispatcher will be done soon),
  we nearly just need to use the correct settings everywhere and especially
  get the IPv6 firewall setup, then test and see that we can run Snap! 100%
  in IPv6 instead of IPv4 (except for the LAN which can remain in IPv4).


