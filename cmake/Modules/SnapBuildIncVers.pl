#!/usr/bin/perl -w

################################################################################
# SnapBuildIncVers.pl
# Author: R. Douglas Barbieri
#
# This perl script goes through the entire tree of projects, finds all of the
# Debianized project (should be all of them) then adds a new changelog record with
# an incremented build number.
#
die "usage: SnapBuildIncVers.pl [cache filename] [dist]\n" unless $#ARGV == 1;

use Cwd;
use Dpkg::Changelog::Parse;
use Dpkg::Control::Info;
use Dpkg::Deps;
use Storable;

my $cache_file   = shift;
my $distribution = shift;
my $change_msg   = "Nightly build."

my %options;

if( not $ENV{"DEBEMAIL"} )
{
    $ENV{"DEBEMAIL"} = "Build Server <build\@m2osw.com>";
}

################################################################################
# Search our folder for all debian projects.
#
die unless stat( $cache_file );

my $hashref = retrieve( $cache_file );
my %DIRHASH = %$hashref;


################################################################################
# Next, go through all of the projects and bump the versions.
#
for my $project (keys %DIRHASH)
{
    my $projectdir = $DIRHASH{$project};
    chdir( $projectdir );

    # Get name and version from changelog
    #
    my @fields = changelog_parse(%options);
    my $name;
    my $dist;
    my $urgency;
    my $version;
    my $changes;
    foreach my $f (@fields)
    {
        $name    = $f->{"Source"}       if exists $f->{"Source"};
        $dist    = $f->{"Distribution"} if exists $f->{"Distribution"};
        $version = $f->{"Version"}      if exists $f->{"Version"};
        $urgency = $f->{"Urgency"}      if exists $f->{"Urgency"};
        $changes = $f->{"Changes"}      if exists $f->{"Changes"};
    }

    # Increment the version
    #
    $version =~ s/~.*$//;
    my $newvers
    if( $version =~ m/^(\d*).(\d+).(\d+)$/ )
    {
        $newvers = "$1.$2.$3.1";
    }
    elsif( $version =~ m/^(\d*).(\d+).(\d+).(\d+)$/ )
    {
        my $num = $4+1;
        $newvers = "$1.$2.$3.$num";
    }

    if( $changes =~ m/\* $change_msg./ )
    {
        system "sed -i.bak -e 's/$name ($version) $dist; urgency=$urgency/$name ($newvers) $distribution; urgency=$urgency/'";
    }
    else()
    {
        # Write a new changelog entry with the new version
        #
        system "dch --newversion $newvers~$distribution --urgency high --distribution $distribution $change_msg";
    }
}

# vim: ts=4 sw=4 et
