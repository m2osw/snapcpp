
* The automatic build+1 code uses the name of your current system distribution.

  This is generally correct although I'd like a better way to determine the
  name and offer the user to select the one they want to use.

* Allow for an old project name

  Right now, the murmur3 project generates warnings about finding entries
  that have a different name: libmurmur3. That older name was in conflict
  so I changed the name of my project. What I think I should do is add a
  file with old valid names so we can avoid the warning altogether. I
  don't think having such under `debian/...` would be wise, but maybe under
  `conf/...`. If the file exists, load it and memorize the old names. We
  could still generate one warning the very first time we find that old
  name so we know it happens, but not on every refresh.

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
  [well, for the list of package/arch, I read the debian/control file
  and that works very well; although we should read it and cache the
  data at the time we upload the source to launchpad]

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
    may not match the folder name one to one. The name has to be taken from
    the control file and all the files defined in the control file should
    be checked for availability.

* Icon showing status/backend process

  It would be cool to add one column with an icon representing the status
  and when the backend is doing work, show that instead. Use animated GIF
  images to dislpay an animation so it is even more exciting.

* Add error field to the project

  Whenever something fails with loading a project info, building, packaging,
  etc. we do not get any feedback in the interface.

  This is to add an `f_error_message` field to the project so the interface
  can retrieve that and display it somewhere. Also if there is an error, we
  can show an icon about it so we can see that there is trouble to take
  care of.

* Make proper use of the status bar message

  Right now I have some `statusbar->showMessage()` calls but none of them
  are visible. Although the backend process may be working on multiple
  projects, it could still be useful when working on just one. At least
  showing that some things are happening.

