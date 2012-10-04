/**********************************************************************************************/
/* user_manager.cpp																			  */
/*                                                                       					  */
/* Igor Nikitin, 2012																		  */
/* MIT license			                                                  					  */
/**********************************************************************************************/

// STD
#include <ctype.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

// MONGOOSE
#include "../third/mongoose/mongoose.h"

// CREST
#include "../include/crest.h"
#include "utils.h"

/**********************************************************************************************/
#ifdef _MSC_VER
#pragma warning( disable: 4996 )
#endif // _WIN32

/**********************************************************************************************/
#ifndef NO_AUTH


//////////////////////////////////////////////////////////////////////////
// properties
//////////////////////////////////////////////////////////////////////////


/**********************************************************************************************/
const char* crest_user_manager::get_auth_file( void ) const
{
	return auth_file_;
}

/**********************************************************************************************/
void crest_user_manager::set_auth_file( const char* file )
{
	clean();
	
	mg_mutex_lock( mutex_ );
	auth_file_ = crest_strdup( file );
	mg_mutex_unlock( mutex_ );
	
	load();
}

/**********************************************************************************************/
size_t crest_user_manager::get_user_count( void ) const
{
	return users_count_;
}

/**********************************************************************************************/
bool crest_user_manager::get_user_is_admin( const char* name ) const
{
	bool res = false;
	
	mg_mutex_lock( mutex_ ); // -----------------------------
	
	crest_user* user = find_user( name );
	if( user )
		res = user->admin_;
	
	mg_mutex_unlock( mutex_ ); // -----------------------------
	
	return res;
}

/**********************************************************************************************/
void crest_user_manager::get_users( 
	size_t&		count,
	char**&		names ) const
{
	mg_mutex_lock( mutex_ ); // -----------------------------

	size_t flen = ( users_count_ + 1 ) * sizeof( crest_user* );
	for( size_t i = 0 ; i < users_count_ ; ++i )
		flen += users_[ i ].name_len_ + 1;
	
	count = users_count_;
	names = (char**) malloc( flen );

	char* s = (char*) names + users_count_ * sizeof( crest_user* );
	
	for( size_t i = 0 ; i < users_count_ ; ++i )
	{
		names[ i ] = s;
		s = add_string( s,users_[ i ].name_, users_[ i ].name_len_ + 1 );
	}

	mg_mutex_unlock( mutex_ ); // -----------------------------
}


//////////////////////////////////////////////////////////////////////////
// methods
//////////////////////////////////////////////////////////////////////////


/**********************************************************************************************/
const char* crest_user_manager::add_user(
	const char*	name,
	const char*	pass,
	bool		admin )
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
	const char* data[] = { name, ":", "", ":", pass };
	size_t len[] = { name_len, 1, 0, 1, pass_len };
	md5( buf, 5, data, len );	

	mg_mutex_lock( mutex_ ); // -----------------------------
	
	crest_user* user = find_user( name );
	if( !user )
	{
		crest_user* new_user = create_user( name );
		memcpy( new_user->password_, buf, 16 );
		new_user->admin_ = admin;
	}
	
	mg_mutex_unlock( mutex_ ); // -----------------------------

	if( user )
		return "User already exists";
	
	flush();
	return NULL;
}

/**********************************************************************************************/
void crest_user_manager::clean( void )
{
	mg_mutex_lock( mutex_ ); // -----------------------------
	
	for( size_t i = 0 ; i < users_count_ ; ++i )
		free( users_[ i ].name_ );
	
	free( users_ );
	users_ = 0;
	users_count_ = 0;
	
	free( auth_file_ );
	auth_file_ = 0;
	
	mg_mutex_unlock( mutex_ ); // -----------------------------
}

/**********************************************************************************************/
const char* crest_user_manager::delete_user( const char* name )
{
	// Check parameters
	
	if( !name || !*name )
		 return "Empty user name";
	
	// Delete user
			
	bool deleted = false;
	
	mg_mutex_lock( mutex_ ); // -----------------------------
	
	for( size_t i = 0 ; i < users_count_ ; ++i )
	{
		if( !strcmp( users_[ i ].name_, name ) )
		{
			free( users_[ i ].name_ );
			if( i + 1 < users_count_ )
				memmove( users_ + i, users_ + i + 1, users_count_ - i - 1 );

			--users_count_;
		}
	}
	mg_mutex_unlock( mutex_ ); // -----------------------------

	if( !deleted )
		return "User not found";
	
	flush();
	return NULL;
}

/**********************************************************************************************/
bool crest_user_manager::get_password( 
	const char*	name,
	char*		pass )
{
	bool res = false;
	
	mg_mutex_lock( mutex_ ); // -----------------------------
	
	crest_user* user = find_user( name );
	if( user )
	{
		memcpy( pass, user->password_, 16 );
		res = true;
	}
	
	mg_mutex_unlock( mutex_ ); // -----------------------------
	
	return res;
}

