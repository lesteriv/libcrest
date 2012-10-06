/**********************************************************************************************/
/* auth_digest.cpp	  		                                                   				  */
/*                                                                       					  */
/* Igor Nikitin, 2012																		  */
/* MIT license			                                                  					  */
/**********************************************************************************************/

// STD
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

// MONGOOSE
#include "../third/mongoose/mongoose.h"

// CREST
#include "../include/crest.h"
#include "utils.h"

/**********************************************************************************************/
#ifndef NO_AUTH


//////////////////////////////////////////////////////////////////////////
// types
//////////////////////////////////////////////////////////////////////////


/**********************************************************************************************/
struct auth_data
{
	char* cnonce;
	char* nc;
	char* nonce;
	char* qop;
	char* response;
	char* uri;
	char* user;
};


//////////////////////////////////////////////////////////////////////////
// helper functions
//////////////////////////////////////////////////////////////////////////


/**********************************************************************************************/
static void bin2str( char* str, const unsigned char* bin, size_t len )
{
	for( ; len-- ; ++bin )
	{
		*str++ = "0123456789abcdef"[ *bin >> 4 ];
		*str++ = "0123456789abcdef"[ *bin & 0x0F ];
	}
	
	*str = '\0';
}

/**********************************************************************************************/
static bool check_password(
	const char*	method,
	const char*	ha1,
	auth_data&	adata )
{
	char ha2[ 16 ], response[ 16 ];
	char ha2_str[ 33 ], response_str[ 33] ;

	char ha1_str[ 33 ];
	bin2str( ha1_str, (unsigned char*) ha1, 16 );

	const char* ha2_data[] = { method, ":", adata.uri };
	size_t ha2_len[] = { strlen( method ), 1, strlen( adata.uri ) };
	cr_md5( ha2, 3, ha2_data, ha2_len );
	bin2str( ha2_str, (unsigned char*) ha2, 16 );

	const char* resp_data[] = { ha1_str, ":", adata.nonce, ":", adata.nc, ":", adata.cnonce, ":", adata.qop, ":", ha2_str };
	size_t resp_len[] = { strlen( ha1_str ), 1, strlen( adata.nonce ), 1, strlen( adata.nc ), 1, strlen( adata.cnonce ), 1, strlen( adata.qop ), 1, 32 };
	cr_md5( response, 11, resp_data, resp_len );
	bin2str( response_str, (unsigned char*) response, 16 );  

	return !memcmp( adata.response, response_str, 32 );
}

/**********************************************************************************************/
static bool parse_auth_header(
	cr_connection&	conn,
	auth_data&			adata )
{
	memset( &adata, 0, sizeof( adata ) );
	
	char* s = (char*) conn.get_http_header( "Authorization" );
	if( !s )
		return false;
	
	if( strncmp( s, "Digest ", 7 ) )
		return false;
	
	s += 7;
	
	while( s )
	{
		while( isspace( *s ) ) ++s;
		char* name = s;
		
		char* value = strchr( s, '=' );
		if( !value )
			break;
		
		*value++ = 0;
		
		if( *value == '"' )
		{
			++value;
			char* end = strchr( value, '"' );
			if( !end )
				break;
			
			*end++ = 0;
			s = strchr( end, ',' );
			if( s )
				++s;
		}
		else
		{
			char* end = strchr( value, ',' );
			if( end )
				*end++ = 0;
			
			s = end;
		}
		
		switch( *name )
		{
			case 'c': adata.cnonce		= value; break;
			case 'q': adata.qop			= value; break;
			case 'r': adata.response	= value; break;
			
			case 'n':
			{
				( *++name == 'c' ) ?
					adata.nc = value :
					adata.nonce = value;
			}
			break;
			
			case 'u':
			{
				( *++name == 'r' ) ?
					adata.uri = value :
					adata.user = value;
			}
			break;
			
			default:
				return false;
		}
	}

	return
		adata.cnonce && adata.nc && adata.nonce &&
		adata.qop && adata.response && adata.uri && adata.user;
}


//////////////////////////////////////////////////////////////////////////
// functions
//////////////////////////////////////////////////////////////////////////


/**********************************************************************************************/
bool auth_digest(
	cr_connection&	conn,
	bool				admin )
{
	bool res = false;
	bool stale = false;
	
	auth_data adata;
	char ha1[ 16 ];

	if( parse_auth_header( conn, adata ) && the_cr_user_manager.get_password( adata.user, ha1 ) )
	{
		if( time( NULL ) - strtol( adata.nonce, NULL, 10 ) > 300 )
		{
			res = false;
			stale = true;
		}
		else if( !admin || the_cr_user_manager.get_user_is_admin( adata.user ) )
		{
			res = check_password( conn.get_http_method(), ha1, adata );
		}
	}
		
	if( !res )
	{
		char responce[ 256 ];
		char* str = responce;
		str = add_string ( str, "HTTP/1.1 401 Unauthorized\r\nContent-Length: 0\r\nWWW-Authenticate: Digest qop=\"auth\", realm=\"\", nonce=\"", 100 );
		str = to_string	 ( str, (long) time( NULL ) );
		str = add_string ( str, "\"", 1 );
		if( stale ) str = add_string( str, ", stale=TRUE", 12 );
		str = add_string ( str, "\r\n\r\n", 4 );
		
		conn.write( responce, str - responce );
	}
	
	return res;
}


/**********************************************************************************************/
#endif // NO_AUTH
