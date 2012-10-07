/**********************************************************************************************/
/* tests.cpp						                                               			  */
/*                                                                       					  */
/* Igor Nikitin, 2012																		  */
/* MIT license			                                                  					  */
/**********************************************************************************************/

// STD
#include <signal.h>
#include <stdio.h>
#include <time.h>

// CREST
#include "../include/crest.h"
#include "../src/utils.h"

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
		bool res = file_exists( "/tmp/auth.passwd" );
		
		remove( "/tmp/auth.passwd" );
		RETURN( res );
	}
	
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

	TEST( cr_user_manager::add_user and
		  cr_user_manager::auth	   and
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
	
	FINISH
}
