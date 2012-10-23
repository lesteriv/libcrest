/**********************************************************************************************/
/* tests.cpp						                                               			  */
/*                                                                       					  */
/* Igor Nikitin, 2012																		  */
/* MIT license			                                                  					  */
/**********************************************************************************************/

// STD
#include <signal.h>
#include <stdio.h>
#include <string>
#include <string.h>
#include <time.h>

// CREST
#include "../include/crest.h"
#include "../include/cr_utils.h"
#include "../src/cr_utils_private.h"

/**********************************************************************************************/
using namespace std;


/**********************************************************************************************/
#define RETURN( x )					\
	bool _res = x;					\
	if( !_res ) raise( SIGTRAP );	\
	return _res;

/**********************************************************************************************/
#define FINISH		)();
#define START		([](){ return true; }
#define TEST( x )	()) && ([]()


/**********************************************************************************************/
int main( void )
{
	START
	
	// -----------------------------------------------------------------------
	// user_manager
	// -----------------------------------------------------------------------
	
	TEST( cr_user_manager::cr_user_manager )
	{
		the_cr_user_manager.clean();
		
		RETURN( the_cr_user_manager.get_user_count() == 0 );
	}

	TEST( cr_user_manager::flush )
	{
		the_cr_user_manager.clean();
		
		the_cr_user_manager.set_auth_file( "/tmp/auth.passwd" );
		bool res = cr_file_exists( "/tmp/auth.passwd" );
		
		remove( "/tmp/auth.passwd" );
		RETURN( res );
	}
	
/*	
	TEST( cr_user_manager::load )
	{
		the_cr_user_manager.clean();
		
		the_cr_user_manager.set_auth_file( "/tmp/auth.passwd" );
		the_cr_user_manager.update_user_password( "root", "qwerty" );
		the_cr_user_manager.set_auth_file( "/tmp/auth.passwd2" );
		
		bool res = !the_cr_user_manager.auth( "root", "qwerty", false );
		the_cr_user_manager.set_auth_file( "/tmp/auth.passwd" );
		res = res && the_cr_user_manager.auth( "root", "qwerty", false );
		
		remove( "/tmp/auth.passwd" );
		remove( "/tmp/auth.passwd2" );
		RETURN( res );
	}
 */

	TEST( cr_user_manager::get_auth_file and
		  cr_user_manager::set_auth_file )
	{
		the_cr_user_manager.clean();
		
		the_cr_user_manager.set_auth_file( "/tmp/auth.passwd" );
		bool res = !strcmp( "/tmp/auth.passwd", the_cr_user_manager.get_auth_file() );
		
		remove( "/tmp/auth.passwd" );
		RETURN( res );
	}
	
	TEST( cr_user_manager::get_user_count )
	{
		the_cr_user_manager.clean();
		
		bool res = the_cr_user_manager.get_user_count() == 0;
		the_cr_user_manager.set_auth_file( "/tmp/auth.passwd" );
		res = res && the_cr_user_manager.get_user_count() == 1;
		
		remove( "/tmp/auth.passwd" );
		RETURN( res );
	}
	
	TEST( cr_user_manager::get_user_flags )
	{
		the_cr_user_manager.clean();
		
		the_cr_user_manager.set_auth_file( "/tmp/auth.passwd" );
		bool res = the_cr_user_manager.get_user_is_admin( "root" );
		
		remove( "/tmp/auth.passwd" );
		RETURN( res );
	}
	
	TEST( cr_user_manager::get_users )
	{
		the_cr_user_manager.clean();
		
		the_cr_user_manager.set_auth_file( "/tmp/auth.passwd" );
		
		size_t count;
		char** users;
		the_cr_user_manager.get_users( count, users );
		
		bool res = ( count == 1 ) && ( !strcmp( users[ 0 ], "root" ) );
		free( users );
		
		remove( "/tmp/auth.passwd" );
		RETURN( res );
	}

/*	
	TEST( cr_user_manager::add_user and
		  cr_user_manager::auth	    and
		  cr_user_manager::clean )
	{
		the_cr_user_manager.clean();
		
		the_cr_user_manager.set_auth_file( "/tmp/auth.passwd" );
		the_cr_user_manager.add_user( "test", "qwerty", true );
		
		bool res = the_cr_user_manager.get_user_count() == 2;
		res = res && the_cr_user_manager.auth( "test", "qwerty", true );
	
		the_cr_user_manager.clean();
		res = res && !the_cr_user_manager.get_auth_file();
		res = res && !the_cr_user_manager.get_user_count();
		
		remove( "/tmp/auth.passwd" );
		RETURN( res );
	}
*/

	TEST( cr_user_manager::delete_user )
	{
		the_cr_user_manager.clean();
		
		the_cr_user_manager.set_auth_file( "/tmp/auth.passwd" );
		the_cr_user_manager.add_user( "test", "qwerty", true );
		
		bool res = the_cr_user_manager.get_user_count() == 2;
		the_cr_user_manager.delete_user( "test" );
		res = res && the_cr_user_manager.get_user_count() == 1;
		
		remove( "/tmp/auth.passwd" );
		RETURN( res );
	}
	
	TEST( cr_user_manager::update_user_flags )
	{
		the_cr_user_manager.clean();
		
		the_cr_user_manager.set_auth_file( "/tmp/auth.passwd" );
		the_cr_user_manager.add_user( "test", "qwerty", false );
		
		the_cr_user_manager.update_user_is_admin( "test", true );
		bool res = the_cr_user_manager.get_user_is_admin( "test" );
		
		remove( "/tmp/auth.passwd" );
		RETURN( res );
	}
	
/*	
	TEST( cr_user_manager::update_user_password )
	{
		the_cr_user_manager.clean();
		
		the_cr_user_manager.set_auth_file( "/tmp/auth.passwd" );
		the_cr_user_manager.add_user( "test", "qwerty", false );
		
		bool res = the_cr_user_manager.auth( "test", "qwerty", false );
		
		the_cr_user_manager.update_user_password( "test", "ytrewq" );
		res = res && the_cr_user_manager.auth( "test", "ytrewq", false );
		
		remove( "/tmp/auth.passwd" );
		RETURN( res );
	}

	TEST( parse_basic_auth )
	{
		const char* auth = "Basic QWxhZGluOnNlc2FtIG9wZW4=";
		size_t auth_len = strlen( auth );
		char buf[ auth_len + 1 ];
		char *user, *pass;
		
		bool res =
			parse_basic_auth( auth, auth_len, buf, user, pass ) &&
			!strcmp( user, "Aladin" ) &&
			!strcmp( pass, "sesam open" );
		
		RETURN( res );
	}
*/

/*	
	TEST( get_cookie_value )
	{
		char* header = cr_strdup( " has_js=1;\na=;b= 1; __utmb= \t27501688.1.10.1350049402 ;\n __utmc=27501688 " );
		size_t len = strlen( header );
		
		bool res = 
			!strcmp( "27501688.1.10.1350049402", get_cookie_value( header, len, "__utmb" ) ) &&
			!strcmp( "27501688.1.10.1350049402", get_cookie_value( header, len, "__utmb" ) ) &&
			!strcmp( "1", get_cookie_value( header, len, "has_js" ) ) &&
			!strcmp( "27501688", get_cookie_value( header, len, "__utmc" ) ) &&
			!strcmp( "", get_cookie_value( header, len, "__d" ) ) &&
			!strcmp( "", get_cookie_value( header, len, "a" ) ) &&
			!strcmp( "1", get_cookie_value( header, len, "b" ) ) &&
			!strcmp( "", get_cookie_value( (char*) "", 0, "__utmc" ) ) &&
			!strcmp( "", get_cookie_value( (char*) "__utmc", 6, "__utmc" ) ) &&
			!strcmp( "", get_cookie_value( (char*) "__utmc =", 8, "__utmc" ) ) &&
			!strcmp( "", get_cookie_value( (char*) "=", 1, "__utmc" ) );
		
		free( header );
		RETURN( res );
	}

	TEST( cr_post_parameters )
	{
		char str0[] = "Name=Jonathan+Doe&Age=23&Formula=a+%2B+b+%3D%3D+13%25%21";
		cr_post_parameters pm0( str0 );
		
		bool res =
			pm0.get_parameter_count() == 3 &&
			pm0.get_parameter_name	( 0 ) == string("Name")				&&
			pm0.get_parameter_name	( 1 ) == string("Age")				&&
			pm0.get_parameter_name	( 2 ) == string("Formula")			&&
			pm0.get_parameter_value	( 0 ) == string("Jonathan Doe")		&&
			pm0.get_parameter_value	( 1 ) == string("23")				&&
			pm0.get_parameter_value	( 2 ) == string("a + b == 13%!");

		RETURN( res );
	}
*/	
	
	// -----------------------------------------------------------------------
	// cr_utils
	// -----------------------------------------------------------------------

	TEST( add_char )
	{
		char buf[ 32 ] = "";
		char* str = buf;
		add_char( str, 'a' );
		add_char( str, 'b' );
		
		RETURN( !strcmp( buf, "ab" ) );
	}	
	
	TEST( add_number )
	{
		char buf[ 32 ] = "";
		char* str = buf;
		add_number( str, 0 );
		add_number( str, -1 );
		add_number( str, 234 );
		
		RETURN( !strcmp( buf, "0-1234" ) );
	}	
	
	TEST( add_string )
	{
		char buf[ 32 ] = "";
		char* str = buf;
		add_string( str, "a", 1 );
		add_string( str, "", 0 );
		add_string( str, "bc", 2 );
		
		RETURN( !strcmp( buf, "abc" ) );
	}	
	
	TEST( cr_strcasecmp )
	{
		RETURN(
			cr_strcasecmp( "a", "B"		) < 0 &&
			cr_strcasecmp( "aZ", "ax"	) > 0 &&
			cr_strcasecmp( "az", "AZ"	) == 0 )
	}	
	
	TEST( cr_strdup )
	{
		char* s1 = cr_strdup( 0 );
		char* s2 = cr_strdup( "" );
		char* s3 = cr_strdup( "abc" );
		
		bool res = !s1 && !strcmp( s2, "" ) && !strcmp( s3, "abc" );
		
		free( s1 );
		free( s2 );
		free( s3 );
		
		RETURN( res );
	}	
	
	TEST( cr_tolower )
	{
		RETURN(
			cr_tolower( 'A' ) == 'a' &&
			cr_tolower( 'a' ) == 'a' &&
			cr_tolower( '1' ) == '1' );
	}
	
	TEST( parse_cookie_header )
	{
		bool res = true;
		
		{
			cr_string_map cookies;
			parse_cookie_header( cookies, 0, 0 );
			res = res && cookies.size() == 0;
		}
		
		{
			char buf[] = "name=value; name2=value2";
			
			cr_string_map cookies;
			parse_cookie_header( cookies, buf, strlen( buf ) );
			res = res &&
				cookies.size() == 2 &&
				!strcmp( cookies[ "name" ], "value" ) &&
				!strcmp( cookies[ "name2" ], "value2" );
		}
		
		{
			char buf[] = "name=value; ";
			
			cr_string_map cookies;
			parse_cookie_header( cookies, buf, strlen( buf ) );
			res = res &&
				cookies.size() == 1 &&
				!strcmp( cookies[ "name" ], "value" );
		}
		
		{
			char buf[] = "name=value; ";
			
			cr_string_map cookies;
			parse_cookie_header( cookies, buf, strlen( buf ) );
			res = res &&
				cookies.size() == 1 &&
				!strcmp( cookies[ "name" ], "value" );
		}		
		
		{
			char buf[] = "SID=31d4d96e407aad42";
			
			cr_string_map cookies;
			parse_cookie_header( cookies, buf, strlen( buf ) );
			res = res &&
				cookies.size() == 1 &&
				!strcmp( cookies[ "SID" ], "31d4d96e407aad42" );
		}		
		
		{
			char buf[] = "\n\rname= value\t;\t\t name2=value2\t";
			
			cr_string_map cookies;
			parse_cookie_header( cookies, buf, strlen( buf ) );
			res = res &&
				cookies.size() == 2 &&
				!strcmp( cookies[ "name" ], "value" ) &&
				!strcmp( cookies[ "name2" ], "value2" );
		}
		
		RETURN( res );
	}

	TEST( parse_post_parameters FORM )
	{
		char str[] = "a=1&b=%20";
		
		cr_string_map pm;
		parse_post_parameters( pm, str );
		
		bool res =
			!strcmp( pm[ "a" ], "1" ) &&
			!strcmp( pm[ "b" ], " " );
		
		RETURN( res );
	}
	
	TEST( parse_post_parameters JSON )
	{
		char str[] = "{ \"a\": 1, \"b\" :[\t], \"c\": [\"aa\" ,{} ,123 ]\n, \"d\": [[[1]]], \"e\": \"\\t\\\"1\\n\", \"f\": null , \"g\": { \"a\": 1 }, \"z\": \"2\" }";
		
		cr_string_map pm;
		parse_post_parameters( pm, str );
		
		bool res =
			!strcmp( pm[ "a" ], "1" ) &&
			!strcmp( pm[ "e" ], "\t\"1\n" ) &&
			!strcmp( pm[ "f" ], "null" ) &&
			!strcmp( pm[ "z" ], "2" );
		
		RETURN( res );
	}
	
	TEST( parse_post_parameters XML )
	{
		char str[] =
			"<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
			"<!DOCTYPE foo [ <!ENTITY greeting \"helloworld\"> ]>"
			"<!-- This is a comment -->"
			"<![CDATA[ content ]]>"
			"<c><a>1</a></c>"
			"<a>1</a>"
			"<b>a&gt;&#33;&</b>";
		
		cr_string_map pm;
		parse_post_parameters( pm, str );
		
		bool res =
			!strcmp( pm[ "a" ], "1" ) &&
			!strcmp( pm[ "b" ], "a>!&" );
		
		RETURN( res );
	}
	
	TEST( parse_query_parameters )
	{
		bool res = true;
		
		{
			char str[] = "a=1&b=%20";

			cr_string_map pm;
			parse_post_parameters( pm, str );

			res = res &&
				!strcmp( pm[ "a" ], "1" ) &&
				!strcmp( pm[ "b" ], " " );
		}

		{
			cr_string_map pm;
			parse_post_parameters( pm, "" );

			res = res && pm.size() == 0;
		}

		{
			char str[] = "a=123%2045";

			cr_string_map pm;
			parse_post_parameters( pm, str );

			res = res && !strcmp( pm[ "a" ], "123 45" );
		}
		
		RETURN( res );
	}
	
	FINISH
}
