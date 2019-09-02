/**
 * file: db_sql.c
 * grist - sql database access routienes 
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

#include <stdarg.h>

#include "grist.h"

// SQL query string templates
char *sql_insert_req = "INSERT INTO requests (address, hostname, sender, recipient, seen, accepted, timestamp) VALUES(%s,%s,%s,%s,'0','0','%d')";
char *sql_update_req = "UPDATE requests SET seen='%d', accepted='%d' WHERE id='%d'"; 
char *sql_select_req = "SELECT id, seen, accepted, timestamp FROM requests WHERE address=%s AND sender=%s AND recipient=%s";

#define MAX_QUERY_ATTEMPTS	10

/*
 * +---------------+----------------------+
 * | FIELD	   | TYPE (SQLite)        |
 * +---------------+----------------------+
 * | id 	   | INTEGER PRIMARY KEY  |
 * | address	   | TEXT		  |
 * | hostname      | TEXT		  |
 * | sender	   | TEXT		  |
 * | recipient     | TEXT		  |
 * | seen	   | INTEGER		  |
 * | accepted      | INTEGER		  |
 * | timestamp     | INTEGER		  |
 * +---------------+----------------------+
 */

char *sql_create_sqlite = "CREATE TABLE requests ( id INTEGER PRIMARY KEY, address TEXT, hostname TEXT, sender TEXT, recipient TEXT, seen INTEGER, accepted INTEGER, timestamp INTEGER)";
char *sql_create_mysql  = "CREATE TABLE requests ( id INTEGER PRIMARY KEY AUTO_INCREMENT, address TEXT, hostname TEXT, sender TEXT, recipient TEXT, seen INTEGER, accepted INTEGER, timestamp INTEGER)";
char *sql_create_pgsql  = "CREATE TABLE requests ( id SERIAL, address TEXT, hostname TEXT, sender TEXT, recipient TEXT, seen INTEGER, accepted INTEGER, timestamp INTEGER)";

void dbi_error_handler( dbi_conn *conn, void *u_arg ) {
	_ASSERT( conn != NULL );	
	
	const char *errmsg;
	int errno;
	
	errno = dbi_conn_error(conn, &errmsg);
	syslog(LOG_DEBUG|LOG_ERR, "dbi: code=%d msg=%s", errno, errmsg);
}

dbi_conn* db_open_database( struct t_grist_config config ) {
	dbi_conn   conn;
	dbi_result result;
	dbi_driver driver;

	int numdrivers;

	// initialize libdbi
	numdrivers = dbi_initialize(NULL);
	if ( numdrivers < 0 ) {
		syslog(LOG_DEBUG|LOG_ERR, "dbi: libdbi initialization failed.");
		return NULL;
	} 
	else if ( numdrivers == 0 ) {
		syslog(LOG_DEBUG|LOG_ERR, "dbi: no database drivers found.");
		return NULL;
	}

	// get a database handle 
	if ( ( conn = dbi_conn_new(config.db_driver) ) == NULL ) {
		syslog(LOG_DEBUG|LOG_ERR, "dbi: unable to load '%s' driver.", config.db_driver);
		return NULL;
	}

	
	// using the callback handler seemed like a good idea, however with sqlite
	// it doesn't seem to return the error messages or code numbers.
	
	// register error handler callback
	dbi_conn_error_handler(conn, (void *)dbi_error_handler, NULL);

	dbi_conn_set_option(conn, "dbname", config.db_name);
	if ( strcmp(config.db_driver,"sqlite")==0 ) {
		dbi_conn_set_option(conn, "sqlite_dbdir", config.db_path);
	} else if ( (strcmp(config.db_driver,"mysql")==0) || (strcmp(config.db_driver,"pgsql")==0) ) {
		dbi_conn_set_option(conn, "host", config.db_host);
		dbi_conn_set_option_numeric(conn, "port", config.db_port);
		
		dbi_conn_set_option(conn, "dbname", config.db_name);
		
		dbi_conn_set_option(conn, "username", config.db_username);
		dbi_conn_set_option(conn, "password", config.db_password);
	} else {
		fprintf(stderr, "unsupported database driver. program will most likely crash soon ...\n");
	}

	if ( dbi_conn_connect(conn) != 0) {
		return NULL;
	}

	return conn;
}

int db_close_database( dbi_conn *conn ) {
	_ASSERT( conn != NULL );

	dbi_conn_close(conn);
	dbi_shutdown();

	return 1;
}
	

int db_create_structure( dbi_conn *conn ) {
	_ASSERT( conn != NULL );

	const char *sql_create_str;
	const char *driver_name;
	dbi_driver cur_driver;
	dbi_result result;

	// get connection driver name
	cur_driver  = dbi_conn_get_driver( conn );
	driver_name = dbi_driver_get_name( cur_driver );

	if ( strcmp(driver_name,"sqlite") == 0 ) {
		sql_create_str = sql_create_sqlite;
	} else if( strcmp(driver_name,"mysql") == 0 ) {
		sql_create_str = sql_create_mysql;
	} else if( strcmp(driver_name,"pgsql") == 0 ) {
		sql_create_str = sql_create_pgsql;
	} else {
		fprintf(stderr, "error: cannot create database, operation not implemented for driver: %s\n", driver_name);
		return -1;
	}
	
	result = dbi_conn_query( conn, sql_create_str );
	if ( result == NULL ) {
		fprintf(stderr, "fatal: unable to create structure, have you already initialized the database?\n");
		return 1;
	}

	return 0;
}

