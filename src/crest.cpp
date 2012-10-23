/**********************************************************************************************/
/* crest.cpp		  		                                                   				  */
/*                                                                       					  */
/* Igor Nikitin, 2012																		  */
/* MIT license			                                                  					  */
/**********************************************************************************************/

// STD
#include <malloc.h>
#include <stdio.h>
#include <stdlib.h>

// MONGOOSE
#include "../third/mongoose/mongoose.h"

// CREST
#include "../include/crest.h"
#include "auth_basic.h"
#include "auth_digest.h"
#include "cr_utils_private.h"

/**********************************************************************************************/
#ifdef _MSC_VER
#pragma warning( disable: 4996 )
#endif // _MSC_VER


//////////////////////////////////////////////////////////////////////////
// static data
//////////////////////////////////////////////////////////////////////////


/**********************************************************************************************/
static cr_http_auth		g_auth_kind;		// Current auth method
static cr_connection*	g_conns[ 20 ];		// Set of current connections
static mg_mutex			g_conns_mutex;		// Mutex for g_conns
static const char*		g_error;			// Error string for cr_error_string
static size_t			g_request_count;	// Statistics - count of processed requests
static bool				g_shutdown;			// TRUE if we wait to process last active connections to stop server
static time_t			g_time_start;		// Time when server was start

/**********************************************************************************************/
bool g_deflate;								// TRUE if server will use deflate if client support it

/**********************************************************************************************/
static bool				g_log_enabled;		// TRUE if server logs all requests to file
static FILE*			g_log_file;			// Destination log file
static char*			g_log_file_path;	// Path of destination log file
static mg_mutex			g_log_mutex;		// Mutex for g_log_file
static size_t			g_log_size;			// Size of current log file


//////////////////////////////////////////////////////////////////////////
// to work without libstdc++, we don't need for 'fair' guard
//////////////////////////////////////////////////////////////////////////


/**********************************************************************************************/
extern "C" int __cxa_guard_acquire( int* guard ) { return !*guard; }
extern "C" int __cxa_guard_release( int* guard ) { return *guard = 1; }


//////////////////////////////////////////////////////////////////////////
// helper classes
//////////////////////////////////////////////////////////////////////////


/**********************************************************************************************/
struct resource
{
	bool				admin_;
	cr_api_callback_t	handler_;
	cr_string_array		keys_;
	bool				public_;
};

/**********************************************************************************************/
struct resource_array
{
	resource_array( void )
	{
		count_ = 0;
		items_ = 0;
	}
	
	size_t		count_;
	resource*	items_;
};

/**********************************************************************************************/
static int compare_resources( const void* a, const void* b )
{
	const resource* ra = (const resource*) a;
	const resource* rb = (const resource*) b;
	
	const cr_string_array& ka = ra->keys_;
	const cr_string_array& kb = rb->keys_;
	
	if( ka.count_ < kb.count_ )
		return -1;
	else if( ka.count_ > kb.count_ )
		return 1;
	
	for( size_t i = 0 ; i < ka.count_ ; ++i )
	{
		// Skip {key} values
		if( !ka.items_[ i ] || !kb.items_[ i ] )
			continue;

		int c = strcmp( ka.items_[ i ], kb.items_[ i ] );
		if( c )
			return c;
	}

	return 0;
}

/**********************************************************************************************/
inline void sort_resource_array( resource_array& arr )
{
	qsort( arr.items_, arr.count_, sizeof( resource ), compare_resources );
}

/**********************************************************************************************/
struct sl_connection : public cr_connection
{
	sl_connection( 
		mg_connection*	conn,
		cr_string_array	params )
	{
		// Properties
		
		conn_					= conn;
		cookies_inited_			= false;
		path_params_			= params;
		
		// Flags
		
		cookies_inited_			= false;
		query_params_inited_	= false;
		
		// Cached data
		
		post_params_buffer_		= 0;
	}
	
