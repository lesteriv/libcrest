/**********************************************************************************************/
/* cr_connection_imp.h				                                                   		  */
/*                                                                       					  */
/* Igor Nikitin, 2012																		  */
/* MIT license			                                                  					  */
/**********************************************************************************************/

#pragma once

// STD
#include <stdlib.h>


//////////////////////////////////////////////////////////////////////////
// properties
//////////////////////////////////////////////////////////////////////////


/**********************************************************************************************/
inline const char* cr_connection::cookie( const std::string& name ) const
{
	return cookie( name.c_str(), name.length() );
}

/**********************************************************************************************/
inline const char* cr_connection::header( const std::string& name ) const
{
	return header( name.c_str(), name.length() );
}

/**********************************************************************************************/
inline const char* cr_connection::post_parameter( const std::string& name )
{
	return post_parameter( name.c_str(), name.length() );
}

/**********************************************************************************************/
inline const char* cr_connection::query_parameter( const std::string& name ) const
{
	return query_parameter( name.c_str(), name.length() );
}


//////////////////////////////////////////////////////////////////////////
// methods
//////////////////////////////////////////////////////////////////////////


/**********************************************************************************************/
inline bool cr_connection::fetch(
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
inline void cr_connection::read( std::string& out )
{
	size_t len = content_length();

	out.resize( len );
	if( len )
		out.resize( read( &out[ 0 ], len ) );
}

/**********************************************************************************************/
inline void cr_connection::respond(
	cr_http_status		rc,
	const std::string&	data,
	cr_string_map*		headers )
{
	respond( rc, data.c_str(), data.length(), headers );
}

/**********************************************************************************************/
inline void cr_connection::send_file( 
	const std::string&	path,
	const std::string&	content_type )
{
	send_file( path.c_str(), content_type.c_str() );
}

/**********************************************************************************************/
inline int cr_connection::write( const std::string& data )
{
	return write( data.c_str(), data.length() );
}
