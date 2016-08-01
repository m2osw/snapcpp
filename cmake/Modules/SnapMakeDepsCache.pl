#!/usr/bin/perl -w

################################################################################
# SnapFindDeps.pl
# Author: R. Douglas Barbieri
#
# This perl script opens a project's debian control file and finds all build
# dependencies. It then iterates through the source directory, finding all
# Debianized projects. It then emits a list of dependencies for the build
# that are found within the archive.
#
die "usage: SnapMakeDepsCache.pl [root source directory] [cache filename]\n" unless $#ARGV == 1;

use File::Find;
use Storable;

my $dir         = shift;
my $cache_file  = shift;

my %DIRHASH;

sub projects_wanted
{
    if( not stat($File::Find::name . "/debian/control") )
    {
        return;
    }

    $DIRHASH{$_} = $File::Find::name;
}

find( {wanted => \&projects_wanted, no_chdir => 0}, $dir );
store \%DIRHASH, $cache_file;

# vim: ts=4 sw=4 et
