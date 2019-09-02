#!/usr/bin/perl -w
#
# file: gristool.pl 
# grist - greylist policy server database managment tool
#
# Copyright (C) 2005 Michael Hubbard <mhubbard@digital-fallout.com> 
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
#
# Authors: Michael Hubbard <mhubbard@digital-fallout.com>
#

use strict;
use Getopt::Long qw(:config gnu_getopt );

use Grist::Config;
use Grist::Prune;
use Grist::Check;

my $appname = `basename $0`; chomp($appname);
my $appver  = '0.1-DEVEL';
my $helpstr = "Try '$appname --help' for more information.";

my $grist_conf = '../src/grist.conf';
my $config = new Config();

my %opts = ( 'verbose' => 0, 'pretend' => 0, 'conf' => $grist_conf );
my @args = ( );

# trap warn signal
$SIG{__WARN__} = sub {
	if ( $_[0] =~ /^Unknown option:|^Option/ ) {
		# getopt option error, make it pretty and use out usage()
		my $message = lc($_[0]); chomp($message);
		usage($helpstr, "$appname: $message");
		return
	} else {
		# handle warning as usual
		warn @_;
	}
};

# version - displays application version and exits
#
sub version {
	print "$appname v$appver\n";
}

# usage - displays command line usage help
#
sub usage {
	my ( $message ) = shift;
	my ( $badopt  ) = shift;
	my ( $cmd     ) = shift;

	if ( $badopt ) { print "$badopt\n"; }
	
	if ( !$cmd ) { 
		$cmd = ''; 
		print "Usage: $appname [OPTIONS] [COMMAND] arguments...\n";
	};

	if ( $cmd eq 'help' ) {
		print "Usage: $appname [OPTIONS] [COMMAND] arguments...\n";
		print "Grist greylist database management tool.\n";
		print "\n";
		print "Options:\n";
		print "  -c, --conf    \tfull path and name of the grist configuration file.\n";
		print "  -p, --pretend \tdisplay what would be done, will not alter the database.\n";
		print "  -v, --verbose \tbe verbose.\n";
		print "  -V, --version \tdisplay version information and exit.\n";
		print "\n";
		print "Commands:\n";
		print "  prune AGE <dead-requests>\tremove requests older than AGE.\n";
		print "  check FIELD VALUE        \tquery FIELD for VALUE and display matches.\n";
		print "\n";
	} elsif ( $cmd eq 'prune' ) {
		print "Usage: $appname [OPTIONS] prune AGE <dead-requests>\n";
		print "$helpstr\n";
	} elsif ( $cmd eq 'check' ) {
		print "Usage: $appname [OPTIONS] check FIELD VALUE\n";
		print "$helpstr\n";
	}
	
	if ( $cmd ne '' ) {
		print "\n";
		print "For more detailed information try `man gristool`.\n";
		print "\n";
		print "Report bugs to <bugs\@digital-fallout.com>.\n";
	}		
	
	if ( $message ) { print "$message\n"; }

	exit(0);
}

# save option arguments
#
sub storeArgument {
	$args[++$#args] = shift;
}

#
# end subs, application code begins
#

# no arguments, show usage.
if ( $#ARGV == -1 ) { usage( $helpstr ); }
	
GetOptions(\%opts,
	   'conf|c=s',
	   'verbose|v',
	   'pretend|p',
	   'V|version', => sub { version(); },
	   'help|h|?' => sub { usage('','','help'); },
	   '<>' => \&storeArgument
	  );

if ( $opts{'verbose'} ) {
	print "$appname: welcome, guess all went well so far.\n";
	print "$appname: obviously running in verbose mode.\n";
	if ( $opts{'pretend'} ) { print "$appname: running in pretend mode.\n"; }
	print "$appname: using grist configuration: $opts{'conf'}\n";
}

# final sanity check on our wanted command in $args[0]
if ( !$args[0] ) { usage($helpstr); } 

# check our $opts{'conf'}
if ( ! -f $opts{'conf'} ) { print STDERR "error: configuration file not found '$opts{'conf'}'.\n"; exit(1); }

$config->setFilename($opts{'conf'});
if ( !$config->parse() ) {
	exit(1);
}

if ( $opts{'verbose'} ) { print "$appname: checking for existance of the sqlite database.\n"; }
if ( $config->get('db_driver') eq 'SQLite' && (! -f $config->get('db_name')) ) {
	print STDERR "error: greylist database '".$config->get('db_name')."' does not exist.\n";
	exit(0);
}

# build database connection string
my $dsn = sprintf('DBI:%s:dbname=%s;host=%s;port=%s',
			$config->get('db_driver'),
			$config->get('db_name'),
			$config->get('db_host'),
			$config->get('db_port')
		  );

if ( $opts{'verbose'} ) { print "$appname: using $dsn\n"; }

# check for command
if ( $opts{'verbose'} ) {
	print "$appname: command arguments follow.\n";
	for( my $idx=0; $idx<=$#args; $idx++ ) {
		print "$appname: \$args[$idx] = $args[$idx]\n";
	}
}

my $command = shift(@args);
if ( lc($command) eq 'prune' ) {
	
	if ( $opts{'verbose'} ) { print "$appname: command 'prune' requested, handing over control ...\n"; }
	# check for prune
	my $prune = new Prune( $opts{'verbose'}, $opts{'pretend'} );
	if ( !$prune->checkArguments(@args) )  { usage('','','prune'); }

	if (! $prune->perform( $dsn, $config->get('db_username'), $config->get('db_password') ) ) {
		if ( $opts{'verbose'} ) { print "$appname: exiting with error condition.\n"; }
		exit(1);
	} 
	
} elsif ( lc($command) eq 'check' ) {
	
	if ( $opts{'verbose'} ) { print "$appname: command 'check' requested, handing over control ...\n"; }
	my $check = new Check( $opts{'verbose'}, $opts{'pretend'} );
	if ( !$check->checkArguments(@args) ) { usage('','','check'); }

	if ( !$check->perform($dsn, $config->get('db_username'), $config->get('db_password')) ) {
		if ( $opts{'verbose'} ) { print "$appname: exiting with error condition.\n"; }
		exit(1);
	} 

} else {
	usage($helpstr, "error: unknown command requested '$args[0]'");
}

if ( $opts{'verbose'} ) { print "$appname: exiting, success.\n"; }

exit(0);
