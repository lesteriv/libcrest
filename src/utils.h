/**********************************************************************************************/
/* utils.h			  		                                                   				  */
/*                                                                       					  */
/* Igor Nikitin, 2012																		  */
/* MIT license   																		  	  */
/**********************************************************************************************/

#pragma once

// STD
#include <stddef.h>

// CREST
#include "../include/crest_types.h"


/**********************************************************************************************/
// Functions

				/** Adds string to another. */
inline char*	add_string(
					char*		dest,
					const char*	src,
					size_t		len )
				{
					memcpy( dest, src, len + 1 );
					return dest + len;
				}

				/** Decode base64-encoded data, return count of bytes in result. */
size_t			base64_decode(
					char*		out,
					const char*	data,
					size_t		data_size );

				/** Returns string with status code, 'Content-Length' header and content. */
void			create_responce(
					char*&				out,
					size_t&				out_len,
					crest_http_status	status,
					const char*			content,
					size_t				content_len );

				/** Returns string with status code and 'Content-Length' header. */
void			create_responce_header(
					char*				out,
					size_t&				out_len,
					crest_http_status	status,
					size_t				content_len );

				/** The same as strdup. */
char*			crest_strdup(
					const char*	str,
					int			en = -1 );

				/** Returns TRUE if this file exists. */
bool			file_exists( const char* path );

				/** Returns file's size. */
size_t			file_size( const char* path );

				/** Calculate md5 hash. */
void			md5(
					char			hash[ 16 ],
					size_t			data_count,
					const char**	data,
					size_t*			len );

				/** Parse string with query parameters */
void			parse_query_parameters(
					size_t&			count,
					char**			names,
					char**			values,
					char*			str );

				/** Converts integer value to string, returns end of string - \0 pos. */
char*			to_string(
					char*	buf,
					int		value );
