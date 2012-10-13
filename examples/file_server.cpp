/**********************************************************************************************/
/* file_server.cpp								                                			  */
/*                                                                       					  */
/* Igor Nikitin, 2012																		  */
/* MIT license			                                                  					  */
/**********************************************************************************************/

// STD
#include <stdio.h>
#include <string.h>

// CREST
#include "../include/crest.h"

/**********************************************************************************************/
static const char	g_root[]	= "/var/log";

/**********************************************************************************************/
static const size_t	ROOT_LEN	= sizeof( g_root ) - 1;
static const size_t	URL_LEN		= 8192;
static const size_t	BUF_LEN		= URL_LEN + ROOT_LEN + 1;

/**********************************************************************************************/
static const char g_index[]	=
	"<html>"
	"<a href=\"apt/history.log\">apt/history.log</a><br>"
	"<a href=\"Xorg.0.log\">Xorg.0.log</a><br>"
	"<a href=\"auth.log\">auth.log</a><br>"
	"<a href=\"dmesg\">dmesg</a><br>"
	"<a href=\"syslog\">syslog</a>"
	"</html>";


//////////////////////////////////////////////////////////////////////////
// handlers
//////////////////////////////////////////////////////////////////////////


/**********************************************************************************************/
GET( * )( cr_connection& conn )
{
	if( conn.url()[ 1 ] )
	{
		char path[ BUF_LEN ];
		memcpy( path, g_root, ROOT_LEN );
		strncpy( path + ROOT_LEN, conn.url(), URL_LEN );
		path[ BUF_LEN - 1 ] = 0;
		
		conn.send_file( path, "text/plain" );
	}
	else
	{
		conn.respond( CR_HTTP_OK, g_index, sizeof( g_index ) - 1 );
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
