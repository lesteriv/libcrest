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
#include "utils.h"


//////////////////////////////////////////////////////////////////////////
// global data
//////////////////////////////////////////////////////////////////////////


/**********************************************************************************************/
static mg_mutex g_auth_mutex = mg_mutex_create();


//////////////////////////////////////////////////////////////////////////
// properties
//////////////////////////////////////////////////////////////////////////


/**********************************************************************************************/
size_t crest_auth_manager::get_user_count( void ) const
{
	return user_count_;
}

/**********************************************************************************************/
int crest_auth_manager::get_user_flags( const char* name ) const
{
	int res = 0;
	
	mg_mutex_lock( g_auth_mutex ); // -----------------------------
	
	crest_user* user = find_user( name );
	if( user )
		res = user->flags_;
	
	mg_mutex_unlock( g_auth_mutex ); // -----------------------------
	
	return res;
}

/**********************************************************************************************/
void crest_auth_manager::get_users( 
	size_t&		count,
	char**&		names ) const
{
	mg_mutex_lock( g_auth_mutex ); // -----------------------------

	count = user_count_;
	names = (char**) malloc( count * sizeof * names );

	for( size_t i = 0 ; i < count ; ++i )
		names[ i ] = crest_strdup( users_[ i ].password_ );

	mg_mutex_unlock( g_auth_mutex ); // -----------------------------
}


//////////////////////////////////////////////////////////////////////////
// methods
//////////////////////////////////////////////////////////////////////////


/**********************************************************************************************/
const char* crest_auth_manager::add_user(
	const char*	name,
	const char*	pass,
	bool		admin,
	bool		ro )
{
	// Check parameters

	if( !name || !*name )
		return "Empty user name";
	
	size_t name_len = strlen( name );
	size_t pass_len = strlen( pass );
	
	if( name_len > 32	  ) return "Name too long";
	if( !isalpha( *name ) ) return "Invalid name";
	if( pass_len > 32	  ) return "Password too long";
	
	for( size_t i = 0 ; i < name_len ; ++i )
	{
		if( !isalnum( name[ i ] ) )
			return "Invalid name";
	}
	
	// Add user
	
	char buf[ 16 ];
	mg_md5( buf, pass, 0 );	

	int flags = 0;
	if( admin ) flags |= CREST_USER_ADMIN;
	if( ro    ) flags |= CREST_USER_READONLY;
	
	mg_mutex_lock( g_auth_mutex ); // -----------------------------
	
	crest_user* user = find_user( name );
	if( !user )
	{
		crest_user* new_user = create_user( name );
		memcpy( new_user->password_, buf, 16 );
		new_user->flags_ = flags;
	}
	
	mg_mutex_unlock( g_auth_mutex ); // -----------------------------

	if( user )
		return "User already exists";
	
	flush();
	return NULL;
}

/**********************************************************************************************/
const char* crest_auth_manager::delete_user( const char* name )
{
	// Check parameters
	
	if( !name || !*name )
		 return "Empty user name";
	
	// Delete user
			
	bool adminable	= false;
	bool deleted	= false;
	int  index		= -1;
	
	mg_mutex_lock( g_auth_mutex ); // -----------------------------
	
	for( size_t i = 0 ; i < user_count_ ; ++i )
	{
		if( !strcmp( users_[ i ].name_, name ) )
			index = (int) i;
		else if( users_[ i ].flags_ & CREST_USER_ADMIN )
			adminable = true;
	}
	
	if( adminable && index >= 0 )
	{
		free( users_[ index ].name_ );
		if( index < (int) user_count_ - 1 )
			memmove( users_ + index, users_ + index + 1, user_count_ - index - 1 );
		
		--user_count_;
		deleted = true;
	}
	
	mg_mutex_unlock( g_auth_mutex ); // -----------------------------

	if( !adminable ) return "Must be atleast one user with admin rights";	
	if( !deleted   ) return "User not found";
	
	flush();
	return NULL;
}

/**********************************************************************************************/
crest_auth_manager& crest_auth_manager::instance( void )
{
	static crest_auth_manager res;
	return res;
}

/**********************************************************************************************/
const char* crest_auth_manager::update_user_flags(
	const char*	name,
	int			flags )
{
	// Check parameters
	
	if( !name || !*name )
		return "Empty user name";
	
	// Update flags
			
	bool updated = false;
	
	mg_mutex_lock( g_auth_mutex ); // -----------------------------
	
	crest_user* user = find_user( name );
	if( user )
	{
		user->flags_ = flags;
		updated = true;
	}
	
	mg_mutex_unlock( g_auth_mutex ); // -----------------------------
	
	if( !updated )
		return "User not found";
	
	flush();
	return NULL;	
}

