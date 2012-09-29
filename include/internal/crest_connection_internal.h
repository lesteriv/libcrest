/**********************************************************************************************/
/* connection_internal.h			                                                   		  */
/*                                                                       					  */
/* Igor Nikitin, 2012																		  */
/* MIT license			                                                  					  */
/**********************************************************************************************/

#pragma once

// STD
#include <string>
#include <vector>

// CREST
#include "../crest_types.h"

/**********************************************************************************************/
struct mg_connection;

/**********************************************************************************************/
using std::string;
using std::vector;


/**********************************************************************************************/
// Internal members for connection
//
class crest_connection_internal
{
	protected://////////////////////////////////////////////////////////////////////////
		
// References
		
		mg_connection*			conn_;


// Properties

		vector<string>			path_params_;
};