	void log( void )
	{
		mg_request_info* request = mg_get_request_info( conn_ );
		time_t t = birth_time();
		
		char stime[ 32 ];
		tm* lt = localtime( &t );
		strftime( stime, 32, "%y-%m-%d %H:%M:%S ", lt );

		size_t mlen = strlen( request->method_ );
		if( mlen > 10 ) mlen = 10;

		size_t ulen = strlen( request->uri_ );
		if( ulen > 128 ) mlen = 128;
		
		char buf[ 256 ];
		char* str = buf;
		add_string	( str, stime, strlen( stime ) );
		add_string	( str, request->method_, mlen );
		add_char	( str, ' ' );
		add_string	( str, request->uri_, ulen );
		add_string	( str, " from ", 6 );
		add_number	( str, request->remote_ip_ >> 24 );
		add_char	( str, '.' );
		add_number	( str, ( request->remote_ip_ >> 16 ) & 0xFF );
		add_char	( str, '.' );
		add_number	( str, ( request->remote_ip_ >> 8  ) & 0xFF );
		add_char	( str, '.' );
		add_number	( str, request->remote_ip_ & 0xFF );
		add_char	( str, ':' );
		add_number	( str, request->remote_port_ );
		add_char	( str, '\n' );
		*str = 0;
		int blen = str - buf;

		mg_mutex_lock( g_log_mutex ); // ------------------------

		if( g_log_file_path && *g_log_file_path )
		{
			fputs( buf, g_log_file );
			fflush( g_log_file );

			// Move too big log file
			g_log_size += blen;
			if( g_log_size > 10000000 )
			{
				size_t glen = strlen( g_log_file_path );
				
				char* nfile = 0;
				char* rfile = cr_strdup( g_log_file_path, glen );
				rfile[ glen - 4 ] = 0;

				int index = 0;
				while( ++index < 1000 )
				{
					char* buf = (char*) alloca( glen + 32 );
					
					char* str = buf;
					add_string ( str, rfile, glen - 4 );
					add_number ( str, index );
					add_string ( str, ".log", 5 );

					if( !cr_file_exists( buf ) )
					{
						nfile = cr_strdup( buf );
						break;
					}
				}

				if( nfile )
				{
					fclose( g_log_file );
					rename( g_log_file_path, nfile );

					g_log_file = fopen( g_log_file_path, "at" );
					g_log_size = cr_file_size( g_log_file_path );				
				}
				
				free( nfile );
				free( rfile );
			}
		}

		mg_mutex_unlock( g_log_mutex ); // ------------------------
	}
};


//////////////////////////////////////////////////////////////////////////
// helper functions
//////////////////////////////////////////////////////////////////////////


/**********************************************************************************************/
static cr_string_array parse_resource_name( 
	const char* url,
	size_t		len )
{
	cr_string_array res;
	res.items_ = (char**) malloc( len + 1 + 16 * sizeof * res.items_ );
	res.count_ = 0;
	
	char* curl = (char*) res.items_ + 16 * sizeof * res.items_;
	memmove( curl, url, len + 1 );
	
	while( *curl )
	{
		char* sp = strchr( curl, '/' );
		if( sp )
		{
			res.items_[ res.count_++ ] = curl;
			*sp = 0;
			curl = sp + 1;
			
			if( res.count_ >= 16 )
				break;
		}
		else
		{
			res.items_[ res.count_++ ] = curl;
			break;
		}
	}
	
	return res;
}

/**********************************************************************************************/
static resource_array& resources( int method )
{
	static resource_array r[ CR_METHOD_COUNT ];
	return r[ method ];
}

/**********************************************************************************************/
static resource* default_resource( int method )
{
	static resource r[ CR_METHOD_COUNT ];
	return &r[ method ];
}

/**********************************************************************************************/
cr_auto_handler_register::cr_auto_handler_register(
	cr_http_method	 method,
	const char*			 name,
	cr_api_callback_t func,
	bool				 for_admin_only,
	bool			 	 publ )
{
	cr_register_handler( method, name, func, for_admin_only, publ );
}

/**********************************************************************************************/
void event_handler( mg_connection* conn ); // To avoid warning from GCC
void event_handler( mg_connection* conn )
{
	++g_request_count;

	mg_request_info* request = mg_get_request_info( conn );
	const char* method_name = request->method_;

	// Get method

	cr_http_method method;

		 if( !strcmp( method_name, "DELETE" ) ) method = CR_METHOD_DELETE;
	else if( !strcmp( method_name, "GET"	) ) method = CR_METHOD_GET;
	else if( !strcmp( method_name, "POST"	) ) method = CR_METHOD_POST;
	else if( !strcmp( method_name, "PUT"	) ) method = CR_METHOD_PUT;
	else
	{
		mg_write( conn, "HTTP/1.1 501 Not Implemented\r\nContent-Length: 24\r\n\r\nNon-supported method", 72 );
		return;
	}

	// Resource

	resource_array& arr = resources( method );
	
	resource key;
	key.keys_ = parse_resource_name( request->uri_ + 1, strlen( request->uri_ + 1 ) );
	resource* it = (resource*) bsearch( &key, arr.items_, arr.count_, sizeof( resource ), compare_resources );

	if( !it )
		it = default_resource( method );
	
	if( it && it->handler_ )
	{
		sl_connection sconn( conn, key.keys_ );
		
		if( !it->public_ )
		{
			if( g_auth_kind == CR_AUTH_BASIC && !auth_basic( sconn, it->admin_ ) )
				return;

			if( g_auth_kind == CR_AUTH_DIGEST && !auth_digest( sconn, it->admin_ ) )
				return;
		}
		
		mg_mutex_lock( g_conns_mutex );
		
		for( size_t i = 0 ; i < 20 ; ++i )
		{
			if( !g_conns[ i ] )
			{
				g_conns[ i ] = &sconn;
				break;
			}
		}
		
		mg_mutex_unlock( g_conns_mutex );
		
		(*it->handler_)( sconn );

		mg_mutex_lock( g_conns_mutex );
		
		for( size_t i = 0 ; i < 20 ; ++i )
		{
			if( g_conns[ i ] == &sconn )
			{
				g_conns[ i ] = 0;
				break;
			}
		}
		
		mg_mutex_unlock( g_conns_mutex );

		if( g_log_file && g_log_enabled )
			sconn.log();
	}
	else
	{
		for( size_t i = 0 ; i < CR_METHOD_COUNT ; ++i )
		{
			if( i != method )
			{
				resource_array& carr = resources( i );
				if( bsearch( &key, carr.items_, carr.count_, sizeof( resource ), compare_resources ) )
				{
					mg_write( conn, "HTTP/1.1 405 Method Not Allowed\r\nContent-Length: 0\r\n\r\n", 54 );
					return;
				}
			}
		}

		mg_write( conn, "HTTP/1.1 404 Not Found\r\nContent-Length: 0\r\n\r\n", 45 );
	}
}


