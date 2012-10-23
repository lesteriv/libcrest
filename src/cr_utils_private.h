/**********************************************************************************************/
/* cr_utils_private.h	  		                                                   			  */
/*                                                                       					  */
/* Igor Nikitin, 2012																		  */
/* MIT license   																		  	  */
/**********************************************************************************************/

#pragma once

// STD
#include <stddef.h>
#include <string.h>

// CREST
#include "../include/cr_string_map.h"


/**********************************************************************************************/
// Functions

				/** Adds char to string. */
inline void		add_char(
					char*&			dest,
					char			c )
				{
					*dest++ = c;
				}


				/** Converts and adds integer value to string. */
void			add_number( char*& buf, int value );

				/** Adds string to another. */
inline void		add_string(
					char*&			dest,
					const char*		src,
					size_t			len )
				{
					memmove( dest, src, len + 1 );
					dest += len;
				}

				/** The same as strcasecmp. */
int				cr_strcasecmp(
					const char*	str1,
					const char* str2 );

				/** The same as strdup. */
char*			cr_strdup( const char* str, int en = -1 );

				/** Extract cookies from header. */
void			parse_cookie_header(
					cr_string_map&	cookies,
					char*			header,
					size_t			header_len );

				/** Parse string with query parameters */
void			parse_post_parameters(
					cr_string_map&	out,
					char*			text );

				/** Parse string with query parameters */
void			parse_query_parameters(
					cr_string_map&	out,
					char*			text );