/**********************************************************************************************/
const char* crest_auth_manager::update_user_password(
	const char*	name,
	const char*	pass )
{
	// Check parameters
	
	if( !pass )	pass = "";
	size_t pass_len = strlen( pass );
	
	if( !name || !*name	) return "Empty user name";
	if( pass_len > 32	) return "Password too long";
	
	// Update password
	
	char buf[ 16 ];
	mg_md5( buf, pass, 0 );	
	
	bool updated = false;
	
	mg_mutex_lock( g_auth_mutex ); // -----------------------------
	
	crest_user* user = find_user( name );
	if( user )
	{
		memcpy( user->password_, buf, 16 );
		updated = true;
	}
	
	mg_mutex_unlock( g_auth_mutex ); // -----------------------------
	
	if( !updated )
		return "User not found";
	
	flush();
	return NULL;
}


//////////////////////////////////////////////////////////////////////////
// internal methods
//////////////////////////////////////////////////////////////////////////


/**********************************************************************************************/
crest_auth_manager_internal::crest_auth_manager_internal( void )
{
	auth_file_			= 0;
	user_count_	= 0;
	users_			= 0;
}

/**********************************************************************************************/
crest_auth_manager_internal::~crest_auth_manager_internal( void )
{
	clean();
}

/**********************************************************************************************/
void crest_auth_manager_internal::clean( void )
{
	mg_mutex_lock( g_auth_mutex ); // -----------------------------
	
	for( size_t i = 0 ; i < user_count_ ; ++i )
		free( users_[ i ].name_ );
	
	free( users_ );
	users_ = 0;
	user_count_ = 0;
	
	free( auth_file_ );
	auth_file_ = 0;
	
	mg_mutex_unlock( g_auth_mutex ); // -----------------------------
}

/**********************************************************************************************/
crest_user* crest_auth_manager_internal::create_user( const char* name )
{
	user_count_++;
	
	if( !users_ )
	{
		users_ = (crest_user*) malloc( sizeof( crest_user ) );
		users_->name_ = crest_strdup( name );
		
		return users_;
	}
	
	users_ = (crest_user*) realloc( users_, sizeof( crest_user ) * user_count_ );
	crest_user* last = users_ + user_count_ - 1;
	last->name_ = crest_strdup( name );
	
	return last;
}

/**********************************************************************************************/
crest_user* crest_auth_manager_internal::find_user( const char* name ) const
{
	for( size_t i = 0 ; i < user_count_ ; ++i )
	{
		if( !strcmp( users_[ i ].name_, name ) )
			return users_ + i;
	}	
	
	return NULL;
}

/**********************************************************************************************/
void crest_auth_manager_internal::flush( void )
{
	if( !auth_file_ )
		return;
	
	FILE* f = fopen( auth_file_, "wt" );
	if( !f )
		return;
	
	mg_mutex_lock( g_auth_mutex ); // -----------------------------
	
	for( size_t i = 0 ; i < user_count_ ; ++i )
	{
		crest_user& user = users_[ i ];
		char flags = '0' + user.flags_;
		
		fwrite( user.name_, strlen( user.name_ ), 1, f );
		fputc( ' ', f );
		fputc( flags, f );
		fputc( ' ', f );
		
		const unsigned char* bt = (const unsigned char*) user.password_;
		for( size_t i = 0 ; i < 16 ; ++i )
		{
			fputc( "0123456789abcdef"[ *bt / 16 ], f );
			fputc( "0123456789abcdef"[ *bt % 16 ], f );
			++bt;
		}

		fputc( '\n', f );
	}
	
	mg_mutex_unlock( g_auth_mutex ); // -----------------------------
	
	fclose( f );
}

/**********************************************************************************************/
void crest_auth_manager_internal::load( void )
{
	if( !auth_file_ )
		return;

	mg_mutex_lock( g_auth_mutex ); // -----------------------------
	
	FILE* f = fopen( auth_file_, "rt" );
	if( f )
	{	
		char bt[ 3 ] = "00";
		char buf[ 512 ];

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

			// Invalid password length - 32 bytes + \n
			if( strlen( passwd ) < 32 )
				continue;

			passwd[ 32 ] = 0;
			
			crest_user* user = create_user( name );
			user->flags_ = *flags - '0';

			char* dpass = user->password_;
			while( *passwd && *(passwd + 1) )
			{
				bt[ 0 ] = *passwd++;
				bt[ 1 ] = *passwd++;

				*dpass++ = strtol( bt, NULL, 16 );
			}
		}	

		fclose( f );
	}
	
	// Add default 'root' user if need
	if( !user_count_ )
	{
		char buf[ 16 ];
		mg_md5( buf, "", 0 );		

		crest_user* user = create_user( "root" );
		user->flags_ = CREST_USER_ADMIN;
		memcpy( user->password_, buf, 16 );
		
		flush();
	}

	mg_mutex_unlock( g_auth_mutex ); // -----------------------------
}
