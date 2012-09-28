/**********************************************************************************************/
/* auth_manager.cpp																			  */
/*                                                                       					  */
/* Igor Nikitin, 2012																		  */
/* MIT license			                                                  					  */
/**********************************************************************************************/

// MONGOOSE
#include "../third/mongoose/mongoose.h"

// CREST
#include "../include/crest.h"

/**********************************************************************************************/
#define ERROR( x ) \
	{ if( err ) *err = x; return false; }

/**********************************************************************************************/
CREST_NAMESPACE_START


//////////////////////////////////////////////////////////////////////////
// global data
//////////////////////////////////////////////////////////////////////////


/**********************************************************************************************/
static mg_mutex g_auth_mutex = mg_mutex_create();


//////////////////////////////////////////////////////////////////////////
// properties
//////////////////////////////////////////////////////////////////////////


/**********************************************************************************************/
int auth_manager::get_user_flags( const string& name ) const
{
	int res = 0;
	
	mg_mutex_lock( g_auth_mutex ); // -----------------------------
	
	map<string,user>::const_iterator it = users_.find( name );
	if( it != users_.end() )
		res = it->second.flags_;
	
	mg_mutex_unlock( g_auth_mutex ); // -----------------------------
	
	return res;
}

/**********************************************************************************************/
vector<string> auth_manager::get_users( void ) const
{
	vector<string> res;

	mg_mutex_lock( g_auth_mutex ); // -----------------------------
	
	map<string,user>::const_iterator it = users_.begin();
	for( ; it != users_.end() ; ++it )
		res.push_back( it->first );
	
	mg_mutex_unlock( g_auth_mutex ); // -----------------------------
	
	return res;
}


//////////////////////////////////////////////////////////////////////////
// methods
//////////////////////////////////////////////////////////////////////////


/**********************************************************************************************/
bool auth_manager::add_user(
	const string&	name,
	const string&	password,
	bool			admin,
	bool			ro,
	string*			err )
{
	// Check parameters

	if( name.empty()			) ERROR( "Empty user name" );
	if( name.length() > 32		) ERROR( "Name too long" );
	if( !isalpha( name[ 0 ] )	) ERROR( "Invalid name" );
	if( password.length() > 32	) ERROR( "Password too long" );
	
	size_t len = name.length();
	for( size_t i = 0 ; i < len ; ++i )
	{
		if( !isalnum( name[ i ] ) )
			ERROR( "Invalid name" );
	}
	
	// Add user
	
	char buf[ 33 ];
	mg_md5( buf, password.c_str(), 0 );	

	int flags = 0;
	if( admin ) flags |= USER_ADMIN;
	if( ro    ) flags |= USER_READONLY;
	
	mg_mutex_lock( g_auth_mutex ); // -----------------------------
	
	bool exists = users_.find( name ) != users_.end();
	if( !exists )
	{
		user& user = users_[ name ];
		user.password_ = buf;
		user.flags_ = flags;
	}
	
	mg_mutex_unlock( g_auth_mutex ); // -----------------------------

	if( exists )
		ERROR( "Already exists user: " + name );
	
	flush();
	return true;	
}

/**********************************************************************************************/
bool auth_manager::delete_user(
	const string&	name,
	string*			err )
{
	// Check parameters
	
	if( name.empty() )
		ERROR( "Empty user name" )
	
	// Delete user
			
	bool adminable = false;
	bool deleted = false;
	
	mg_mutex_lock( g_auth_mutex ); // -----------------------------
	
	map<string,user>::iterator it = users_.begin();
	for( ; it != users_.end() ; ++it )
	{
		if( it->first != name && ( it->second.flags_ & USER_ADMIN ) )
		{
			adminable = true;
			break;
		}
	}
	
	if( adminable )
	{
		it = users_.find( name );
		if( it != users_.end() )
		{
			users_.erase( it );
			deleted = true;
		}
	}
	
	mg_mutex_unlock( g_auth_mutex ); // -----------------------------

	if( !adminable ) ERROR( "Must be atleast one user with admin rights" );	
	if( !deleted   ) ERROR( "User not found: " + name );
	
	flush();
	return true;
}

