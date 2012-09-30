/**********************************************************************************************/
/* crest.cpp		  		                                                   				  */
/*                                                                       					  */
/* Igor Nikitin, 2012																		  */
/* MIT license			                                                  					  */
/**********************************************************************************************/

// STD
#include <list>
#include <string>

// MONGOOSE
#include "../third/mongoose/mongoose.h"

// CREST
#include "../include/crest.h"
#include "utils.h"

/**********************************************************************************************/
using std::list;
using std::string;


//////////////////////////////////////////////////////////////////////////
// static data
//////////////////////////////////////////////////////////////////////////


/**********************************************************************************************/
static bool						g_auth_enabled		= false;
static list<crest_connection*>	g_conns;
static mg_mutex					g_conns_mutex		= mg_mutex_create();
static string					g_error;
static bool						g_log_enabled		= false;
static FILE*					g_log_file			= 0;
static char*					g_log_file_path		= 0;
static mg_mutex					g_log_mutex			= mg_mutex_create();
static size_t					g_log_size			= 0;
static size_t					g_request_count		= 0;
static bool						g_shutdown			= false;
static time_t					g_time_start		= 0;


//////////////////////////////////////////////////////////////////////////
// helper classes
//////////////////////////////////////////////////////////////////////////


/**********************************************************************************************/
struct resource
{
	bool					admin_;
	crest_api_callback_t	handler_;
	crest_string_array		keys_;
	bool					read_only_;
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
	}
	
	void log( void )
	{
		mg_request_info* request = mg_get_request_info( conn_ );
		time_t t = get_birth_time();
		
		char stime[ 32 ];
		tm* lt = localtime( &t );
		strftime( stime, 32, "%y-%m-%d %H:%M:%S ", lt );

		char host[ 64 ];
		snprintf( host, 64, " from %d.%d.%d.%d:%d",
			int( request->remote_ip >> 24 ),
			int( ( request->remote_ip >> 16 ) & 0xFF ),
			int( ( request->remote_ip >> 8  ) & 0xFF ),
			int( request->remote_ip & 0xFF ),
			int( request->remote_port ) );

		string log;
		log.reserve( 128 );
		log  = stime;
		log += request->request_method;

		int count = 6 - strlen( request->request_method );
		for( int i = 0 ; i < count ; ++i )
			log.push_back( ' ' );

		log.push_back( ' ' );
		log += request->uri;

		if( log.length() < 63 )
			log.append( 63 - log.length(), ' ' );

		log += host;

/*		if( request->remote_user )
		{
			log += " (";
			log += request->remote_user;
			log.push_back( ')' );
		}*/

		log.push_back( '\n' );

		mg_mutex_lock( g_log_mutex ); // ------------------------

		if( g_log_file_path && *g_log_file_path )
		{
			fputs( log.c_str(), g_log_file );
			fflush( g_log_file );

			// Move too big log file
			g_log_size += log.length();
			if( g_log_size > 100000 )
			{
				size_t glen = strlen( g_log_file_path );
				
				char* nfile = 0;
				char* rfile = crest_strdup( g_log_file_path, glen );
				rfile[ glen - 4 ] = 0;

				size_t index = 0;
				while( ++index < 1000 )
				{
					char buf[ glen + 32 ];
					snprintf( buf, glen + 32, "%s%zu.log", rfile, index );				
					
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
crest_handler_register::crest_handler_register(
	crest_http_method	 method,
	const char*			 name,
	crest_api_callback_t func,
	bool				 for_admin_only,
	bool			 	 read_only )
{
	resource* rs;
	
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
	
	rs->admin_		= for_admin_only;
	rs->handler_	= func;
	rs->read_only_	= read_only;
}

/**********************************************************************************************/
static void event_handler( mg_connection* conn )
{
	++g_request_count;

	mg_request_info* request = mg_get_request_info( conn );
	const char* method_name = request->request_method;

	// Get method

	crest_http_method method;

	if( !strcmp( method_name, "DELETE" ) ) method = CREST_METHOD_DELETE;
	else if( !strcmp( method_name, "GET" ) ) method = CREST_METHOD_GET;
	else if( !strcmp( method_name, "POST" ) ) method = CREST_METHOD_POST;
	else if( !strcmp( method_name, "PUT" ) ) method = CREST_METHOD_PUT;
	else
	{
		char* buf;
		size_t len;
		create_responce( buf, len, CREST_HTTP_BAD_REQUEST, "Non-supported method", 20 );
		mg_write( conn, buf, len );
		
		free( buf );
		return;
	}

	// Resource

	resource_array& arr = resources( method );
	
	resource key;
	key.keys_ = parse_resource_name( request->uri + 1, strlen( request->uri + 1 ) );
	resource* it = (resource*) bsearch( &key, arr.items_, arr.count_, sizeof( resource ), compare_resources );

	if( it )
	{
		sl_connection sconn( conn, key.keys_ );

		mg_mutex_lock( g_conns_mutex );
		g_conns.push_back( &sconn );
		mg_mutex_unlock( g_conns_mutex );

		(*it->handler_)( sconn );

		mg_mutex_lock( g_conns_mutex );
		g_conns.remove( &sconn );
		mg_mutex_unlock( g_conns_mutex );

		if( g_log_file && g_log_enabled )
			sconn.log();
	}
	else
	{
		char* buf;
		size_t len;		
		create_responce( buf, len, CREST_HTTP_NOT_FOUND, "", 0 );
		mg_write( conn, buf, len );
		
		free( buf );
	}
}


//////////////////////////////////////////////////////////////////////////
// functions
//////////////////////////////////////////////////////////////////////////


/**********************************************************************************************/
const char* crest_error_string( void )
{
	return g_error.c_str();
}

/**********************************************************************************************/
bool crest_get_auth_enabled( void )
{
	return g_auth_enabled;
}

/**********************************************************************************************/
void crest_set_auth_enabled( bool value )
{
	g_auth_enabled = value;
}

/**********************************************************************************************/
bool crest_get_log_enabled( void )
{
	return g_log_enabled;
}

/**********************************************************************************************/
void crest_set_log_enabled( bool value )
{
	g_log_enabled = value;
}

/**********************************************************************************************/
size_t crest_request_count( void )
{
	return g_request_count;
}

/**********************************************************************************************/
bool crest_start(
	const char*	ports,
	const char*	auth_file,
	const char*	log_file,
	const char*	pem_file,
	bool		auth_enabled,
	bool		log_enabled )
{
	if( g_time_start )
	{
		g_error = "Server already running";
		return false;
	}
	
	// Prepare resources
	for( size_t i = 0 ; i < CREST_METHOD_COUNT ; ++i )
		sort_resource_array( resources( i ) );
	
	// Set global flags
	g_auth_enabled = auth_enabled;
	g_log_enabled = log_enabled;
	
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
	mg_context* ctx = mg_start( &event_handler, ports, pem_file );
	if( ctx )
	{	
		g_time_start = time( NULL );
		
		while( !g_shutdown )
			mg_sleep( 1000 );
		
		g_time_start = 0;
		mg_stop( ctx );
		
		the_crest_auth_manager.clean();
		
		free( g_log_file_path );
		g_log_file_path = 0;
		
		if( g_log_file )
		{
			fclose( g_log_file );
			g_log_file = 0;
		}
	}
	else
	{
		g_error = mg_error_string();
		return false;
	}
	
	return true;
}

/**********************************************************************************************/
void crest_stop( void )
{
	g_shutdown = true;
}

/**********************************************************************************************/
const char* crest_version( void )
{
	return "0.01 alpha";
}
