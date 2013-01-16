/**********************************************************************************************/
/* cr_cache.h			  		                                                   			  */
/*                                                                       					  */
/* Igor Nikitin, 2013																		  */
/* MIT license			                                                  					  */
/**********************************************************************************************/

#pragma once

// CREST
#include "internal/cr_cache_internal.h"
#include "cr_connection.h"


/**********************************************************************************************/
// Cache for responces
//
class cr_cache : public cr_cache_internal
{
	public://////////////////////////////////////////////////////////////////////////
		
							cr_cache( int expire_time = 0 );
	
	public://////////////////////////////////////////////////////////////////////////

// This class API:		
		
	// ---------------------
	// Methods
		
							/** Remove entry from cache. */
		void				erase( cr_connection& conn );
		
							/** Search for match entry, if found - send it to client
							 *  and returns TRUE. */
		bool				find( cr_connection& conn );
		
							/** Generates and sends HTTP responce with 
							 *  status code, data-length header and data. */
		void				push(
								cr_connection&		conn,
								cr_http_status		rc,
								const char*			data	 = NULL,
								size_t				data_len = 0,
								cr_string_map		headers	 = cr_string_map() );

							/** 'insert' method for std::string. */
		void				push(
								cr_connection&		conn,
								cr_http_status		rc,
								const std::string&	data,
								cr_string_map		headers = cr_string_map() );
};
