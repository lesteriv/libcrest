/**********************************************************************************************/
/* Connection.h			  		                                                   			  */
/*                                                                       					  */
/* Igor Nikitin, 2012																		  */
/* MIT license			                                                  					  */
/**********************************************************************************************/

#pragma once

// CREST
#include "internal/connection_internal.h"

/**********************************************************************************************/
CREST_NAMESPACE_START


/**********************************************************************************************/
// Represents connection for request
//
class connection : public connection_internal
{
	public://////////////////////////////////////////////////////////////////////////

// This class API:		
		
	// ---------------------
	// Properties

								/** Time of connection creation. */
		time_t					get_birth_time( void ) const;
		
								/** Returns value of request header by name. */
		const char*				get_http_header( const char* name );

								/** Returns value of parameter passed via url,
								 *  see examples. */
		const string&			get_path_parameter( size_t index );
		
								/** Returns value of parameter passed via query string. */
		string					get_query_parameter( const char* name );
		
		
	public://////////////////////////////////////////////////////////////////////////
		
	// ---------------------
	// Methods
		
								/** Reads data from the remote end, return number of bytes read. */
		size_t					read( char* buf, size_t len );

								/** Generates and sends HTTP server responce with 
								 *  status code, data-length header and data. */
		void					respond(
									http_status		rc,
									const string&	data );

								/** Sends content of file, or respond HTTP_BAD_REQUEST if
								 *  file doesn't exist or not readable. */
		void					send_file( const string& path );
		
								/** Sends data to the client. Returns
								 *  0 when the connection has been closed,
								 *  -1 on error,
								 *  or number of bytes written on success. */
		int						write( const string& str );
};


/**********************************************************************************************/
CREST_NAMESPACE_END
