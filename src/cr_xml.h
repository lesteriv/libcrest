/**********************************************************************************************/
/* cr_xml.h					  		                                                   		  */
/*                                                                       					  */
/* Igor Nikitin, 2012																		  */
/* MIT license   																		  	  */
/**********************************************************************************************/

#pragma once

// STD
#include <setjmp.h>

// CREST
#include "../include/cr_string_map.h"


/**********************************************************************************************/
class cr_xml
{
	public://////////////////////////////////////////////////////////////////////////
		
// This class API:

	// ---------------------
	// Methods

		void				parse(
								cr_string_map&	out,
								char*			text );
							
		
	protected://////////////////////////////////////////////////////////////////////////
		
	// ---------------------
	// Internal methods
		
		void				decode_character( 
								char*&			text,
								unsigned long	code );
		
		char				parse_and_append_data( char*& text );
		void				parse_element( char*& text );
		void				parse_node( char*& text );
		void				parse_node_contents( char*& text );
		
		char*				skip_and_expand_character_refs( char*& text );
		void				skip_cdata( char*& text );
		void				skip_comment( char*& text );
		void				skip_declaration( char*& text );
		void				skip_doctype( char*& text );
		

	protected://////////////////////////////////////////////////////////////////////////
		
// Internal data		
		
		jmp_buf				jbuf_;
		char*				name_;
		size_t				name_len_;
		cr_string_map*		out_;
};
