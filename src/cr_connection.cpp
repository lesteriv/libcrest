/**********************************************************************************************/
/* cr_connection.cpp				                                                  		  */
/*                                                                       					  */
/* Igor Nikitin, 2012																		  */
/* MIT license			                                                  					  */
/**********************************************************************************************/

// STD
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// MONGOOSE
#include "../third/mongoose/mongoose.h"

// ZLIB
#include "../third/zlib/zlib.h"

// CREST
#include "../include/crest.h"
#include "utils.h"

/**********************************************************************************************/
#ifdef _MSC_VER
#pragma warning( disable: 4996 )
#endif // _WIN32


//////////////////////////////////////////////////////////////////////////
// properties
//////////////////////////////////////////////////////////////////////////


/**********************************************************************************************/
time_t cr_connection::get_birth_time( void ) const
{
	return mg_get_birth_time( conn_ );
}

/**********************************************************************************************/
size_t cr_connection::get_content_length( void ) const
{
	return mg_get_content_len( conn_ );
}

/**********************************************************************************************/
const char* cr_connection::get_http_header( const char* name ) const
{
	return mg_get_header( conn_, name );
}

/**********************************************************************************************/
const char*	cr_connection::get_http_method( void ) const
{
	return mg_get_request_info( conn_ )->method_;
}

/**********************************************************************************************/
const char* cr_connection::get_path_parameter( size_t index ) const
{
	return path_params_.count_ > index ?
		path_params_.items_[ index ] :
		"";
}

/**********************************************************************************************/
const char* cr_connection::get_query_parameter( const char* name ) const
{
	if( query_params_count_ == (size_t) -1 )
	{
		parse_query_parameters(
			query_params_count_,
			query_params_names_,
			query_params_values_,
			mg_get_request_info( conn_ )->query_parameters_ );
	}

	for( size_t i = 0 ; i < query_params_count_ ; ++i )
	{
		if( !strcmp( name, query_params_names_[ i ] ) )
			return query_params_values_[ i ];
	}
	
	return "";
}

/**********************************************************************************************/
const char* cr_connection::get_url( void ) const
{
	return mg_get_request_info( conn_ )->uri_;
}


//////////////////////////////////////////////////////////////////////////
// methods
//////////////////////////////////////////////////////////////////////////


/**********************************************************************************************/
size_t cr_connection::read( char* buf, size_t len )
{
	return mg_read( conn_, buf, len );
}

/**********************************************************************************************/
void cr_connection::respond(
	cr_http_status	rc,
	const char*			data,
	size_t				data_len )
{
	// Compress data if need
#ifndef NO_DEFLATE	
	if( data && data_len > 128 )
	{
		const char* enc_header = mg_get_header( conn_, "accept-encoding" );
		if( enc_header && strstr( enc_header, "deflate" ) )
		{
			size_t out_len = compressBound( data_len );
			char* out = (char*) alloca( out_len );
			data_len = deflate( data, data_len, out, out_len );

			char header[ 128 ];
			size_t header_len;
			create_responce_header( header, header_len, rc, data_len, true );
			mg_write( conn_, header, header_len );
			mg_write( conn_, out, data_len );
			
			return;
		}
	}
#endif // NO_DEFLATE	
	
	// Write non-compressed data
	char header[ 128 ];
	size_t header_len;
	create_responce_header( header, header_len, rc, data_len );
	mg_write( conn_, header, header_len );
	mg_write( conn_, data, data_len );
}

/**********************************************************************************************/
void cr_connection::send_file( const char* path )
{
	FILE* f = fopen( path, "rb" );
	if( !f )
	{
		respond( CR_HTTP_NOT_FOUND, 0, 0 );
		return;
	}

	fseek( f, 0, SEEK_END );
	
	// Header
	char header[ 128 ];
	size_t header_len;
	create_responce_header( header, header_len, CR_HTTP_OK, ftell( f ) );
	mg_write( conn_, header, header_len );

	fseek( f, 0, SEEK_SET );
	
	// File data
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
int cr_connection::write( const char* buf, size_t len )
{
	return mg_write( conn_, buf, len );
}


//////////////////////////////////////////////////////////////////////////
// internal methods
//////////////////////////////////////////////////////////////////////////


/**********************************************************************************************/
cr_connection_internal::~cr_connection_internal( void )
{
	// Free temporary data
	free( path_params_.items_ );
}
