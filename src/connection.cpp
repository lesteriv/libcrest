/**********************************************************************************************/
/* Connection.cpp				                                                  			  */
/*                                                                       					  */
/* Igor Nikitin, 2012																		  */
/* MIT license			                                                  					  */
/**********************************************************************************************/

// MONGOOSE
#include "../third/mongoose/mongoose.h"

// CREST
#include "../include/crest.h"

/**********************************************************************************************/
CREST_NAMESPACE_START


//////////////////////////////////////////////////////////////////////////
// static data
//////////////////////////////////////////////////////////////////////////


/**********************************************************************************************/
static const string EMPTY_STRING;


//////////////////////////////////////////////////////////////////////////
// properties
//////////////////////////////////////////////////////////////////////////


/**********************************************************************************************/
time_t connection::get_birth_time( void ) const
{
	return mg_get_birth_time( conn_ );
}

/**********************************************************************************************/
const char* connection::get_http_header( const char* name )
{
	return mg_get_header( conn_, name );
}

/**********************************************************************************************/
const string& connection::get_path_parameter( size_t index )
{
	return path_params_.size() > index ?
		path_params_[ index ] :
		EMPTY_STRING;
}

/**********************************************************************************************/
string connection::get_query_parameter( const char* name )
{
	string res;
	
	const char* qs = mg_get_request_info( conn_ )->query_string;
	if( !qs )
		return res;
	
	size_t len = strlen( qs );
	vector<char> buf( len + 1 );
	if( mg_get_var( qs, strlen( qs ), name, &buf[ 0 ], len + 1 ) )
		res.assign( &buf[ 0 ] );
		
	return res;
}


//////////////////////////////////////////////////////////////////////////
// methods
//////////////////////////////////////////////////////////////////////////


/**********************************************************************************************/
size_t connection::read( char* buf, size_t len )
{
	return mg_read( conn_, buf, len );
}

/**********************************************************************************************/
void connection::respond(
	http_status		rc,
	const string&	msg )
{
	write( responce( rc, msg ) );
}

/**********************************************************************************************/
void connection::send_file( const string& path )
{
	mg_send_file( conn_, path.c_str() );
}

/**********************************************************************************************/
int connection::write( const string& str )
{
	return mg_write( conn_, str.c_str(), str.length() );
}


/**********************************************************************************************/
CREST_NAMESPACE_END
