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
#include <string.h>

// MONGOOSE
#include "../third/mongoose/mongoose.h"

// CREST
#include "../include/crest.h"
#include "utils.h"

/**********************************************************************************************/
#ifdef _MSC_VER
#define snprintf _snprintf
#pragma warning( disable: 4996 )
#endif // _MSC_VER


//////////////////////////////////////////////////////////////////////////
// static data
//////////////////////////////////////////////////////////////////////////


/**********************************************************************************************/
static crest_http_auth		g_auth_kind;
static crest_connection*	g_conns[ 20 ];
static mg_mutex				g_conns_mutex;
static const char*			g_error;
static bool					g_log_enabled;
static FILE*				g_log_file;
static char*				g_log_file_path;
static mg_mutex				g_log_mutex;
static size_t				g_log_size;
static size_t				g_request_count;
static bool					g_shutdown;
static time_t				g_time_start;


//////////////////////////////////////////////////////////////////////////
// fix build without libstdc++
//////////////////////////////////////////////////////////////////////////


/**********************************************************************************************/
extern "C" int __cxa_guard_acquire( void ) { return 0; }
extern "C" int __cxa_guard_release( int  ) { return 0; }


//////////////////////////////////////////////////////////////////////////
// helper classes
//////////////////////////////////////////////////////////////////////////


/**********************************************************************************************/
struct resource
{
	bool					admin_;
	crest_api_callback_t	handler_;
	crest_string_array		keys_;
	bool					public_;
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
	
	const crest_string_array& ka = ra->keys_;
	const crest_string_array& kb = rb->keys_;
	
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
struct sl_connection : public crest_connection
{
	sl_connection( 
		mg_connection*		conn,
		crest_string_array	params )
	{
		// Properties
		
		conn_ = conn;
		path_params_ = params;
		query_params_count_ = (size_t) -1;
	}
	
