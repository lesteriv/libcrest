/**********************************************************************************************/
/* connection_internal.h			                                                   		  */
/*                                                                       					  */
/* Igor Nikitin, 2012																		  */
/* MIT license			                                                  					  */
/**********************************************************************************************/

#pragma once

// CREST
#include "../crest_types.h"

/**********************************************************************************************/
struct mg_connection;


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
		crest_string_array		query_params_;
};
