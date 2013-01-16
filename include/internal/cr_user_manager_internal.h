/**********************************************************************************************/
/* cr_user_manager_internal.h  		                                               			  */
/*                                                                       					  */
/* Igor Nikitin, 2013																		  */
/* MIT license			                                                  					  */
/**********************************************************************************************/

#pragma once

// STD
#include <mutex>
#include <unordered_map>
#include <vector>


/**********************************************************************************************/
// Internal members for user_manager
//
class cr_user_manager_internal
{
	protected://////////////////////////////////////////////////////////////////////////	

	// ---------------------
	// Sub types
		
		struct user_t
		{
			bool admin_;
			char password_[ 16 ];
		};

		typedef std::unordered_map<std::string, user_t> users_t;


	protected://////////////////////////////////////////////////////////////////////////
		
	// ---------------------
	// Internal methods		
		
		void				flush( void );
		void				load( void );

	
	protected://////////////////////////////////////////////////////////////////////////

// Properties		
		
		std::string			auth_file_;
mutable	std::mutex			mutex_;
		users_t				users_;
};
