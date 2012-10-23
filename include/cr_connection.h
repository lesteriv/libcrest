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
class cr_connection : protected cr_connection_internal
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
		const char*				cookie(
									const char*	name,
									size_t		name_len = (size_t) -1 ) const;
		
								/** 'cookie' method for std::string. */
		const char*				cookie( const std::string& name ) const;
		
								/** Returns cookies for request.
								 *  NB: method change value of 'cookie' header. */
		const cr_string_map&	cookies( void ) const;

								/** Returns value of single HTTP header. */
		const char*				header(
									const char*	name,
									size_t		name_len = (size_t) -1 ) const;

								/** 'header' method for std::string. */
		const char*				header( const std::string& name ) const;
		
								/** Returns HTTP headers for request. */
		const cr_string_map&	headers( void ) const;

								/** Returns request's method. */
		const char*				method( void ) const;
		
								/** Returns value of parameter passed via url or empty
								 *  string if index too big. */
		const char*				path_parameter( size_t index ) const;

								/** Returns value of parameter passed via POST data. */
		const char*				post_parameter( 
									const char*	name,
									size_t		name_len = (size_t) -1 );
		
								/** 'post_parameter' method for std::string. */
		const char*				post_parameter( const std::string& name );
								
								/** Returns all post parameters. */
		const cr_string_map&	post_parameters( void );
		
								/** Returns value of parameter passed via query string. */
		const char*				query_parameter( 
									const char*	name,
									size_t		name_len = (size_t) -1 ) const;
		
								/** 'query_parameter' method for std::string. */
		const char*				query_parameter( const std::string& name ) const;
								
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
									const char*			url,
									char*&				out,
									size_t&				out_len,
									cr_string_map*		headers = NULL );
		
								/** 'fetch' method for std::string. */
		bool					fetch(
									const std::string&	url,
									std::string&		out,
									cr_string_map*		headers = NULL );
								
								/** Reads data from the remote end, return number of bytes read. */
		size_t					read( char* buf, size_t len );
		
								/** Reads data from the remote end to std::string or std::vector<char>. */
		void					read( std::string& out );
								
								/** Generates and sends HTTP responce with 
								 *  status code, data-length header and data. */
		void					respond(
									cr_http_status		rc,
									const char*			data	 = NULL,
									size_t				data_len = 0,
									cr_string_map*		headers	 = NULL );

								/** 'respond' method for std::string. */
		void					respond(
									cr_http_status		rc,
									const std::string&	data,
									cr_string_map*		headers = NULL );
								
								/** Generates and sends HTTP server responce with 
								 *  status code and data-length header. */
		void					respond_header(
									cr_http_status		rc,
									size_t				data_len,
									cr_string_map*		headers	 = NULL );
		
								/** Sends content of file, or respond HTTP_BAD_REQUEST if
								 *  file doesn't exist or not readable. */
		void					send_file( 
									const char* path,
									const char* content_type = NULL );
		
								/** 'send_file' method for std::string. */
		void					send_file( 
									const std::string&	path,
									const std::string&	content_type = "" );

								/** Sends data to the client. Returns
								 *  0 when the connection has been closed,
								 *  -1 on error,
								 *  or number of bytes written on success. */
		int						write( const char* buf, size_t len );
		
								/** 'write' method for std::string. */
		int						write( const std::string& data );
};


/**********************************************************************************************/
// Implement methods for std::string
//
#include "internal/cr_connection_imp.h"
