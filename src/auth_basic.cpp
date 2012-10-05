/**********************************************************************************************/
/* auth_basic.cpp	  		                                                   				  */
/*                                                                       					  */
/* Igor Nikitin, 2012																		  */
/* MIT license			                                                  					  */
/**********************************************************************************************/

// STD
#include <malloc.h>
#include <string.h>
#include <stdlib.h>

// CREST
#include "../include/crest.h"
#include "utils.h"

/**********************************************************************************************/
#ifndef NO_AUTH


//////////////////////////////////////////////////////////////////////////
// helper functions
//////////////////////////////////////////////////////////////////////////


/**********************************************************************************************/
bool parse_basic_auth(
	const char*	auth,
	size_t		auth_len,
	char*		buf,
	char*&		user,
	char*&		password )
{
	if( strncmp( "Basic ", auth, 6 ) )
		return false;
	
	auth += 6;
	size_t len = base64_decode( buf, auth, auth_len - 6 );
	buf[ len ] = 0;

	char* sp = strchr( buf, ':' );
	if( !sp )
		return false;
	
	*sp = 0;
	user = buf;
	password = sp + 1;
	
	return true;
}


//////////////////////////////////////////////////////////////////////////
// functions
//////////////////////////////////////////////////////////////////////////


/**********************************************************************************************/
bool auth_basic(
	crest_connection&	conn,
	bool				admin )
{
	bool res = false;
	const char* auth = conn.get_http_header( "Authorization" );
	
	size_t auth_len = auth ? strlen( auth ) : 0;
	char *buf = (char*) alloca( auth_len + 1 );
	char *user, *pass;

	if( auth && parse_basic_auth( auth, auth_len, buf, user, pass ) )
	{
		if( !admin || the_crest_user_manager.get_user_is_admin( user ) )
		{
			char stored_hash[ 16 ];
			if( the_crest_user_manager.get_password( user, stored_hash ) )
			{
				const char* data[] = { user, ":", "", ":", pass };
				size_t len[] = { strlen( user ), 1, 0, 1, strlen( pass ) };
				
				char auth_hash[ 16 ];
				md5( auth_hash, 5, data, len );
				
				res = !memcmp( stored_hash, auth_hash, 16 );
			}
		}
	}
	
	if( !res )
		conn.write( "HTTP/1.1 401 Unauthorized\r\nContent-Length: 0\r\nWWW-Authenticate: Basic\r\n\r\n", 73 );
	
	return res;	
}


/**********************************************************************************************/
#endif // NO_AUTH
