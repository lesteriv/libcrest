/**********************************************************************************************/
/* cr_xml.h			  		                                                   				  */
/*                                                                       					  */
/* Igor Nikitin, 2012																		  */
/* MIT license			                                                  					  */
/**********************************************************************************************/

// Used some code from RapidXML, really great library - use it if you search for
// XML parser in C++, cr_xml is only part of XML parser.

// Copyright (C) 2006, 2009 Marcin Kalicinski
// Version 1.13
// Revision $DateTime: 2009/05/13 01:46:17 $
// Site: http://rapidxml.sourceforge.net/
// License: http://rapidxml.sourceforge.net/license.txt

#pragma once

// STD
#include <setjmp.h>
#include <stdlib.h>
#include <string.h>

/**********************************************************************************************/
#define XML_ERROR		longjmp( jbuf_, 1 );
#define XML_POOL_SIZE	64

/**********************************************************************************************/
typedef unsigned char byte;

/**********************************************************************************************/
struct cr_xml;
struct cr_xml_node;


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


/**********************************************************************************************/
inline void skip_whitespace( char*& text )
{
	while( strchr( "\n\r\t ", *text ) ) ++text;
}

/**********************************************************************************************/
inline byte to_digit( char ch )
{
	if( ch >= '0' && ch <= '9' ) return ch - '0';
	if( ch >= 'a' && ch <= 'f' ) return ch - 'a' + 10;
	if( ch >= 'A' && ch <= 'F' ) return ch - 'A' + 10;
	
	return 0xFF;
}


/**********************************************************************************************/
struct cr_xml_node
{
	void append_child_node( cr_xml_node* node )
	{
		if( !node )
			return;
		
		first_child_ ?
			last_child_->next_sibling_ = node :
			first_child_ = node;

		last_child_ = node;
		node->next_sibling_ = 0;
	}
	
	cr_xml_node*	first_child_;
	cr_xml_node*	last_child_;
	char*			name_;
	size_t			name_len_;
	cr_xml_node*	next_sibling_;
	char*			value_;
	size_t			value_len_;
};

/**********************************************************************************************/
struct cr_xml: public cr_xml_node
{
	cr_xml( char* text )
	{
		first_child_	= 0;
		pool_pos_		= 0;

		setjmp( jbuf_ );
		parse( text );
	}

	void decode_character( char*& text, unsigned long code )
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
			text[ 0 ] = byte( code | 0xF0);

