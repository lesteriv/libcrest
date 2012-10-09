/**********************************************************************************************/
/* server.cpp								                                				  */
/*                                                                       					  */
/* Igor Nikitin, 2012																		  */
/* MIT license			                                                  					  */
/**********************************************************************************************/

// STD
#include <stdio.h>

// CREST
#include "../include/crest.h"

/**********************************************************************************************/
static const char	g_default_doc[]	= "/var/log/syslog";
static const char	g_root[]		= "/var/log";

/**********************************************************************************************/
static const size_t	ROOT_LEN		= sizeof( g_root ) - 1;
static const size_t	URL_LEN			= 8192;
static const size_t	BUF_LEN			= URL_LEN + ROOT_LEN + 1;


//////////////////////////////////////////////////////////////////////////
// handlers
//////////////////////////////////////////////////////////////////////////


/**********************************************************************************************/
GET( * )( cr_connection& conn )
{
	if( conn.get_url()[ 1 ] )
	{
		// Use "C" strings instead of std::string to not link with libstdc++
		char path[ BUF_LEN ];
		memcpy( path, g_root, ROOT_LEN );
		strncpy( path + ROOT_LEN, conn.get_url(), URL_LEN );
		path[ BUF_LEN - 1 ] = 0;
		
		conn.send_file( path );
	}
	else
	{
		conn.send_file( g_default_doc );
	}
}


//////////////////////////////////////////////////////////////////////////
// entry point
//////////////////////////////////////////////////////////////////////////


/**********************************************************************************************/
int main( void )
{
	cr_options opts;
	opts.thread_count = 1;
	
	if( !cr_start( opts ) )
		printf( "%s\n", cr_error_string() );
}
