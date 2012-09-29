/**********************************************************************************************/
/* auth_manager_internal.h  		                                               			  */
/*                                                                       					  */
/* Igor Nikitin, 2012																		  */
/* MIT license			                                                  					  */
/**********************************************************************************************/

#pragma once


/**********************************************************************************************/
// Internal members for auth_manager
//
class crest_auth_manager_internal
{
	friend class crest;

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
		
		string				file_;
		map<string,user>	users_;
};
