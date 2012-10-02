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
#ifdef _MSC_VER
#pragma warning( disable: 4996 )
#endif // _MSC_VER

/**********************************************************************************************/
extern "C" int gethostname( char*, size_t );


/**********************************************************************************************/
static void send_output(
	crest_connection&	conn,
	const char*			cmd )
{
	char file[ 64 ];
	sprintf( file, "/tmp/proc_out_%ld", (long) &conn );
	
	char buf[ 1024 ];
	strcpy( buf, cmd );
	strcat( buf, " > " );
	strcat( buf, file );
	system( buf );
	
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
	char buf[ 65536 ];
	strcpy( buf, conn.get_query_parameter( "cmd" ) );
	strcat( buf, " &" );
	
	send_output( conn, buf );
}

/**********************************************************************************************/
GET( proc/{pid} )( crest_connection& conn )
{
	char buf[ 65536 ];
	strcpy( buf, "cat /proc/" );
	strcat( buf, conn.get_path_parameter( 1 ) );
	strcat( buf, "/status" );
	
	send_output( conn, buf );
}

/**********************************************************************************************/
DELETE( proc/{pid} )( crest_connection& conn )
{
	char buf[ 65536 ];
	strcpy( buf, "kill " );
	strcat( buf, conn.get_path_parameter( 1 ) );
	
	send_output( conn, buf );
}

/**********************************************************************************************/
GET( proc/{pid}/cmdline )( crest_connection& conn )
{
	char buf[ 65536 ];
	strcpy( buf, "cat /proc/" );
	strcat( buf, conn.get_path_parameter( 1 ) );
	strcat( buf, "/cmdline" );
	
	send_output( conn, buf );
}

/**********************************************************************************************/
GET( proc/{pid}/limits )( crest_connection& conn )
{
	char buf[ 65536 ];
	strcpy( buf, "cat /proc/" );
	strcat( buf, conn.get_path_parameter( 1 ) );
	strcat( buf, "/limits" );
	
	send_output( conn, buf );
}

/**********************************************************************************************/
GET( * )( crest_connection& conn )
{
	char host[ 1024 ];
	gethostname( host, 1024 );
	
	char buf[ 65536 ];
	strcpy( buf, conn.get_url() );
	strcat( buf, " at " );
	strcat( buf, host );
	
	conn.respond( CREST_HTTP_OK, buf, strlen( buf ) );
}


/**********************************************************************************************/
int main( void )
{
	if( !crest_start( "8080" ) )
		fputs( crest_error_string(), stdout );
}
