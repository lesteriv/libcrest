/**********************************************************************************************/
/* auth_manager_internal.h  		                                               			  */
/*                                                                       					  */
/* Igor Nikitin, 2012																		  */
/* MIT license			                                                  					  */
/**********************************************************************************************/

#pragma once

// STD
#include <stdlib.h>


/**********************************************************************************************/
// Internal structure to store single user's info
//
struct crest_user
{
	int		flags_;
	char*	name_;
	size_t	name_len_;
	char	password_[ 16 ];
};


/**********************************************************************************************/
// Internal members for auth_manager
//
class crest_auth_manager_internal
{
	protected://////////////////////////////////////////////////////////////////////////
	
							crest_auth_manager_internal( void );

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
		size_t				user_count_;
		crest_user*			users_;
};
