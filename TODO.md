
# general

* Look for any references to "blacklist"/"whitelist" and change with
  "denylist"/"allowlist".
* Look for any references to "master"/"slave" and change with
  "primary"/"replica" or "primary"/"secondary".
* Call the `snapdev::drop_root_privileges()` on startup of tools/services
  that should not ever be `root` while running. If the service is not to
  be started as root anyway, do that in the .service file as well.
* Make sure all services exit with code 9 on a configuration failure.
  (and make the .service know that--see snaprfs daemon/.service)
* Look into removing mysql from the list of dependencies.
* Remove all the pragma -Wrestrict once we're done with Lunar.
* The lcov now includes tests and docs and should have a top directory
  that allows us to go to a specific project and then within that go
  to a specific version (when available, like for coverage).
  Also, the docs top folder needs a few images and the top lcov folder
  should have a robots.txt defined there.
* Look at the clean-dependencies.svg creation; it seems to work with
  development dependencies opposed to installation dependencies and
  thus it may look a bit funky at times (or there is a bug in the
  _simplifier_)


# advgetopt

* Work more on getting edit-config to generally work better.
* Add support for the JSON format.
* Add support for the XML format.
* Add support for the yml format.
* Note: extending the advgetopt is now much less of a priority since we have
        the fluid-settings which replace that in our services.


# as2js

* Move sub-classes outside of their classes (to avoid one namespace).
* Move private sub-classes as details declared & implemented in our .cpp.


# basic-xml


# commonmarkcpp

* Hmm... this was a test, mainly, it's way more complicated than I thought.


# communicatord

* Implement proper graph support.


# cppthread

* Add tests.


# csspp

* Implement version 2.x which compiles everything to objects and then have
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


# libtld

* Update the TLDs once in a while.
* Consider v2 which would be 100% C++ with a C interface on top.


# libutf8

* Implement UTF-8 normalization.
* With the normalization, we also get upper/lower and similar capabilities.


# murmur3


# prinbee (snapdb / snapdatabase)

* Implement our own datastore with indexes that work best for our environment.


# serverplugins


# sitter (snapwatchdog)

* Implement tripwire like process that makes use of multiple computers to
  determine validity of the data.


# snapbuilder

* Review the dependency tree of the list (or keep alphabetized but have clicks
  work in the graph!).
* Implement the "one click" build process.


# snapcatch2

* Update whenever a new version is posted.


# snapdev

* Add functions as required by other libraries and tools.
* Finish up writing unit tests for entire set of functions.


# snaplogger

* Write unit tests to attain full coverage.


# snaprfs

* Finish implementing.


# snapwebsite

* Replace old code (cppthread, snaplogger, eventdispatcher, etc.)
* Change scheme to allow for a mostly client side implementation of websites
  (i.e. the server becomes mainly a datastore).


# versiontheca

* Enhance the client interface.


# zipios

* Look at whether we can implement a ZipFile() constructor with istream.
  (this is partially implemented now; it does not quite work but it's moving
  forward...)
* Look at having support for 64 bits.


# Local Build Server

* Test that it works with Ubuntu 24.04.
* Look into running all unit tests on our build server (not launchpad).


# IPv6 Support

* Make sure everything works with IPv6.
  * eventdispatcher (probably done? but not tested)
  * communicatord

  We already have the low level handled (eventdispatcher will be done soon),
  we nearly just need to use the correct settings everywhere, then test and
  see that we can run Snap! 100% in IPv6 instead of IPv4 (except for the
  LAN which can remain in IPv4).


