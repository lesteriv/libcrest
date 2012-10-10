/**********************************************************************************************/
/* cache_mirror.cpp					                                           				  */
/*                                                                       					  */
/* Igor Nikitin, 2012																		  */
/* MIT license			                                                  					  */
/**********************************************************************************************/

// STD
#include <memory>
#include <string>
#include <unordered_map>

// CREST
#include "../include/crest.h"
#include "../include/cr_utils.h"

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
// types
//////////////////////////////////////////////////////////////////////////


/**********************************************************************************************/
struct cached_data
{
	~cached_data( void ) { free( ptr_ ); }
	
	size_t	len_;
	char*	ptr_;
};


/**********************************************************************************************/
struct resource
{
	shared_ptr<cached_data>	data_;
	shared_ptr<string>		etag_;
	time_t					expire_;
};


//////////////////////////////////////////////////////////////////////////
// global data
//////////////////////////////////////////////////////////////////////////


/**********************************************************************************************/
static unordered_map<string,resource> g_cache;
static size_t	g_memory_usage;
static cr_mutex g_mutex_cache;
static string	g_source;


//////////////////////////////////////////////////////////////////////////
// http handlers
//////////////////////////////////////////////////////////////////////////


/**********************************************************************************************/
GET( * )( cr_connection& conn )
{
	time_t t = time( NULL );
	string url = conn.get_url();
	
	// Add query parameters to url
	const char* qs = conn.get_query_string();
	if( *qs ) { url += '?';	url += qs; }

	LOCK_CACHE
	
	// Search for non-expired cached content
	auto it = g_cache.find( url );
	if( it != g_cache.end() && it->second.expire_ > t )
	{
		auto data = it->second.data_;
		auto etag = it->second.etag_;
		
		UNLOCK_CACHE
		
		// Send 403 status if client already has cached version of resource.
		const char* client_etag = conn.get_http_header( "if-none-match" );
		if( client_etag && *etag == client_etag )
		{
			conn.respond( CR_HTTP_NOT_MODIFIED );
			return;
		}
		
		// Send responce with resource's content
		conn.write( data->ptr_, data->len_ );
	}
	// Fetch value from mirrored site
	else
	{
		// Forget expired data
		if( it != g_cache.end() )
			g_cache.erase( it );
		
		UNLOCK_CACHE
			
		char*		content;
		size_t		content_size;
		string		curl = g_source + url;
		cr_headers	source_headers;
		
		// Fetch resource
		if( conn.fetch( curl.c_str(), content, content_size, &source_headers ) )
		{
			cr_headers cache_headers;
			
			// Keep original 'content-type' header
			const char* type = source_headers.value( "content-type", 12 );
			if( type )
				cache_headers.add( "content-type", type, 12 );
			
			// Add 'ETag' so client can use cached version of resource
			auto etag = make_shared<string>( to_string( time( NULL ) ) );
			cache_headers.add( "etag", etag->c_str(), 4, etag->length() );
			
			// Generate responce with status, headers and content
			char* data;
			size_t data_len;
			create_responce( data, data_len, CR_HTTP_OK, content, content_size, &cache_headers );
			free( content );
			
			// Save responce into cache if it not too big
			if( data_len < MAX_RESOURCE_SIZE )
			{
				resource rs;
				rs.data_		= make_shared<cached_data>();
				rs.data_->len_	= data_len;
				rs.data_->ptr_	= data;
				rs.etag_		= etag;
				rs.expire_		= time( NULL ) + EXPIRATION_TIME_SEC;
				
				LOCK_CACHE

				// Check total memory usage bound
				if( g_memory_usage + data_len > MAX_MEMORY_USAGE )
				{
					g_memory_usage = 0;
					g_cache.clear();
				}

				// Add data to cache
				g_cache[ url ] = rs;
				g_memory_usage += data_len;
				
				UNLOCK_CACHE
			}
				
			// Send responce to client
			conn.write( data, data_len );
			
			// Free resources if need
			if( data_len >= MAX_RESOURCE_SIZE )
				free( data );
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
