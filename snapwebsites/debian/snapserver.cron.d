# Run snapbackend once per hour to perform critical clean-up and other processing.
#
0 */1 * * * snapwebsites [ -x /usr/bin/snapbackend ] && /usr/bin/snapbackend