//////////////////////////////////////////////////////////////////////////
// functions
//////////////////////////////////////////////////////////////////////////


/**********************************************************************************************/
const char* cr_error_string( void )
{
	return g_error ? g_error : "";
}

/**********************************************************************************************/
cr_http_auth cr_get_auth_kind( void )
{
	return g_auth_kind;
}

/**********************************************************************************************/
void crest_set_auth_kind( cr_http_auth auth )
{
	if( the_cr_user_manager.get_auth_file() )
		g_auth_kind = auth;
}

/**********************************************************************************************/
bool cr_get_log_enabled( void )
{
	return g_log_enabled;
}

/**********************************************************************************************/
void crest_set_log_enabled( bool value )
{
	g_log_enabled = value && g_log_file;
}

/**********************************************************************************************/
void cr_register_handler(
	cr_http_method		method,
	const char*			name,
	cr_api_callback_t	func,
	bool				for_admin_only,
	bool			 	publ )
{
	resource* rs;
	
	if( *name == '*' )
	{
		rs = default_resource( method );
	}
	else
	{
		resource_array& arr = resources( method );
		if( !arr.count_ )
		{
			arr.count_ = 1;
			arr.items_ = (resource*) malloc( sizeof( resource ) );
			rs = arr.items_;
		}
		else
		{
			arr.count_++;
			arr.items_ = (resource*) realloc( arr.items_, sizeof( resource ) * arr.count_ );
			rs = arr.items_ + arr.count_ - 1;
		}

		rs->keys_ = parse_resource_name( name, strlen( name ) );

		size_t count = rs->keys_.count_;
		for( size_t i = 0 ; i < count ; ++i )
		{
			if( rs->keys_.items_[ i ][ 0 ] == '{' )
				rs->keys_.items_[ i ] = 0;
		}
	}
	
	rs->admin_		= for_admin_only;
	rs->handler_	= func;
	rs->public_		= publ;	
}

/**********************************************************************************************/
size_t cr_request_count( void )
{
	return g_request_count;
}

/**********************************************************************************************/
bool cr_start( cr_options& opts )
{
	if( g_time_start )
	{
		g_error = "Server already running";
		return false;
	}

	if( !opts.thread_count || opts.thread_count > 128 )
	{
		g_error = "Invalid thread count";
		return false;
	}
	
	// Allocate mutexes
	g_conns_mutex = mg_mutex_create();
	
	// Prepare resources
	for( size_t i = 0 ; i < CR_METHOD_COUNT ; ++i )
		sort_resource_array( resources( i ) );
	
	// Init authentification
	g_auth_kind = opts.auth_file && *opts.auth_file ? opts.auth_kind : CR_AUTH_NONE;
	the_cr_user_manager.set_auth_file( opts.auth_file );
	
	g_deflate = opts.deflate;
		
	// Init logging
	g_log_enabled = opts.log_enabled && opts.log_file && *opts.log_file;
	g_log_mutex   = mg_mutex_create();
	
	if( opts.log_file && *opts.log_file )
		g_log_file_path = cr_strdup( opts.log_file );
	
	if( g_log_file_path )
	{
		size_t len = strlen( g_log_file_path );
		if( len < 4 || strcmp( g_log_file_path + len - 4, ".log" ) )
		{
			g_log_file_path = (char*) realloc( g_log_file_path, len + 5 );
			memmove( g_log_file_path + len, ".log", 5 );
		}
		
		g_log_file = fopen( g_log_file_path, "at" );
		g_log_size = cr_file_size( g_log_file_path );
	}
	
	// Start server
	mg_context* ctx = mg_start( opts.ports, opts.pem_file, opts.thread_count );
	if( ctx )
	{	
		g_time_start = time( NULL );
		
		while( !g_shutdown )
			mg_sleep( 1000 );
		
		g_time_start = 0;
		mg_stop( ctx );
	}
	else
	{
		g_error = mg_get_error_string();
	}
	
	// Clean
	mg_mutex_destroy( g_conns_mutex );

	the_cr_user_manager.clean();

	mg_mutex_destroy( g_log_mutex );
	
	free( g_log_file_path );
	g_log_file_path = 0;

	if( g_log_file )
	{
		fclose( g_log_file );
		g_log_file = 0;
	}
	
	return ctx != NULL;
}

/**********************************************************************************************/
void cr_stop( void )
{
	g_shutdown = true;
}

/**********************************************************************************************/
const char* cr_version( void )
{
	return "0.1";
}
