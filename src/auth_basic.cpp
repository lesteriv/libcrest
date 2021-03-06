/**********************************************************************************************/
/* auth_basic.cpp	  		                                                   				  */
/*                                                                       					  */
/* Igor Nikitin, 2013																		  */
/* MIT license			                                                  					  */
/**********************************************************************************************/

// STD
#include <malloc.h>
#include <string.h>
#include <stdlib.h>

// CREST
#include "../include/crest.h"
#include "../include/cr_utils.h"


//////////////////////////////////////////////////////////////////////////
// helper functions
//////////////////////////////////////////////////////////////////////////


/**********************************************************************************************/
static bool parse_basic_auth(
	const char*	auth_header,
	size_t		auth_header_len,
	char*		buf,
	char*&		user,
	char*&		password )
{
	if( strcmp( "Basic ", auth_header ) )
		return false;
	
	auth_header += 6;
	size_t len = cr_base64_decode( buf, auth_header, auth_header_len - 6 );
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
	cr_connection&	conn,
	bool			admin )
{
	bool res = false;
	
	const char* auth = conn.header( "authorization" );
	size_t auth_len = auth ? strlen( auth ) : 0;
	
	char *buf = (char*) alloca( auth_len + 16 );
	char *user, *pass;

	if( auth && parse_basic_auth( auth, auth_len, buf, user, pass ) )
	{
		if( !admin || the_cr_user_manager.get_user_is_admin( user ) )
		{
			char stored_hash[ 16 ];
			if( the_cr_user_manager.get_password_hash( user, stored_hash ) )
			{
				const char* data[] = { user, ":", "", ":", pass };
				size_t len[] = { strlen( user ), 1, 0, 1, strlen( pass ) };
				
				char auth_hash[ 16 ];
				cr_md5( auth_hash, 5, data, len );
				
				res = !memcmp( stored_hash, auth_hash, 16 );
			}
		}
	}
	
	if( !res )
		conn.write( "HTTP/1.1 401 Unauthorized\r\nContent-Length: 0\r\nWWW-Authenticate: Basic\r\n\r\n", 73 );
	
	return res;	
}
