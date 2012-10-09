/**********************************************************************************************/
/* cache_mirror.cpp					                                           				  */
/*                                                                       					  */
/* Igor Nikitin, 2012																		  */
/* MIT license			                                                  					  */
/**********************************************************************************************/

// TODO: update from source by timeout + etag

// STD
#include <memory>
#include <unordered_map>
#include <string>

// CREST
#include "../include/crest.h"
#include "../src/utils.h"

/**********************************************************************************************/
using namespace std;

/**********************************************************************************************/
#define EXPIRATION_TIME_SEC	600
#define MAX_MEMORY_USAGE	500000000
#define MAX_RESOURCE_SIZE	10000000

/**********************************************************************************************/
#define LOCK_CACHE			g_mutex_cache.lock();
#define UNLOCK_CACHE		g_mutex_cache.unlock();


//////////////////////////////////////////////////////////////////////////
// global data
//////////////////////////////////////////////////////////////////////////


/**********************************************************************************************/
struct resource
{
	shared_ptr<string>	data;
	shared_ptr<string>	etag;
	time_t				expire;
};

/**********************************************************************************************/
static unordered_map<string,resource> g_cache;
static size_t	g_memory_usage	= 0;
static string	g_source;

/**********************************************************************************************/
static cr_mutex g_mutex_cache;


//////////////////////////////////////////////////////////////////////////
// handlers
//////////////////////////////////////////////////////////////////////////


/**********************************************************************************************/
GET_PUBLIC( * )( cr_connection& conn )
{
	time_t t = time( NULL );
	string url = conn.get_url();
	
	// Add query parameters to url
	const char* qs = conn.get_query_string();
	if( *qs )
	{
		url += '?';
		url += qs;
	}

	LOCK_CACHE
	
	// Search for non-expired cached content
	auto it = g_cache.find( url );
	if( it != g_cache.end() && ( EXPIRATION_TIME_SEC == 0 || it->second.expire > t ) )
	{
		auto etag = it->second.etag;
		auto data = it->second.data;
		
		UNLOCK_CACHE
		
		// Send 403 status if client already has cached version of resource.
		const char* r_etag = conn.get_http_header( "if-none-match" );
		if( r_etag && *etag == r_etag )
		{
			conn.respond( CR_HTTP_NOT_MODIFIED );
			return;
		}
		
		// Send responce with resource's content
		conn.write( *data );
	}
	// Fetch value from mirrored site
	else
	{
		if( it != g_cache.end() )
			g_cache.erase( it );
		
		UNLOCK_CACHE
			
		string curl = g_source + url;
		
		char* content;
		size_t content_size;
		cr_headers headers;
		
		// Fetch resource
		if( conn.fetch( curl.c_str(), content, content_size, &headers ) )
		{
			cr_headers cache_headers;
			
			// Keep original 'content-type' header
			const char* type = headers.value( "content-type" );
			if( type )
				cache_headers.add( "content-type", type );
			
			// Add 'ETag' so client can use cached version of resource
			auto etag = make_shared<string>( to_string( time( NULL ) ) );
			cache_headers.add( "etag", etag->c_str() );
			
			// Generate responce with status, headers and content
			char* rbuf;
			size_t rlen;
			create_responce( rbuf, rlen, CR_HTTP_OK, content, content_size, &cache_headers );
			free( content );
			
			// Store responce into shared_ptr<string>
			auto data = make_shared<string>();
			data->assign( rbuf, rlen );
			free( rbuf );
			
			// 'Send' responce into cache if it not too big
			if( data->length() < MAX_RESOURCE_SIZE )
			{
				LOCK_CACHE

				if( g_memory_usage + data->length() > MAX_MEMORY_USAGE )
				{
					g_memory_usage = 0;
					g_cache.clear();
				}
				
				g_memory_usage += data->length();
				
				auto& rs = g_cache[ url ];
				rs.data = data;
				rs.etag = etag;
				if( EXPIRATION_TIME_SEC )
					rs.expire = time( NULL ) + EXPIRATION_TIME_SEC;
				
				UNLOCK_CACHE
			}
				
			// Send responce to client
			conn.write( *data );
		}
		// We cannot fetach resource - send 404 status to client
		else
		{
			conn.respond( CR_HTTP_NOT_FOUND, "404 Not Found", 13 );
		}
	}
}


//////////////////////////////////////////////////////////////////////////
// entry point
//////////////////////////////////////////////////////////////////////////


/**********************************************************************************************/
int main( int argc, char** argv )
{
	// 'Usage' help
	if( argc < 2 )
	{
		printf( "Usage: http_mirror host [[ip_address:]port[,...]]\nFor example: http_mirror www.ubuntu.com 8080\n" );
		return 1;
	}
	
	// Destination host
	if( strncmp( "http://", argv[ 1 ], 7 ) ) g_source = "http://";
	g_source += argv[ 1 ];
	
	// Ports to listen on
	cr_options opts;
	opts.ports = "80";
	if( argc > 2 )
		opts.ports = argv[ 2 ];
		
	// Start server
	if( !cr_start( opts ) )
		printf( "%s\n", cr_error_string() );
}
