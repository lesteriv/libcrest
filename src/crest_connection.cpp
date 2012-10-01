/**********************************************************************************************/
/* Connection.cpp				                                                  			  */
/*                                                                       					  */
/* Igor Nikitin, 2012																		  */
/* MIT license			                                                  					  */
/**********************************************************************************************/

// STD
#include <stdio.h>
#include <stdlib.h>

// MONGOOSE
#include "../third/mongoose/mongoose.h"

// CREST
#include "../include/crest.h"
#include "utils.h"


//////////////////////////////////////////////////////////////////////////
// properties
//////////////////////////////////////////////////////////////////////////


/**********************************************************************************************/
time_t crest_connection::get_birth_time( void ) const
{
	return mg_get_birth_time( conn_ );
}

/**********************************************************************************************/
size_t crest_connection::get_content_length( void ) const
{
	return mg_get_content_len( conn_ );
}

/**********************************************************************************************/
const char* crest_connection::get_http_header( const char* name ) const
{
	return mg_get_header( conn_, name );
}

/**********************************************************************************************/
const char* crest_connection::get_path_parameter( size_t index ) const
{
	return path_params_.count_ > index ?
		path_params_.items_[ index ] :
		"";
}

/**********************************************************************************************/
const char* crest_connection::get_query_parameter( const char* name ) const
{
	if( query_params_count_ == (size_t) -1 )
	{
		parse_query_parameters(
			query_params_count_,
			query_params_names_,
			query_params_values_,
			mg_get_request_info( conn_ )->query_string );
	}

	for( size_t i = 0 ; i < query_params_count_ ; ++i )
	{
		if( !strcmp( name, query_params_names_[ i ] ) )
			return query_params_values_[ i ];
	}
	
	return "";
}

/**********************************************************************************************/
const char* crest_connection::get_url( void ) const
{
	return mg_get_request_info( conn_ )->uri;
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
	crest_http_status	rc,
	const char*			msg,
	size_t				msg_len )
{
	char* str;
	size_t len;
	create_responce_header( str, len, rc, msg_len );
	mg_write( conn_, str, len );
	free( str );
	
	mg_write( conn_, msg, msg_len );
}

/**********************************************************************************************/
void crest_connection::send_file( const char* path )
{
	FILE* f = fopen( path, "rb" );
	if( !f )
	{
		mg_write( conn_, "HTTP/1.1 500 Internal Server Error\r\nContent-Length: 19\r\n\r\nUnable to open file", 77 );
		return;
	}

	fseek( f, 0, SEEK_END );
	
	char* header;
	size_t header_len;
	create_responce_header( header, header_len, CREST_HTTP_OK, ftell( f ) );
	
	mg_write( conn_, header, header_len );
	free( header );

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
int crest_connection::write( const char* buf, size_t len )
{
	return mg_write( conn_, buf, len );
}


//////////////////////////////////////////////////////////////////////////
// internal methods
//////////////////////////////////////////////////////////////////////////


/**********************************************************************************************/
crest_connection_internal::~crest_connection_internal( void )
{
	// Free path params
	free( path_params_.items_ );
}
