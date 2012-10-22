/**********************************************************************************************/
/* cr_xml.cpp				                                                   				  */
/*                                                                       					  */
/* Igor Nikitin, 2012																		  */
/* MIT license			                                                  					  */
/**********************************************************************************************/

// STD
#include <string.h>

// CREST
#include "cr_xml.h"

/**********************************************************************************************/
typedef unsigned char byte;


//////////////////////////////////////////////////////////////////////////
// macroses
//////////////////////////////////////////////////////////////////////////


/**********************************************************************************************/
#define XML_ERROR longjmp( jbuf_, 1 );


//////////////////////////////////////////////////////////////////////////
// constants
//////////////////////////////////////////////////////////////////////////


/**********************************************************************************************/
static const byte g_mask_name[] =
{
	0, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 1, 1, 0, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1
};


//////////////////////////////////////////////////////////////////////////
// functions
//////////////////////////////////////////////////////////////////////////


/**********************************************************************************************/
inline void skip_whitespace_xml( char*& text )
{
	if( text )
	{
		while( strchr( "\n\r\t ", *text ) )
			++text;
	}
}

/**********************************************************************************************/
inline byte to_digit( char ch )
{
	if( ch >= '0' && ch <= '9' ) return ch - '0';
	if( ch >= 'a' && ch <= 'f' ) return ch - 'a' + 10;
	if( ch >= 'A' && ch <= 'F' ) return ch - 'A' + 10;
	
	return 0xFF;
}


//////////////////////////////////////////////////////////////////////////
// methods
//////////////////////////////////////////////////////////////////////////


/**********************************************************************************************/
void cr_xml::parse(
	cr_string_map&	out,
	char*			text )
{
	setjmp( jbuf_ );
	out_ = &out;
	
	while( *text )
	{
		skip_whitespace_xml( text );

		if( *text == '<' )
		{
			++text;
			parse_node( text, 0 );
		}
		else if( *text )
		{
			XML_ERROR;
		}
	}	
}


//////////////////////////////////////////////////////////////////////////
// internal methods
//////////////////////////////////////////////////////////////////////////


/**********************************************************************************************/
void cr_xml::decode_character(
	char*&			text,
	unsigned long	code )
{
	if( code < 0x80 )
	{
		text[ 0 ] = byte( code );
		text += 1;
	}
	else if( code < 0x800 )
	{
		text[ 1 ] = byte( ( code | 0x80 ) & 0xBF ); code >>= 6;
		text[ 0 ] = byte( code | 0xC0 );

		text += 2;
	}
	else if( code < 0x10000 )
	{
		text[ 2 ] = byte( (code | 0x80 ) & 0xBF ); code >>= 6;
		text[ 1 ] = byte( (code | 0x80 ) & 0xBF ); code >>= 6;
		text[ 0 ] = byte( code | 0xE0 );

		text += 3;
	}
	else if( code < 0x110000 )
	{
		text[ 3 ] = byte( ( code | 0x80 ) & 0xBF ); code >>= 6;
		text[ 2 ] = byte( ( code | 0x80 ) & 0xBF ); code >>= 6;
		text[ 1 ] = byte( ( code | 0x80 ) & 0xBF ); code >>= 6;
		text[ 0 ] = byte( code | 0xF0 );

		text += 4;
	}
	else
	{
		XML_ERROR;
	}
}

/**********************************************************************************************/
char cr_xml::parse_and_append_data( char*& text, size_t level )
{
	char* value = text;
	char* end	= skip_and_expand_character_refs( text );

	while( strchr( "\r\n\t ", *( end - 1 ) ) )
		--end;

	if( !level )
		out_->add( name_, value, name_len_, end - value );
	
	char ch = *text;
	*end = 0;
	return ch;
}
	
/**********************************************************************************************/
void cr_xml::parse_element( char*& text, size_t level )
{
	name_ = text;
	while( g_mask_name[ byte(*text) ] )
		++text;
	
	if( text == name_ )
		XML_ERROR;

	name_len_ = text - name_;

	while( *text && *text != '>' && *text != '/' )
		++text;

	if( *text == '>' )
	{
		++text;
		parse_node_contents( text, level );
	}
	else if( *text == '/' )
	{
		++text;

		if( *text != '>' )
			XML_ERROR;
		
		++text;
	}
	else
	{
		XML_ERROR;
	}

	name_[ name_len_ ] = 0;
}