	void log( void )
	{
		mg_request_info* request = mg_get_request_info( conn_ );
		time_t t = get_birth_time();
		
		char stime[ 32 ];
		tm* lt = localtime( &t );
		strftime( stime, 32, "%y-%m-%d %H:%M:%S", lt );

		char buf[ 128 ];
		int blen = snprintf( 
			buf, 127, "%s %-6s %-40s from %d.%d.%d.%d:%d\n",
			stime, request->method_, request->uri_,
			int( request->remote_ip_ >> 24 ),
			int( ( request->remote_ip_ >> 16 ) & 0xFF ),
			int( ( request->remote_ip_ >> 8  ) & 0xFF ),
			int( request->remote_ip_ & 0xFF ),
			int( request->remote_port_ ) );

		buf[ 127 ] = 0;
		mg_mutex_lock( g_log_mutex ); // ------------------------

		if( g_log_file_path && *g_log_file_path )
		{
			fputs( buf, g_log_file );
			fflush( g_log_file );

			// Move too big log file
			g_log_size += blen;
			if( g_log_size > 100000 )
			{
				size_t glen = strlen( g_log_file_path );
				
				char* nfile = 0;
				char* rfile = crest_strdup( g_log_file_path, glen );
				rfile[ glen - 4 ] = 0;

				int index = 0;
				while( ++index < 1000 )
				{
					char* buf = (char*) alloca( glen + 32 );
					
					char* str = buf;
					str = add_string ( str, rfile, glen - 4 );
					str = to_string  ( index, str );
					str = add_string ( str, ".log", 5 );

					if( !file_exists( buf ) )
					{
						nfile = crest_strdup( buf );
						break;
					}
				}

				if( nfile )
				{
					fclose( g_log_file );
					rename( g_log_file_path, nfile );

					g_log_file = fopen( g_log_file_path, "at" );
					g_log_size = file_size( g_log_file_path );				
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
static crest_string_array parse_resource_name( 
	const char* url,
	size_t		len )
{
	crest_string_array res;
	res.items_ = (char**) malloc( len + 1 + 16 * sizeof * res.items_ );
	res.count_ = 0;
	
	char* curl = (char*) res.items_ + 16 * sizeof * res.items_;
	memcpy( curl, url, len + 1 );
	
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
	static resource_array r[ CREST_METHOD_COUNT ];
	return r[ method ];
}

/**********************************************************************************************/
static resource* default_resource( int method )
{
	static resource r[ CREST_METHOD_COUNT ];
	return &r[ method ];
}

/**********************************************************************************************/
crest_handler_register::crest_handler_register(
	crest_http_method	 method,
	const char*			 name,
	crest_api_callback_t func,
	bool				 for_admin_only,
	bool			 	 publ )
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
void event_handler( mg_connection* conn );
void event_handler( mg_connection* conn )
{
	++g_request_count;

	mg_request_info* request = mg_get_request_info( conn );
	const char* method_name = request->method_;

	// Get method

	crest_http_method method;

	if( !strcmp( method_name, "DELETE" ) ) method = CREST_METHOD_DELETE;
	else if( !strcmp( method_name, "GET" ) ) method = CREST_METHOD_GET;
	else if( !strcmp( method_name, "POST" ) ) method = CREST_METHOD_POST;
	else if( !strcmp( method_name, "PUT" ) ) method = CREST_METHOD_PUT;
	else
	{
		mg_write( conn, "HTTP/1.1 400 Bad Request\r\nContent-Length: 20\r\n\r\nNon-supported method", 68 );
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
		if( !it->public_ && g_auth_kind == CREST_AUTH_BASIC )
		{
			const char* auth = mg_get_header( conn, "Authorization" );
			size_t auth_len = auth ? strlen( auth ) : 0;
			char* buf = (char*) alloca( auth_len + 1 );
			char *user, *pass;
			
			if( !auth ||
				!parse_basic_auth( auth, auth_len, buf, user, pass ) ||
				!the_crest_auth_manager.auth( user, pass, it->admin_ ) )
			{
				mg_write( conn, "HTTP/1.1 401 Unauthorized\r\nContent-Length: 0\r\nWWW-Authenticate: Basic\r\n\r\n", 73 );
				return;
			}
		}
		
		sl_connection sconn( conn, key.keys_ );

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
		for( size_t i = 0 ; i < CREST_METHOD_COUNT ; ++i )
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
const char* crest_error_string( void )
{
	return g_error ? g_error : "";
}

/**********************************************************************************************/
crest_http_auth crest_get_auth_kind( void )
{
	return g_auth_kind;
}

/**********************************************************************************************/
void crest_set_auth_kind( crest_http_auth auth )
{
	if( the_crest_auth_manager.get_auth_file() )
		g_auth_kind = auth;
}

/**********************************************************************************************/
bool crest_get_log_enabled( void )
{
	return g_log_enabled;
}

/**********************************************************************************************/
void crest_set_log_enabled( bool value )
{
	g_log_enabled = value && g_log_file;
}

/**********************************************************************************************/
size_t crest_request_count( void )
{
	return g_request_count;
}

/**********************************************************************************************/
bool crest_start(
	const char*		ports,
	const char*		auth_file,
	const char*		log_file,
	const char*		pem_file,
	crest_http_auth	auth_kind,
	bool			log_enabled )
{
	if( g_time_start )
	{
		g_error = crest_strdup( "Server already running" );
		return false;
	}
	
	// Allocate mutexes
	g_conns_mutex = mg_mutex_create();
	g_log_mutex   = mg_mutex_create();
	
	// Prepare resources
	for( size_t i = 0 ; i < CREST_METHOD_COUNT ; ++i )
		sort_resource_array( resources( i ) );
	
	// Set global flags
	g_auth_kind = auth_file ? auth_kind : CREST_AUTH_NONE;
	g_log_enabled = log_enabled && log_file;
	
	// Prepare and set options
	the_crest_auth_manager.set_auth_file( auth_file );
	
	if( log_file && *log_file )
		g_log_file_path = crest_strdup( log_file );
	
	if( g_log_file_path )
	{
		size_t len = strlen( g_log_file_path );
		if( len < 4 || strcmp( g_log_file_path + len - 4, ".log" ) )
		{
			g_log_file_path = (char*) realloc( g_log_file_path, len + 5 );
			memcpy( g_log_file_path + len, ".log", 5 );
		}
		
		g_log_file = fopen( g_log_file_path, "at" );
		g_log_size = file_size( g_log_file_path );
	}
	
	// Start server
	mg_context* ctx = mg_start( ports, pem_file );
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
	mg_mutex_destroy( g_log_mutex );

	the_crest_auth_manager.clean();

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
void crest_stop( void )
{
	g_shutdown = true;
}

/**********************************************************************************************/
const char* crest_version( void )
{
	return "0.01";
}
