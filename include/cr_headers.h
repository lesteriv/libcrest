/**********************************************************************************************/
/* cr_headers.h			  		                                                   			  */
/*                                                                       					  */
/* Igor Nikitin, 2012																		  */
/* MIT license			                                                  					  */
/**********************************************************************************************/

#pragma once

// CREST
#include "internal/cr_headers_internal.h"


/**********************************************************************************************/
class cr_headers : public cr_headers_internal
{
	public://////////////////////////////////////////////////////////////////////////
		
							cr_headers( void );
							
							cr_headers(
								const char* name,
								const char* value );
	
	public://////////////////////////////////////////////////////////////////////////
		
// This class API:		
		
	// ---------------------
	// Methods

		void				add(
								const char*	name,
								const char*	value,
								size_t		name_len = (size_t) -1,
								size_t		value_len = (size_t) -1 );
		
		size_t				count( void ) const;
		
		const char*			name( size_t index ) const;
		size_t				name_len( size_t index ) const;
		
		void				reset( void );
		size_t				size( void );
		
		const char*			value( size_t index ) const;
		const char*			value( const char* name ) const;
		size_t				value_len( size_t index ) const;
};