/**********************************************************************************************/
crest_user_manager& crest_user_manager::instance( void )
{
	static crest_user_manager res;
	return res;
}

/**********************************************************************************************/
const char* crest_user_manager::update_user_is_admin(
	const char*	name,
	bool		value )
{
	// Check parameters
	
	if( !name || !*name )
		return "Empty user name";
	
	// Update flags
			
	mg_mutex_lock( mutex_ ); // -----------------------------
	
	crest_user* user = find_user( name );
	if( user )
		user->admin_ = value;
	
	mg_mutex_unlock( mutex_ ); // -----------------------------
	
	if( !user )
		return "User not found";
	
	flush();
	return NULL;	
}

/**********************************************************************************************/
const char* crest_user_manager::update_user_password(
	const char*	name,
	const char*	pass )
{
	// Check parameters
	
	if( !pass )	pass = "";
	size_t name_len = strlen( name );
	size_t pass_len = strlen( pass );
	
	if( !name || !*name	) return "Empty user name";
	if( pass_len > 32	) return "Password too long";
	
	// Update password
	
	char buf[ 16 ];
	const char* data[] = { name, ":", "", ":", pass };
	size_t len[] = { name_len, 1, 0, 1, pass_len };	
	md5( buf, 5, data, len );	
	
	mg_mutex_lock( mutex_ ); // -----------------------------
	
	crest_user* user = find_user( name );
	if( user )
		memcpy( user->password_, buf, 16 );
	
	mg_mutex_unlock( mutex_ ); // -----------------------------
	
	if( !user )
		return "User not found";
	
	flush();
	return NULL;
}


//////////////////////////////////////////////////////////////////////////
// internal methods
//////////////////////////////////////////////////////////////////////////


/**********************************************************************************************/
crest_user_manager_internal::crest_user_manager_internal( void )
{
	auth_file_		= 0;
	mutex_			= mg_mutex_create();
	users_count_	= 0;
	users_			= 0;
}

/**********************************************************************************************/
crest_user* crest_user_manager_internal::create_user( const char* name )
{
	++users_count_;
	
	if( !users_ )
	{
		users_ = (crest_user*) malloc( sizeof( crest_user ) );
		users_->name_len_ = strlen( name );
		users_->name_ = crest_strdup( name, users_->name_len_ );
		
		return users_;
	}
	
	users_ = (crest_user*) realloc( users_, sizeof( crest_user ) * users_count_ );
	crest_user* last = users_ + users_count_ - 1;
	last->name_len_ = strlen( name );
	last->name_ = crest_strdup( name, last->name_len_ );
	
	return last;
}

/**********************************************************************************************/
crest_user* crest_user_manager_internal::find_user( const char* name ) const
{
	for( size_t i = 0 ; i < users_count_ ; ++i )
	{
		if( !strcmp( users_[ i ].name_, name ) )
			return users_ + i;
	}	
	
	return NULL;
}

/**********************************************************************************************/
void crest_user_manager_internal::flush( void )
{
	mg_mutex_lock( mutex_ ); // -----------------------------

	if( auth_file_ )
	{
		FILE* f = fopen( auth_file_, "wt" );
		if( f )
		{
			for( size_t i = 0 ; i < users_count_ ; ++i )
			{
				crest_user& user = users_[ i ];

				fwrite( user.name_, strlen( user.name_ ), 1, f );
				fputc( ' ', f );
				fputc( user.admin_ ? '1' : '0', f );
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
			
			fclose( f );
		}
	}
	
	mg_mutex_unlock( mutex_ ); // -----------------------------
}

/**********************************************************************************************/
void crest_user_manager_internal::load( void )
{
	bool need_flush = false;
	
	mg_mutex_lock( mutex_ ); // -----------------------------
	
	if( auth_file_ )
	{
		FILE* f = fopen( auth_file_, "rt" );
		if( f )
		{	
			char bt[ 3 ] = "00";
			char buf[ 512 ];

			while( !feof( f ) )
			{
				if( !fgets( buf, 512, f ) )
					break;

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
				user->admin_ = ( *flags == '1' );

				char* dpass = user->password_;
				while( *passwd && *(passwd + 1) )
				{
					bt[ 0 ] = *passwd++;
					bt[ 1 ] = *passwd++;

					*dpass++ = (char) strtol( bt, NULL, 16 );
				}
			}	

			fclose( f );
		}

		// Add default 'root' user if need
		if( !users_count_ )
		{
			crest_user* user = create_user( "root" );
			user->admin_ = true;
			
			const char* data[] = { "root", ":", "", ":", "" };
			size_t len[] = { 4, 1, 0, 1, 0 };	
			md5( user->password_, 5, data, len );		

			need_flush = true;
		}
	}

	mg_mutex_unlock( mutex_ ); // -----------------------------
	
	if( need_flush )
		flush();
}


/**********************************************************************************************/
#endif // NO_AUTH
