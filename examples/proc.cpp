/**********************************************************************************************/
/* proc.cpp						                                               				  */
/*                                                                       					  */
/* Igor Nikitin, 2012																		  */
/* MIT license			                                                  					  */
/**********************************************************************************************/

// STD
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// CREST
#include "../include/crest.h"

/**********************************************************************************************/
// strcat and strcpy - very bad, I know, that's just example


/**********************************************************************************************/
static void send_output(
	crest_connection&	conn,
	const char*			cmd )
{
	char file[ 64 ];
	snprintf( file, 64, "/tmp/proc_out_%ld", (long) &conn );
	file[ 63 ] = 0;
	
	char buf[ 1024 ];
	strcpy( buf, cmd );
	strcat( buf, " > " );
	strcat( buf, file );
	
	(void) system( buf );
	conn.send_file( file );
	remove( file );
}


/**********************************************************************************************/
GET( proc )( crest_connection& conn )
{
	send_output( conn, "ps -A ww" );
}

/**********************************************************************************************/
POST( proc )( crest_connection& conn )
{
	char buf[ 1024 ];
	strcpy( buf, conn.get_query_parameter( "cmd" ) );
	strcat( buf, " &" );
	
	send_output( conn, buf );
}

/**********************************************************************************************/
GET( proc/{pid} )( crest_connection& conn )
{
	char buf[ 1024 ];
	strcpy( buf, "cat /proc/" );
	strcat( buf, conn.get_path_parameter( 1 ) );
	strcat( buf, "/status" );
	
	send_output( conn, buf );
}

/**********************************************************************************************/
DELETE( proc/{pid} )( crest_connection& conn )
{
	char buf[ 1024 ];
	strcpy( buf, "kill " );
	strcat( buf, conn.get_path_parameter( 1 ) );
	
	send_output( conn, buf );
}

/**********************************************************************************************/
GET( proc/{pid}/cmdline )( crest_connection& conn )
{
	char buf[ 1024 ];
	strcpy( buf, "cat /proc/" );
	strcat( buf, conn.get_path_parameter( 1 ) );
	strcat( buf, "/cmdline" );
	
	send_output( conn, buf );
}

/**********************************************************************************************/
GET( proc/{pid}/limits )( crest_connection& conn )
{
	char buf[ 1024 ];
	strcpy( buf, "cat /proc/" );
	strcat( buf, conn.get_path_parameter( 1 ) );
	strcat( buf, "/limits" );
	
	send_output( conn, buf );
}

/**********************************************************************************************/
GET( * )( crest_connection& conn )
{
	conn.respond( CREST_HTTP_OK, conn.get_url(), strlen( conn.get_url() ) );
}


/**********************************************************************************************/
int main( void )
{
	if( !crest_start( "8080" ) )
		fputs( crest_error_string(), stdout );
}
