/**********************************************************************************************/
/* cr_post_parameters.h			  	                                                   		  */
/*                                                                       					  */
/* Igor Nikitin, 2012																		  */
/* MIT license			                                                  					  */
/**********************************************************************************************/

#pragma once

// CREST
#include "internal/cr_post_parameters_internal.h"


/**********************************************************************************************/
class cr_post_parameters : public cr_post_parameters_internal
{
	public://////////////////////////////////////////////////////////////////////////
		
								cr_post_parameters( char* str );
								cr_post_parameters( cr_connection& conn );

	public://////////////////////////////////////////////////////////////////////////

// This class API:

	// ---------------------
	// Properties

		size_t					get_parameter_count	( void ) const;
		const char*				get_parameter_name	( size_t index ) const;
		const char*				get_parameter_value	( size_t index ) const;
};
