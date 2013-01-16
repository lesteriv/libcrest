/**********************************************************************************************/
/* cr_utils_private.h	  		                                                   			  */
/*                                                                       					  */
/* Igor Nikitin, 2013																		  */
/* MIT license   																		  	  */
/**********************************************************************************************/

#pragma once

// STD
#include <string>
#include <string.h>
#include <vector>

// CREST
#include "../include/cr_string_map.h"

/**********************************************************************************************/
using namespace std;


/**********************************************************************************************/
// Types

/** Single destination port description. */
struct cr_port
{
	int		a, b, c, d;
	int		port;
	bool	ssl;
};


/**********************************************************************************************/
// Functions

				/** Adds char to string. */
inline void		add_char(
					char*&		dest,
					char		c )
				{
					*dest++ = c;
				}

				/** Converts and adds integer value to string. */
void			add_number( 
					char*&		buf,
					int			value );

				/** Adds string to another. */
inline void		add_string(
					char*&		dest,
					const char*	src,
					size_t		len )
				{
					memmove( dest, src, len + 1 );
					dest += len;
				}

				/** The same as strcasecmp. */
int				cr_strcasecmp(
					const char*	str1,
					const char* str2 );

				/** Converts A..Z to a..z. */
inline char		cr_tolower( char ch )
				{
					return ( ch >= 'A' && ch <= 'Z' ) ? ch + 32 : ch;
				}

				/** Extract cookies from header. */
void			parse_cookie_header(
					cr_string_map&	cookies,
					char*			header,
					size_t			header_len );

				/** Parse string with ports. */
vector<cr_port>	parse_ports( const string& ports );

				/** Parse string with query parameters */
void			parse_post_parameters(
					cr_string_map&	out,
					char*			text );

				/** Parse string with query parameters */
void			parse_query_parameters(
					cr_string_map&	out,
					char*			text );

