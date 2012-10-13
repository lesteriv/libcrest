/**********************************************************************************************/
/* cr_connection.h		  		                                                   			  */
/*                                                                       					  */
/* Igor Nikitin, 2012																		  */
/* MIT license			                                                  					  */
/**********************************************************************************************/

#pragma once

// CREST
#include "internal/cr_connection_internal.h"


/**********************************************************************************************/
// Connection for single request
//
class cr_connection : public cr_connection_internal
{
	public://////////////////////////////////////////////////////////////////////////

// This class API:		
		
	// ---------------------
	// Properties

								/** Returns connection's birth time. */
		time_t					birth_time( void ) const;

								/** Returns count of bytes in content for POST/PUT requests. */
		size_t					content_length( void ) const;
		
								/** Returns single cookie's value.
								 *  NB: method change value of 'cookie' header. */
		const char*				cookie( const char* name ) const;
		
								/** Returns cookies for request.
								 *  NB: method change value of 'cookie' header. */
		const cr_string_map&	cookies( void ) const;

								/** Returns value of single HTTP header. */
		const char*				header( const char* name ) const;

								/** Returns HTTP headers for request. */
		const cr_string_map&	headers( void ) const;

								/** Returns request's method. */
		const char*				method( void ) const;
		
								/** Returns value of parameter passed via url.
								 *  TODO */
		const char*				path_parameter( size_t index ) const;
		
								/** Returns value of parameter passed via query string. */
		const char*				query_parameter( const char* name ) const;
		
								/** Returns all query parameters. */
		const cr_string_map&	query_parameters( void ) const;
		
								/** Returns 'raw' query string after ? symbol. */
		const char*				query_string( void ) const;
		
								/** Returns URL of request. */
		const char*				url( void ) const;
		
		
	public://////////////////////////////////////////////////////////////////////////
		
	// ---------------------
	// Methods

								/** Fetch data from remote HTTP server. */
		bool					fetch(
									const char*		url,
									char*&			out,
									size_t&			out_len,
									cr_string_map*	headers = NULL );
		
								/** Reads data from the remote end, return number of bytes read. */
		size_t					read( char* buf, size_t len );
		
								/** Generates and sends HTTP server responce with 
								 *  status code, data-length header and data. */
		void					respond(
									cr_http_status	rc,
									const char*		data	 = NULL,
									size_t			data_len = 0,
									cr_string_map*	headers	 = NULL );

								/** Generates and sends HTTP server responce with 
								 *  status code and data-length header. */
		void					respond_header(
									cr_http_status	rc,
									size_t			data_len,
									cr_string_map*	headers	 = NULL );
		
								/** Sends content of file, or respond HTTP_BAD_REQUEST if
								 *  file doesn't exist or not readable. That's very limited method,
								 *  don't use it for heavy tasks. */
		void					send_file( 
									const char* path,
									const char* content_type = NULL );
		
								/** Sends data to the client. Returns
								 *  0 when the connection has been closed,
								 *  -1 on error,
								 *  or number of bytes written on success. */
		int						write( const char* buf, size_t len );
		

	public://////////////////////////////////////////////////////////////////////////
		
	// ---------------------
	// Methods for std::string

								template< class T >
		bool					fetch(
									const T&		url,
									T&				out,
									cr_string_map*	headers = NULL )
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

								template< class T >
		void					read( T& out )
								{
									size_t len = content_length();
									
									out.resize( len );
									if( len )
										out.resize( read( &out[ 0 ], len ) );
								}

								template< class T >
		void					respond(
									cr_http_status	rc,
									const T&		data,
									cr_string_map*	headers = NULL )
								{
									respond( rc, data.c_str(), data.length(), headers );
								}
								
								template< class T >
		void					send_file( 
									const T&	path,
									const T&	content_type = "" )
								{
									send_file( path.c_str(), content_type.c_str() );
								}

		void					send_file( 
									char*		path,
									const char*	content_type = "" )
								{
									send_file( (const char*) path, content_type );
								}
								
								template< class T >
		void					send_file( 
									const T&	path,
									const char*	content_type = "" )
								{
									send_file( path.c_str(), content_type );
								}

								template< class T >
		int						write( const T&	data )
								{
									return write( data.c_str(), data.size() );
								}
};
