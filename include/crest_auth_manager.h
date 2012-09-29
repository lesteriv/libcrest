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
// Class to manage users and it's passwords
//
class crest_auth_manager : public crest_auth_manager_internal
{
	public://////////////////////////////////////////////////////////////////////////

	// ---------------------
	// Properties
		
							/** Returns count of users. */
		size_t				get_user_count( void ) const;
		
							/** Returns flags for user. */
		int					get_user_flags( const char* name ) const;

							/** Returns list of users, it should be deleted by caller -
							 *  each string and top pointer. */
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
