
# Launchpad API

The project on Launchpad is accessible via the Launchpad API. By default
the API returns JSON data. What is returned is documented on Launchpad:

* https://launchpad.net/+apidoc/devel.html

The Snap! C++ project is found here:

* https://api.launchpad.net/devel/snapcpp

That URI defines many other URIs that can automatically be followed.

The following works to access the source of one of our libraries:

* https://api.launchpad.net/devel/ubuntu/bionic/+source/libcasswrapper

This includes notes about bugs and a few URLs, but still nothing about
the last build.

However, this one seems to do it as expected, that is, it returns build
information for the specified source:

* https://api.launchpad.net/devel/ubuntu/bionic?ws.op=getBuildRecords&source=libcasswrapper

However, we do not get any entries for our sources. Here is one URI for
Haskel, though:

* https://api.launchpad.net/devel/ubuntu/+source/haskell-tldr/0.2.3-2/+build/14220957


