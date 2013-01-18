/**********************************************************************************************/
/* crest.cpp		  		                                                   				  */
/*                                                                       					  */
/* Igor Nikitin, 2013																		  */
/* MIT license			                                                  					  */
/**********************************************************************************************/

// STD
#include <map>
#include <mutex>
#include <thread>

// MONGOOSE
#include "cr_event_loop.h"

// CREST
#include "../include/crest.h"
#include "auth_basic.h"
#include "auth_digest.h"
#include "cr_utils_private.h"

/**********************************************************************************************/
using namespace std;

/**********************************************************************************************/
#ifdef _MSC_VER
#pragma warning( disable: 4996 )
#endif // _MSC_VER


//////////////////////////////////////////////////////////////////////////
// constants
//////////////////////////////////////////////////////////////////////////


/**********************************************************************************************/
#define MAX_PATH_PARAMETERS 16


//////////////////////////////////////////////////////////////////////////
// static data
//////////////////////////////////////////////////////////////////////////


/**********************************************************************************************/
static cr_http_auth		g_auth_kind;		// Current auth method
	   const char*		g_error = "";		// Error string for cr_error_string
static size_t			g_request_count;	// Statistics - count of processed requests
static cr_result_format	g_result_format;	// Returns default result format
static time_t			g_time_start;		// Time when server was start

/**********************************************************************************************/
static bool				g_log_enabled;		// TRUE if server logs all requests to file
static FILE*			g_log_file;			// Destination log file
static string			g_log_file_path;	// Path of destination log file
static mutex			g_log_mutex;		// Mutex for g_log_file
static size_t			g_log_size;			// Size of current log file


//////////////////////////////////////////////////////////////////////////
// helper classes
//////////////////////////////////////////////////////////////////////////


/**********************************************************************************************/
struct resource
{
	bool				admin_only_;
	cr_api_callback_t	handler_;
	bool				public_;
};

/**********************************************************************************************/
struct compare_keys
{
	bool operator()(
		const cr_string_array& a,
		const cr_string_array& b ) const
	{
		if( a.count < b.count )
			return true;
		else if( a.count > b.count )
			return false;

		for( size_t i = 0 ; i < a.count ; ++i )
		{
			// Skip {key} values
			if( !a.items[ i ] || !b.items[ i ] )
				continue;

			int c = strcmp( a.items[ i ], b.items[ i ] );
			if( c )
				return c < 0;
		}

		return false;
	}
};

/**********************************************************************************************/
struct sl_connection : public cr_connection
{
	sl_connection( 
		cr_connection_data&		conn,
		const cr_string_array&	params )
	{
		// Properties
		
		conn_					= &conn;
		path_params_			= params;
		
		// Flags
		
		cookies_inited_			= false;
		query_params_inited_	= false;
		
		// Cached data
		
		post_params_buffer_		= 0;
	}
	
