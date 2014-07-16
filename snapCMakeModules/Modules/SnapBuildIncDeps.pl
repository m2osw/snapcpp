#!/usr/bin/perl -w
#
die "You must specify at least 3 arguments!\n" unless $#ARGV > 1;

use Cwd;

my $pwd          = getcwd;
my $dir          = shift;
my $packagename  = shift;
my @dependencies = @ARGV;

my %DEPHASH;

for my $dep ( @dependencies )
{
	chdir( $dir . "/" . $dep );

	die "Cannot parse changelog of ".$dep."!\n" unless open( $input, "dpkg-parsechangelog --show-field Source|" );
	my $name = <$input>;
	chomp $name;

	die "Cannot parse changelog of ".$dep."!\n" unless open( $input, "dpkg-parsechangelog --show-field Version|" );
	my $version = <$input>;
	chomp $version;

	die "Cannot parse control file of ".$dep."!\n" unless open( $input, "grep Package: debian/control|" );
	my @packages;
	push @packages, $version;
	while (<$input>)
	{
		s/Package: //;
		chomp;
		push @packages, $_;
	}

	$DEPHASH{$name} = [@packages];
}

chdir( $dir . "/" . $packagename );

rename "debian/control", "debian/control.orig";
my $input_file  = "debian/control.orig";
my $output_file = "debian/control";

die "Cannot open $input_file for reading!\n"  unless open( my $input,  "<", $input_file  );
die "Cannot open $output_file for writing!\n" unless open( my $output, ">", $output_file );

while( <$input> )
{
	for my $key ( keys %DEPHASH )
	{
		my @package = @{$DEPHASH{$key}};
		my $version = $package[0];
		shift @package;
		for my $name ( @package )
		{
			s/$name \(>= [^\)]+\)/$name \(>= $version\)/;
		}
	}

	print $output $_;
}


close( $input  );
close( $output );

