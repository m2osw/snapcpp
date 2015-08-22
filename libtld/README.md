# Introduction

libtld is an easy to use C/C++/PHP library that gives you information
about domain names (TLD) found in a URI. The result can be used to know
whether someone entered a valid URI without having to hit the Internet.
It is also important in the event you wanted to check an SSL certificate
(i.e. the exact TLD + one name must be checked in that case.)

# Notes:

* Version 1.5.0 offers a function to fold the characters of a domain
  to lowercase; including support for %XX encoding and UTF-8.

* Version 1.4.17 and following version includes even more new
  global TLD entries.
  
* "Intermediate" versions are "lost" to our nightly build server.

* Version 1.4.6 includes many (most) of the new allowed global TLD entries.
  (INA expects about 960 new TLDs within the next few years.)

* Version 1.4.3, 1.4.4, and 1.4.5 are missing because these were used to
  finish up the Debian installer. Although one version included a memory
  leak but I did not then find the time to upload a new version here.

* The .deb versions for Ubuntu can be downloaded from our snapcpp PPA
  on launchpad (this is not being updated anymore...):

      https://code.launchpad.net/~snapcpp/+archive/ppa/+packages

* The PHP extension was not downloaded in the previous version, so I
  won't upload a new version here. It did not change anyway.