	void log( void )
	{
		time_t t = time( 0 );
		
		char stime[ 32 ];
		tm* lt = localtime( &t );
		strftime( stime, 32, "%y-%m-%d %H:%M:%S ", lt );

		size_t mlen = strlen( conn_->method_ );
		if( mlen > 10 ) mlen = 10;

		size_t ulen = strlen( conn_->uri_ );
		if( ulen > 128 ) mlen = 128;
		
		char buf[ 256 ];
		char* str = buf;
		add_string	( str, stime, strlen( stime ) );
		add_string	( str, conn_->method_, mlen );
		add_char	( str, ' ' );
		add_string	( str, conn_->uri_, ulen );
		add_string	( str, " from ", 6 );
		add_number	( str, conn_->remote_ip_ >> 24 );
		add_char	( str, '.' );
		add_number	( str, ( conn_->remote_ip_ >> 16 ) & 0xFF );
		add_char	( str, '.' );
		add_number	( str, ( conn_->remote_ip_ >> 8  ) & 0xFF );
		add_char	( str, '.' );
		add_number	( str, conn_->remote_ip_ & 0xFF );
		add_char	( str, '\n' );
		*str = 0;
		int blen = str - buf;

		g_log_mutex.lock(); // ------------------------

		if( g_log_file_path.length() )
		{
			fputs( buf, g_log_file );
			fflush( g_log_file );

			// Move too big log file
			g_log_size += blen;
			if( g_log_size > 10000000 )
			{
				string nfile;
				string rfile = g_log_file_path.substr( 0, g_log_file_path.length() - 4 );

				int index = 0;
				while( ++index < 1000 )
				{
					string str = rfile + to_string( index ) + ".log";
					if( !cr_file_exists( str ) )
					{
						nfile = str;
						break;
					}
				}

				if( nfile.length() )
				{
					fclose( g_log_file );
					rename( g_log_file_path.c_str(), nfile.c_str() );

					g_log_file = fopen( g_log_file_path.c_str(), "at" );
					g_log_size = cr_file_size( g_log_file_path.c_str() );				
				}
			}
		}

		g_log_mutex.unlock(); // ------------------------
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
	res.items = (char**) malloc( len + 1 + MAX_PATH_PARAMETERS * sizeof * res.items );
	res.count = 0;
	
	char* curl = (char*) res.items + MAX_PATH_PARAMETERS * sizeof * res.items;
	memmove( curl, url, len + 1 );
	
	while( *curl )
	{
		char* sp = strchr( curl, '/' );
		if( sp )
		{
			res.items[ res.count++ ] = curl;
			*sp = 0;
			curl = sp + 1;
			
			if( res.count >= MAX_PATH_PARAMETERS )
				break;
		}
		else
		{
			res.items[ res.count++ ] = curl;
			break;
		}
	}
	
	return res;
}

/**********************************************************************************************/
static map<cr_string_array,resource,compare_keys>& resources( int method )
{
	static map<cr_string_array,resource,compare_keys> r[ CR_METHOD_COUNT ];
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
	cr_http_method		method,
	const char*			name,
	cr_api_callback_t	func,
	bool				for_admin_only,
	bool			 	publ )
{
	cr_register_handler( method, name, func, for_admin_only, publ );
}

/**********************************************************************************************/
void event_handler( cr_connection_data& conn ); // To avoid warning from GCC
void event_handler( cr_connection_data& conn )
{
	++g_request_count;

	const char*		 method_name = conn.method_;
	cr_http_method	 method;

	// Get method

		 if( !strcmp( method_name, "DELETE" ) ) method = CR_METHOD_DELETE;
	else if( !strcmp( method_name, "GET"	) ) method = CR_METHOD_GET;
	else if( !strcmp( method_name, "POST"	) ) method = CR_METHOD_POST;
	else if( !strcmp( method_name, "PUT"	) ) method = CR_METHOD_PUT;
	else
	{
		cr_write( conn, "HTTP/1.1 501 Not Implemented\r\nContent-Length: 24\r\n\r\nNon-supported method", 72 );
		return;
	}

	// Resource

	auto& rset = resources( method );
	cr_string_array key = parse_resource_name( conn.uri_ + 1, strlen( conn.uri_ + 1 ) );

	auto it = rset.find( key );
	resource* rs = ( it != rset.end() ) ?
		&it->second :
		default_resource( method );
	
	if( rs && rs->handler_ )
	{
		sl_connection sconn( conn, key );
		
		if( !rs->public_ )
		{
			if( g_auth_kind == CR_AUTH_BASIC && !auth_basic( sconn, rs->admin_only_ ) )
				return;

			if( g_auth_kind == CR_AUTH_DIGEST && !auth_digest( sconn, rs->admin_only_ ) )
				return;
		}
		
		(*rs->handler_)( sconn );

		if( g_log_file && g_log_enabled )
			sconn.log();
	}
	else
	{
		for( size_t i = 0 ; i < CR_METHOD_COUNT ; ++i )
		{
			if( i != method )
			{
				auto& crset = resources( i );
				if( crset.find( key ) != crset.end() )
				{
					cr_write( conn, "HTTP/1.1 405 Method Not Allowed\r\nContent-Length: 0\r\n\r\n", 54 );
					return;
				}
			}
		}

		cr_write( conn, "HTTP/1.1 404 Not Found\r\nContent-Length: 0\r\n\r\n", 45 );
	}

	free( key.items );
}


//////////////////////////////////////////////////////////////////////////
// functions
//////////////////////////////////////////////////////////////////////////


/**********************************************************************************************/
const char* cr_error_string( void )
{
	return g_error;
}

/**********************************************************************************************/
cr_http_auth cr_get_auth_kind( void )
{
	return g_auth_kind;
}

/**********************************************************************************************/
void cr_set_auth_kind( cr_http_auth auth )
{
	if( the_cr_user_manager.get_auth_file().length() )
		g_auth_kind = auth;
}

/**********************************************************************************************/
cr_result_format cr_get_default_result_format( void )
{
	return g_result_format;
}

/**********************************************************************************************/
void cr_set_default_result_format( cr_result_format format )
{
	g_result_format = format;
}

/**********************************************************************************************/
bool cr_get_log_enabled( void )
{
	return g_log_enabled;
}

/**********************************************************************************************/
void cr_set_log_enabled( bool value )
{
	g_log_enabled = value && g_log_file;
}

/**********************************************************************************************/
void cr_register_handler(
	cr_http_method		method,
	const char*			name,
	cr_api_callback_t	func,
	bool				admin_only,
	bool			 	publ )
{
	resource* rs = ( *name == '*' ) ?
		default_resource( method ) :
		&resources( method )[ parse_resource_name( name, strlen( name ) ) ];
	
	rs->admin_only_	= admin_only;
	rs->handler_	= func;
	rs->public_		= publ;	
}

/**********************************************************************************************/
size_t cr_request_count( void )
{
	return g_request_count;
}

/**********************************************************************************************/
bool cr_start( const cr_options& opts )
{
	g_error = "";
	
	if( g_time_start )
	{
		g_error = "Server already running";
		return false;
	}

	if( !opts.thread_count || opts.thread_count > CR_MAX_THREADS )
	{
		g_error = "Invalid thread count";
		return false;
	}
	
	// Init authentification
	g_auth_kind = opts.auth_file.length() ? opts.auth_kind : CR_AUTH_NONE;
	the_cr_user_manager.set_auth_file( opts.auth_file );
	
	// Init logging
	g_log_enabled	= opts.log_enabled && opts.log_file.length();
	g_log_file_path = opts.log_file;
	
	if( g_log_file_path.length() )
	{
		size_t len = g_log_file_path.length();
		if( len < 4 || strcmp( g_log_file_path.c_str() + len - 4, ".log" ) )
			g_log_file_path += ".log";
		
		g_log_size = cr_file_size( g_log_file_path );
		g_log_file = fopen( g_log_file_path.c_str(), "at" );
	}

	// Other
	g_result_format = opts.result_format;
	g_time_start = time( 0 );
	
	// Start server
	cr_event_loop( parse_ports( opts.ports ), opts.pem_file, opts.thread_count );

	g_time_start = 0;
	
	// Clean
	if( g_log_file )
	{
		fclose( g_log_file );
		g_log_file = 0;
	}
	
	return !g_error;
}
