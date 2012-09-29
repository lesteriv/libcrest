/**********************************************************************************************/
/* auth_manager_internal.h  		                                               			  */
/*                                                                       					  */
/* Igor Nikitin, 2012																		  */
/* MIT license			                                                  					  */
/**********************************************************************************************/

#pragma once

// STD
#include <map>
#include <stdlib.h>
#include <string>

/**********************************************************************************************/
using std::map;
using std::string;


/**********************************************************************************************/
// Internal members for auth_manager
//
class crest_auth_manager_internal
{
	friend class crest;

	protected://////////////////////////////////////////////////////////////////////////
	
		crest_auth_manager_internal( void )
		{
			file_ = 0;
		}
		
		~crest_auth_manager_internal( void )
		{
			free( file_ );
		}
	
	protected://////////////////////////////////////////////////////////////////////////
		
	// ---------------------
	// Internal methods		
		
		void				flush( void );
		void				load( void );

		
	protected://////////////////////////////////////////////////////////////////////////

// Properties		
		
		struct user
		{
			int				flags_;
			char			password_[ 16 ];
		};
		
		char*				file_;
		map<string,user>	users_;
};
