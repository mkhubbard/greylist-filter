/**
 * file: cfg_parser.c
 * grist - configuration file parser
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

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#include "grist.h"

#define STR_MAX 2048

char* s_trim( char* string ) {
	char *p1, *p2;

	if ( string == NULL ) { return string; }

	// remove trailing whitespace
	p2 = string + strlen(string)-1;
	while (isspace(*p2) && p2>string) {
		*p2--='\0';
	}
	// remove leading whitespace
	p1 = string;
	while ( isspace(*p1) ) {
		p1++;
	}

	return p1;
}

int parse_config_file( char *filename, struct t_grist_config* grist_cfg ) {
	FILE *fh;
	char *line, *p;
	char *key, *value;
	int  parse_error = 0;

	if ( (fh = fopen(filename, "r")) == NULL ) {
		perror("config");
		return 0;
	}

	// initialize structure
	grist_cfg->db_driver[0]    = '\0';
	grist_cfg->db_path[0] 	   = '\0';
	grist_cfg->db_name[0]	   = '\0';
	grist_cfg->db_host[0] 	   = '\0';
	grist_cfg->db_port 	   = 0;
	grist_cfg->db_username[0]  = '\0';
	grist_cfg->db_password[0]  = '\0';
	grist_cfg->rq_cooldown     = 120;
	grist_cfg->rq_defer_msg[0] = '\0';

	line = (char *)malloc( sizeof(char)*STR_MAX );

	while ( !feof(fh) ) {
		bzero( line, sizeof(line) );
		fgets(line, STR_MAX, fh);
		p = s_trim(line);
		if ( *p=='#' || *p=='\0') { continue; }

		if ( index(p,'=') == NULL ) { continue; }

		key   = s_trim( strtok(p, "=") );
		value = s_trim( strtok(NULL,"=") );
		
		if ( value == NULL ) { continue; }
	
		if (strcmp(key,"db_driver")==0) {
			int dest_size = sizeof(grist_cfg->db_driver);
			value[dest_size]='\0'; 
			strncpy(grist_cfg->db_driver, value, dest_size);
		} else
		if (strcmp(key,"db_path")==0) {
			int dest_size = sizeof(grist_cfg->db_path);
			value[dest_size]='\0'; 
			strncpy(grist_cfg->db_path, value, dest_size);
		} else 
		if (strcmp(key,"db_name")==0) {
			int dest_size = sizeof(grist_cfg->db_name);
			value[dest_size]='\0'; 
			strncpy(grist_cfg->db_name, value, dest_size);
		} else 
		if (strcmp(key,"db_host")==0) {
			int dest_size = sizeof(grist_cfg->db_host);
			value[dest_size]='\0'; 
			strncpy(grist_cfg->db_host, value, dest_size);
		} else 
		if (strcmp(key,"db_port")==0) {
			long tmp_port = strtol( value, NULL, 10 );
			if ( tmp_port <= 0 ) {
				//errmsg = (char *)errstr_badport;
				//fprintf(stderr,"config: invalid port value.\n");
				parse_error = CFG_BADPORT;
			}
			grist_cfg->db_port = tmp_port;
		} else 
		if (strcmp(key,"db_username")==0) {
			int dest_size = sizeof(grist_cfg->db_username);
			value[dest_size]='\0'; 
			strncpy(grist_cfg->db_username, value, dest_size);
		} else 
		if (strcmp(key,"db_password")==0) {
			int dest_size = sizeof(grist_cfg->db_password);
			value[dest_size]='\0'; 
			strncpy(grist_cfg->db_password, value, dest_size);
		} else 
		if (strcmp(key,"rq_cooldown")==0) {
			long tmp_cooldown = strtol(value, NULL, 10 );
			if ( tmp_cooldown <= 0 ) {
				//fprintf(stderr,"config: invalid request cooldown value.\n");
				parse_error = CFG_BADCOOLDOWN;
			}
			grist_cfg->rq_cooldown = tmp_cooldown;
		} else 
		if (strcmp(key,"rq_defer_msg")==0) {
			int dest_size = sizeof(grist_cfg->rq_defer_msg);
			value[dest_size]='\0'; // ensure we have a null at last char
			strncpy(grist_cfg->rq_defer_msg, value, dest_size);
		} 
	
	}

	_FREE(line);

	// check for previous error.
	if ( parse_error > 0 ) { return parse_error; }

	// check for sanity of retrieved values
	if ( grist_cfg->db_driver == NULL ) {
		return CFG_BADDRIVER;
	} 
	
	if ( grist_cfg->db_host == NULL ) {
		strcpy(grist_cfg->db_host,"127.0.0.1");
	} 

	if ( grist_cfg->db_port == 0 ) {
		if (strcmp(grist_cfg->db_driver,"mysql")==0) {
			grist_cfg->db_port = 3306;
		} else 
		if (strcmp(grist_cfg->db_driver,"pgsql")==0) {
			grist_cfg->db_port = 5432;
		}
	}

	fclose(fh);

	return 1;
} // parse_config_file
