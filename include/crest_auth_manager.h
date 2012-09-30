/**********************************************************************************************/
/* auth_manager.h																			  */
/*                                                                       					  */
/* Igor Nikitin, 2012																		  */
/* MIT license			                                                  					  */
/**********************************************************************************************/

#pragma once

// CREST
#include "internal/crest_auth_manager_internal.h"

/**********************************************************************************************/
#define the_crest_auth_manager ( crest_auth_manager::instance() )


/**********************************************************************************************/
// Class to manage users and it's passwords
//
class crest_auth_manager : public crest_auth_manager_internal
{
	public://////////////////////////////////////////////////////////////////////////

	// ---------------------
	// Properties
		
							/** Returns path to file to store auth info. */
		const char*			get_auth_file( void ) const;
		void				set_auth_file( const char* file );
		
							/** Returns count of users. */
		size_t				get_user_count( void ) const;
		
							/** Returns flags for user. */
		int					get_user_flags( const char* name ) const;

							/** Returns list of users, it should be deleted by caller
							 * via "free( names );". */
		void				get_users( 
								size_t&		count,
								char**&		names ) const;

		
	public://////////////////////////////////////////////////////////////////////////

	// ---------------------
	// Methods		

							/** Adds new user, returns error's description on fail,
							 *  and NULL on success. */
		const char*			add_user(
								const char*	name,
								const char*	password,
								bool		admin,
								bool		ro );
		
							/** Returns TRUE if user @name exists, has password
							 *  @password and match flags. */
		bool				auth(
								const char*	name,
								const char*	password,
								bool		admin,
								bool		ro );								

							/** Reset all info and free used memory. */
		void				clean( void );

							/** Deletes existing user, returns error's description on fail,
							 *  and NULL on success. */
		const char*			delete_user( const char* name );
		
							/** Returns singleton. */
static	crest_auth_manager&	instance( void );
		
							/** Changes user's flags, returns error's description on fail,
							 *  and NULL on success. */
		const char*			update_user_flags(
								const char*	name,
								int			flags );
		
							/** Changes user's password, returns error's description on fail,
							 *  and NULL on success. */
		const char*			update_user_password(
								const char*	name,
								const char*	password );
};
