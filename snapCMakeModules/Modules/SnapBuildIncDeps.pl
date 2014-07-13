#!/usr/bin/perl -w
#

LINE: while (<STDIN>)
{
    next LINE if /^#/;	# discard comments
	print $_;
}


