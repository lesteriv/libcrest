/**********************************************************************************************/
/* cr_string_map.h				  	                                                   		  */
/*                                                                       					  */
/* Igor Nikitin, 2013																		  */
/* MIT license			                                                  					  */
/**********************************************************************************************/

#pragma once

// CREST
#include "internal/cr_string_map_internal.h"


/**********************************************************************************************/
class cr_string_map : protected cr_string_map_internal
{
	public://////////////////////////////////////////////////////////////////////////
	
								cr_string_map( void );
								
								cr_string_map(
									const char* name,
									const char* value );
								
	public://////////////////////////////////////////////////////////////////////////

// This class API:

	// ---------------------
	// Properties

		const char*				name( size_t index ) const;
		size_t					name_len( size_t index ) const;
		
		size_t					size( void ) const;
		
		const char*				value( size_t index ) const;
		size_t					value_len( size_t index ) const;


	public://////////////////////////////////////////////////////////////////////////

	// ---------------------
	// Methods
		
		void					add(
									const char*	name,
									const char*	value,
									size_t		name_len = (size_t) -1,
									size_t		value_len = (size_t) -1 );
		
		void					clear( void );
		
		int						find(
									const char* name,
									size_t		name_len = (size_t) -1 ) const;
		
		const char*				value(
									const char*	name,
									size_t		name_len = (size_t) -1 ) const;
		
		
	public://////////////////////////////////////////////////////////////////////////
		
	// ---------------------
	// Operators
		
		const char*				operator[]( const char* name ) const;
};
