/**********************************************************************************************/
/* utils.cpp		  		                                                   				  */
/*                                                                       					  */
/* Igor Nikitin, 2012																		  */
/* MIT license			                                                  					  */
/**********************************************************************************************/

// STD
#include <sys/stat.h>

// CREST
#include "../include/crest.h"

/**********************************************************************************************/
#ifdef _WIN32
#define stat _stat
#endif // _WIN32

/**********************************************************************************************/
CREST_NAMESPACE_START


//////////////////////////////////////////////////////////////////////////
// constants
//////////////////////////////////////////////////////////////////////////


/**********************************************************************************************/
static const char* const RESPONCE_PREFIX[ HTTP_INTERNAL_ERROR + 1 ] =
{
	"200 OK",
	"201 Created",
	"202 Accepted",
	"400 Bad Request",
	"401 Unauthorized",
	"404 Not Found",
	"405 Method Not Allowed",
	"411 Length Required",
	"500 Internal Server Error"
};


//////////////////////////////////////////////////////////////////////////
// functions
//////////////////////////////////////////////////////////////////////////


/**********************************************************************************************/
bool file_exists( const string& path )
{
	struct stat st;
	return stat( path.c_str(), &st ) == 0 && ( st.st_mode & S_IFREG );
}

/**********************************************************************************************/
int64_t file_size( const string& path )
{
	int64_t size = 0;
	
	FILE* f = fopen( path.c_str(), "rb" );
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
	http_status		status,
	const string&	str )
{
	char buf[ 48 ];
	snprintf( buf, 48, "\r\nContent-Length: %zu\r\n\r\n", str.length() );
	
	return
		string( "HTTP/1.1 " ) + RESPONCE_PREFIX[ status ] +	buf + str;			
}


/**********************************************************************************************/
CREST_NAMESPACE_END
