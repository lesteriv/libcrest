/**********************************************************************************************/
/* cr_connection_internal.h			                                                   		  */
/*                                                                       					  */
/* Igor Nikitin, 2012																		  */
/* MIT license			                                                  					  */
/**********************************************************************************************/

#pragma once

// STD
#include <stddef.h>
#include <string>
#include <time.h>

// CREST
#include "../cr_string_map.h"
#include "../cr_types.h"

/**********************************************************************************************/
struct mg_connection;

/**********************************************************************************************/
struct cr_string_array
{
	size_t	count_;
	char**	items_;
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
		
		mg_connection*			conn_;


// Properties

		char					buf_headers_[ 16384 ];
mutable	cr_string_map			cookies_;
		cr_string_array			path_params_;
mutable	cr_string_map			post_params_;
mutable	cr_string_map			query_params_;


// Flags

mutable	bool					cookies_inited_			: 1;
mutable	bool					post_params_inited_		: 1;
mutable	bool					query_params_inited_	: 1;
};
