/**********************************************************************************************/
/* connection_internal.h			                                                   		  */
/*                                                                       					  */
/* Igor Nikitin, 2012																		  */
/* MIT license			                                                  					  */
/**********************************************************************************************/

#pragma once

// STD
#include <stddef.h>
#include <time.h>

// CREST
#include "../crest_types.h"

/**********************************************************************************************/
struct mg_connection;

/**********************************************************************************************/
struct crest_string_array
{
	size_t	count_;
	char**	items_;
};


/**********************************************************************************************/
// Internal members for connection
//
class crest_connection_internal
{
	protected://////////////////////////////////////////////////////////////////////////
		
								~crest_connection_internal( void );
	
	protected://////////////////////////////////////////////////////////////////////////
		
// References
		
		mg_connection*			conn_;


// Properties

		crest_string_array		path_params_;
		
mutable	size_t					query_params_count_;
mutable	char*					query_params_names_[ 64 ];
mutable	char*					query_params_values_[ 64 ];
};
