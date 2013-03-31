/**********************************************************************************************/
/* cr_result.h			  	                                                   				  */
/*                                                                       					  */
/* Igor Nikitin, 2013																		  */
/* MIT license			                                                  					  */
/**********************************************************************************************/

#pragma once

// CREST
#include "internal/cr_result_internal.h"


/**********************************************************************************************/
class cr_result : public cr_result_internal
{
	public://////////////////////////////////////////////////////////////////////////
		
							cr_result( cr_result_format format );
							cr_result( cr_connection& conn );
								
	public://////////////////////////////////////////////////////////////////////////
		
// This class API:

	// ---------------------
	// Properties methods
		
		void				add_property(
								const std::string&	key,
								const std::string&	value );

		
	public://////////////////////////////////////////////////////////////////////////

	// ---------------------
	// Table methods

		void				add_double( double value );
		void				add_int( int64_t value );
		void				add_null( void );
		void				add_text( const std::string& value );

		void				set_record_fields( const cr_result_fields& fields );
		void				set_record_name( const std::string& name );


	public://////////////////////////////////////////////////////////////////////////

	// ---------------------
	// Export methods
		
		std::string&		data( void );


	public://////////////////////////////////////////////////////////////////////////

	// ---------------------
	// Operators
		
		operator			std::string&() { return data(); }
};