/**********************************************************************************************/
auth_manager& auth_manager::instance( void )
{
	static auth_manager res;
	return res;
}

/**********************************************************************************************/
bool auth_manager::update_user_flags(
	const string&	name,
	int				flags,
	string*			err )
{
	// Check parameters
	
	if( name.empty() )
		ERROR( "Empty user name" )
	
	// Update flags
			
	bool updated = false;
	
	mg_mutex_lock( g_auth_mutex ); // -----------------------------
	
	map<string,user>::iterator it = users_.find( name );
	if( it != users_.end() )
	{
		it->second.flags_ = flags;
		updated = true;
	}
	
	mg_mutex_unlock( g_auth_mutex ); // -----------------------------
	
	if( !updated )
		ERROR( "User not found: " + name );
	
	flush();
	return true;	
}

/**********************************************************************************************/
bool auth_manager::update_user_password(
	const string&	name,
	const string&	password,
	string*			err )
{
	// Check parameters
	
	if( name.empty()		   ) ERROR( "Empty user name" )
	if( password.length() > 32 ) ERROR( "Password too long" );
	
	// Update password
	
	char buf[ 33 ];
	mg_md5( buf, password.c_str(), 0 );	
	
	bool updated = false;
	
	mg_mutex_lock( g_auth_mutex ); // -----------------------------
	
	map<string,user>::iterator it = users_.find( name );
	if( it != users_.end() )
	{
		it->second.password_ = buf;
		updated = true;
	}
	
	mg_mutex_unlock( g_auth_mutex ); // -----------------------------
	
	if( !updated )
		ERROR( "User not found: " + name );
	
	flush();
	return true;	
}


//////////////////////////////////////////////////////////////////////////
// internal methods
//////////////////////////////////////////////////////////////////////////


/**********************************************************************************************/
void auth_manager_internal::flush( void )
{
	if( file_.empty() )
		return;
	
	FILE* f = fopen( file_.c_str(), "wt" );
	if( !f )
		return;
	
	mg_mutex_lock( g_auth_mutex ); // -----------------------------
	
	map<string,user>::const_iterator it = users_.begin();
	for( ; it != users_.end() ; ++it )
	{
		char flags = '0' + it->second.flags_;
		
		fwrite( it->first.c_str(), it->first.length(), 1, f );
		fwrite( " ", 1, 1, f );
		fwrite( &flags, 1, 1, f );
		fwrite( " ", 1, 1, f );
		fwrite( it->second.password_.c_str(), it->second.password_.length(), 1, f );
		fwrite( "\n", 1, 1, f );
	}
	
	mg_mutex_unlock( g_auth_mutex ); // -----------------------------
	
	fclose( f );
}

/**********************************************************************************************/
void auth_manager_internal::load( void )
{
	if( file_.empty() )
		return;

	FILE* f = fopen( file_.c_str(), "rt" );
	if( !f )
		return;
	
	char buf[ 512 ];
		
	mg_mutex_lock( g_auth_mutex ); // -----------------------------
	
    while( !feof( f ) )
    {
		(void) fgets( buf, 512, f );
		
		char* name = buf;
		
		char* flags = strchr( buf, ' ' );
		if( !flags ) continue;
		*flags++ = 0;
		
		char* passwd = strchr( flags, ' ' );
		if( !passwd ) continue;
		*passwd++ = 0;

		user& user = users_[ name ];
		user.flags_ = *flags - '0';
		user.password_ = passwd;
	}	
	
	mg_mutex_unlock( g_auth_mutex ); // -----------------------------
	
	fclose( f );
	
	// Add default 'root' user if need
	if( users_.empty() )
	{
		char buf[ 33 ];
		mg_md5( buf, "", 0 );		

		mg_mutex_lock( g_auth_mutex ); // -----------------------------
		
		user& user = users_[ "root" ];
		user.flags_ = USER_ADMIN;
		user.password_ = buf;
		
		mg_mutex_unlock( g_auth_mutex ); // -----------------------------
		
		flush();
	}
}


/**********************************************************************************************/
CREST_NAMESPACE_END
