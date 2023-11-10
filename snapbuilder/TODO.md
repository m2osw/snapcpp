
* Use thread to work on the packages in a separate task

  I want to create a version where I have a separate task (a thread)
  because right now it takes forever to load all the data and it can very
  badly block the main process.

  I need proper synchronization when updating a project, but other than
  that, I think it's doable. I also need a signal to the main thread so
  the window can be updated.

* New Version while building

  If you change the version in a changelog file before the build is finished,
  the snapbuilder gets "entangled" in that infinite loop waiting to find that
  newer version which is never sent to the build server. i.e. it searches
  all the entries and does not find that version since it is new locally only.

  To a minimum we would need to detect that the version is not visible on
  launchpad and report that. This can be detected and reported whenever
  the build flag is true.

  I now save the version & date to the .build file so we can reuse that
  part, and I'm going to also save the list of package/arch because we
  need those too in order to verify that the packages were built.

* Add a file so we can know whether the tests passed.

  i.e. if we run `./mk -t` then we can know whether the tests pass or not.
  If not, delete the file, if it passes create the file. If the binary file
  to run the test has a timestamp more recent than the test file the
  snapbuilder creates, then the tests were not yet run against the latest
  version and we should do so before we run a Launchpad build.

  (i.e. the file is like a flag, similar to the one we use with the build
  to know whether we sent the latest version to the server or not)

* Verify that the new package is indeed available

  This somewhat works, I have an issue with the name & architecture which
  may not match the folder name one to one. Also I want to move that code
  in a thread so it doesn't completely block the interface.

