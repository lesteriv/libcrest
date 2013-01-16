/**********************************************************************************************/
/* cr_result_internal.h								                                   		  */
/*                                                                       					  */
/* Igor Nikitin, 2013																		  */
/* MIT license			                                                  					  */
/**********************************************************************************************/

#pragma once

// CREST
#include "../cr_types.h"

/**********************************************************************************************/
class cr_connection;


/**********************************************************************************************/
// Internal members for cr_result
//
class cr_result_internal
{
	protected://////////////////////////////////////////////////////////////////////////

	// ---------------------
	// Internal methods

		void					finish( void );
		
		
	protected://////////////////////////////////////////////////////////////////////////
		
// Properties
		
		const cr_result_fields*	fields_			= NULL;
		bool					finished_		= false;
		cr_result_format		format_;
		bool					has_properties_	= false;
		const std::string*		name_;


// Runtime data
		
		size_t					column_			= 0;
		
		
//  Result
		
		std::string				data_;
		size_t					properties_		= 0;
		size_t					records_		= 0;
};
