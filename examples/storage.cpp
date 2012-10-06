/**********************************************************************************************/
/* storage.cpp					                                               				  */
/*                                                                       					  */
/* Igor Nikitin, 2012																		  */
/* MIT license			                                                  					  */
/**********************************************************************************************/

// STD
#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>

// CREST
#include "../include/crest.h"

/**********************************************************************************************/
using namespace std;

/**********************************************************************************************/
#define COLLECTION	conn.get_path_parameter( 1 )
#define KEY			conn.get_path_parameter( 2 )

/**********************************************************************************************/
#define LOCK_HASH	g_mutex_hash.lock();
#define UNLOCK_HASH	g_mutex_hash.unlock();

/**********************************************************************************************/
#define LOCK_SET	g_mutex_set.lock();
#define UNLOCK_SET	g_mutex_set.unlock();

/**********************************************************************************************/
static unordered_map<string,unordered_map<string,shared_ptr<string>>> g_hashes;
static int64_t	g_hash_counter = 1 << 20;
static cr_mutex g_mutex_hash;
static cr_mutex g_mutex_hash_counter;

/**********************************************************************************************/
static unordered_map<string,unordered_set<string>> g_sets;
static cr_mutex g_mutex_set;


//////////////////////////////////////////////////////////////////////////
// HASH
//////////////////////////////////////////////////////////////////////////


/**********************************************************************************************/
GET( hash )( cr_connection& conn )
{
	string data;
	
	LOCK_HASH
	
	data.reserve( g_hashes.size() * 32 );
	for( auto& coll : g_hashes )
	{
		data += coll.first;
		data.push_back( '\n' );
	}
	
	UNLOCK_HASH
	
	conn.respond( CR_HTTP_OK, data );
}

/**********************************************************************************************/
DELETE( hash/{hash} )( cr_connection& conn )
{
	LOCK_HASH
	
	auto it = g_hashes.find( COLLECTION );
	if( it == g_hashes.end() )
	{
		UNLOCK_HASH
		
		conn.respond( CR_HTTP_NOT_FOUND );
	}
	else
	{
		g_hashes.erase( it );
		
		UNLOCK_HASH
		
		conn.respond( CR_HTTP_OK );
	}
}

/**********************************************************************************************/
GET( hash/{hash} )( cr_connection& conn )
{
	LOCK_HASH
	
	auto it = g_hashes.find( COLLECTION );
	if( it == g_hashes.end() )
	{
		UNLOCK_HASH
		
		conn.respond( CR_HTTP_NOT_FOUND );
		return;
	}
	
	auto& coll = g_hashes[ COLLECTION ];
		
	string data;
	data.reserve( coll.size() * 32 );
	
	for( auto& key : coll )
	{
		data += key.first;
		data.push_back( '\n' );
	}
	
	UNLOCK_HASH
	
	conn.respond( CR_HTTP_OK, data );	
}

/**********************************************************************************************/
POST( hash/{hash} )( cr_connection& conn )
{
	g_mutex_hash_counter.lock();
	string id = to_string( ++g_hash_counter );
	g_mutex_hash_counter.unlock();
	
	auto str = make_shared<string>();
	conn.read( *str );

	LOCK_HASH
	
	g_hashes[ COLLECTION ][ id ] = str;

	UNLOCK_HASH
	
	conn.respond( CR_HTTP_OK, id );
}

/**********************************************************************************************/
DELETE( hash/{hash}/{key} )( cr_connection& conn )
{
	LOCK_HASH
	
	auto it = g_hashes.find( COLLECTION );
	if( it != g_hashes.end() )
	{
		g_hashes[ COLLECTION ].erase( KEY );
		
		UNLOCK_HASH
		
		conn.respond( CR_HTTP_OK );
	}
	else
	{
		UNLOCK_HASH
		
		conn.respond( CR_HTTP_NOT_FOUND );
	}
}

