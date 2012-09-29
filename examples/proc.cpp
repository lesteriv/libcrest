/**********************************************************************************************/
/* proc.cpp						                                               				  */
/*                                                                       					  */
/* Igor Nikitin, 2012																		  */
/* MIT license			                                                  					  */
/**********************************************************************************************/

// STD
#include <cstdlib>

// CREST
#include "../include/crest.h"


/**********************************************************************************************/
static void send_output(
	crest_connection&	conn,
	string		cmd )
{
	char file[ 64 ];
	snprintf( file, 64, "/tmp/proc_out_%ld", (long) &conn );
	
	system( ( cmd + " > " + file ).c_str() );
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
	send_output( conn, conn.get_query_parameter( "cmd" ) + " &" );
}

/**********************************************************************************************/
GET( proc/{pid} )( crest_connection& conn )
{
	send_output( conn, "cat /proc/" + conn.get_path_parameter( 1 ) + "/status" );
}

/**********************************************************************************************/
DELETE( proc/{pid} )( crest_connection& conn )
{
	send_output( conn, "kill " + conn.get_path_parameter( 1 ) );
}

/**********************************************************************************************/
GET( proc/{pid}/cmdline )( crest_connection& conn )
{
	send_output( conn, "cat /proc/" + conn.get_path_parameter( 1 ) + "/cmdline" );
}

/**********************************************************************************************/
GET( proc/{pid}/limits )( crest_connection& conn )
{
	send_output( conn, "cat /proc/" + conn.get_path_parameter( 1 ) + "/limits" );
}


/**********************************************************************************************/
int main( void )
{
	if( !crest_start( "8080" ) )
		fputs( crest_error_string(), stdout );
}
