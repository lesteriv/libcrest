/**********************************************************************************************/
/* cr_cache.cpp							                                            		  */
/*                                                                       					  */
/* Igor Nikitin, 2013																		  */
/* MIT license			                                                  					  */
/**********************************************************************************************/

// CREST
#include "../include/crest.h"

/**********************************************************************************************/
using std::make_shared;
using std::shared_ptr;
using std::string;
using std::to_string;

/**********************************************************************************************/
#define LOCK_CACHE		mutex_.lock();
#define UNLOCK_CACHE	mutex_.unlock();


//////////////////////////////////////////////////////////////////////////
// construction
//////////////////////////////////////////////////////////////////////////


/**********************************************************************************************/
cr_cache::cr_cache( int expire_time )
{
	expire_time_ = expire_time;
}


//////////////////////////////////////////////////////////////////////////
// methods
//////////////////////////////////////////////////////////////////////////


/**********************************************************************************************/
void cr_cache::erase( cr_connection& conn )
{
	string url = string( conn.url() ) + '?' + conn.query_string();

	LOCK_CACHE
	
	entries_.erase( url );
	
	UNLOCK_CACHE
}

/**********************************************************************************************/
bool cr_cache::find( cr_connection& conn )
{
	time_t t   = time( NULL );
	string url = string( conn.url() ) + '?' + conn.query_string();

	LOCK_CACHE
	
	// Search for non-expired cached content
	auto it = entries_.find( url );
	if( it != entries_.end() && ( expire_time_ <= 0 || it->second.expire > t ) )
	{
		auto data = it->second.data;
		auto etag = it->second.etag;
		
		UNLOCK_CACHE
		
		// Send 403 status if client already has cached version of resource,
		// or send responce with resource's content otherwise
		auto client_etag = conn.header( "if-none-match" );
		if( client_etag && *etag == client_etag )
			conn.respond( CR_HTTP_NOT_MODIFIED );
		else		
			conn.write( *data );
		
		return true;
	}
	
	// Forget expired entry
	if( it != entries_.end() )
		entries_.erase( it );

	UNLOCK_CACHE
	
	return false;
}

/**********************************************************************************************/
void cr_cache::push(
	cr_connection&		conn,
	cr_http_status		rc,
	const char*			data,
	size_t				data_len,
	cr_string_map		headers )
{
	time_t			t	= time( NULL );
	string			url = string( conn.url() ) + '?' + conn.query_string();

	// Add 'ETag' so client can use cached version of resource
	auto etag = make_shared<string>( to_string( t ) );
	headers.add( "etag", etag->c_str() );

	// Generate responce with status, headers and content
	char* rdata;
	size_t rlen;	
	cr_create_responce( rdata, rlen, rc, data, data_len, &headers );

	auto responce = make_shared<string>( string( rdata, rlen ) );
	
	// Save responce into cache
	LOCK_CACHE
	entries_[ url ] = { responce, etag, t + expire_time_ };
	UNLOCK_CACHE

	// Send responce to client
	conn.write( *responce );
}

/**********************************************************************************************/
void cr_cache::push(
	cr_connection&		conn,
	cr_http_status		rc,
	const std::string&	data,
	cr_string_map		headers )
{
	push( conn, rc, data.c_str(), data.length(), headers );
}
