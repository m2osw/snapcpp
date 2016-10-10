
Administrator Modified Files
============================

Please copy files from the `/etc/iplock` directory in this `/etc/iplock/iplock.d`
sub-directory and modify them here.

That way, you will continue to get the default configuration changes from
the source package.

Note that the `iplock.conf` file gets loaded from `/etc/iplock` and then
from `/etc/iplock/iplock.d`. So options defined in the second file replace
options defined in the first file.

However, all the other files do not get loaded twice. Only the one under
`/etc/iplock/iplock.d` gets loaded if it exists. Those under `/etc/iplock`
are ignored in that case. Those other files are those that define the
rules to use to block an IP address in your iptables filter.

