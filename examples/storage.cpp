/**********************************************************************************************/
/* storage.cpp					                                               				  */
/*                                                                       					  */
/* Igor Nikitin, 2012																		  */
/* MIT license			                                                  					  */
/**********************************************************************************************/

// STD
#include <map>
#include <string>

// CREST
#include "../include/crest.h"

/**********************************************************************************************/
using std::map;
using std::string;
using std::to_string;

/**********************************************************************************************/
#define COLLECTION	conn.get_path_parameter( 0 )
#define GUARD		cr_lock_guard guard( g_mutex );
#define KEY			conn.get_path_parameter( 1 )

/**********************************************************************************************/
static map<string,map<string,string>> g_colls;
static size_t g_counter;
static cr_mutex g_mutex;


//////////////////////////////////////////////////////////////////////////
// handlers
//////////////////////////////////////////////////////////////////////////


/**********************************************************************************************/
GET()( cr_connection& conn )
{
	GUARD
	
	string data;
	for( auto& coll : g_colls )
		data += "<a href=\"" + coll.first + "\">" + coll.first + "</a><br>\n";
	
	conn.respond( CR_HTTP_OK, data );
}

/**********************************************************************************************/
DELETE( {collection} )( cr_connection& conn )
{
	GUARD
	
	g_colls.erase( COLLECTION );
	conn.respond( CR_HTTP_OK );
}

/**********************************************************************************************/
GET( {collection} )( cr_connection& conn )
{
	GUARD
	
	auto& coll = g_colls[ COLLECTION ];
	string pref = COLLECTION + string( "/" );
		
	string data;
	for( auto& key : coll )
		data += "<a href=\"" + pref + key.first + "\">" + key.first + "</a><br>\n";
	
	conn.respond( CR_HTTP_OK, data );	
}

/**********************************************************************************************/
POST( {collection} )( cr_connection& conn )
{
	GUARD
	
	string id = to_string( ++g_counter );
	conn.read( g_colls[ COLLECTION ][ id ] );
	
	conn.respond( CR_HTTP_OK, id );
}

/**********************************************************************************************/
DELETE( {collection}/{key} )( cr_connection& conn )
{
	GUARD
	
	g_colls[ COLLECTION ].erase( KEY );
	conn.respond( CR_HTTP_OK );
}

/**********************************************************************************************/
GET( {collection}/{key} )( cr_connection& conn )
{
	GUARD
	
	conn.respond( CR_HTTP_OK, g_colls[ COLLECTION ][ KEY ] );
}

/**********************************************************************************************/
PUT( {collection}/{key} )( cr_connection& conn )
{
	GUARD
	
	conn.read( g_colls[ COLLECTION ][ KEY ] );
	conn.respond( CR_HTTP_OK );
}


//////////////////////////////////////////////////////////////////////////
// entry point
//////////////////////////////////////////////////////////////////////////


/**********************************************************************************************/
int main( void )
{
	// Predefined values
	g_colls[ "A" ][ "A1" ] = "<b>test</b>";
	g_colls[ "A" ][ "A2" ] = "{ \"test\": 1 }";
	g_colls[ "B" ][ "B1" ] = "<test>1</test>";
	g_colls[ "B" ][ "B2" ] = "test";
	
	// Start server
	if( !cr_start( "8080" ) )
		fputs( cr_error_string(), stdout );
}
