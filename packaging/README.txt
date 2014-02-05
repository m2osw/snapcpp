Tue Feb  4 19:11:40 PST 2014 (DB):

This directory contains child dirs, on for each project that gets built into
a debian package ultimately. The reason why each debian folder doesn't just live
in each project is a legacy reason due to the way launchpad.net's recipes work.

In a nutshell, in order to nest a subdir into a project on launchpad, you can
only nest folders into folders. You cannot nest a project directly into the root
folder, file by file. So I must have the debian folders separate from the src
folders themselves.

When updates are pushed into the archive at sf.net, launchpad.net eventually
sees that there are updates waiting, and then imports them into the local
mirror. Once per day, a nightly build is kicked off, but only if the local mirror
has changes. However, if you do not also bump the version number in the
changelog for each project, the system will fail to upload the source package.
So it is critical that you run "dch" in each of the project folders under
snapcpp/packages/<project_name>.
