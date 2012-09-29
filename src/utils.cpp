/**********************************************************************************************/
/* utils.cpp		  		                                                   				  */
/*                                                                       					  */
/* Igor Nikitin, 2012																		  */
/* MIT license			                                                  					  */
/**********************************************************************************************/

// STD
#include <sys/stat.h>
#include <cstdio>

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
static const string RESPONCE_PREFIX[ CREST_HTTP_STATUS_COUNT ] =
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


//////////////////////////////////////////////////////////////////////////
// functions
//////////////////////////////////////////////////////////////////////////


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

/**********************************************************************************************/
string responce(
	crest_http_status		status,
	const string&	content )
{
	char buf[ 24 ];
	to_string( content.length(), buf );
	
	string str;
	str.reserve( 85 + content.length() );
	str  = RESPONCE_PREFIX[ status ];
	str += buf;
	str += "\r\n\r\n";
	str += content;
	
	return str;
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
