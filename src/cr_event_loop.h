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
#include <time.h>

// CREST
#include "../include/cr_string_map.h"
#include "cr_utils_private.h"

/**********************************************************************************************/
#define MAX_REQUEST_SIZE 16384

/**********************************************************************************************/
struct cr_in_socket;


/**********************************************************************************************/
struct cr_connection_data
{
	cr_in_socket*	client;								// Connected client
	int				consumed_content	= 0;			// How many bytes of content have been read
	int				content_len;						// Content-Length header value
	int				data_len			= 0;			// Total size of data in a buffer
	cr_string_map	headers_;							// Headers
	const char*		method_;							// "GET", "POST", etc
	char*			query_parameters_;					// URL part after '?', not including '?', or NULL
	long			remote_ip_;							// Client's IP address
	int				request_len			= 0;			// Size of the request + headers in a buffer
	void*			ssl					= 0;			// SSL descriptor
	char*			uri_;								// URL-decoded URI

	char			request_buffer[ MAX_REQUEST_SIZE ];	// Buffer for received data
};


/**********************************************************************************************/
int					cr_read( cr_connection_data&, void* buf, size_t len );
int					cr_write( cr_connection_data&, const char* buf, size_t len );


/**********************************************************************************************/
bool				cr_fetch(
						char*					hbuf,
						char*&					out,
						size_t&					out_size,
						const char*				url,
						cr_string_map*			headers,
						int						redirect_count = 0 /* For internal use */ );


/**********************************************************************************************/
bool				cr_event_loop(
						const vector<cr_port>&	ports,
						const string&			pem_file,
						size_t					thread_count );
