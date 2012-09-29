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


//////////////////////////////////////////////////////////////////////////
// static data
//////////////////////////////////////////////////////////////////////////


/**********************************************************************************************/
static const string EMPTY_STRING;


//////////////////////////////////////////////////////////////////////////
// properties
//////////////////////////////////////////////////////////////////////////


/**********************************************************************************************/
time_t crest_connection::get_birth_time( void ) const
{
	return mg_get_birth_time( conn_ );
}

/**********************************************************************************************/
const char* crest_connection::get_http_header( const char* name )
{
	return mg_get_header( conn_, name );
}

/**********************************************************************************************/
const string& crest_connection::get_path_parameter( size_t index )
{
	return path_params_.size() > index ?
		path_params_[ index ] :
		EMPTY_STRING;
}

/**********************************************************************************************/
string crest_connection::get_query_parameter( const char* name )
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
size_t crest_connection::read( char* buf, size_t len )
{
	return mg_read( conn_, buf, len );
}

/**********************************************************************************************/
void crest_connection::respond(
	crest_http_status		rc,
	const string&	msg )
{
	write( responce( rc, msg ) );
}

/**********************************************************************************************/
void crest_connection::send_file( const string& path )
{
	FILE* f = fopen( path.c_str(), "rb" );
	if( !f )
	{
		respond( HTTP_INTERNAL_ERROR, "Unable to open file: " + path );
		return;
	}

	fseek( f, 0, SEEK_END );
	
	char buf[ 24 ];
	to_string( ftell( f ), buf );
	string pref = "HTTP/1.1 200 OK\r\nContent-Length: ";
	pref += buf;
	pref += "\r\n\r\n";
	mg_write( conn_, pref.c_str(), pref.length() );

	fseek( f, 0, SEEK_SET );
	
	char data[ 16384 ];
	while( !feof( f ) )
	{
		int r = fread( data, 1, 16384, f );
		if( r <= 0 )
			break;
		
		int w = mg_write( conn_, data, r );
		if( w != r )
			break;
	}
	
	fclose( f );
}

/**********************************************************************************************/
int crest_connection::write( const string& str )
{
	return mg_write( conn_, str.c_str(), str.length() );
}
