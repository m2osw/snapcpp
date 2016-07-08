
The system functions using plugins in order to allow any number of
features to be added to the manager. This is specifically important
for some plugins may have some special needs that have to be in
the snapmanager.cgi instead of /admin/settings/... (i.e. it may
require a root process to handle the settings.)

The manager is composed of:

  . snapmanagercgi/lib/* -- a library with common functions and classes
  . snapmanagercgi/cgi/* -- the snapmanager.cgi binary
  . snapmanagercgi/daemon/* -- the snapmanagerdaemon binary
  . snapmanagercgi/plugins/*/* -- a set of plugins

From the outside the user can access snapmanager.cgi which will run with
the same user/group restrictions as Apache (www-data:www-data). It can
connect to snapmanagerdaemon which accepts messages to act on the system
by adding/removing software and tweaking their settings as required.

The snapmanager.cgi and snapmanagerdaemon are both linked against the
library and can both load the plugins.

The plugins are useful for three main tasks:

  . gathering data on a computer, this is mainly an installation status
    opposed to the current status as snapwatchdog provides (although
    there may be some overlap)
  . transform the gathered data in HTML for display
  . allow for tweaking the data

The plugin implementation makes use of the libsnapwebsites library to
handle the loading. Note that the snapmanagercgi system always loads
all the plugins available. We think this is important because it always
installs and removes software and as it does so, adds and removes
various plugins. For example, the snapwatchdog is an optional (although
recommanded) feature and as such its plugin extension will not be
installed by default.

However, to know what is installable... at this point I have not
real clue on how to get that attached to the system (i.e. if something
is not yet installed, how could snapmanager know about it?!)

Organization:

  +-----------------------------+
  |                             |
  | libsnapmanagercgi           |
  | (common code/functions)     |
  |                             |
  +-----------------------------+
       ^
       |
       | linked
       |
       +-------------------------------------+------------------------+
       |                                     |                        |
  +-----------------------------+   +-----------------------------+   |
  |                             |   |                             |   |
  | snapmanager.cgi             |   | snapmanagerdaemon           |   |
  |                             |   |                             |   |
  +-----------------------------+   +-----------------------------+   |
                           |                  |                       |
                           +------------------+                       |
			   |                                          |
			   | load (dynamic link)                      |
			   v                                          |
                    +-----------------------------+                   |
                    |                             |-------------------+
                    | plugins                     |
                    |                             |
                    +-----------------------------+




