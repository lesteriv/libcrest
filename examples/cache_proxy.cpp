/**********************************************************************************************/
/* cache_proxy.cpp					                                           				  */
/*                                                                       					  */
/* Igor Nikitin, 2012																		  */
/* MIT license			                                                  					  */
/**********************************************************************************************/

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
#define LOCK_CACHE		g_mutex_cache.lock();
#define LOCK_ETAG		g_mutex_etag.lock();
#define LOCK_SOURCE		g_mutex_source.lock();
#define UNLOCK_CACHE	g_mutex_cache.unlock();
#define UNLOCK_ETAG		g_mutex_etag.unlock();
#define UNLOCK_SOURCE	g_mutex_source.unlock();


//////////////////////////////////////////////////////////////////////////
// global data
//////////////////////////////////////////////////////////////////////////


/**********************************************************************************************/
static unordered_map<string,shared_ptr<string>> g_cache;
static int64_t	g_etag_counter 	= time( NULL );
static string	g_etag 			= to_string( g_etag_counter );
static cr_mutex g_mutex_cache;
static cr_mutex g_mutex_etag;
static cr_mutex g_mutex_source;
static string	g_source 		= "http://www.ubuntu.com";


//////////////////////////////////////////////////////////////////////////
// handlers
//////////////////////////////////////////////////////////////////////////


/**********************************************************************************************/
GET( * )( cr_connection& conn )
{
	string url = conn.get_url();
	
	LOCK_CACHE
	
	auto it = g_cache.find( url );
	if( it != g_cache.end() )
	{
		auto str = it->second;
		
		UNLOCK_CACHE
		
		const char* etag = conn.get_http_header( "if-none-match" );
		if( etag )
		{
			LOCK_ETAG
			
			bool cached = g_etag == etag;
			
			UNLOCK_ETAG
			
			if( cached )
			{
				conn.respond( CR_HTTP_NOT_MODIFIED );
				return;
			}
		}
		
		conn.write( *str );
	}
	else
	{
		UNLOCK_CACHE
			
		LOCK_SOURCE		
		
		string curl = g_source + url;
		
		UNLOCK_SOURCE
		
		char* content;
		size_t content_size;
		cr_headers headers;
		
		if( conn.fetch( curl.c_str(), content, content_size, &headers ) )
		{
			cr_headers cache_headers;
			const char* type = headers.value( "content-type", 12 );
			if( type )
				cache_headers.add( "content-type", type, 12 );
			
			LOCK_ETAG
			
			string etag = g_etag;
			
			UNLOCK_ETAG
				
			cache_headers.add( "ETag", etag.c_str() );
			
			char* rbuf;
			size_t rlen;
			create_responce( rbuf, rlen, CR_HTTP_OK, content, content_size, &cache_headers );
			free( content );
			
			auto str = make_shared<string>();
			str->assign( rbuf, rlen );
			free( rbuf );

			LOCK_CACHE
			
			g_cache[ url ] = str;
			
			UNLOCK_CACHE
				
			conn.write( *str );
		}
		else
		{
			conn.respond( CR_HTTP_NOT_FOUND );
		}
	}
}

/**********************************************************************************************/
POST( _reset )( cr_connection& conn )
{
	LOCK_CACHE
	
	g_cache.clear();

	LOCK_ETAG
	
	g_etag = to_string( ++g_etag_counter );
	
	UNLOCK_ETAG
	
	UNLOCK_CACHE

	conn.respond( CR_HTTP_OK );
}

/**********************************************************************************************/
PUT( _source )( cr_connection& conn )
{
	LOCK_SOURCE
	
	conn.read( g_source );
	
	UNLOCK_SOURCE
	
	LOCK_CACHE
	
	g_cache.clear();
	
	UNLOCK_CACHE
	
	conn.respond( CR_HTTP_OK );
}


//////////////////////////////////////////////////////////////////////////
// entry point
//////////////////////////////////////////////////////////////////////////


/**********************************************************************************************/
int main( void )
{
	cr_options opts;
	opts.thread_count = 64;
	
	if( !cr_start( opts ) )
		fputs( cr_error_string(), stdout );
}
