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
	// AUTH_MANAGER
	// -----------------------------------------------------------------------
	
	TEST( crest_auth_manager::crest_auth_manager )
	{
		the_crest_auth_manager.clean();
		
		RETURN( the_crest_auth_manager.get_user_count() == 0 );
	}

	TEST( crest_auth_manager::flush )
	{
		the_crest_auth_manager.clean();
		
		the_crest_auth_manager.set_auth_file( "/tmp/auth.passwd" );
		bool res = file_exists( "/tmp/auth.passwd" );
		
		remove( "/tmp/auth.passwd" );
		RETURN( res );
	}
	
	TEST( crest_auth_manager::load )
	{
		the_crest_auth_manager.clean();
		
		the_crest_auth_manager.set_auth_file( "/tmp/auth.passwd" );
		the_crest_auth_manager.update_user_password( "root", "qwerty" );
		the_crest_auth_manager.set_auth_file( "/tmp/auth.passwd2" );
		
		bool res = !the_crest_auth_manager.auth( "root", "qwerty", false, false );
		the_crest_auth_manager.set_auth_file( "/tmp/auth.passwd" );
		res = res && the_crest_auth_manager.auth( "root", "qwerty", false, false );
		
		remove( "/tmp/auth.passwd" );
		remove( "/tmp/auth.passwd2" );
		RETURN( res );
	}
	
	TEST( crest_auth_manager::get_auth_file and
		  crest_auth_manager::set_auth_file )
	{
		the_crest_auth_manager.clean();
		
		the_crest_auth_manager.set_auth_file( "/tmp/auth.passwd" );
		bool res = !strcmp( "/tmp/auth.passwd", the_crest_auth_manager.get_auth_file() );
		
		remove( "/tmp/auth.passwd" );
		RETURN( res );
	}
	
	TEST( crest_auth_manager::get_user_count )
	{
		the_crest_auth_manager.clean();
		
		bool res = the_crest_auth_manager.get_user_count() == 0;
		the_crest_auth_manager.set_auth_file( "/tmp/auth.passwd" );
		res = res && the_crest_auth_manager.get_user_count() == 1;
		
		remove( "/tmp/auth.passwd" );
		RETURN( res );
	}
	
	TEST( crest_auth_manager::get_user_flags )
	{
		the_crest_auth_manager.clean();
		
		the_crest_auth_manager.set_auth_file( "/tmp/auth.passwd" );
		bool res = the_crest_auth_manager.get_user_flags( "root" ) == CREST_USER_ADMIN;
		
		remove( "/tmp/auth.passwd" );
		RETURN( res );
	}
	
	TEST( crest_auth_manager::get_users )
	{
		the_crest_auth_manager.clean();
		
		the_crest_auth_manager.set_auth_file( "/tmp/auth.passwd" );
		
		size_t count;
		char** users;
		the_crest_auth_manager.get_users( count, users );
		
		bool res = ( count == 1 ) && ( !strcmp( users[ 0 ], "root" ) );
		free( users );
		
		remove( "/tmp/auth.passwd" );
		RETURN( res );
	}

	TEST( crest_auth_manager::add_user and
		  crest_auth_manager::auth	   and
		  crest_auth_manager::clean )
	{
		the_crest_auth_manager.clean();
		
		the_crest_auth_manager.set_auth_file( "/tmp/auth.passwd" );
		the_crest_auth_manager.add_user( "test", "qwerty", true, true );
		
		bool res = the_crest_auth_manager.get_user_count() == 2;
		res = res && the_crest_auth_manager.auth( "test", "qwerty", true, true );
	
		the_crest_auth_manager.clean();
		res = res && !the_crest_auth_manager.get_auth_file();
		res = res && !the_crest_auth_manager.get_user_count();
		
		remove( "/tmp/auth.passwd" );
		RETURN( res );
	}

	TEST( crest_auth_manager::delete_user )
	{
		the_crest_auth_manager.clean();
		
		the_crest_auth_manager.set_auth_file( "/tmp/auth.passwd" );
		the_crest_auth_manager.add_user( "test", "qwerty", true, true );
		
		bool res = the_crest_auth_manager.get_user_count() == 2;
		the_crest_auth_manager.delete_user( "test" );
		res = res && the_crest_auth_manager.get_user_count() == 1;
		
		remove( "/tmp/auth.passwd" );
		RETURN( res );
	}
	
	TEST( crest_auth_manager::update_user_flags )
	{
		the_crest_auth_manager.clean();
		
		the_crest_auth_manager.set_auth_file( "/tmp/auth.passwd" );
		the_crest_auth_manager.add_user( "test", "qwerty", false, false );
		
		the_crest_auth_manager.update_user_flags( "test", CREST_USER_ADMIN );
		bool res = the_crest_auth_manager.get_user_flags( "test" ) == CREST_USER_ADMIN;
		
		remove( "/tmp/auth.passwd" );
		RETURN( res );
	}
	
	TEST( crest_auth_manager::update_user_password )
	{
		the_crest_auth_manager.clean();
		
		the_crest_auth_manager.set_auth_file( "/tmp/auth.passwd" );
		the_crest_auth_manager.add_user( "test", "qwerty", false, false );
		
		bool res = the_crest_auth_manager.auth( "test", "qwerty", false, false );
		
		the_crest_auth_manager.update_user_password( "test", "ytrewq" );
		res = res && the_crest_auth_manager.auth( "test", "ytrewq", false, false );
		
		remove( "/tmp/auth.passwd" );
		RETURN( res );
	}

	FINISH
}
