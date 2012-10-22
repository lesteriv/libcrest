/**********************************************************************************************/
/* cr_json.cpp				                                                   				  */
/*                                                                       					  */
/* Igor Nikitin, 2012																		  */
/* MIT license			                                                  					  */
/**********************************************************************************************/

// STD
#include <string.h>

// CREST
#include "cr_json.h"


//////////////////////////////////////////////////////////////////////////
// macroses
//////////////////////////////////////////////////////////////////////////


/**********************************************************************************************/
#define BREAK { text = 0; return; }


//////////////////////////////////////////////////////////////////////////
// prototypes
//////////////////////////////////////////////////////////////////////////


/**********************************************************************************************/
static void parse_object ( cr_string_map* out_map, char*& text );
static void parse_value	 ( char*& text, char** out, size_t* out_len );


//////////////////////////////////////////////////////////////////////////
// helper functions
//////////////////////////////////////////////////////////////////////////


/**********************************************************************************************/
inline void skip_whitespace_json( char*& text )
{
	if( text )
	{
		while( *text && (unsigned char) *text <= 32 )
			++text;
	}
}

/**********************************************************************************************/
static void parse_array( char*& text )
{
	if( !text )
		return;
	
	++text;
	skip_whitespace_json( text );
	
	if( *text == ']' )
	{
		++text;
		return;
	}

	do
	{
		skip_whitespace_json( text );
		parse_value( text, NULL, NULL );
		skip_whitespace_json( text );
	}
	while( text && *text == ',' && ++text );

	if( !text || *text++ != ']' )
		BREAK;
}

/**********************************************************************************************/
static void parse_string( 
	char*&	text,
	char**	out,
	size_t*	out_len )
{
	if( !text || !*text || *text++ != '"' )
		BREAK;
	
	if( out )
		*out = text;
	
	char* tail = text;
	
	while( *text && *text != '"' )
	{
		if( *text == '\\' )
		{
			switch( *++text )
			{
				case 'b': *tail++ = '\b'; break;
				case 'f': *tail++ = '\f'; break;
				case 'n': *tail++ = '\n'; break;
				case 'r': *tail++ = '\r'; break;
				case 't': *tail++ = '\t'; break;
				
				default:
				{
					if( *text )
						*tail++ = *text;
				}
			}
			
			if( *text )
				++text;
		}
		else
		{
			*tail++ = *text++;
		}
	}
	
	if( *text )
		++text;
	
	if( out_len )
		*out_len = tail - *out;
}

/**********************************************************************************************/
static void parse_value(
	char*&	text,
	char**	out,
	size_t*	out_len )
{
	if( !text )
		return;
	
	skip_whitespace_json( text );
	
	if( !strncmp( text, "false", 5 ) )
	{
		if( out )
		{
			*out = text;
			*out_len = 5;
		}
		
		text += 5;
	}
	else if( !strncmp( text, "true", 4 ) || !strncmp( text, "null", 4 ) )
	{
		if( out )
		{
			*out = text;
			*out_len = 4;
		}
		
		text += 4;
	}
	else if( *text == '\"' )
	{
		parse_string( text, out, out_len );
	}
	else if( *text == '-' || ( *text >= '0' && *text <= '9' ) )
	{
		if( out )
			*out = text;
		
		while( strchr( "01234567890-+.eE", *text ) )
			++text;
		
		if( out_len )
			*out_len = text - *out;
	}
	else if( *text == '[' )
	{
		if( out )
		{
			*out = text;
			*out_len = 0;
		}
		
		parse_array( text );
	}
	else if( *text == '{' )
	{
		if( out )
		{
			*out = text;
			*out_len = 0;
		}
		
		parse_object( NULL, text );
	}
	else
	{
		BREAK;
	}
}

/**********************************************************************************************/
static void parse_object( 
	cr_string_map*	out_map,
	char*&			text )
{
	if( !text )
		return;
	
	skip_whitespace_json( text );
	if( *text++ != '{' )
		BREAK

	skip_whitespace_json( text );
	
	if( *text == '}' )
	{
		++text;
		return;
	}
	
	do
	{
		skip_whitespace_json( text );
		
		char* name = NULL;
		size_t name_len;
		
		parse_string( text, &name, &name_len );
		if( !name || !text )
			return;
		
		skip_whitespace_json( text );
		if( *text++ != ':' )
			BREAK;

		char* value = NULL;
		size_t value_len;
		
		parse_value( text, &value, &value_len );

		skip_whitespace_json( text );
		if( !text || !*text )
			BREAK;
		
		if( out_map )
			out_map->add( name, value, name_len, value_len );
	}
	while( text && *text++ == ',' );

	if( !text )
		return;
	
	--text;
	if( *text++ != '}' )
		BREAK;
}


//////////////////////////////////////////////////////////////////////////
// methods
//////////////////////////////////////////////////////////////////////////


/**********************************************************************************************/
void cr_json::parse(
	cr_string_map&	out,
	char*			text )
{
	parse_object( &out, text );
	
	size_t count = out.size();
	for( size_t i = 0 ; i < count ; ++i )
	{
		*((char*) ( out.name( i ) + out.name_len( i ) )) = 0;
		*((char*) ( out.value( i ) + out.value_len( i ) )) = 0;
	}
}