char *db_build_query_string( char *fmtstr, ... ) {
	int n, size = 20;
	char *p;
	va_list ap;

	if ( (p=(char *)malloc(sizeof(char)*size)) == NULL)  return NULL;

	while (1) {
		va_start(ap, fmtstr);
		n = vsnprintf(p, size, fmtstr, ap);
		va_end(ap);

		// if we had enough space, we are done.
		if ( n > -1 && n < size ) return p;

		// check size requirement
		if ( n > -1 ) {
			size = n+1;
		} else {
			size *= 2;
		}

		// allocate more space
		if ( (p=realloc(p,size)) == NULL) return NULL;
	}
}

int db_check_request(dbi_conn *conn, struct t_request request, struct t_grist_config config) {
	_ASSERT( conn != NULL );

	dbi_result result;
	long r_id, r_seen, r_timestamp, r_accepted;
	int  return_code;
	char *query_str;
	char *q_client_address, *q_client_name, *q_sender, *q_recipient;

	// sanitize input strings
	dbi_driver driver = dbi_conn_get_driver(conn);
	dbi_driver_quote_string_copy(driver, request.client_address, &q_client_address);
	dbi_driver_quote_string_copy(driver, request.client_name, &q_client_name);
	dbi_driver_quote_string_copy(driver, request.sender, &q_sender);
	dbi_driver_quote_string_copy(driver, request.recipient, &q_recipient);

	// in a perfect world the API would conform to it's documentation. apparently you
	// cannot use printf style stuff with dbi_conn_query
	//result = dbi_conn_query(conn, sql_select_req, request->client_address, request->sender, request->recipient);

	// build query string
	query_str = db_build_query_string(sql_select_req, q_client_address, q_sender, q_recipient);
	_DBG("dbi: %s", query_str);
	result = dbi_conn_query(conn, query_str);
	_FREE(query_str);
	if ( result == NULL ) {
		syslog(LOG_DEBUG|LOG_ERR, "dbi: unable to query database.");
		return CHECK_ERR;
	}

	int result_rows = dbi_result_get_numrows(result);
	_DBG("query result rows = %d", result_rows);

	if ( result_rows > 0 ) {
		_DBG("record exists, performing check.");

		// exists
 		dbi_result_next_row( result );	
		
		r_id        = dbi_result_get_long(result, "id");
		r_seen      = dbi_result_get_long(result, "seen");
		r_accepted  = dbi_result_get_long(result, "accepted");
		r_timestamp = dbi_result_get_long(result, "timestamp");
	
		dbi_result_free( result );
	
		// increment 'seen' count
		++r_seen;
	
		int elapsed = request.timestamp - r_timestamp;		
		_DBG("request has been on ice for %d second(s) valid at %d second(s)", elapsed, config.rq_cooldown);
		if ( elapsed < config.rq_cooldown ) {
			// defer, cooling
			_DBG("record still too hot.");
			return_code = CHECK_COOLING;	
		} else {
			// permit, cool.
			++r_accepted;
			_DBG("record accepted.");
			return_code = CHECK_OKAY;	
		}

		// update record 
		query_str = db_build_query_string(sql_update_req, r_seen, r_accepted, r_id);
		_DBG("dbi: %s", query_str);

		// this nasty 'retry loop' is to prevent issues with sqlite locking, i think.
		int dbi_attempts  = 0;
		int query_success = 0;
		while ( dbi_attempts < MAX_QUERY_ATTEMPTS && query_success == 0 ) {
			_DBG("dbi: attempting update #%d", dbi_attempts);
			result = dbi_conn_query(conn, query_str);
			if ( result != NULL ) {
				query_success = 1;
				break; 
			}
			sleep(1);
			++dbi_attempts;
		}

		if ( !query_success ) {
			syslog(LOG_DEBUG,"dbi: warning unable to update counts for record id=%d", r_id);
			syslog(LOG_ERR,"dbi: warning unable to update counts for record id=%d", r_id);
			return CHECK_ERR;
		}
		/*
		result = dbi_conn_query(conn, query_str);
		if ( result == NULL ) {
			syslog(LOG_DEBUG|LOG_ERR,"dbi: warning unable to update counts for record id=%d", r_id);
		}
		*/
		_FREE(query_str);
	} else {
		_DBG("record not found, adding to database");
	
		// new
		query_str = db_build_query_string(sql_insert_req, q_client_address, q_client_name, q_sender, q_recipient, request.timestamp);
		_DBG("dbi: %s", query_str);
		
		// this nasty 'retry loop' is to prevent issues with sqlite locking, i think.
		int dbi_attempts  = 0;
		int query_success = 0;
		while ( dbi_attempts < MAX_QUERY_ATTEMPTS && query_success == 0 ) {
			_DBG("dbi: attempting insert #%d", dbi_attempts);
			result = dbi_conn_query(conn, query_str);
			if ( result != NULL ) {
				query_success = 1;
				break;
			}
			sleep(1);
			++dbi_attempts;
		}

		if ( !query_success ) {
			syslog(LOG_DEBUG,"dbi: error inserting new request record.");
			syslog(LOG_ERR,"dbi: error inserting new request record.");
			return CHECK_ERR;
		}

		/* 
		result = dbi_conn_query(conn, query_str);
		if ( result == NULL ) {
			syslog(LOG_DEBUG|LOG_ERR,"dbi: error inserting new request record.");
		}
		*/

		return_code = CHECK_NEW;
		_FREE(query_str);
	}

	if ( result != NULL ) { dbi_result_free(result); }

	// memwatch reports these as WILD since memory is allocated by libdbi they still
	// must be released by us.
	_FREE(q_client_address);
	_FREE(q_client_name);
	_FREE(q_sender);
	_FREE(q_recipient);
	
	return return_code;
}
