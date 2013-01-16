/**********************************************************************************************/
/* cr_utils.h			  		                                                   			  */
/*                                                                       					  */
/* Igor Nikitin, 2013																		  */
/* MIT license   																		  	  */
/**********************************************************************************************/

#pragma once

// STD
#include <stddef.h>
#include <string>
#include <time.h>

// CREST
#include "cr_string_map.h"
#include "cr_types.h"
#include "cr_connection.h"


/**********************************************************************************************/
// Functions

					/** Decodes base64-encoded data, returns count of bytes in result. */
size_t				cr_base64_decode(
						char*			out,
						const char*		data,
						size_t			data_size );

					/** Returns needed size of buffer for deflate. */
size_t				cr_compress_bound( size_t len );

					/** Returns string with status code, 'Content-Length' header and content. */
void				cr_create_responce(
						char*&			out,
						size_t&			out_len,
						cr_http_status	status,
						const char*		content,
						size_t			content_len,
						cr_string_map*	headers = NULL );

					/** Returns string with status code, 'Content-Length' header and content. */
std::string			cr_create_responce(
						cr_http_status		status,
						const std::string&	content,
						cr_string_map*		headers = NULL );

					/** Returns string with status code and 'Content-Length' header. */
void				cr_create_responce_header(
						char*			out,
						size_t&			out_len,
						cr_http_status	status,
						size_t			content_len,
						cr_string_map*	headers = NULL );

					/** Compress data. */
size_t				cr_deflate(
						const char*		buf,
						size_t			len,
						char*			out,
						size_t			out_len );

					/** Returns TRUE if this file exists. */
bool				cr_file_exists( const std::string& path );

					/** Returns time of last file modification. */
time_t				cr_file_modification_time( const std::string& path );

					/** Returns file's size. */
size_t				cr_file_size( const std::string& path );

					/** Returns name of auth kind. */
const char*			cr_get_auth_name( cr_http_auth auth );

					/** Returns name of result format. */
const char*			cr_get_format_name( cr_result_format format );

					/** Returns esult format for connection. */
cr_result_format	cr_get_result_format( cr_connection& conn );

					/** Calculate md5 hash. */
void				cr_md5(
						char			hash[ 16 ],
						size_t			data_count,
						const char**	data,
						size_t*			len );

					/** Adds header to set cookie. */
void				cr_set_cookie(
						cr_string_map&	headers,
						const char*		name,
						const char*		value );

					/** Sleep for @ms milliseconds. */
void				cr_sleep( int ms );
