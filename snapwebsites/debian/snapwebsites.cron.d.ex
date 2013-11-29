#
# Regular cron jobs for the snapwebsites package
#
0 4	* * *	root	[ -x /usr/bin/snapwebsites_maintenance ] && /usr/bin/snapwebsites_maintenance
