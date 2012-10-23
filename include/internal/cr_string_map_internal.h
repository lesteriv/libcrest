/**********************************************************************************************/
/* cr_string_map_internal.h		                                                   			  */
/*                                                                       					  */
/* Igor Nikitin, 2012																		  */
/* MIT license			                                                  					  */
/**********************************************************************************************/

#pragma once

// STD
#include <stddef.h>

/**********************************************************************************************/
#define CREST_MAP_SIZE 64


/**********************************************************************************************/
class cr_string_map_internal
{
	protected://////////////////////////////////////////////////////////////////////////

// Properties
		
		const char*			name_[ CREST_MAP_SIZE ];
		size_t				name_len_[ CREST_MAP_SIZE ];
		size_t				size_;
		const char*			value_[ CREST_MAP_SIZE ];
		size_t				value_len_[ CREST_MAP_SIZE ];
};
