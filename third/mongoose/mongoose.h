/**********************************************************************************************/
/* mongoose.h		  		                                                   				  */
/*                                                                       					  */
/* (c) 2004-2012 Sergey Lyubka																  */
/* MIT license   																		  	  */
/**********************************************************************************************/

// Copyright (c) 2004-2012 Sergey Lyubka
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.

#pragma once

// STD
#include <stddef.h>
#include <time.h>

/**********************************************************************************************/
struct mg_connection;
struct mg_context;

/**********************************************************************************************/
typedef void* mg_mutex;


/**********************************************************************************************/
struct mg_request_info
{
	int				headers_count_;     // Number of headers
	bool			is_ssl_;            // TRUE if SSL-ed
	const char*		method_;			// "GET", "POST", etc
	char*			query_parameters_;	// URL part after '?', not including '?', or NULL
	long			remote_ip_;         // Client's IP address
	int				remote_port_;       // Client's port
	char*			uri_;			    // URL-decoded URI
	
	struct mg_header
	{
		const char* name_;				// HTTP header name
		const char* value_;				// HTTP header value
	}
	headers_[ 64 ];						// Maximum 64 headers
};


/**********************************************************************************************/
time_t				mg_get_birth_time( mg_connection* );
size_t				mg_get_content_len( mg_connection* );
const char*			mg_get_error_string( void );
const char*			mg_get_header( mg_connection*, const char* name );
mg_request_info*	mg_get_request_info( mg_connection* );

/**********************************************************************************************/
mg_mutex			mg_mutex_create( void );
void				mg_mutex_destroy( mg_mutex mtx );
void				mg_mutex_lock( mg_mutex mtx );
void				mg_mutex_unlock( mg_mutex mtx );

/**********************************************************************************************/
int					mg_read( mg_connection*, void* buf, size_t len );
void				mg_sleep( int ms );
mg_context*			mg_start( const char* ports, const char* pem_file, size_t thread_count );
void				mg_stop( mg_context* );
int					mg_write( mg_connection*, const char* buf, size_t len );
