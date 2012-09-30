/**********************************************************************************************/
/* utils.cpp		  		                                                   				  */
/*                                                                       					  */
/* Igor Nikitin, 2012																		  */
/* MIT license			                                                  					  */
/**********************************************************************************************/

// STD
#include <sys/stat.h>
#include <ctype.h>
#include <stdio.h>

// CREST
#include "../include/crest.h"
#include "utils.h"


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
	33,	38,	39,	42,	43,	40,	49,	46,	51
};


//////////////////////////////////////////////////////////////////////////
// helper functions
//////////////////////////////////////////////////////////////////////////


/**********************************************************************************************/
inline char hex_to_int( char ch )
{
	return isdigit( ch ) ? ch - '0' : ch - 'W';
}


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
	size_t pref_len = RESPONCE_PREFIX_SIZE[ status ];
	
	out = (char*) malloc( content_len + 85 );
	memcpy( out, RESPONCE_PREFIX[ status ], pref_len );
	
	char* str = out + pref_len;
	to_string( (int) content_len, str );
	str = (char*) memchr( str, 0, 24 );

	*str++ = '\r'; *str++ = '\n';
	*str++ = '\r'; *str++ = '\n';

	memcpy( str, content, content_len );
	str[ content_len ] = 0;
	out_len = str - out + content_len;
}

/**********************************************************************************************/
void create_responce_header(
	char*&				out,
	size_t&				out_len,
	crest_http_status	status,
	size_t				content_len )
{
	size_t plen = RESPONCE_PREFIX_SIZE[ status ];
	
	out = (char*) malloc( 85 );
	memcpy( out, RESPONCE_PREFIX[ status ], plen );
	
	char* str = out + plen;
	to_string( (int) content_len, str );
	str = (char*) memchr( str, 0, 24 );

	*str++ = '\r'; *str++ = '\n';
	*str++ = '\r'; *str++ = '\n';
	*str = 0;
	
	out_len = str - out;
}

/**********************************************************************************************/
char* crest_strdup( 
	const char* str,
	int			len )
{
	if( !str )
		return 0;

	if( len < 0 )
		len = strlen( str );
	
	char* res = (char*) malloc( len + 1 );
	memcpy( res, str, len );
	res[ len ] = 0;
	
	return res;
}

/**********************************************************************************************/
bool file_exists( const char* path )
{
	FILE* f = fopen( path, "rb" );
	if( f )
		fclose( f );
	
	return f != NULL;
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
void parse_query_parameters(
	size_t&	count,
	char**	names,
	char**	values,
	char*	str )
{
	count = 0;
	
	while( str && *str )
	{
		// Name
		names[ count ] = str;

		// Search for '='
		for( ; *str && *str != '=' ; ++str );

		// If found = read value
		if( *str == '=' )
		{
			*str++ = 0;
			values[ count ] = str;
			count++;
			char* v = str;
			
			for( ; *str && *str != '&' ; ++str, ++v )
			{
				if( *str == '+' )
				{
					*v = ' ';
				}
				else if( *str == '%' && isxdigit( str[ 1 ] ) && isxdigit( str[ 2 ] ) )
				{
					*v = ( hex_to_int( tolower( str[ 1 ] ) ) << 4 ) | hex_to_int( tolower( str[ 2 ] ) );
					str += 2;
				}
				else
				{
					if( v != str )
						*v = *str;
				}
			}

			*v = 0;
			if( !*str++ || count > 63 )
				break;
		}
	}
}
