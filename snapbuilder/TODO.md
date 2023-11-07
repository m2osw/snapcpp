
* New Version while building

  If you change the version in a changelog file before the build is finished,
  the snapbuilder gets "entangled" in that infinite loop waiting to find that
  newer version which is never sent to the build server. i.e. it searches
  all the entries and does not find that version since it is new locally only.

  To a minimum we would need to detect that the version is not visible on
  launchpad and report that. This can be detected and reported whenever
  the build flag is true.

  I think the simplest is to save the version we are sending in the .build
  file (instead of an empty file). When testing again, read that version from
  that file and not from the current changelog (and possibly show the
  discrepancy in the list?)

* Add a file so we can know whether the tests passed.

  i.e. if we run `./mk -t` then we can know whether the tests pass or not.
  If not, delete the file, if it passes create the file. If the binary file
  to run the test has a timestamp more recent than the test file the
  snapbuilder creates, then the tests were not yet run against the latest
  version and we should do so before we run a Launchpad build.

  (i.e. the file is like a flag, similar to the one we use with the build
  to know whether we sent the latest version to the server or not)

* Verify that the new package is indeed available

  It looks like new packages are not available right away and when we try
  to build back to back, the build server may end up using an old version.

  I'd like to see whether that's visible in the JSON we get from the server.
  If not, then look at how we can download those packages. If we can't then
  it's not yet ready and we have to wait.

  One way to make more sure that we can download the correct version would
  be to implement what Doug had done: a change of all the versions in the
  control file. That way, if the build service cannot find the file, it
  can either fail or wait on the file before moving forward. [this is not
  correct, the launchpad build mechanism doesn't wait on "internal"
  dependencies--those that are in the same PPA; that said, it would fail
  with a clear error "version unavailable" which may be better than what
  we have now]

