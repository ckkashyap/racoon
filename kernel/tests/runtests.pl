#!/usr/bin/perl -w


use strict;

# ------- GLOBAL VARIABLES ---------------------------


my @TESTS;

my $COMPILE_INSTRUCTION = "compile";
my $CLEAN_INSTRUCTION = "clean";
my $DEBUG_MODE = 0;


# ------- Don't change these parameters -------------

my $DEBUG_MAKE = "> /dev/null 2> /dev/null";


$DEBUG_MAKE="" if ( $DEBUG_MODE != 0 );

# ------- Command line parsing -----------------------


my  $directory = shift;
	
$directory = "./" if ( ! $directory );

die "No tests were found in directory: $directory!\n"
	if ( ! defined load_tests($directory) );


	@TESTS = sort(@TESTS);

	while ( my $test = pop @TESTS )
	{
		run_test( $test );
		clean_test( $test );
	}


exit(0);


# ----------------------------------------------------


sub run_test
{
	my ($test) = @_;

	system( "make -C $test -f ../Makefile $COMPILE_INSTRUCTION $DEBUG_MAKE" );

	if ( $? != 0 )
	{
		die "Unable to compile test: $test\n";
	}

	system( "cd $test && ./$test" );

	if ( $? != 0 )
	{
		printf("FAILED: $test\n" );
	}
	else
	{
		printf("OK: $test\n" );
	}
}

sub clean_test
{
	my ($test) = @_;
	system( "make -C $test -f ../Makefile $CLEAN_INSTRUCTION $DEBUG_MAKE" );
}






sub load_tests
{
	my ($directory) = @_;
	my $result = undef;

	opendir( DIR, $directory )
		or die "Unable to open directory: $directory\n";
	
		while ( my $filename = readdir DIR  )
		{
			next if ( $filename =~ /^[.]{1,2}$/ );
			next if ( $filename =~ /^[.]{1,2}$/ );
			next if ( ! -d $filename );

			if ( -e "$filename/IGNORE" )
			{
				print "ignoring: $filename\n";
				next;
			}

			print "WARNING: $filename does not have a make.includes" 
						if ( ! -e "$filename/make.includes" );

			next if ( ! -e "$filename/make.includes" );
	
			push @TESTS, $filename;
			$result = $filename;
		}
	
	
	closedir( DIR );
	return $result;
}




