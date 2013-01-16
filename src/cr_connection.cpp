/**********************************************************************************************/
/* cr_connection.cpp				                                                  		  */
/*                                                                       					  */
/* Igor Nikitin, 2013																		  */
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
#include "../include/cr_utils.h"
#include "cr_utils_private.h"

/**********************************************************************************************/
#ifdef _MSC_VER
#pragma warning( disable: 4996 )
#endif // _WIN32

/**********************************************************************************************/
static cr_string_map g_deflate_headers( "Content-Encoding", "deflate" );


//////////////////////////////////////////////////////////////////////////
// properties
//////////////////////////////////////////////////////////////////////////


/**********************************************************************************************/
time_t cr_connection::birth_time( void ) const
{
	return mg_get_birth_time( conn_ );
}

/**********************************************************************************************/
size_t cr_connection::content_length( void ) const
{
	return mg_get_content_len( conn_ );
}

/**********************************************************************************************/
const char*	cr_connection::cookie(
	const char* name,
	size_t		name_len ) const
{
	return cookies().value( name, name_len );
}

/**********************************************************************************************/
const char*	cr_connection::cookie( const std::string& name ) const
{
	return cookies().value( name.c_str(), name.length() );
}

/**********************************************************************************************/
const cr_string_map& cr_connection::cookies( void ) const
{
	if( !cookies_inited_ )
	{
		cookies_inited_ = true;
		cr_string_map& headers = mg_get_request_info( conn_ )->headers_;
		
		int index = headers.find( "cookie" );
		if( index >= 0 )
		{
			parse_cookie_header(
				cookies_,
				(char*) headers.value( index ),
				headers.value_len( index ) );
		}
	}
	
	return cookies_;
}

/**********************************************************************************************/
const char*	cr_connection::header(
	const char* name,
	size_t		name_len ) const
{
	return mg_get_request_info( conn_ )->headers_.value( name, name_len );
}

/**********************************************************************************************/
const char*	cr_connection::header( const std::string& name ) const
{
	return header( name.c_str(), name.length() );
}

/**********************************************************************************************/
const cr_string_map& cr_connection::headers( void ) const
{
	return mg_get_request_info( conn_ )->headers_;
}

/**********************************************************************************************/
const char*	cr_connection::method( void ) const
{
	return mg_get_request_info( conn_ )->method_;
}

/**********************************************************************************************/
const char* cr_connection::path_parameter( size_t index ) const
{
	return path_params_.count > index ?
		path_params_.items[ index ] :
		"";
}

/**********************************************************************************************/
const char* cr_connection::post_parameter( 
	const char*	name,
	size_t		name_len )
{
	return post_parameters().value( name, name_len );
}

/**********************************************************************************************/
const char* cr_connection::post_parameter( const std::string& name )
{
	return post_parameter( name.c_str(), name.length() );
}

/**********************************************************************************************/
const cr_string_map& cr_connection::post_parameters( void )
{
	if( !post_params_buffer_ )
	{
		size_t len = content_length();
		if( len )
		{
			post_params_buffer_ = (char*) malloc( len + 1 );
			
			len = read( post_params_buffer_, len );
			if( len )
			{
				post_params_buffer_[ len ] = 0;
				parse_post_parameters( post_params_, post_params_buffer_ );
			}
		}
	}
	
	return post_params_;
}

/**********************************************************************************************/
const char* cr_connection::query_parameter( 
	const char*	name,
	size_t		name_len ) const
{
	return query_parameters().value( name, name_len );
}

/**********************************************************************************************/
const char* cr_connection::query_parameter( const std::string& name ) const
{
	return query_parameter( name.c_str(), name.length() );
}

/**********************************************************************************************/
const cr_string_map& cr_connection::query_parameters( void ) const
{
	if( !query_params_inited_ )
	{
		query_params_inited_ = true;
		
		parse_query_parameters(
			query_params_,
			mg_get_request_info( conn_ )->query_parameters_ );
	}
	
	return query_params_;
}

/**********************************************************************************************/
const char* cr_connection::query_string( void ) const
{
	const char* res = mg_get_request_info( conn_ )->query_parameters_;
	return res ? res : "";
}

/**********************************************************************************************/
const char* cr_connection::url( void ) const
{
	return mg_get_request_info( conn_ )->uri_;
}


//////////////////////////////////////////////////////////////////////////
// methods
//////////////////////////////////////////////////////////////////////////


/**********************************************************************************************/
bool cr_connection::fetch(
	const char*		url,
	char*&			out,
	size_t&			out_len,
	cr_string_map*	headers )
{
	return mg_fetch( buf_headers_, out, out_len, mg_get_context( conn_ ), url, headers );
}

