/**
 * file: main.c
 * grist - greylist policy server designed for postfix
 *
 * Copyright (C) 2005 Michael Hubbard <mhubbard@digital-fallout.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 */

#include "grist.h"

void grist_safe_exit( void );

void grist_cleanup() {
	_DBG("grist_cleanup(): executed.");

	// cleanup
	_FREE(request.client_address);
	_FREE(request.client_name);
	_FREE(request.sender);
	_FREE(request.recipient);
}

void trap_sigint( int sig ) {
	syslog(LOG_INFO, "greylist: caught sigint, terminating ...");	
	syslog(LOG_DEBUG, "greylist: caught sigint, terminating ...");	
	grist_safe_exit();	
}

void grist_safe_exit( void ) {
	syslog(LOG_ERR, "greylist: performing safe exit due to internal error.");
	syslog(LOG_DEBUG, "greylist: performing safe exit due to internal error.");
	fprintf(stdout,"action=%s\n", RESPOND_QUEUE);
	fprintf(stdout,"\n");
	grist_cleanup();
	exit(0);
}

int get_policy_attributes( struct t_request *request) {

	char *input, *ret;
	int  client_done = 0;
	char eol[] 	 = "\n"; 

	char *key, *value;
	input = (char *)malloc( sizeof(char)*INPUT_BUFFER_MAX );
	
	int  ignore_client = 0; 
	while ( !client_done ) {

		bzero(input, INPUT_BUFFER_MAX);
	 	ret = fgets( input, INPUT_BUFFER_MAX, stdin );	
		if ( !ret ) {
			if ( ret == NULL ) {
				perror("client_read(null)");
			} else {		
				perror("client_read");	
			}
			break;	
		}

		if ( strcmp(input, eol) == 0 ) {
			_DBG("received end of policy request.");
			client_done = 1; // just in case
			break;
		}
	
		if ( ignore_client ) { continue; }

		_DBG("received: %s", input);

		if ( index(input,'=') == NULL ) {
			_DBG("malformed request attribute, skipping.");
			continue;
		}

		key = strtok(input, "=");
		value = strtok(NULL, "=");
				
		_DBG("attribute parsed: key=%s value=%s", key, value);

		// check request type (if we want to check for protocol sanity
		// this string should always come first from the client)	
		if ( strcmp(key,(char*)"request" ) == 0 ) {
			if ( strcmp(value, (char *)"smtpd_access_policy") != 0 ){
				// asking us for a response we don't handle
				syslog(LOG_WARNING, "unsupported request: %s, ignoring client.", value);
				ignore_client = 1;	
			}	
		}
			
		// save keys we want to work with
		if (strcmp(key,(char*)"sender") == 0 ) {
			request->sender = strdup(value); 
		} else 
		if (strcmp(key,(char*)"recipient") == 0 ) {
			request->recipient = strdup(value);	
		} else
		if (strcmp(key,(char*)"client_address") == 0 ) {
			request->client_address = strdup(value);	
		} else
		if (strcmp(key,(char*)"client_name") == 0 ) {
			request->client_name = strdup(value);	
		}
			
	}

	_FREE( input );

	request->timestamp = time(NULL);

	return 0;

}

/**
 * main
 */
int main( int argc, char **argv ) {

	struct t_grist_config config;
	int action;
	int perform_db_setup = 0;
	char *opt_config = "/usr/local/etc/grist.conf";

	// install signal traps
	signal(SIGINT, trap_sigint);

	// open handle to syslogd
	openlog("grist", LOG_PID|LOG_NDELAY|LOG_CONS, LOG_MAIL);

	// quick and dirty command line checker thingy..
	// all other options reside in /usr/local/etc/grist.conf
	int idx = 0;
	int valid_opt = 0;
	while( idx < argc ) {
		if (strcmp(argv[idx],"setup")==0) {
			valid_opt = 1;
			perform_db_setup = 1;
		} else
		if (strcmp(argv[idx],"--conf")==0) {
			valid_opt = 1;
			if ( argc >= (idx+1) ){
				opt_config = argv[++idx];
			}
		} else
		if (strcmp(argv[idx],"--version")==0) {
			printf("%s\n", VERSION);
			exit(0);
		}

		++idx;
	}

	if ( (argc > 1) && (valid_opt == 0) ) {
		        printf("usage: grist [--version,--conf <filename>,setup]\n");
			printf("Try 'man grist' for more information.\n");
			exit(1);
	}

	// parse configuration file
	if ( !parse_config_file(opt_config, &config) ) {
		fprintf(stderr,"unable to parse configuration file: %s\n", opt_config);
		fprintf(stderr,"try passing the correct path of your configuration file with the --conf option\n.");
		grist_cleanup();
		exit(1);
	}

	if ( perform_db_setup ) {
		// create database table structure
		dbi_conn conn = db_open_database( config );
		
		if ( conn == NULL ) {
			fprintf(stderr,"error establishing a connection with the database.\n");
			grist_cleanup();
			exit(1);
		}
		
		db_create_structure(conn);
		db_close_database(conn);
		exit(0);
	} 

	// read from stdin (client)
	get_policy_attributes(&request);

	// make sure we have everything before passing over to the database
	if ( request.client_address == NULL ||
	     request.client_name    == NULL ||
	     request.sender         == NULL ||
	     request.recipient      == NULL 
	   ) 
	{
		syslog(LOG_WARNING,"skipping lookup, received incomplete request criteria.");
		grist_safe_exit();
	}

	dbi_conn conn = db_open_database(config);
	action = db_check_request(conn, request, config);
	db_close_database(conn);

	// NOTE: The following redundant code needs to be refactored before 1.x
	//	 let's just focus on needed features atm.

	// log results
	switch ( action ) {
		case CHECK_OKAY: syslog(LOG_INFO,"greylist: action=%s; client=%s from=<%s> to=<%s>", 
					   RESPOND_QUEUE, request.client_address, request.sender, request.recipient);
				    break;
		case CHECK_COOLING: syslog(LOG_INFO,"greylist: action=%s, cooling; client=%s from=<%s> to=<%s>",
					   RESPOND_DEFER, request.client_address, request.sender, request.recipient);
				    break;
		case CHECK_NEW    : syslog(LOG_INFO,"greylist: action=%s, new; client=%s from=<%s> to=<%s>",
					   RESPOND_DEFER, request.client_address, request.sender, request.recipient);
				    break;
		default: syslog(LOG_INFO,"greylist: action=%s, internal error; client=%s from=<%s> to=<%s>",
					   RESPOND_QUEUE, request.client_address, request.sender, request.recipient);
	}
	
	// notify the client of our decision
	switch ( action ) {
		case CHECK_ERR    : 
		case CHECK_OKAY   : fprintf(stdout,"action=%s\n", RESPOND_QUEUE);
				    break;
		case CHECK_COOLING: 
		case CHECK_NEW    : if ( config.rq_defer_msg == NULL ) {
			  	  	fprintf(stdout,"action=%s %s\n", RESPOND_DEFER, DEFAULT_DEFER_MSG);
			       	    } else {
				  	fprintf(stdout,"action=%s %s\n", RESPOND_DEFER, config.rq_defer_msg);
			       	    }
			       	    break;
		default: syslog(LOG_DEBUG|LOG_ERR,"got invalid response code, internal error allowing anyway.");
			 fprintf(stdout,"action=%s\n", RESPOND_QUEUE);
	}

	grist_cleanup();
	
	_DBG("grist exiting.");		

	// tell client we are finished
	fprintf(stdout,"\n");
	
	return 0;
}


