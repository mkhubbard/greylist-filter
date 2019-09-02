#!/usr/bin/perl -w
#
# file: Prune.pm		
# perl module for pruning the grist greylist database.
#
# Copyright (C) 2004 Michael Hubbard <mhubbard@digital-fallout.com>
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

package Prune;

use strict;
use Carp;
use DBI;

#
# constructor
#
sub new {
        my ( $package  ) = shift;	
	my ( $verbose  ) = shift;
	my ( $pretend  ) = shift;
	if ( !$verbose ) { $verbose = 0; } else { $verbose = 1; };
	if ( !$pretend ) { $pretend = 0; } else { $pretend = 1; };
	my %hash = ( 'age' => -1, 'remove_dead' => 0, 'verbose' => $verbose, 'pretend' => $pretend );
	bless \%hash => $package;
}

#
# checkArguments - checks / processes command arguments
#
sub checkArguments {
	my ( $self ) = shift;
	my ( @args ) = @_;

	if ( $#args < 0 || $#args > 1 ) { return 0; }
	if ( $args[0] !~ /^[0-9]*[smhd]$/ ) {
		print STDERR "prune: '$args[0]' is not a valid age format.\n";
		return 0;
	}

	if ( $args[1] && (lc($args[1]) eq 'dead-requests') ) {
		if ( $self->{'verbose'} ) { print STDOUT "prune: will remove requests with a seen count of zero.\n"; }
		$self->{'remove_dead'} = 1;
	}

	my ( $value, $type ) = $args[0] =~ /^([0-9]*)([smhd])$/;

	my $seconds = 0;
	if ( $type eq 'd' ) { $seconds = $value * 86400; }
	if ( $type eq 'h' ) { $seconds = $value * 3600; }
	if ( $type eq 'm' ) { $seconds = $value * 60; }
	if ( $type eq 's' ) { $seconds = $value; }

	if ( $seconds < 3600 ) {
		# just in case they aren't sure what they are doing, beside typo's happen to.
		if ( $self->{'verbose'} ) { print "prune: checking for sanity of user.\n"; }
		print STDOUT "WARNING: The age value specified will result in removal of nearly all\n";
		print STDOUT "request records. Are you sure you want to do this? [yes/no]: ";

		my $continue = 0;
		while ( !$continue ) {
			my $user_input = <STDIN>; chomp($user_input);
			$user_input = lc($user_input);
		
			if ( $user_input eq 'no' ) {
				print "\nWhew! Glad I asked, I'll exit now.\n";
				exit(0);
			} elsif( $user_input eq 'yes' ) {
				$continue = 1;
			} else {
				print "\n\nHuh? Please type YES or NO.\n\n";
				print "Would you like to continue? [yes/no]: ";
			}
		}
		print "\n";
	}

	$self->{'age'} = time() - $seconds;

	return 1;
}

#
# perform - prune the database base
#
sub perform {
	my ( $self ) = shift;
	my ( $dsn  ) = shift;
	my ( $db_username ) = shift;
	my ( $db_password ) = shift;


	my $sql_clause = '';
	if ( $self->{'remove_dead'} ) {
		$sql_clause = 'AND seen = 0';
	}

	# couple more sanity checks, efficent aren't we?
	if ( $self->{'age'} < 0 ) {
		print STDERR "prune: internal error, age is less than zero. strange. gremlins?\n";
		return 0;
	}

	my $dbh = DBI->connect( $dsn, $db_username, $db_password ) or 
		die('db: unable to establish a connection with the database.');

	if ( $self->{'pretend'} ) {
		my $sql_query = "SELECT id FROM requests WHERE timestamp <= $self->{'age'} $sql_clause";
		if ( $self->{'verbose'} ) { print "prune: $sql_query\n"; }

		my $sth = $dbh->prepare($sql_query) or die('prune: unable to prepare database query.');
		$sth->execute();

		my $rows = 0;
		while ( my @junk = $sth->fetchrow_array() ) {
			++$rows;
		}
		print STDOUT "I would have removed $rows request record(s), but I didn't.\n";
	} else {
		my $sql_query = "DELETE FROM requests WHERE timestamp <= $self->{'age'} $sql_clause";
		if ( $self->{'verbose'} ) { print "prune: $sql_query\n"; }
		
		my $rows = $dbh->do($sql_query) or die('prune: unable to do query. records not removed.');

		if ( $self->{'verbose'} ) { print "prune: removed $rows request(s).\n"; }
		if ( $self->{'verbose'} ) { print "prune: vacuuming database. *click* vrhmmmmm.\n"; }
		$dbh->do('VACUUM');
	}

	if ( $self->{'verbose'} ) { print "prune: disconnecting from database, take care.\n"; }
	$dbh->disconnect();

	return 1;
}

1;
