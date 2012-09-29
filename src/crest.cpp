/**********************************************************************************************/
/* crest.cpp		  		                                                   				  */
/*                                                                       					  */
/* Igor Nikitin, 2012																		  */
/* MIT license			                                                  					  */
/**********************************************************************************************/

// STD
#include <list>

// MONGOOSE
#include "../third/mongoose/mongoose.h"

// CREST
#include "../include/crest.h"
#include "utils.h"

/**********************************************************************************************/
using std::list;


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
static string					g_log_file_path;
static mg_mutex					g_log_mutex			= mg_mutex_create();
static size_t					g_log_size			= 0;
static size_t					g_request_count		= 0;
static bool						g_shutdown			= false;
static time_t					g_time_start		= 0;


//////////////////////////////////////////////////////////////////////////
// helper classes
//////////////////////////////////////////////////////////////////////////


/**********************************************************************************************/
struct resource_handler
{
	bool					admin_;
	crest_api_callback_t	func_;
	bool					ro_;
};

/**********************************************************************************************/
struct resource_key
{
	resource_key( void ) {}
	resource_key( const resource_key& key )	{ keys_ = key.keys_; }
	
	bool operator<( const resource_key& v ) const
	{
		size_t count = keys_.size();
		if( v.keys_.size() > count )
			return true;
		else if( v.keys_.size() < count )
			return false;
		
		for( size_t i = 0 ; i < count ; ++i )
		{
			if( keys_[ i ].empty() || v.keys_[ i ].empty() || keys_[ i ] == v.keys_[ i ] )
				continue;
			
			return keys_[ i ] < v.keys_[ i ];
		}
		
		return false;
	}

	vector<string> keys_;
};

/**********************************************************************************************/
struct sl_connection : public crest_connection
{
	sl_connection( 
		mg_connection*	conn,
		vector<string>&	params )
	{
		// Properties
		
		conn_ = conn;
		path_params_.swap( params );
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

		if( g_log_file_path.length() )
		{
			fputs( log.c_str(), g_log_file );
			fflush( g_log_file );

			// Move too big log file
			g_log_size += log.length();
			if( g_log_size > 100000 )
			{
				string nfile;
				string rfile = g_log_file_path;
				rfile.resize( rfile.length() - 4 );

				size_t index = 0;
				while( ++index < 1000 )
				{
					char buf[ 32 ];
					snprintf( buf, 32, "%zu.log", index );				
					nfile = rfile + buf;
					
					if( !file_exists( nfile.c_str() ) )
						break;
				}

				fclose( g_log_file );
				rename( g_log_file_path.c_str(), nfile.c_str() );

				g_log_file = fopen( g_log_file_path.c_str(), "at" );
				g_log_size = file_size( g_log_file_path.c_str() );				
			}
		}

		mg_mutex_unlock( g_log_mutex ); // ------------------------
	}
};


/**********************************************************************************************/
class crest
{
	public://////////////////////////////////////////////////////////////////////////

// This class API:		

	// ---------------------
	// Methods

	static void set_auth_file( const char* path )
	{
		crest_auth_manager::instance().file_ = path ? path : "";
		crest_auth_manager::instance().load();
	}
};


//////////////////////////////////////////////////////////////////////////
// helper functions
//////////////////////////////////////////////////////////////////////////


/**********************************************************************************************/
static vector<string> parse_resource_name( const char* url )
{
	vector<string> res;
	
	while( *url )
	{
		const char* sp = strchr( url, '/' );
		if( sp )
		{
			res.resize( res.size() + 1 );
			res.back().assign( url, sp - url );
			
			url = sp + 1;
		}
		else
		{
			res.push_back( url );
			break;
		}
	}
	
	return res;
}

/**********************************************************************************************/
static map<resource_key,resource_handler>& resources( crest_http_method method )
{
	static map<resource_key,resource_handler> r[ CREST_METHOD_COUNT ];
	return r[ method ];
}

/**********************************************************************************************/
crest_handler_register::crest_handler_register(
	crest_http_method	 method,
	const char*			 resource,
	crest_api_callback_t func,
	bool				 for_admin_only,
	bool			 	 read_only )
{
	resource_key key;
	key.keys_ = parse_resource_name( resource );
	
	size_t count = key.keys_.size();
	for( size_t i = 0 ; i < count ; ++i )
	{
		if( key.keys_[ i ][ 0 ] == '{' )
			key.keys_[ i ].clear();
	}
	
	resource_handler& hnd  = resources( method )[ key ];
	hnd.admin_ = for_admin_only;
	hnd.func_  = func;
	hnd.ro_    = read_only;
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
		string str = responce( CREST_HTTP_BAD_REQUEST, "Non-supported method" );
		mg_write( conn, str.c_str(), str.length() );
		return;
	}

	// Resource

	resource_key key;
	key.keys_ = parse_resource_name( request->uri + 1 );

	map<resource_key,resource_handler>::const_iterator it = resources( method ).find( key );
	if( it != resources( method ).end() )
	{
		sl_connection sconn( conn, key.keys_ );

		mg_mutex_lock( g_conns_mutex );
		g_conns.push_back( &sconn );
		mg_mutex_unlock( g_conns_mutex );

		(*it->second.func_)( sconn );

		mg_mutex_lock( g_conns_mutex );
		g_conns.remove( &sconn );
		mg_mutex_unlock( g_conns_mutex );

		if( g_log_file && g_log_enabled )
			sconn.log();
	}
	else
	{
		string data = responce( CREST_HTTP_NOT_FOUND, "" );
		mg_write( conn, data.c_str(), data.length() );
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
	
	// Set global flags
	g_auth_enabled = auth_enabled;
	g_log_enabled = log_enabled;
	
	// Prepare and set options
	crest::crest::set_auth_file( auth_file );
	
	g_log_file_path = log_file ? log_file : "";
	if( g_log_file_path.length() )
	{
		size_t len = g_log_file_path.length();
		if( len < 4 || strcmp( g_log_file_path.c_str() + len - 4, ".log" ) )
			g_log_file_path += ".log";
		
		g_log_file = fopen( g_log_file_path.c_str(), "at" );
		g_log_size = file_size( g_log_file_path.c_str() );
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
