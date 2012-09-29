/**********************************************************************************************/
/* utils.cpp		  		                                                   				  */
/*                                                                       					  */
/* Igor Nikitin, 2012																		  */
/* MIT license			                                                  					  */
/**********************************************************************************************/

// STD
#include <sys/stat.h>
#include <stdio.h>

// CREST
#include "../include/crest.h"
#include "utils.h"

/**********************************************************************************************/
#ifdef _WIN32
#define stat _stat
#endif // _WIN32


//////////////////////////////////////////////////////////////////////////
// constants
//////////////////////////////////////////////////////////////////////////


/**********************************************************************************************/
static const char* const RESPONCE_PREFIX[ CREST_HTTP_STATUS_COUNT ] =
{
	"HTTP/1.1 200 OK\r\nContent-Length: ",
	"HTTP/1.1 201 Created\r\nContent-Length: ",
	"HTTP/1.1 202 Accepted\r\nContent-Length: ",
	"HTTP/1.1 400 Bad Request\r\nContent-Length: ",
	"HTTP/1.1 401 Unauthorized\r\nContent-Length: ",
	"HTTP/1.1 404 Not Found\r\nContent-Length: ",
	"HTTP/1.1 405 Method Not Allowed\r\nContent-Length: ",
	"HTTP/1.1 411 Length Required\r\nContent-Length: ",
	"HTTP/1.1 500 Internal Server Error\r\nContent-Length: "
};

/**********************************************************************************************/
static const size_t RESPONCE_PREFIX_SIZE[ CREST_HTTP_STATUS_COUNT ] =
{
	35,	40,	41,	44,	45,	42,	51,	48,	53
};


//////////////////////////////////////////////////////////////////////////
// functions
//////////////////////////////////////////////////////////////////////////


/**********************************************************************************************/
void create_responce(
	char*&				out,
	size_t&				out_len,
	crest_http_status	status,
	const char*			content,
	size_t				content_len )
{
	size_t plen = RESPONCE_PREFIX_SIZE[ status ];
	
	out = (char*) malloc( content_len + 85 );
	memccpy( out, RESPONCE_PREFIX[ status ], plen, 1 );
	
	char* str = out + plen;
	to_string( (int) content_len, str );
	str = (char*) memchr( str, 0, 1 );

	*str++ = 'r';
	*str++ = 'n';
	*str++ = 'r';
	*str++ = 'n';

	memcpy( str, content, content_len );
	str[ content_len ] = 0;
	out_len = str - out + content_len;
}

/**********************************************************************************************/
char* crest_strdup( const char* str )
{
	if( !str )
		return 0;

	size_t n = strlen( str );
	char* res = (char*) malloc( n + 1 );
	memcpy( res, str, n );
	res[ n ] = 0;
	
	return res;
}

/**********************************************************************************************/
bool file_exists( const char* path )
{
	struct stat st;
	return stat( path, &st ) == 0 && ( st.st_mode & S_IFREG );
}

/**********************************************************************************************/
size_t file_size( const char* path )
{
	size_t size = 0;
	
	FILE* f = fopen( path, "rb" );
	if( f )
	{
		fseek( f, 0, SEEK_END );
		size = ftell( f );
		fclose( f );
	}

	return size;
}
