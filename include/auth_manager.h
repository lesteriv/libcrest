/**********************************************************************************************/
/* auth_manager.h																			  */
/*                                                                       					  */
/* Igor Nikitin, 2012																		  */
/* MIT license			                                                  					  */
/**********************************************************************************************/

#pragma once

// CREST
#include "internal/auth_manager_internal.h"

/**********************************************************************************************/
CREST_NAMESPACE_START


/**********************************************************************************************/
// Class to manage users and it's passwords
//
class auth_manager : public auth_manager_internal
{
	public://////////////////////////////////////////////////////////////////////////

	// ---------------------
	// Properties
		
							/** Returns flags for user. */
		int					get_user_flags( const string& user ) const;

							/** Returns list of users. */
		vector<string>		get_users( void ) const;

		
	public://////////////////////////////////////////////////////////////////////////

	// ---------------------
	// Methods		

							/** Adds new user, if fail optionally returns
							 *  error's description in @err. */
		bool				add_user(
								const string&	name,
								const string&	password,
								bool			admin,
								bool			ro,
								string*			err = NULL );
		
							/** Deletes existing user, if fail optionally returns
							 *  error's description in @err. */
		bool				delete_user( 
								const string&	name,
								string*			err = NULL );
		
							/** Returns singleton. */
static	auth_manager&		instance( void );
		
							/** Changes user's flags. */
		bool				update_user_flags(
								const string&	name,
								int				flags,
								string*			err = NULL );
		
							/** Changes user's password. */
		bool				update_user_password(
								const string&	name,
								const string&	password,
								string*			err = NULL );
};


/**********************************************************************************************/
CREST_NAMESPACE_END