/**********************************************************************************************/
void cr_xml::parse_node( char*& text, size_t level )
{
	switch( *text )
	{
		case '?':
		{
			++text;

			if( ( text[ 0 ] == 'x' || text[ 0 ] == 'X' ) &&
				( text[ 1 ] == 'm' || text[ 1 ] == 'M' ) &&
				( text[ 2 ] == 'l' || text[ 2 ] == 'L' ) &&
				strchr( "\n\r\t ", text[ 3 ] ) )
			{
				text += 4;
			}
			
			skip_declaration( text );
		}
		break;

		case '!':
		{
			++text;
			
			switch( *text )
			{
				case '-':
				{
					if( text[ 1 ] == '-' )
					{
						text += 2;
						skip_comment( text );
						return;
					}
				}
				break;

				case '[':
				{
					if( !strncmp( text + 1, "CDATA[", 6 ) )
					{
						text += 7;
						skip_cdata( text );
						return;
					}
				}
				break;

				case 'D':
				{
					if( !strncmp( text + 1, "OCTYPE", 6 ) && strchr( "\r\n\t ", text[ 7 ] ) )
					{
						text += 8;
						skip_doctype( text );
						return;
					}
				}

				default:;
			}

			for( ++text ; *text != '>' ; ++text )
			{
				if( !*text )
					XML_ERROR;
			}

			++text;
		}
		break;

		default:
			parse_element( text, level );
			break;
	}
}

/**********************************************************************************************/
void cr_xml::parse_node_contents( char*& text, size_t level )
{
	for(;;)
	{
		skip_whitespace_xml( text );
		char ch = *text;

		for(;;)
		{
			if( !ch )
				XML_ERROR;

			if( ch == '<' )
			{
				if( text[ 1 ] == '/' )
				{
					text += 2;
					while( g_mask_name[ byte(*text) ] )
						++text;

					skip_whitespace_xml( text );
					if( *text != '>' )
						XML_ERROR;

					++text;
					return;
				}
				else
				{
					parse_node( ++text, level + 1 );
				}
				
				break;
			}
			else
			{
				ch = parse_and_append_data( text, level );
			}
		}
	}
}

/**********************************************************************************************/
char* cr_xml::skip_and_expand_character_refs( char*& text )
{
	while( *text && *text != '&' && *text != '<' )
		++text;

	char* dest = text;
	char* src  = text;
	
	while( *src && *src != '<' )
	{
		if( *src == '&' )
		{
			switch( src[ 1 ] )
			{
				case 'a':
				{
					if( src[ 2 ] == 'm' && src[ 3 ] == 'p' && src[ 4 ] == ';' )
					{
						*dest++ = '&';
						src += 5;
						
						continue;
					}
					else if( src[ 2 ] == 'p' && src[ 3 ] == 'o' && src[ 4 ] == 's' && src[ 5 ] == ';' )
					{
						*dest++ = '\'';
						src += 6;
						
						continue;
					}
				}
				break;

				case 'g':
				{
					if( src[ 2 ] == 't' && src[ 3 ] == ';' )
					{
						*dest++ = '>';
						src += 4;
						
						continue;
					}
				}
				break;

				case 'l':
				{
					if( src[ 2 ] == 't' && src[ 3 ] == ';' )
					{
						*dest++ = '<';
						src += 4;

						continue;
					}
				}
				break;

				case 'q':
				{
					if( src[ 2 ] == 'u' && src[ 3 ] == 'o' && src[ 4 ] == 't' && src[ 5 ] == ';' )
					{
						*dest++ = '"';
						src += 6;
						
						continue;
					}
				}
				break;

				case '#':
				{
					if( src[ 2 ] == 'x' )
					{
						unsigned long code = 0;
						src += 3;
						
						for(;;)
						{
							byte digit = to_digit( byte(*src) );
							if( digit == 0xFF )
								break;
							
							code = code * 16 + digit;
							++src;
						}
						
						decode_character( dest, code );
					}
					else
					{
						unsigned long code = 0;
						src += 2;
						
						for(;;)
						{
							byte digit = to_digit( byte(*src) );
							if( digit == 0xFF )
								break;

							code = code * 10 + digit;
							++src;
						}
						
						decode_character( dest, code );
					}
					
					if( *src++ != ';' )
						XML_ERROR;
					
					continue;
				}
				break;
				
				default:;
			}
		}

		*dest++ = *src++;
	}
	
	text = src;
	return dest;
}
	
/**********************************************************************************************/
void cr_xml::skip_cdata( char*& text )
{
	while( text[ 0 ] != ']' || text[ 1 ] != ']' || text[ 2 ] != '>' )
	{
		if( !*text++ )
			XML_ERROR;
	}

	text += 3;
}
	
/**********************************************************************************************/
void cr_xml::skip_comment( char*& text )
{
	while( text[ 0 ] != '-' || text[ 1 ] != '-' || text[ 2 ] != '>' )
	{
		if( !*text++ )
			XML_ERROR;
	}

	text += 3;
}
	
/**********************************************************************************************/
void cr_xml::skip_declaration( char*& text )
{
	while( text[ 0 ] != '?' || text[ 1 ] != '>' )
	{
		if( !*text++ )
			XML_ERROR;
	}

	text += 2;
}

/**********************************************************************************************/
void cr_xml::skip_doctype( char*& text )
{
	while( *text != '>' )
	{
		if( !*text )
		{
			XML_ERROR;
		}
		else if( *text++ == '[' )
		{
			int depth = 1;
			while( depth )
			{
				switch( *text++ )
				{
					case '[': ++depth; break;
					case ']': --depth; break;
					case 0  : XML_ERROR;
					default :;
				}
			}			
		}
	}

	++text;
}
