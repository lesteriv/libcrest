/**********************************************************************************************/
/* cr_user_manager.cpp																		  */
/*                                                                       					  */
/* Igor Nikitin, 2013																		  */
/* MIT license			                                                  					  */
/**********************************************************************************************/

// CREST
#include "../include/crest.h"
#include "../include/cr_utils.h"
#include "cr_utils_private.h"

/**********************************************************************************************/
#ifdef _MSC_VER
#pragma warning( disable: 4996 )
#endif // _WIN32


//////////////////////////////////////////////////////////////////////////
// global data
//////////////////////////////////////////////////////////////////////////


/**********************************************************************************************/
static cr_user_manager g_user_manager;
	

//////////////////////////////////////////////////////////////////////////
// properties
//////////////////////////////////////////////////////////////////////////


/**********************************************************************************************/
const std::string& cr_user_manager::get_auth_file( void ) const
{
	return auth_file_;
}

/**********************************************************************************************/
void cr_user_manager::set_auth_file( const std::string& file )
{
	mutex_.lock(); // -----------------------------
	
	auth_file_ = file;
	users_.clear();
	
	mutex_.unlock(); // -----------------------------
	
	load();
}

/**********************************************************************************************/
size_t cr_user_manager::get_user_count( void ) const
{
	return users_.size();
}

/**********************************************************************************************/
bool cr_user_manager::get_user_is_admin( const std::string& name ) const
{
	bool res = false;
	
	mutex_.lock(); // -----------------------------

	auto it = users_.find( name );	
	if( it != users_.end() )
		res = it->second.admin_;
	
	mutex_.unlock(); // -----------------------------
	
	return res;
}

/**********************************************************************************************/
std::vector<std::string> cr_user_manager::get_users( void ) const
{
	std::vector<std::string> res;
	
	mutex_.lock(); // -----------------------------

	res.reserve( users_.size() );
	
	for( auto it : users_ )
		res.push_back( it.first );

	mutex_.unlock(); // -----------------------------
	
	return res;
}


//////////////////////////////////////////////////////////////////////////
// methods
//////////////////////////////////////////////////////////////////////////


/**********************************************************************************************/
const char* cr_user_manager::add_user(
	const std::string&	name,
	const std::string&	pass,
	bool				admin )
{
	// Check parameters

	if( name.empty() )
		return "Empty user name";
	
	if( name.length() > 32 ) return "Name too long";
	if( pass.length() > 32 ) return "Password too long";
	
	for( char c : name )
	{
		if( !isalnum( c ) )
			return "Invalid name";
	}
	
	// Add user
	
	const char* data[] = {  name.c_str(), ":", "", ":",  pass.c_str() };
	size_t		len[]  = { name.length(),   1,   0,  1, pass.length() };

	char buf[ 16 ];
	cr_md5( buf, 5, data, len );	

	mutex_.lock(); // -----------------------------
	
	auto it = users_.find( name );
	bool exists = ( it != users_.end() );
	
	if( !exists )
	{
		user_t& user = users_[ name ];
		memmove( user.password_, buf, 16 );
		user.admin_ = admin;
	}
	
	mutex_.unlock(); // -----------------------------

	if( exists )
		return "User already exists";
	
	flush();
	return NULL;
}

/**********************************************************************************************/
const char* cr_user_manager::delete_user( const std::string& name )
{
	// Check parameters
	
	if( name.empty() )
		 return "Empty user name";
	
	// Delete user
			
	mutex_.lock(); // -----------------------------

	auto it = users_.find( name );

	bool exists = ( it != users_.end() );
	if( exists )
		users_.erase( it );
	
	mutex_.unlock(); // -----------------------------

	if( !exists )
		return "User not found";
	
	flush();
	return NULL;
}

/**********************************************************************************************/
bool cr_user_manager::get_password_hash( 
	const std::string&	name,
	char*				pass )
{
	bool res = false;
	
	mutex_.lock(); // -----------------------------
	
	auto it = users_.find( name );
	if( it != users_.end() )
	{
		memmove( pass, it->second.password_, 16 );
		res = true;
	}
	
	mutex_.unlock(); // -----------------------------
	
	return res;
}

/**********************************************************************************************/
cr_user_manager& cr_user_manager::instance( void )
{
	return g_user_manager;
}

/**********************************************************************************************/
const char* cr_user_manager::update_user_is_admin(
	const std::string&	name,
	bool				value )
{
	// Check parameters
	
	if( name.empty() )
		return "Empty user name";
	
	// Update flags
			
	mutex_.lock(); // -----------------------------
	
	auto it = users_.find( name );
	bool exists = it != users_.end();
	
	if( exists )
		it->second.admin_ = value;
	
	mutex_.unlock(); // -----------------------------
	
	if( !exists )
		return "User not found";
	
	flush();
	return NULL;	
}

/**********************************************************************************************/
const char* cr_user_manager::update_user_password(
	const std::string&	name,
	const std::string&	pass )
{
	// Check parameters
	
	if( name.empty()		) return "Empty user name";
	if( pass.length() > 32	) return "Password too long";
	
	// Update password
	
	const char* data[] = {  name.c_str(), ":", "", ":",  pass.c_str() };
	size_t		len[]  = { name.length(),   1,   0,  1, pass.length() };
	
	char buf[ 16 ];
	cr_md5( buf, 5, data, len );	
	
	mutex_.lock(); // -----------------------------
	
	auto it = users_.find( name );
	bool exists = it != users_.end();
	
	if( exists )
		memmove( it->second.password_, buf, 16 );
	
	mutex_.unlock(); // -----------------------------
	
	if( !exists )
		return "User not found";
	
	flush();
	return NULL;
}


//////////////////////////////////////////////////////////////////////////
// internal methods
//////////////////////////////////////////////////////////////////////////


/**********************************************************************************************/
void cr_user_manager_internal::flush( void )
{
	mutex_.lock(); // -----------------------------

	if( auth_file_.length() )
	{
		FILE* f = fopen( auth_file_.c_str(), "wt" );
		if( f )
		{
			for( auto it : users_ )
			{
				fwrite( it.first.c_str(), it.first.length(), 1, f );
				fputc( ' ', f );
				fputc( it.second.admin_ ? '1' : '0', f );
				fputc( ' ', f );

				const unsigned char* bt = (const unsigned char*) it.second.password_;
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
	
	mutex_.unlock(); // -----------------------------
}

/**********************************************************************************************/
void cr_user_manager_internal::load( void )
{
	bool need_flush = false;
	
	mutex_.lock(); // -----------------------------
	
	if( auth_file_.length() )
	{
		FILE* f = fopen( auth_file_.c_str(), "rt" );
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

				user_t& user = users_[ name ];
				user.admin_ = ( *flags == '1' );

				char* dpass = user.password_;
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
		if( users_.empty() )
		{
			user_t& user = users_[  "root" ];
			user.admin_ = true;
			
			const char* data[] = { "root", ":", "", ":", "" };
			size_t len[] = { 4, 1, 0, 1, 0 };	
			cr_md5( user.password_, 5, data, len );		

			need_flush = true;
		}
	}

	mutex_.unlock(); // -----------------------------
	
	if( need_flush )
		flush();
}
