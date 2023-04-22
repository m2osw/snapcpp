
# general

* Look for any references to "blacklist"/"whitelist" and change with
  "denylist"/"allowlist".
* Look for any references to "master"/"slave" and change with
  "primary"/"replica" or "primary"/"secondary".
* Call the `snapdev::drop_root_privileges()` on startup of tools/services
  that should not ever be `root` while running.
* Make sure all services exit with code 9 on a configuration failure.
  (and make the .service know that--see snaprfs daemon/.service)

# advgetopt

* Work more on getting edit-config to generally work better.
* Add support for the JSON format.
* Add support for the XML format.
* Add support for the yml format.
* Note: extending the advgetopt is now much less of a priority since we have
        the fluid-settings which replace that in our services.


# as2js

* Rework the tests to use SnapCatch2 (started).
* Make it possible to compute expressions (like the snapexpr.cpp/h would do).
* Remove the advgetopt dependency to allow for JSON inside advgetopt.
* Move sub-classes outside of their classes (to avoid one namespace).
* Move private sub-classes as details declared & implemented in our .cpp.


# commonmarkcpp

* Hmm... this was a test, mainly, it's way more complicated than I thought.


# communicatord

* Implement proper graph support.


# cppthread

* Add tests.


# csspp

* Implement version 2.x which compiles everything to object and then have
  a function that spits the data back out. This way we can have a real
  optimizer. Also the CSS specs have grown quite dramatically.


# edhttp

* Implement the HTTP 1, 1.1, 2, 3 client/server objects.
  (I'll look into using whatever NodeJS and/or Golang uses since they have
  that and it works.)


# eventdispatcher

* Create an object capable of opening any number of ports and Unix sockets
  (maybe even FIFO sockets?) to listen on.
* Create an object capable of connecting to any number of addr:port.
* Look at having templates to handle the higher level buffer/message support
  (that would be instead of the virtual functions). (semi-started some tests)


# fastjournal

* Implement.


# fluid-settings



# ftmesh

* This was for another project... although it can still be useful if we want
  to use fonts in HTML graph blocks.


# iplock (snapfirewall -> ipwall & ipload)

* Move snapfirewall to iplock (nearly complete, the ipload works, the rules
  are not quite complete).
* Finish the ipwall implementation, it needs to access a database to save
  the IP addresses we want blocked on all our computers.


# ipmgr

* Serial number issue (conflict with our counter & letsencrypt counter).
* Implement the slave counterpart (at the moment I only have code for a
  master server).


# libaddr

* Look at having a more yacc like parser. The current parser makes a lot
  of avoidable mistakes when more than one address/port is defined.


# libexcept

* Find out why the stack doesn't get generated automatically.
* Determine why my compiled libraries and tools do not get function names.


# libmimemail

* Test that it works.
* Write unit tests.


# libmurmur3


# libtld

* Update the TLDs once in a while.


# libutf8

* Implement UTF-8 normalization.
* With the normalization, we also get upper/lower and similar capabilities.


# serverplugins


# sitter (snapwatchdog)

* Implement tripwire like process that makes use of multiple computers to
  determine validity of the data.


# snapbuilder

* Information from Launchpad: still missing .deb in repo event.
* Review the dependency tree of the list (or keep alphabetized but have clicks
  work in the graph!).
* Implement the "one click" build process.


# snapdb / snapdatabase

* Implement our own datastore with indexes that work best for our environment.


# snapcatch2

* Update whenever a new version is posted.


# snapdev

* Add functions as required by other libraries and tools.


# snaplogger

* Write unit tests to attain full coverage.


# snaprfs

* Implement (Depends: communicatord).


# snapwebsite

* Replace old code (cppthread, snaplogger, eventdispatcher, etc.)
* Change scheme to allow for a mostly client side implementation of websites
  (i.e. the server becomes mainly a datastore).
* snapdatabase -- see snapdb above instead


# zipios

* Look at whether we can implement a ZipFile() constructor with istream.
  (this is partially implemented now; it does not quite work but it's moving
  forward...)


# Local Build Server

* Test that it works with Ubuntu 20.04 & 22.04.
* Look into running the tests (but only in our build server, not launchpad).


# IPv6 Support

* Make sure everything works with IPv6.
  * eventdispatcher (probably done? but not tested)
  * communicatord

  We already have the low level handled (eventdispatcher will be done soon),
  we nearly just need to use the correct settings everywhere, then test and
  see that we can run Snap! 100% in IPv6 instead of IPv4 (except for the
  LAN which can remain in IPv4).


