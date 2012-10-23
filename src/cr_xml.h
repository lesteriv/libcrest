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
		
		void				parse_element		( char*& text, size_t level );
		void				parse_node			( char*& text, size_t level );
		void				parse_node_contents	( char*& text, size_t level );
		char				parse_value			( char*& text, size_t level );
		
		void				skip_cdata			( char*& text );
		void				skip_comment		( char*& text );
		void				skip_declaration	( char*& text );
		void				skip_doctype		( char*& text );
		char*				skip_string			( char*& text );
		

	protected://////////////////////////////////////////////////////////////////////////
		
// Internal data		
		
		jmp_buf				jbuf_;
		char*				name_;
		size_t				name_len_;
		cr_string_map*		out_;
};
