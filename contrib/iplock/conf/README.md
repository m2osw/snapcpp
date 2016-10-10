
Administrator Modified Files
============================

Please copy files from the `/etc/iplock` directory to this
`/etc/iplock/iplock.d` sub-directory and modify them here.

That way, you will continue to get the default configuration
changes from the source package under `/etc/iplock`.

All files get first loaded from `/etc/iplock` and then again
from `/etc/iplock/iplock.d`. Any parameter redefined in the
sub-directory overwrites the parameter of the same name in
the main directory.

