/**********************************************************************************************/
/* cr_connection.h		  		                                                   			  */
/*                                                                       					  */
/* Igor Nikitin, 2012																		  */
/* MIT license			                                                  					  */
/**********************************************************************************************/

#pragma once

// STD
#include <string.h>

// CREST
#include "internal/cr_connection_internal.h"
#include "cr_headers.h"


/**********************************************************************************************/
// Connection for single request
//
class cr_connection : public cr_connection_internal
{
	public://////////////////////////////////////////////////////////////////////////

// This class API:		
		
	// ---------------------
	// Properties

								/** Time of connection creation. */
		time_t					get_birth_time( void ) const;

								/** Returns count of bytes in content. */
		size_t					get_content_length( void ) const;
		
								/** Returns value of request header by name,
								 *  name must be in lower case. */
		const char*				get_http_header( 
									const char*	name,
									size_t		name_len = (size_t) -1 ) const;

								/** Returns request's method. */
		const char*				get_http_method( void ) const;
		
								/** Returns value of parameter passed via url,
								 *  see examples. */
		const char*				get_path_parameter( size_t index ) const;
		
								/** Returns value of parameter passed via query string. */
		const char*				get_query_parameter( const char* name ) const;
		
								/** Returns URL of request. */
		const char*				get_url( void ) const;
		
		
	public://////////////////////////////////////////////////////////////////////////
		
	// ---------------------
	// Methods

								/** Fetch data from remote HTTP server. */
		bool					fetch(
									const char*	url,
									char*&		out,
									size_t&		out_len,
									cr_headers*	headers = NULL );
		
								/** The same method for str::string. */
								template< class T >
		bool					fetch(
									const T&	url,
									T&			out,
									cr_headers*	headers = NULL )
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
								
								/** Reads data from the remote end, return number of bytes read. */
		size_t					read( char* buf, size_t len );
		
								/** Reads data from the remote end into string or vector. */
								template< class T >
		void					read( T& out )
								{
									size_t len = get_content_length();
									
									out.resize( len );
									if( len )
										out.resize( read( &out[ 0 ], len ) );
								}
								
								/** Generates and sends HTTP server responce with 
								 *  status code, data-length header and data. */
		void					respond(
									cr_http_status	rc,
									const char*		data	 = NULL,
									size_t			data_len = 0,
									cr_headers*		headers	 = NULL );

								/** Generates and sends HTTP server responce with 
								 *  status code, data-length header and data from string/vector. */
								template< class T >
		void					respond(
									cr_http_status	rc,
									const T&		data,
									cr_headers*		headers = NULL )
								{
									respond( rc, data.c_str(), data.size(), headers );
								}
		
								/** Sends content of file, or respond HTTP_BAD_REQUEST if
								 *  file doesn't exist or not readable. That's very limited method,
								 *  don't use it for heavy tasks. */
		void					send_file( const char* path );
		
								/** Sends data to the client. Returns
								 *  0 when the connection has been closed,
								 *  -1 on error,
								 *  or number of bytes written on success. */
		int						write( const char* buf, size_t len );
		
								/** The same method to send data from string. */
								template< class T >
		int						write( const T&	data )
								{
									return write( data.c_str(), data.size() );
								}
								
};
