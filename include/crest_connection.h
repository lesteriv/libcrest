/**********************************************************************************************/
/* Connection.h			  		                                                   			  */
/*                                                                       					  */
/* Igor Nikitin, 2012																		  */
/* MIT license			                                                  					  */
/**********************************************************************************************/

#pragma once

// CREST
#include "internal/crest_connection_internal.h"


/**********************************************************************************************/
// Represents connection for request
//
class crest_connection : public crest_connection_internal
{
	public://////////////////////////////////////////////////////////////////////////

// This class API:		
		
	// ---------------------
	// Properties

								/** Time of connection creation. */
		time_t					get_birth_time( void ) const;

								/** Returns count of bytes in content. */
		size_t					get_content_length( void ) const;
		
								/** Returns value of request header by name. */
		const char*				get_http_header( const char* name ) const;

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
		
								/** Reads data from the remote end, return number of bytes read. */
		size_t					read( char* buf, size_t len );

								/** Generates and sends HTTP server responce with 
								 *  status code, data-length header and data. */
		void					respond(
									crest_http_status	rc,
									const char*			msg,
									size_t				msg_len );

								/** Sends content of file, or respond HTTP_BAD_REQUEST if
								 *  file doesn't exist or not readable. */
		void					send_file( const char* path );
		
								/** Sends data to the client. Returns
								 *  0 when the connection has been closed,
								 *  -1 on error,
								 *  or number of bytes written on success. */
		int						write( const char* buf, size_t len );
};
