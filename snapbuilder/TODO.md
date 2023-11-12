
* Allow for an old project name

  Right now, the murmur3 project generates warnings about finding entries
  that have a different name: libmurmur3. That older name was in conflict
  so I change the name of my project. What I think I should do is add a
  file with old valid names so we can avoid the warning altogether. I
  don't think having such under `debian/...` would be wise, but maybe under
  `conf/...`. If the file exists, load it and memorize the old names. We
  could still generate one warning the very first time we find that old
  name so we know it happens, but not on every refresh.

* Implement a Refresh for the currently selected project

  With the backend tasks, it is possible to just update one package instead
  of resetting the whole list with F5 so we should implement that now.

  Also we could look at having Ctrl-F5 to reload everything (as F5 does
  now) and have F5 to send a reload signal but not reset the list.
  The difference would be F5 does not detect old & new project changes.
  It only works with its existing list.

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

* Full Build Capabilities

  * Have several "Build Level"

    Implement the complete process with:

    * local compile in Debug, Sanitize, Release
    * run tests in Debug, Sanitize, Release
    * run coverage
    * make sure the project is committed & pushed
    * make sure the version is valid for an upload to launchpad
      - if rebuilding the tree, auto-bump version as required
    * load to launchpad
    * wait for status "built" (or some error)
    * repeat with the next project so the entire tree can be reworked

    In some cases we can build multiple projects simultaneously because
    they do not depend on each other. In that case, run the processes
    simultaneously (especially on launchpad, it reduces the total amount
    it takes to build everything).

  * Add a file so we can know whether the tests/coverage passed.

    i.e. if we run `./mk -t` then we can know whether the tests pass or not.
    If not, delete the file, if it passes create the file. If the binary file
    to run the test has a timestamp more recent than the test file the
    snapbuilder created, then the tests were not yet run against the latest
    version and we should do so before we run a Launchpad build.

    (i.e. the file is like a flag, similar to the one we use with the build
    to know whether we sent the latest version to the server or not)

  * Verify that the new package is indeed available

    This somewhat works, I have an issue with the name & architecture which
    may not match the folder name one to one. Also I want to move that code
    in a thread so it doesn't completely block the interface.

* Icon showing status/backend process

  It would be cool to add one column with an icon representing the status
  and when the backend is doing work, show that instead. Use animated GIF
  images to dislpay an animation so it is even more exiting.

* Add error field to the project

  Whenever something fails with loading a project info, building, packaging,
  etc. we do not get any feedback in the interface.

  This is to add an `f_error_message` field to the project so the interface
  can retrieve that and display it somewhere. Also if there is an error, we
  can show an icon about it so we can see that there is trouble to take
  care of.

* The About box shows GPL-3 text file as HTML

