/**********************************************************************************************/
/* cr_user_manager_internal.h  		                                               			  */
/*                                                                       					  */
/* Igor Nikitin, 2012																		  */
/* MIT license			                                                  					  */
/**********************************************************************************************/

#pragma once

// STD
#include <stddef.h>

/**********************************************************************************************/
#ifndef NO_AUTH


/**********************************************************************************************/
// Internal structure to store single user's info
//
struct crest_user
{
	bool	admin_;
	char*	name_;
	size_t	name_len_;
	char	password_[ 16 ];
};


/**********************************************************************************************/
// Internal members for user_manager
//
class cr_user_manager_internal
{
	protected://////////////////////////////////////////////////////////////////////////
	
							cr_user_manager_internal( void );

	protected://////////////////////////////////////////////////////////////////////////
		
	// ---------------------
	// Internal methods		
		
		crest_user*			create_user( const char* name );
		crest_user*			find_user( const char* name ) const;
		void				flush( void );
		void				load( void );

		
	protected://////////////////////////////////////////////////////////////////////////

// Properties		
		
		char*				auth_file_;
		void*				mutex_;
		size_t				users_count_;
		crest_user*			users_;
};


/**********************************************************************************************/
#endif // NO_AUTH