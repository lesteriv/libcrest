/**********************************************************************************************/
/* cr_connection_internal.h			                                                   		  */
/*                                                                       					  */
/* Igor Nikitin, 2013																		  */
/* MIT license			                                                  					  */
/**********************************************************************************************/

#pragma once

// STD
#include <string>
#include <time.h>
#include <vector>

// CREST
#include "../cr_string_map.h"
#include "../cr_types.h"

/**********************************************************************************************/
struct cr_connection_data;

/**********************************************************************************************/
struct cr_string_array
{
	size_t	count;
	char**	items;
};


/**********************************************************************************************/
// Internal members for connection
//
class cr_connection_internal
{
	protected://////////////////////////////////////////////////////////////////////////
		
								~cr_connection_internal( void );
	
	protected://////////////////////////////////////////////////////////////////////////
		
// References
		
		cr_connection_data*		conn_;


// Properties

mutable	cr_string_map			cookies_;
		cr_string_array			path_params_;
mutable	cr_string_map			post_params_;
mutable	cr_string_map			query_params_;


// Flags

mutable	bool					cookies_inited_			: 1;
mutable	bool					query_params_inited_	: 1;


// Cached data

		char					fetch_headers_data_[ 16384 ];
mutable	char*					post_params_buffer_;
};