			text += 4;
		}
		else
		{
			XML_ERROR;
		}
	}
	
	void parse( char* text )
	{
		while( *text )
		{
			skip_whitespace( text );
			
			if( *text == '<' )
			{
				++text;
				append_child_node( parse_node( text ) );
			}
			else if( *text )
			{
				XML_ERROR;
			}
		}
	}
	
	char* skip_and_expand_character_refs( char*& text )
	{
		while( *text && *text != '&' && *text != '<' )
			++text;
		
		char *src = text;
		char *dest = src;
		while( *src && *src != '<' )
		{
				if (src[0] == char('&'))
				{
					switch (src[1])
					{
					case char('a'):
						if (src[2] == char('m') && src[3] == char('p') && src[4] == char(';'))
						{
							*dest = char('&');
							++dest;
							src += 5;
							continue;
						}
						if (src[2] == char('p') && src[3] == char('o') && src[4] == char('s') && src[5] == char(';'))
						{
							*dest = char('\'');
							++dest;
							src += 6;
							continue;
						}
						break;
					case char('q'):
						if (src[2] == char('u') && src[3] == char('o') && src[4] == char('t') && src[5] == char(';'))
						{
							*dest = char('"');
							++dest;
							src += 6;
							continue;
						}
						break;
					case char('g'):
						if (src[2] == char('t') && src[3] == char(';'))
						{
							*dest = char('>');
							++dest;
							src += 4;
							continue;
						}
						break;
					case char('l'):
						if (src[2] == char('t') && src[3] == char(';'))
						{
							*dest = char('<');
							++dest;
							src += 4;
							continue;
						}
						break;
					case char('#'):
						if (src[2] == char('x'))
						{
							unsigned long code = 0;
							src += 3;
							while (1)
							{
								byte digit = to_digit( byte(*src) );
								if (digit == 0xFF)
									break;
								code = code * 16 + digit;
								++src;
							}
							decode_character(dest, code);
						}
						else
						{
							unsigned long code = 0;
							src += 2;
							while (1)
							{
								byte digit = to_digit( byte(*src) );
								if (digit == 0xFF)
									break;
								code = code * 10 + digit;
								++src;
							}
							decode_character(dest, code);
						}
						if (*src == char(';'))
							++src;
						else
							XML_ERROR;
						continue;
					default:
						break;
					}
				}

			*dest++ = *src++;
		}
		text = src;
		return dest;
	}

	cr_xml_node *parse_xml_declaration(char *&text)
	{
		while (text[0] != char('?') || text[1] != char('>'))
		{
			if (!text[0])
				XML_ERROR;
			++text;
		}
		text += 2;
		return 0;
	}

	cr_xml_node *parse_comment(char *&text)
	{
		while (text[0] != char('-') || text[1] != char('-') || text[2] != char('>'))
		{
			if (!text[0])
				XML_ERROR;
			++text;
		}
		text += 3;
		return 0;
	}

	cr_xml_node *parse_doctype(char *&text)
	{
		while (*text != char('>'))
		{
			switch (*text)
			{
			case char('['):
			{
				++text;
				int depth = 1;
				while (depth > 0)
				{
					switch (*text)
					{
						case char('['): ++depth; break;
						case char(']'): --depth; break;
						case 0: XML_ERROR;
						default:
							break;
					}
					++text;
				}
				break;
			}
			case char('\0'):
				XML_ERROR;
			default:
				++text;
			}
		}

		text += 1;
		return 0;
	}

	cr_xml_node *parse_pi(char *&text)
	{
		while (text[0] != char('?') || text[1] != char('>'))
		{
			if (*text == char('\0'))
				XML_ERROR;
			++text;
		}
		text += 2;
		return 0;
	}

	char parse_and_append_data(cr_xml_node *node, char *&text)
	{
		char *tvalue = text, *end;
			end = skip_and_expand_character_refs(text);

			while( strchr( "\r\n\t ", *(end - 1)) )
				--end;

			if( *node->value_ == 0 )
			{
				node->value_ = tvalue;
				node->value_len_ = end - tvalue;
			}
			
			char ch = *text;
			*end = char('\0');
			return ch;
	}

	cr_xml_node *parse_cdata(char *&text)
	{
		while (text[0] != char(']') || text[1] != char(']') || text[2] != char('>'))
		{
			if( !*text++ )
				XML_ERROR;
		}
		
		text += 3;
		return 0;
	}

	cr_xml_node *parse_element(char *&text)
	{
		if( pool_pos_ >= XML_POOL_SIZE )
			XML_ERROR;

		cr_xml_node& element = pool_[ pool_pos_++ ];
		element.first_child_	= 0;
		element.value_			= 0;
		
		char* tname = text;
		while( g_mask_name[ byte(*text) ] ) ++text;
		if (text == tname)
			XML_ERROR;
		
		element.name_ = tname;
		element.name_len_ = text - tname;
		
		while( *text && *text != '>' && *text != '/' )
			++text;
		
		if (*text == char('>'))
		{
			++text;
			parse_node_contents(text, &element);
		}
		else if (*text == char('/'))
		{
			++text;
			if (*text != char('>'))
				XML_ERROR;
			++text;
		}
		else
			XML_ERROR;
		element.name_[element.name_len_] = char('\0');
		return &element;
	}

	cr_xml_node *parse_node(char *&text)
	{
		switch (text[0])
		{
		default:
			return parse_element(text);
		case char('?'):
			++text;
			if ((text[0] == char('x') || text[0] == char('X')) &&
				(text[1] == char('m') || text[1] == char('M')) &&
				(text[2] == char('l') || text[2] == char('L')) &&
				strchr( "\n\r\t ", text[ 3 ] ) )
			{
				text += 4;
				return parse_xml_declaration(text);
			}
			else
			{
				return parse_pi(text);
			}
		case char('!'):
			switch (text[1])
			{
			case char('-'):
				if (text[2] == char('-'))
				{
					text += 3;
					return parse_comment(text);
				}
				break;
			case char('['):
				if (text[2] == char('C') && text[3] == char('D') && text[4] == char('A') &&
					text[5] == char('T') && text[6] == char('A') && text[7] == char('['))
				{
					text += 8;
					return parse_cdata(text);
				}
				break;
			case char('D'):
				if (text[2] == char('O') && text[3] == char('C') && text[4] == char('T') &&
					text[5] == char('Y') && text[6] == char('P') && text[7] == char('E') &&
					strchr( "\r\n\t ", text[ 8 ] ))
				{
					text += 9;
					return parse_doctype(text);
				}
				default:
					break;
			}
			++text;
			while (*text != char('>'))
			{
				if (*text == 0)
					XML_ERROR;
				++text;
			}
			++text;
			return 0;
		}
	}

	void parse_node_contents(
		char*&			text,
		cr_xml_node*	node )
	{
		for(;;)
		{
			skip_whitespace( text );
			char next_char = *text;
			
after_data_node:
			switch( next_char )
			{
				case '<':
				{
					if( text[ 1 ] == '/' )
					{
						text += 2;
						while( g_mask_name[ byte(*text) ] ) ++text;

						skip_whitespace( text );
						if( *text != '>' )
							XML_ERROR;
						
						++text;
						return;
					}
					else
					{
						++text;
						if( cr_xml_node *child = parse_node( text ) )
							node->append_child_node( child );
					}
				}
				break;
				
				case 0:
				{
					XML_ERROR;
				}
					
				default:
					next_char = parse_and_append_data( node, text );
					goto after_data_node;
			}
		}
	}
	
	jmp_buf		jbuf_;
	cr_xml_node	pool_[ XML_POOL_SIZE ];
	size_t		pool_pos_;
};