/**********************************************************************************************/
GET( hash/{hash}/{key} )( cr_connection& conn )
{
	LOCK_HASH
	
	auto it = g_hashes.find( COLLECTION );
	if( it != g_hashes.end() )
	{
		auto it2 = it->second.find( KEY );
		if( it2 != it->second.end() )
		{
			shared_ptr<string> data = it2->second;
			
			UNLOCK_HASH
			
			conn.respond( CR_HTTP_OK, *data );
			return;
		}
	}

	UNLOCK_HASH
	
	conn.respond( CR_HTTP_NOT_FOUND );
}

/**********************************************************************************************/
PUT( hash/{hash}/{key} )( cr_connection& conn )
{
	auto str = make_shared<string>();
	conn.read( *str );
	
	LOCK_HASH
	
	g_hashes[ COLLECTION ][ KEY ] = str;
	
	UNLOCK_HASH
	
	conn.respond( CR_HTTP_OK );
}


//////////////////////////////////////////////////////////////////////////
// SET
//////////////////////////////////////////////////////////////////////////


/**********************************************************************************************/
GET( set )( cr_connection& conn )
{
	string data;
	
	LOCK_SET
	
	data.reserve( g_sets.size() * 32 );
	for( auto& set : g_sets )
	{
		data += set.first;
		data.push_back( '\n' );
	}
	
	UNLOCK_SET
	
	conn.respond( CR_HTTP_OK, data );
}

/**********************************************************************************************/
DELETE( set/{set} )( cr_connection& conn )
{
	LOCK_SET
	
	auto it = g_sets.find( COLLECTION );
	if( it == g_sets.end() )
	{
		UNLOCK_SET
		
		conn.respond( CR_HTTP_NOT_FOUND );
	}
	else
	{
		g_sets.erase( it );
		
		UNLOCK_SET
		
		conn.respond( CR_HTTP_OK );
	}
}

/**********************************************************************************************/
GET( set/{set} )( cr_connection& conn )
{
	LOCK_SET
	
	auto it = g_sets.find( COLLECTION );
	if( it == g_sets.end() )
	{
		UNLOCK_SET
		
		conn.respond( CR_HTTP_NOT_FOUND );
		return;
	}
	
	auto& set = g_sets[ COLLECTION ];
		
	string data;
	data.reserve( set.size() * 32 );
	
	for( auto& value : set )
	{
		data += value;
		data.push_back( '\n' );
	}
	
	UNLOCK_SET
	
	conn.respond( CR_HTTP_OK, data );	
}

/**********************************************************************************************/
POST( set/{set} )( cr_connection& conn )
{
	string str;
	conn.read( str );

	LOCK_SET
	
	g_sets[ COLLECTION ].insert( str );

	UNLOCK_SET
	
	conn.respond( CR_HTTP_OK );
}

/**********************************************************************************************/
DELETE( set/{set}/{value} )( cr_connection& conn )
{
	LOCK_SET
	
	auto it = g_sets.find( COLLECTION );
	if( it != g_sets.end() )
	{
		g_sets[ COLLECTION ].erase( KEY );
		
		UNLOCK_SET
		
		conn.respond( CR_HTTP_OK );
	}
	else
	{
		UNLOCK_SET
		
		conn.respond( CR_HTTP_NOT_FOUND );
	}
}


//////////////////////////////////////////////////////////////////////////
// common
//////////////////////////////////////////////////////////////////////////


/**********************************************************************************************/
GET()( cr_connection& conn )
{
	conn.respond( CR_HTTP_OK, "It works!", 9 );
}


//////////////////////////////////////////////////////////////////////////
// entry point
//////////////////////////////////////////////////////////////////////////


/**********************************************************************************************/
int main( void )
{
	// Start server
	cr_options opts;
	opts.thread_count = 64;
	
	if( !cr_start( opts ) )
		fputs( cr_error_string(), stdout );
}
