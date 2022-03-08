
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
  can either fail or wait on the file before moving forward.

