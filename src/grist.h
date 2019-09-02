/**
 * file: grist.h
 * grist - common definitions 
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

#ifdef HAVE_CONFIG_H
	#include "config.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <syslog.h>
#include <signal.h>
#include <ctype.h> 
#include <time.h>
#include <unistd.h>

struct t_grist_config {
	char db_driver[30];
	char db_path[4096];
	char db_host[60];
	long db_port;
	char db_name[30];
	char db_username[30];
	char db_password[30];
	long rq_cooldown;
	char rq_defer_msg[1024];
};

#include "db_sql.h"

#ifdef MEMWATCH
	#include "memwatch.h"
#endif

// macros
#ifdef _DEBUG
	#define _DBG(msg,...) syslog(LOG_DEBUG, msg, ##__VA_ARGS__)
	#define _ASSERT( condition ) if ( !(condition) ) { fprintf(stderr, "[%s:%d] assertion failed.\n", __FILE__, __LINE__); }
#else
	#define _DBG(msg,...) 
	#define _ASSERT( condition ) 
#endif

#define _FREE( ptr ) if ( ptr != NULL ) { free(ptr); }

// defines
#define DB_NAME		 "grist_greylist.sqlite"
#define DB_SQLITE_PATH 	 "/tmp"
#define SQL_QUERYSTR_MAX 2048
#define INPUT_BUFFER_MAX 1024	 // 1k for 1 line of text input should be WAY more than is needed 
#define REQUEST_COOLDOWN 120 	 // wait in seconds before a req can be approved

//
// ******* NEW STUFF FOLLOWS *******
//

#define BUFFER_LEN	1024
#define QUERY_LEN	2048

// Seconds to wait before the next request will be allowed
#define COOLDOWN_SECS	120

// Greylist response actions
#define RESPOND_QUEUE	"DUNNO"
#define RESPOND_DEFER	"DEFER_IF_PERMIT"

// Message sent to client along with RESPOND_DEFER
#define DEFAULT_DEFER_MSG "Service temporarily unavailable."

#define CFG_BADDRIVER	5
#define CFG_BADPORT 	10
#define CFG_BADCOOLDOWN 15

#define CHECK_ERR     0
#define CHECK_OKAY    1
#define CHECK_COOLING 2
#define CHECK_NEW     3

struct t_request {
	char   *client_address;
	char   *client_name;
	char   *sender;
	char   *recipient;
	time_t timestamp;
} request;	