/**********************************************************************************************/
bool cr_connection::fetch(
	const std::string&	url,
	std::string&		out,
	cr_string_map*		headers )
{
	char* data;
	size_t len;

	if( fetch( url.c_str(), data, len, headers ) )
	{
		out.assign( data, len );

		free( data );
		return true;
	}

	return false;
}

/**********************************************************************************************/
bool cr_connection::fetch(
	const std::string&	url,
	std::vector<char>&	out,
	cr_string_map*		headers )
{
	char* data;
	size_t len;

	if( fetch( url.c_str(), data, len, headers ) )
	{
		out.resize( len );
		memmove( out.data(), data, len );
		
		free( data );
		return true;
	}

	return false;
}

/**********************************************************************************************/
size_t cr_connection::read( char* buf, size_t len )
{
	return mg_read( conn_, buf, len );
}

/**********************************************************************************************/
void cr_connection::read( std::string& out )
{
	size_t len = content_length();

	out.resize( len );
	if( len )
		out.resize( read( &out[ 0 ], len ) );
}

/**********************************************************************************************/
void cr_connection::read( std::vector<char>& out )
{
	size_t len = content_length();

	out.resize( len );
	if( len )
		out.resize( read( &out[ 0 ], len ) );	
}

/**********************************************************************************************/
void cr_connection::respond(
	cr_http_status	rc,
	const char*		data,
	size_t			data_len,
	cr_string_map*	headers )
{
	if( data && !data_len )
		data_len = strlen( data );
	
	// Compress data if need
	if( data && data_len > 1300 )
	{
		const char* enc_header = header( "accept-encoding" );
		if( enc_header && strstr( enc_header, "deflate" ) )
		{
			size_t out_len = cr_compress_bound( data_len );
			char* out = (char*) malloc( out_len );
			data_len = cr_deflate( data, data_len, out, out_len );

			if( !headers )
				headers = &g_deflate_headers;
			else						
				headers->add( "Content-Encoding", "deflate", 16, 7 );
			
			char header[ 16384 ];
			size_t header_len;
			cr_create_responce_header( header, header_len, rc, data_len, headers );
			mg_write( conn_, header, header_len );
			mg_write( conn_, out, data_len );
			
			free( out );
			return;
		}
	}
	
	// Write non-compressed data
	char header[ 16384 ];
	size_t header_len;
	cr_create_responce_header( header, header_len, rc, data_len, headers );
	mg_write( conn_, header, header_len );
	mg_write( conn_, data, data_len );
}

/**********************************************************************************************/
void cr_connection::respond(
	cr_http_status		rc,
	const std::string&	data,
	cr_string_map*		headers )
{
	respond( rc, data.c_str(), data.length(), headers );
}

/**********************************************************************************************/
void cr_connection::respond_header(
	cr_http_status	rc,
	size_t			data_len,
	cr_string_map*	headers )
{
	char header[ 16384 ];
	size_t header_len;
	cr_create_responce_header( header, header_len, rc, data_len, headers );
	mg_write( conn_, header, header_len );	
}
		
/**********************************************************************************************/
void cr_connection::send_file( 
	const std::string&	path,
	const std::string&	content_type )
{
	time_t t = cr_file_modification_time( path );
	if( !t )
	{
		respond( CR_HTTP_NOT_FOUND );
		return;
	}

	char tstr[ 64 ];
	char* buf = tstr;
	add_number( buf, t );
	
	const char* etag = header( "if-none-match" );
	if( etag && !strcmp( etag, tstr ) )
	{
		if( content_type.length() )
		{
			cr_string_map headers;
			headers.add( "content-type", content_type.c_str(), 12, content_type.length() );
			respond( CR_HTTP_NOT_MODIFIED, NULL, 0, &headers );
		}
		else
		{
			respond( CR_HTTP_NOT_MODIFIED );
		}
		
		return;
	}
	
	FILE* f = fopen( path.c_str(), "rb" );
	if( !f )
	{
		respond( CR_HTTP_FORBIDDEN );
		return;
	}

	fseek( f, 0, SEEK_END );
	
	// Header
	cr_string_map headers;
	headers.add( "etag", tstr, 4 );
	
	if( content_type.length() )
		headers.add( "content-type", content_type.c_str(), 12, content_type.length() );
	
	char header[ 256 ];
	size_t header_len;
	cr_create_responce_header( header, header_len, CR_HTTP_OK, ftell( f ), &headers );
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

/**********************************************************************************************/
int cr_connection::write( const std::string& data )
{
	return write( data.c_str(), data.length() );
}


//////////////////////////////////////////////////////////////////////////
// internal methods
//////////////////////////////////////////////////////////////////////////


/**********************************************************************************************/
cr_connection_internal::~cr_connection_internal( void )
{
	// Free cached data
	free( path_params_.items );
	free( post_params_buffer_ );
}
