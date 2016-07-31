#!/usr/bin/perl -w

################################################################################
# SnapBuildIncDeps.pl
# Author: R. Douglas Barbieri
#
# This perl script goes through the entire tree of projects, finds all of the
# Debianized project (should be all of them), adds a new changelog record with
# an incremented build number, then updates all control files which rely on
# dependencies to require the new version.
#
die "usage: SnapBuildIncDeps.pl [dist] [root source directory]\n" unless $#ARGV == 1;

use Cwd;
use Dpkg::Changelog::Parse;
use Dpkg::Control::Info;
use Dpkg::Deps;
use File::Find;

my $distribution = shift;
my $dir          = shift;

my %DEPHASH;
my %DIRHASH;
my %options;

my $DEBEMAIL = $ENV{"DEBEMAIL"};
if( $DEBEMAIL eq "" )
{
    $ENV{"DEBEMAIL"} = "Build Server <build\@m2osw.com>";
}


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
# Go through each project and update the dependencies according to the new
# updated versions.
#
sub update_dependencies
{
    my ($project_name) = @_;
    my $projectdir = $DIRHASH{$project_name};
    my %project_dep_hash;
    chdir( $projectdir );

    my $control       = Dpkg::Control::Info->new();
    my $fields        = $control->get_source();
    my $build_depends = deps_parse($fields->{'Build-Depends'});

    for my $dep ( $build_depends->get_deps() )
    {
        my $full_line = $dep;
        $dep =~ s/([^ ]+) [^\$]+/$1/;
        $project_dep_hash{$dep} = $full_line;
    }

    # Modify the list of dependencies if we find a match. Add the new version.
    #
    for my $key ( keys %DEPHASH )
    {
        my @package = @{$DEPHASH{$key}};
        my $version = $package[0];
        shift @package;
        for my $name ( @package )
        {
            for my $dep (keys %project_dep_hash)
            {
                if( $name eq $dep )
                {
                    $project_dep_hash{$dep} = "$name (>= $version)";
                }
            }
        }
    }

    # Now, open the control file of our project and modify the "Build-Depends" section.
    #
    chdir( $projectdir );

    my $input_file  = "debian/control";
    my $output_file = "debian/control.new";

    die "Cannot open $input_file for reading!\n"  unless open( my $input,  "<", $input_file  );
    die "Cannot open $output_file for writing!\n" unless open( my $output, ">", $output_file );

    my $replace_lines = 0;
    while( <$input> )
    {
        # Only evaluate the build-depends section
        if( /^Build-Depends:/ )
        {
            $replace_lines = 1;

            print $output "Build-Depends: ";
            my $prefix = "";
            my @sorted_keys = sort keys %project_dep_hash;
            for my $name (@sorted_keys)
            {
                print $output $prefix . $project_dep_hash{$name};
                $prefix = ",\n    ";
            }
            print $output "\n";
        }
        elsif( /^[A-Za-z-]+:/ )
        {
            $replace_lines = 0;
        }

        if( !$replace_lines )
        {
            print $output $_;
        }
    }

    close( $input  );
    close( $output );

    rename "debian/control",     "debian/control.orig";
    rename "debian/control.new", "debian/control";
}


################################################################################
# First, find every debian project within the specified tree.
#
find( {wanted => \&projects_wanted, no_chdir => 0}, $dir );


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
    my $version;
    foreach my $f (@fields)
    {
        $name    = $f->{"Source"}  if exists $f->{"Source"};
        $version = $f->{"Version"} if exists $f->{"Version"};
    }

    # Increment the version
    #
    if( $version =~ m/^(\d*).(\d+).(\d+)\$/ )
    {
        $version = "$1.$2.$3.1";
    }
    elsif( $version =~ m/^(\d*).(\d+).(\d+).(\d+)\$/ )
    {
        $version = "$1.$2.$3." . $4+1;
    }

    # Write a new changelog entry with the new version
    #
    system "dch --newversion $version~$distribution --urgency high --distribution $distribution Nightly build.";

    # Now add to the DEPHASH of all of the packages
    #
    my @packages;
    push @packages, $version;

    my $dep_ctl = Dpkg::Control::Info->new();
    my @control_pkgs = $dep_ctl->get_packages();
    foreach my $p (@control_pkgs)
    {
        push @packages, $p->{"Package"};
    }

    $DEPHASH{$name} = [@packages];
}


################################################################################
# Now update all of the dependencies for each project
#
for my $project (keys %DIRHASH)
{
    update_dependencies( $project );
}

# vim: ts=4 sw=4 et
