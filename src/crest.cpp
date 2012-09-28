/**********************************************************************************************/
/* crest.cpp		  		                                                   				  */
/*                                                                       					  */
/* Igor Nikitin, 2012																		  */
/* MIT license			                                                  					  */
/**********************************************************************************************/

// STD
#include <unistd.h>

// MONGOOSE
#include "../third/mongoose/mongoose.h"

// CREST
#include "../include/crest.h"

/**********************************************************************************************/
CREST_NAMESPACE_START


//////////////////////////////////////////////////////////////////////////
// static data
//////////////////////////////////////////////////////////////////////////


/**********************************************************************************************/
static list<connection*>	g_conns;
static mg_mutex				g_conns_mutex		= mg_mutex_create();
static string				g_error;
static FILE*				g_log_file			= 0;
static string				g_log_file_path;
static mg_mutex				g_log_mutex			= mg_mutex_create();
static size_t				g_log_size			= 0;
static size_t				g_request_count		= 0;
static bool					g_shutdown			= false;
static time_t				g_time_start		= 0;


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
struct sl_connection : public connection
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
		strftime( stime, 32, "%y-%m-%d %H:%M:%S", lt );

		char host[ 64 ];
		snprintf( host, 64, "%d.%d.%d.%d:%d",
			int( request->remote_ip >> 24 ),
			int( ( request->remote_ip >> 16 ) & 0xFF ),
			int( ( request->remote_ip >> 8  ) & 0xFF ),
			int( request->remote_ip & 0xFF ),
			int( request->remote_port ) );

		string log = string( stime ) + " " + request->request_method;

		int count = 6 - strlen( request->request_method );
		for( int i = 0 ; i < count ; ++i )
			log.push_back( ' ' );

		log.push_back( ' ' );
		log += request->uri;

		if( log.length() < 63 )
			log.append( 63 - log.length(), ' ' );

		log += " from ";
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
					
					if( !file_exists( nfile ) )
						break;
				}

				fclose( g_log_file );
				rename( g_log_file_path.c_str(), nfile.c_str() );

				g_log_file = fopen( g_log_file_path.c_str(), "at" );
				g_log_size = file_size( g_log_file_path );				
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
	  
	void set_auth_file( const string& path )
	{
		auth_manager::instance().file_ = path;
		auth_manager::instance().load();
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
static map<resource_key,resource_handler>& resources( http_method method )
{
	static map<resource_key,resource_handler> r[ 4 ];
	return r[ method ];
}

/**********************************************************************************************/
crest_register_api::crest_register_api(
	http_method				method,
	bool					admin,
	bool					ro,
	const char*				name,
	crest_api_callback_t	func )
{
	resource_key key;
	key.keys_ = parse_resource_name( name );
	
	size_t count = key.keys_.size();
	for( size_t i = 0 ; i < count ; ++i )
	{
		if( key.keys_[ i ][ 0 ] == '{' )
			key.keys_[ i ].clear();
	}
	
	resource_handler& hnd  = resources( method )[ key ];
	hnd.admin_ = admin;
	hnd.func_  = func;
	hnd.ro_    = ro;
}

/**********************************************************************************************/
static void event_handler(
	mg_event		event,
	mg_connection*	conn )
{
	if( event == MG_EVENT_LOG )
	{
		mg_request_info* request = mg_get_request_info( conn );
		g_error = (const char*) request->ev_data;
		return;
	}
	
	if( event == MG_NEW_REQUEST )
	{
		++g_request_count;
		
		mg_request_info* request = mg_get_request_info( conn );
		const char* method_name = request->request_method;
		
		// Get method
		
		http_method method;
			 
		if( !strcmp( method_name, "DELETE" ) ) method = METHOD_DELETE;
		else if( !strcmp( method_name, "GET" ) ) method = METHOD_GET;
		else if( !strcmp( method_name, "POST" ) ) method = METHOD_POST;
		else if( !strcmp( method_name, "PUT" ) ) method = METHOD_PUT;
		else
		{
			string str = responce( HTTP_BAD_REQUEST, "" );
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
			
			if( g_log_file )
				sconn.log();
		}
		else
		{
			string data = responce( HTTP_NOT_FOUND, "" );
			mg_write( conn, data.c_str(), data.length() );
		}
	}
}


//////////////////////////////////////////////////////////////////////////
// functions
//////////////////////////////////////////////////////////////////////////


/**********************************************************************************************/
string crest_error_string( void )
{
	return g_error;
}

/**********************************************************************************************/
size_t crest_request_count( void )
{
	return g_request_count;
}

/**********************************************************************************************/
bool crest_start(
	size_t			port,
	size_t			port_ssl,
	const string&	auth_file,
	const string&	log_file,
	const string&	pem_file )
{
	if( g_time_start )
	{
		g_error = "Server already running";
		return false;
	}
	
	crest sl;
	sl.set_auth_file( auth_file );
	
	g_log_file_path = log_file;
	if( log_file.length() )
	{
		if( right( log_file, 4 ) != ".log" )
			g_log_file_path += ".log";
		
		g_log_file = fopen( g_log_file_path.c_str(), "at" );
		g_log_size = file_size( g_log_file_path );
	}
	
	char ports[ 64 ];
	port_ssl ?
		snprintf( ports, 64, "%zu,%zus", port, port_ssl ) :
		snprintf( ports, 64, "%zu", port );
	
	// Start server
	const char* options[] =
	{
		"listening_ports"			, ports,
		"num_threads"				, "20",
		"ssl_certificate"			, pem_file.c_str(),
		NULL
	};
	
	if( pem_file.empty() )
		options[ 4 ] = 0;
	
	mg_context* ctx = mg_start( &event_handler, options );
	if( ctx )
	{	
		g_time_start = time( NULL );
		
		while( !g_shutdown )
			sleep( 1 );
		
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
CREST_NAMESPACE_END
