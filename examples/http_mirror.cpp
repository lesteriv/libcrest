/**********************************************************************************************/
/* http_mirror.cpp					                                           				  */
/*                                                                       					  */
/* Igor Nikitin, 2013																		  */
/* MIT license			                                                  					  */
/**********************************************************************************************/

// CREST
#include "../include/crest.h"

/**********************************************************************************************/
using namespace std;

/**********************************************************************************************/
cr_cache g_cache( 600 /*sec*/ );
string	 g_source;


/**********************************************************************************************/
GET( * )( cr_connection& conn )
{
	if( g_cache.find( conn ) )
		return;
	
	string		  content;
	cr_string_map headers;
	string		  uri = g_source + conn.url() + '?' + conn.query_string();

	conn.fetch( uri, content, &headers ) ?
		g_cache.push( conn, CR_HTTP_OK, content, { "content-type", headers[ "content-type" ] } ) :
		g_cache.push( conn, CR_HTTP_NOT_FOUND, "404 Not Found" );
}

/**********************************************************************************************/
int main( int argc, char** argv )
{
	// 'Usage' help
	if( argc < 2 )
	{
		printf( "Usage: http_mirror base_url [[ip_address:]port[,...]]\nFor example: http_mirror http://www.ubuntu.com 8080\n" );
		return 1;
	}
	
	// Destination host
	g_source = argv[ 1 ];
	
	// Ports to listen on
	cr_options opts;
	opts.thread_count = 1;
	if( argc > 2 )
		opts.ports = argv[ 2 ];
		
	// Start server
	if( !cr_start( opts ) )
		printf( "%s\n", cr_error_string() );
}
