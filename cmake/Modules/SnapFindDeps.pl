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
die "usage: SnapFindDeps.pl [root source directory] [project]\n" unless $#ARGV == 1;

use Cwd;
use Dpkg::Changelog::Parse;
use Dpkg::Control::Info;
use Dpkg::Deps;
use File::Find;
use Storable;

my $dir     = shift;
my $project = shift;

my %DEPHASH;
my %DIRHASH;
my %options;


################################################################################
# Search our folder for all debian projects.
#
sub projects_wanted
{
    if( not stat($File::Find::name . "/debian/control") )
    {
        return;
    }

    $DIRHASH{$_} = $File::Find::name;
}


################################################################################
# First, find every debian project within the specified tree.
#
my $hashfile = "/tmp/SnapFindDeps.pl.hash";
if( stat( $hashfile ) )
{
    my $hashref = retrieve( $hashfile );
    %DIRHASH = %$hashref;
}
else
{
    find( {wanted => \&projects_wanted, no_chdir => 0}, $dir );
    store \%DIRHASH, $hashfile;
}


################################################################################
# Next, read all of the dependencies in the specified project.
#
my %dep_list;
my $projectdir = $DIRHASH{$project};
chdir( $projectdir );
#
my $control       = Dpkg::Control::Info->new();
my $fields        = $control->get_source();
my $build_depends = deps_parse($fields->{'Build-Depends'});
#
for my $dep ( $build_depends->get_deps() )
{
    $dep =~ s/([^ ]+) [^\$]+/$1/;
    $dep_list{$dep} = 1;
}


################################################################################
# Next, go through all of the projects and find a match
#
for my $proj (keys %DIRHASH)
{
    if( $proj eq $project )
    {
        # Don't bother scanning this project, because it's the one we're
        # analysing...
        next;
    }

    my $projdir = $DIRHASH{$proj};
    chdir( $projdir );

    my $dep_ctl = Dpkg::Control::Info->new();
    my @control_pkgs = $dep_ctl->get_packages();
    foreach my $p (@control_pkgs)
    {
        my $pkg = $p->{"Package"};
        if( $dep_list{$pkg} )
        {
            print "$proj ";
            last; # We're done, so skip checking the other dependencies in the package
        }
    }
}


# vim: ts=4 sw=4 et
