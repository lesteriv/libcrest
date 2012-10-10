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
#include "cr_headers.h"
#include "cr_types.h"


/**********************************************************************************************/
// Functions

				/** Adds char to string. */
inline char*	add_char(
					char*			dest,
					char			c )
				{
					*dest++ = c;
					return dest;
				}

				/** Adds string to another. */
inline char*	add_string(
					char*			dest,
					const char*		src,
					size_t			len )
				{
					memmove( dest, src, len + 1 );
					return dest + len;
				}

				/** Decode base64-encoded data, return count of bytes in result. */
size_t			base64_decode(
					char*			out,
					const char*		data,
					size_t			data_size );

				/** Returns string with status code, 'Content-Length' header and content. */
void			create_responce(
					char*&			out,
					size_t&			out_len,
					cr_http_status	status,
					const char*		content,
					size_t			content_len,
					cr_headers*		headers = NULL );

				/** Returns string with status code and 'Content-Length' header. */
void			create_responce_header(
					char*			out,
					size_t&			out_len,
					cr_http_status	status,
					size_t			content_len,
					cr_headers*		headers = NULL );

				/** The same as strdup. */
char*			cr_strdup( const char* str, int en = -1 );

				/** Compress data. */
size_t			deflate(
					const char*		buf,
					size_t			len,
					char*			out,
					size_t			out_len );

				/** Returns TRUE if this file exists. */
bool			file_exists( const char* path );

				/** Returns time of last file modification. */
time_t			file_modification_time( const char* path );

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
char*			to_string( char* buf, int value );
