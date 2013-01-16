/**********************************************************************************************/
/* cr_user_manager.h																		  */
/*                                                                       					  */
/* Igor Nikitin, 2013																		  */
/* MIT license			                                                  					  */
/**********************************************************************************************/

#pragma once

// CREST
#include "internal/cr_user_manager_internal.h"

/**********************************************************************************************/
#define the_cr_user_manager ( cr_user_manager::instance() )


/**********************************************************************************************/
// Class to manage users and it's passwords
//
class cr_user_manager : protected cr_user_manager_internal
{
	public://////////////////////////////////////////////////////////////////////////

	// ---------------------
	// Properties
		
									/** Returns path to file to store auth info. */
		const std::string&			get_auth_file( void ) const;
		void						set_auth_file( const std::string& file );
		
									/** Returns count of users. */
		size_t						get_user_count( void ) const;
		
									/** Returns TRUE if user is admin. */
		bool						get_user_is_admin( const std::string& name ) const;

									/** Returns list of users names. */
		std::vector<std::string>	get_users( void ) const;

		
	public://////////////////////////////////////////////////////////////////////////

	// ---------------------
	// Methods		

									/** Adds new user, returns error's description on fail,
									 *  and NULL on success. */
		const char*					add_user(
										const std::string&	name,
										const std::string&	password,
										bool				admin );

									/** Deletes existing user, returns error's description on fail,
									 *  and NULL on success. */
		const char*					delete_user( const std::string& name );
		
									/** Returns hash of password for user and TRUE if such user exists. */
		bool						get_password_hash(
										const std::string&	name,
										char*				pass );
		
									/** Returns instance of user manager. */
static	cr_user_manager&			instance( void );
		
									/** Changes user's flags, returns error's description on fail,
									 *  and NULL on success. */
		const char*					update_user_is_admin(
										const std::string&	name,
										bool				value );
		
									/** Changes user's password, returns error's description on fail,
									 *  and NULL on success. */
		const char*					update_user_password(
										const std::string&	name,
										const std::string&	password );
};
