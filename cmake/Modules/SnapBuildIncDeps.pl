#!/usr/bin/perl -w
#
die "usage: SnapBuildIncDeps.pl [root source directory] [project name]\n" unless $#ARGV == 1;

use Cwd;
use Dpkg::Changelog::Parse;
use Dpkg::Control::Info;
use Dpkg::Deps;
use File::Find;

my $dir          = shift;
my $projectname  = shift;

my $projectdir;
my @potential_dep_list;

# search our folder for all debian projects. Make a special note of our project
# file along the way
#
sub projects_wanted
{
    if( not stat($File::Find::name . "/debian/control") )
    {
        return;
    }

    if( $_ eq $projectname )
    {
        $projectdir = $File::Find::name;
    }
    else
    {
        push( @potential_dep_list, $File::Find::name );
    }
}
#
find( {wanted => \&projects_wanted, no_chdir => 0}, $dir );


# now open up the project debian control file and read in the depends fields
#
my %options;
my %project_dep_hash;

die "Project folder not found!" if not $projectdir;

chdir( $projectdir );

my $control       = Dpkg::Control::Info->new();
my $fields        = $control->get_source();
my @changelog     = changelog_parse(%options);
my $build_depends = deps_parse($fields->{'Build-Depends'});

for my $dep ( $build_depends->get_deps() )
{
    my $full_line = $dep;
    $dep =~ s/([^ ]+) [^\$]+/$1/;
    $project_dep_hash{$dep} = $full_line;
}


# Now peruse the list of potential depedencies we gleaned
# and alter any matches we find in the project hash.
#
my %DEPHASH;

for my $depdir ( @potential_dep_list )
{
    chdir( $depdir );

    my @fields = changelog_parse(%options);
    my $name;
    my $version;
    foreach my $f (@fields)
    {
        $name    = $f->{"Source"}  if exists $f->{"Source"};
        $version = $f->{"Version"} if exists $f->{"Version"};
    }

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

