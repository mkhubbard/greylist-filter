#!/usr/bin/perl -w
#
# file: Config.pm		
# perl module for parsing the grist greylist server configuration file.
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

package Config;

use strict;
use Carp;

#
# constructor
#
sub new {
        my ( $package  ) = shift;	
      
	my %hash = ( 'filename'    => '/etc/grist.conf',
		     'db_driver'   => '',
		     'db_path'     => '',
		     'db_name'     => '',
		     'db_host'     => '',
		     'db_port'     => '',
		     'db_username' => '',
		     'db_password' => '',
		     'rq_cooldown' => 0,
		     'rq_defer_msg'=> '');	
	bless \%hash => $package;
}

#
# setFilename - set the location and name of the config file
#
sub setFilename {
	my ( $self ) = shift;
	$self->{'filename'} = shift;

	return 1;
}

#
# getFilename - retreive the current config filename
#
sub getFilename {
	my ( $self ) = shift;
	return $self->{'filename'};
}

#
# parse - parse the current configuration file
#
sub parse {
	my ( $self ) = shift;

	open(CONF, "<$self->{'filename'}") or die('unable to open configuration file: '.$self->{'filename'});
	while (<CONF>) {
		my $line = $_; chomp($line);
		if ( !$line ) { next; } # empty, skip
		if ( $line =~ /^#/ )   { next; } # comment skip

		if ( $line !~ /.*=.*/ ) {
			print STDERR "the file '$self->{'filename'}' does not appear to be a grist configuration file.\n";
			print STDERR "please check that this is the correct file and is formatted properly.\n";
			return 0;
		}


		my ( $key, $value ) = $line =~ /(.*)=(.*)/;
		
		$key   =~ s/\s//g; # trim whitespace, this removes all spaces. 
		$value =~ s/\s//g; # see above. =)

		$key = lc($key);
		if ( exists($self->{$key}) ) {
			$self->{$key} = $value;
		} else {
			print STDERR $self->{'filename'}.": unknown key found: $key; ignoring.\n";
		}
	}
	close(CONF);

	# sqlite fix up
	if ( lc($self->{'db_driver'}) eq 'sqlite' ) {
		$self->{'db_driver'} = 'SQLite';
		$self->{'db_name'} = $self->{'db_path'}.'/'.$self->{'db_name'};
	}

	return 1;
}

#
# get configuration value
# 
sub get {
	my ( $self ) = shift;
	my ( $key )  = shift;

	if ( !exists($self->{$key}) ) {
		print STDERR "internal: request made for non-existant configuration key.";
		return -1;
	}

	return $self->{$key};
}

1;
