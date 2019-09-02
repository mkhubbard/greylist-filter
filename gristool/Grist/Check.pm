#!/usr/bin/perl -w
#
# file: Check.pm		
# perl module for performing simply queries against the grist greylist database.
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

package Check;

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

	if ( $verbose && $pretend ) { print "check: pretend mode not applicable, ignoring.\n"; }

	my %hash = ( 'field' => '', 'value' => '', 'verbose' => $verbose, 'pretend' => $pretend );
	bless \%hash => $package;
}

#
# checkArguments - checks / processes command arguments
#
sub checkArguments {
	my ( $self ) = shift;
	my ( @args ) = @_;

	my %fields = ( 'sender' => 'sender',
		       'recipient' => 'recipient',
		       'from' => 'sender', 
		       'to' => 'recipient',
		       'host' => 'host',
		       'sourceip' => 'address',
		       'ip' => 'address'
		     );

	if ( $#args != 1 ) { return 0; }

	if ( !exists($fields{$args[0]}) ) {
		print STDERR "check: unknown field specified '$args[0]'.\n";
		return 0;
	}

	$self->{'field'} = $fields{$args[0]};
	$self->{'value'} = $args[1];
	
	if ( $self->{'verbose'} ) { print "check: will look for '$self->{'value'}' in '$self->{'field'}' field.\n"; }

	return 1;
}

#
# pad string
#
sub resize {
	my ( $self ) = shift;
	my ( $str  ) = shift;
	my ( $len  ) = shift;


	my $idx = length($str);
	if ( $idx > $len ) {
		return substr($str,0,$len-1).'^';
	}
	return $str;
}

#
# perform - prune the database base
#
sub perform {
	my ( $self ) = shift;
	my ( $dsn  ) = shift;
	my ( $db_username ) = shift;
	my ( $db_password ) = shift;

	# couple more sanity checks, efficent aren't we?
	if ( !$self->{'field'} || !$self->{'value'} ) {
		print STDERR "check: internal error, field or value is corrupt. strange. gremlins?\n";
		return 0;
	}
	
	my $sql_clause = "WHERE $self->{'field'} = '$self->{'value'}'";

	my $dbh = DBI->connect( $dsn, $db_username, $db_password ) or 
		die('db: unable to establish a connection with the database.');

	my $sql_query = "SELECT * FROM requests $sql_clause";
	if ( $self->{'verbose'} ) { print "check: $sql_query\n"; }

	my $sth = $dbh->prepare($sql_query) or die('prune: unable to prepare database query.');
	$sth->execute();

	my $rows = 0;
	my $dead = 0;
	my $validated = 0;
	while ( my @request = $sth->fetchrow_array() ) {

		my $id = $self->resize( $request[0], 8 );
		my $sourceip   = $request[1];
		my $sourcehost = $self->resize( $request[2], 80);
		my $sender     = $self->resize( $request[3], 80);
		my $recipient  = $self->resize( $request[4], 80);
		my $seen       = $request[5];
		my $accepted   = $request[6];

		my $date = localtime $request[7];
		print "request id: $id";
		print "\n";
		print "  ip      : $sourceip\n";
		print "  host    : $sourcehost\n";
		print "  from    : $sender\n";
		print "  to      : $recipient\n";
		print "  seen    : $seen\n";
		print "  accepted: $accepted\n";
		print "  uts     : $request[7]\n";
		print "  received: $date\n"; 
		if ( $accepted > 0 ) { ++$validated; }
		if ( $seen == 0 ) { print "considered: DEAD-REQUEST\n"; ++$dead; } 
		if ( $accepted > 50 ) {
			print "considered: WHITELIST-CANDIDATE (accepted>50)\n";
		}
		my $diff = $seen - $accepted;
		if ( $diff > 5 ) {
			print "considered: IMPATIENT-SOURCE\n";
		}
		print "\n";
		++$rows;
	}

	# summary
	print "total requests: $rows   validated: $validated   dead/pending: $dead\n";
	
	if ( $self->{'verbose'} ) { print "found $rows request record(s).\n"; }

	if ( $self->{'verbose'} ) { print "check: disconnecting from database, take care.\n"; }
	$dbh->disconnect();

	return 1;
}

1;
