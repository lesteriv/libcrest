/**********************************************************************************************/
/* storage.cpp					                                               				  */
/*                                                                       					  */
/* Igor Nikitin, 2012																		  */
/* MIT license			                                                  					  */
/**********************************************************************************************/

// STD
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>

// CREST
#include "../include/crest.h"

/**********************************************************************************************/
using namespace std;

/**********************************************************************************************/
#define HASH	conn.path_parameter( 0 )
#define KEY		conn.path_parameter( 1 )
#define LOCK	g_mutex_hash.lock();
#define UNLOCK	g_mutex_hash.unlock();


//////////////////////////////////////////////////////////////////////////
// global data
//////////////////////////////////////////////////////////////////////////


/**********************************************************************************************/
unordered_map<string,unordered_map<string,shared_ptr<string>>> g_hashes;
int64_t		g_counter = 1 << 20;

/**********************************************************************************************/
mutex		g_mutex_counter;
mutex		g_mutex_hash;


//////////////////////////////////////////////////////////////////////////
// handlers
//////////////////////////////////////////////////////////////////////////


/**********************************************************************************************/
GET()( cr_connection& conn )
{
	static vector<cr_result_field> fields = { { "name", CR_TEXT }, { "count", CR_INTEGER } };
	
	cr_result r( conn );
	r.set_record_fields( fields );
		
	LOCK
	
	for( auto& coll : g_hashes )
	{
		r.add_text( coll.first );
		r.add_int( coll.second.size() );
	}
	
	UNLOCK
	
	conn.respond( CR_HTTP_OK, r );
}

/**********************************************************************************************/
DELETE( {hash} )( cr_connection& conn )
{
	LOCK
	
	auto it = g_hashes.find( HASH );
	if( it == g_hashes.end() )
	{
		UNLOCK
		
		conn.respond( CR_HTTP_NOT_FOUND );
	}
	else
	{
		g_hashes.erase( it );
		
		UNLOCK
		
		conn.respond( CR_HTTP_OK );
	}
}

/**********************************************************************************************/
GET( {hash} )( cr_connection& conn )
{
	LOCK
	
	auto it = g_hashes.find( HASH );
	if( it == g_hashes.end() )
	{
		UNLOCK

		conn.respond( CR_HTTP_NOT_FOUND );
		return;
	}
	
	auto& coll = g_hashes[ HASH ];
		
	string data;
	for( auto& key : coll )
		data += key.first + '\n';
	
	UNLOCK
	
	conn.respond( CR_HTTP_OK, data );	
}

/**********************************************************************************************/
POST( {hash} )( cr_connection& conn )
{
	g_mutex_counter.lock();
	string id = to_string( ++g_counter );
	g_mutex_counter.unlock();
	
	auto str = make_shared<string>();
	conn.read( *str );

	LOCK
	g_hashes[ HASH ][ id ] = str;
	UNLOCK
	
	conn.respond( CR_HTTP_OK, id );
}

/**********************************************************************************************/
DELETE( {hash}/{key} )( cr_connection& conn )
{
	LOCK
	
	auto it = g_hashes.find( HASH );
	if( it != g_hashes.end() )
	{
		it->second.erase( KEY );
		
		UNLOCK
		
		conn.respond( CR_HTTP_OK );
	}
	else
	{
		UNLOCK
		
		conn.respond( CR_HTTP_NOT_FOUND );
	}
}

/**********************************************************************************************/
GET( {hash}/{key} )( cr_connection& conn )
{
	LOCK
	
	auto it = g_hashes.find( HASH );
	if( it != g_hashes.end() )
	{
		auto it2 = it->second.find( KEY );
		if( it2 != it->second.end() )
		{
			shared_ptr<string> data = it2->second;
			
			UNLOCK
			
			conn.respond( CR_HTTP_OK, *data );
			return;
		}
	}

	UNLOCK
	
	conn.respond( CR_HTTP_NOT_FOUND );
}

/**********************************************************************************************/
PUT( {hash}/{key} )( cr_connection& conn )
{
	auto str = make_shared<string>();
	conn.read( *str );
	
	LOCK
	g_hashes[ HASH ][ KEY ] = str;
	UNLOCK
	
	conn.respond( CR_HTTP_OK );
}


//////////////////////////////////////////////////////////////////////////
// entry point
//////////////////////////////////////////////////////////////////////////


/**********************************************************************************************/
int main( void )
{
	cr_options opts;
	opts.ports = "8080";
	
	g_hashes[ "A" ][ "1" ] = make_shared<string>( "1" );
	g_hashes[ "A" ][ "2" ] = make_shared<string>( "2" );
	g_hashes[ "B" ][ "3" ] = make_shared<string>( "3" );
	g_hashes[ "B" ][ "4" ] = make_shared<string>( "4" );
	g_hashes[ "B" ][ "5" ] = make_shared<string>( "5" );
	
	if( !cr_start( opts ) )
		printf( "%s\n", cr_error_string() );
}
