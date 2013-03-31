/**********************************************************************************************/
/* cr_event_loop.cpp 		                                                   				  */
/*                                                                       					  */
/* (c) 2004-2012 Sergey Lyubka																  */
/* (c) 2013      Igor Nikitin																  */
/* MIT license   																		  	  */
/**********************************************************************************************/

// Copyright (c) 2004-2012 Sergey Lyubka
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.

/**********************************************************************************************/
#ifdef _FORTIFY_SOURCE
#undef _FORTIFY_SOURCE
#define _FORTIFY_SOURCE 0
#endif // _FORTIFY_SOURCE

// STD
#include <condition_variable>
#include <ctype.h>
#include <queue>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <thread>
#include <vector>

// LIBCREST
#include "../include/cr_utils.h"
#include "cr_event_loop.h"
#include "cr_utils_private.h"

/**********************************************************************************************/
using namespace std;

/**********************************************************************************************/
extern const char* g_error;

/**********************************************************************************************/
static bool g_stop;

#ifdef _WIN32

#    define _CRT_SECURE_NO_WARNINGS
#    define _WIN32_WINNT 0x0400

#    include <windows.h>
#    include <direct.h>
#    include <io.h>
#    include <process.h>

#    define NO_SOCKLEN_T
#    define _POSIX_

#    define ERRNO		GetLastError()
#    define CRYPTO_LIB  "libeay32.dll"
#    define O_NONBLOCK  0
#    define SHUT_WR		1
#    define SSL_LIB		"ssleay32.dll"

#    define close(x)	_close(x)
#    define dlsym(x,y)	GetProcAddress((HINSTANCE) (x), (y))
#    define RTLD_LAZY	0
#    define write(x, y, z) _write((x), (y), (unsigned) z)
#    define read(x, y, z) _read((x), (y), (unsigned) z)

// Mark required libraries
#    pragma comment(lib, "Ws2_32.lib")

#else // _WIN32

#    include <dlfcn.h>
#    include <netinet/in.h>
#	 include <netdb.h>
#    include <unistd.h>

#    ifdef __MACH__
#        define SSL_LIB "libssl.dylib"
#        define CRYPTO_LIB "libcrypto.dylib"
#    else // __MACH__
#        define SSL_LIB "libssl.so"
#        define CRYPTO_LIB "libcrypto.so"
#    endif // __MACH__

#    define closesocket(a)	close(a)
#    define ERRNO			errno
#    define INVALID_SOCKET	(-1)

typedef int SOCKET;

#endif // _WIN32

// Darwin prior to 7.0 and Win32 do not have socklen_t
#ifdef NO_SOCKLEN_T
typedef int socklen_t;
#endif // NO_SOCKLEN_T

#define _DARWIN_UNLIMITED_SELECT

#ifndef MSG_NOSIGNAL
#    define MSG_NOSIGNAL 0
#endif

#ifndef SOMAXCONN
#    define SOMAXCONN 100
#endif

/**********************************************************************************************/
typedef struct ssl_st			SSL;
typedef struct ssl_method_st	SSL_METHOD;
typedef struct ssl_ctx_st		SSL_CTX;

/**********************************************************************************************/
#define SSL_FILETYPE_PEM		1
#define CRYPTO_LOCK				1


/**********************************************************************************************/
struct ssl_func
{
	const char* name;		// SSL function name
	void  (*ptr)( void );	// Function pointer
};

#define SSL_free							(* (void (*)(SSL *)) ssl_sw[0].ptr)
#define SSL_accept							(* (int (*)(SSL *)) ssl_sw[1].ptr)
#define SSL_connect							(* (int (*)(SSL *)) ssl_sw[2].ptr)
#define SSL_read							(* (int (*)(SSL *, void *, int)) ssl_sw[3].ptr)
#define SSL_write							(* (int (*)(SSL *, const void *,int)) ssl_sw[4].ptr)
#define SSL_get_error						(* (int (*)(SSL *, int)) ssl_sw[5].ptr)
#define SSL_set_fd							(* (int (*)(SSL *, SOCKET)) ssl_sw[6].ptr)
#define SSL_new								(* (SSL * (*)(SSL_CTX *)) ssl_sw[7].ptr)
#define SSL_CTX_new							(* (SSL_CTX * (*)(SSL_METHOD *)) ssl_sw[8].ptr)
#define SSLv23_server_method				(* (SSL_METHOD * (*)(void)) ssl_sw[9].ptr)
#define SSL_library_init					(* (int (*)(void)) ssl_sw[10].ptr)
#define SSL_CTX_use_PrivateKey_file			(* (int (*)(SSL_CTX *, const char*, int)) ssl_sw[11].ptr)
#define SSL_CTX_use_certificate_file		(* (int (*)(SSL_CTX *, const char *, int)) ssl_sw[12].ptr)

#define SSL_CTX_free						(* (void (*)(SSL_CTX *)) ssl_sw[13].ptr)
#define SSL_CTX_use_certificate_chain_file	(* (int (*)(SSL_CTX *, const char *)) ssl_sw[14].ptr)
#define SSLv23_client_method				(* (SSL_METHOD * (*)(void)) ssl_sw[15].ptr)

#define CRYPTO_num_locks					(* (int (*)(void)) crypto_sw[0].ptr)
#define CRYPTO_set_locking_callback			(* (void (*)(void (*)(int, int, const char *, int))) crypto_sw[1].ptr)
#define CRYPTO_set_id_callback				(* (void (*)(unsigned long (*)(void))) crypto_sw[2].ptr)


// set_ssl_option() function updates this array.
// It loads SSL library dynamically and changes NULLs to the actual addresses
// of respective functions. The macros above (like SSL_connect()) are really
// just calling these functions indirectly via the pointer.
static struct ssl_func ssl_sw[] = {
	{ "SSL_free"							, NULL },
	{ "SSL_accept"							, NULL },
	{ "SSL_connect"							, NULL },
	{ "SSL_read"							, NULL },
	{ "SSL_write"							, NULL },
	{ "SSL_get_error"						, NULL },
	{ "SSL_set_fd"							, NULL },
	{ "SSL_new"								, NULL },
	{ "SSL_CTX_new"							, NULL },
	{ "SSLv23_server_method"				, NULL },
	{ "SSL_library_init"					, NULL },
	{ "SSL_CTX_use_PrivateKey_file"			, NULL },
	{ "SSL_CTX_use_certificate_file"		, NULL },
	{ "SSL_CTX_free"						, NULL },
	{ "SSL_CTX_use_certificate_chain_file"	, NULL },
	{ "SSLv23_client_method"				, NULL },
	{ NULL									, NULL }
};

// Similar array as ssl_sw. These functions could be located in different lib.
static struct ssl_func crypto_sw[] = {
	{ "CRYPTO_num_locks"					, NULL },
	{ "CRYPTO_set_locking_callback"			, NULL },
	{ "CRYPTO_set_id_callback"				, NULL },
	{ NULL									, NULL }
};

/**********************************************************************************************/
// Unified socket address. For IPv6 support, add IPv6 address structure
// in the union u.
//
union usa
{
	sockaddr	sa;
	sockaddr_in	sin;

#ifdef USE_IPV6
	sockaddr_in6 sin6;
#endif // USE_IPV6
};

/**********************************************************************************************/
// Describes listening socket, or socket which was accept()-ed by the master
// thread and queued for future handling by the worker thread.
//
struct cr_socket
{
	SOCKET		sock;   // Listening socket
	usa			lsa;	// Local socket address
	usa			rsa;	// Remote socket address
	bool		is_ssl; // Is socket SSL-ed
};

/**********************************************************************************************/
struct cr_in_socket
{
	SOCKET		sock;   // Listening socket
	usa			rsa;	// Remote socket address
	bool		is_ssl; // Is socket SSL-ed
};


/**********************************************************************************************/
class cr_thread_pool
{
	public://////////////////////////////////////////////////////////////////////////
	  
							cr_thread_pool( size_t count );
							~cr_thread_pool( void )
							{
								condition_.notify_all();
									
								for( auto& it : workers_ )
									it.join();
							}
								
	public://////////////////////////////////////////////////////////////////////////								
						
// This class API:

	// ---------------------
	// Methods

		void				enqueue( const cr_in_socket& skt )
							{
								queue_mutex_.lock();
								tasks_.push( skt );
								queue_mutex_.unlock();

								condition_.notify_one();
							}								

								
	public://////////////////////////////////////////////////////////////////////////

// Properties		
		
		condition_variable	condition_;
		mutex				queue_mutex_;
		queue<cr_in_socket>	tasks_;
		vector<thread>		workers_;
};

/**********************************************************************************************/
static void process_connection( cr_in_socket& socket );

/**********************************************************************************************/
static vector<cr_socket>	g_listening_sockets;
static SSL_CTX*				g_ssl;
static SSL_CTX*				g_ssl_client;
static vector<mutex*>		g_ssl_mutexes;


//////////////////////////////////////////////////////////////////////////
// helper class
//////////////////////////////////////////////////////////////////////////


/**********************************************************************************************/
class cr_worker
{
	public://////////////////////////////////////////////////////////////////////////
	  
		void operator()()
		{
			cr_in_socket skt;

			while( 1 )
			{
				{
					unique_lock<mutex> lock( pool_.queue_mutex_ );

					while( !g_stop && pool_.tasks_.empty() )
						pool_.condition_.wait( lock );

					if( g_stop )
						return;

					skt = pool_.tasks_.front();
					pool_.tasks_.pop();
				}

				process_connection( skt );
			}										
		}

	public://////////////////////////////////////////////////////////////////////////

		cr_thread_pool& pool_;
};


//////////////////////////////////////////////////////////////////////////
// construction
//////////////////////////////////////////////////////////////////////////


/**********************************************************************************************/
cr_thread_pool::cr_thread_pool( size_t count )
{
	for( size_t i = 0 ; i < count ; ++i )
		workers_.push_back( thread( cr_worker( { *this } ) ) );
}

/**********************************************************************************************/
static int set_non_blocking_mode( SOCKET sock );

/**********************************************************************************************/
static void close_socket_gracefully( int sock )
{
	struct linger linger;

	// Set linger option to avoid socket hanging out after close. This prevent
	// ephemeral port exhaust problem under high QPS.
	linger.l_onoff = 1;
	linger.l_linger = 1;
	setsockopt( sock, SOL_SOCKET, SO_LINGER, (char*) &linger, sizeof( linger ) );

	// Send FIN to the client
	shutdown(sock, SHUT_WR);
	set_non_blocking_mode( sock );

#ifdef _WIN32
  
	char buf[ 4096 ];
	int n;
	
	do
	{
		n = pull( NULL, sock, buf, sizeof( buf ) );
	}
	while( n > 0 );
	
#endif // _WIN32

  // Now we know that our FIN is ACK-ed, safe to close
  closesocket( sock );
}

/**********************************************************************************************/
static void close_all_listening_sockets( void )
{
	for( auto& sp : g_listening_sockets )
		close_socket_gracefully( sp.sock );

	g_listening_sockets.clear();
}

/**********************************************************************************************/
static bool set_ports( 
	const vector<cr_port>&	ports,
	const string&			pem )
{
	if( ports.empty() )
	{
		g_error = "Invalid port spec. Expecting list of: [IP_ADDRESS:]PORT[s|p]";
		return false;
	}
	
	bool res = true;
	
	int on = 1;
	SOCKET sock;

	for( size_t i = 0 ; i < ports.size() ; ++i )
	{
		cr_socket so;
		memset( &so, 0, sizeof( so ) );

		const cr_port& port = ports[ i++ ];
		if( port.a )
			so.lsa.sin.sin_addr.s_addr = htonl( ( port.a << 24 ) | ( port.b << 16 ) | ( port.c << 8 ) | port.d );

		so.is_ssl = port.ssl;

#ifdef USE_IPV6

		so.lsa.sin6.sin6_family = AF_INET6;
		so.lsa.sin6.sin6_port	= htons( (uint16_t) port );

#else // USE_IPV6

		so.lsa.sin.sin_family	= AF_INET;
		so.lsa.sin.sin_port		= htons( (uint16_t) port.port );

#endif // USE_IPV6

		if ( so.is_ssl && ( !g_ssl || pem.empty() ) )
		{
			g_error = "Cannot add SSL socket, is ssl certificate option set?";
			res = false;
			break;
		}
		else if( 
			( sock = socket( so.lsa.sa.sa_family, SOCK_STREAM, 6 ) ) == INVALID_SOCKET ||
			setsockopt( sock, SOL_SOCKET, SO_REUSEADDR, (const char *) &on, sizeof( on ) ) != 0 ||
			bind( sock, &so.lsa.sa, sizeof(so.lsa ) ) != 0 || listen( sock, SOMAXCONN ) != 0 )
		{
			close_socket_gracefully( sock );
			g_error = "Cannot bind socket, another socket is already listening on the same port or you must have more privileges";
			res = false;
			
			break;
		}
		else
		{
#ifndef _WIN32				
			fcntl( sock, F_SETFD, FD_CLOEXEC );
#endif // _WIN32

			so.sock = sock;
			g_listening_sockets.push_back( so );
		}
	}

	if( !res )
		close_all_listening_sockets();

	return res;
}


#if defined(_WIN32)

/**********************************************************************************************/
static HANDLE dlopen( const char* dll_name, int )
{
	return LoadLibraryA( dll_name );
}

/**********************************************************************************************/
static int set_non_blocking_mode( SOCKET sock )
{
	unsigned long on = 1;
	return ioctlsocket( sock, FIONBIO, &on );
}

#else

/**********************************************************************************************/
static int set_non_blocking_mode( SOCKET sock )
{
	int flags = fcntl( sock, F_GETFL, 0 );
	fcntl( sock, F_SETFL, flags | O_NONBLOCK );

	return 0;
}

#endif // _WIN32

/**********************************************************************************************/
// Read from IO channel - opened file descriptor, socket, or SSL descriptor.
// Return negative value on error, or number of bytes read on success.
//
static int pull( cr_connection_data& conn, char* buf, size_t len )
{
	return conn.ssl ?
		SSL_read( (SSL*) conn.ssl, buf, len ) :
		recv( conn.client->sock, buf, len, 0 );
}

/**********************************************************************************************/
int cr_read( cr_connection_data& conn, void* buf, size_t len )
{
	if( conn.consumed_content >= conn.content_len )
		return 0;
		
	int nread = 0;

	len = min( len, conn.content_len - conn.consumed_content );

	// Add buffered data
	const char* buffered = conn.request_buffer + conn.request_len + conn.consumed_content;

	int buffered_len = conn.request_buffer + conn.data_len - buffered;
	if( buffered_len > 0 )
	{
		if( len < (size_t) buffered_len )
			buffered_len = (int) len;

		memmove( buf, buffered, buffered_len );
		len -= buffered_len;

		conn.consumed_content += buffered_len;
		nread += buffered_len;
		buf = (char*) buf + buffered_len;
	}

	// We have returned all buffered data. Read new data from the remote socket.
	while( len > 0 )
	{
		int n = pull( conn, (char*) buf, len );
		if( n < 0 )
			return -1;
		
		if( n )
		{
			buf = (char*) buf + n;

			conn.consumed_content += n;
			nread += n;
			len -= n;
		}
		else
		{
			break;
		}
	}
	
	return nread;
}

/**********************************************************************************************/
int cr_write( cr_connection_data& conn, const char* buf, size_t len )
{
	size_t sent = 0;

	while( sent < len )
	{
		int n = conn.ssl ?
			SSL_write( (SSL*) conn.ssl, buf + sent, len - sent ) :
			send( conn.client->sock, buf + sent, len - sent, MSG_NOSIGNAL );

		if( n < 0 )
			break;

		sent += n;
	}

	return sent;
}

/**********************************************************************************************/
inline char hex_to_int( char ch )
{
	return ( ch >= '0' && ch <= '9' ) ? ch - '0' : ch - 'W';
}

/**********************************************************************************************/
static size_t decode_url( char* str )
{
	char* end = str;
	char* value = str;

	for( ; *str ; ++str, ++end )
	{
		if( *str == '+' )
		{
			*end = ' ';
		}
		else if( *str == '%' && isxdigit( str[ 1 ] ) && isxdigit( str[ 2 ] ) )
		{
			*end = ( hex_to_int( cr_tolower( str[ 1 ] ) ) << 4 ) | hex_to_int( cr_tolower( str[ 2 ] ) );
			str += 2;
		}
		else
		{
			if( end != str )
				*end = *str;
		}
	}

	*end = 0;
	return end - value;
}

/**********************************************************************************************/
static int sslize( cr_connection_data& conn, SSL_CTX* s, int (*func )( SSL * ) )
{
	return ( conn.ssl = SSL_new( s ) ) &&
		SSL_set_fd( (SSL*) conn.ssl, conn.client->sock ) == 1 &&
		func( (SSL*) conn.ssl ) == 1;
}

/**********************************************************************************************/
static int get_request_len( const char* buf, int len )
{
	auto* e = buf + len - 1;
	for( auto s = buf ; s < e ; ++s )
	{
		if( s[ 0 ] == '\n' && s[ 1 ] == '\r' && s[ 2 ] == '\n' )
			return s - buf - 1;
	}

	return 0;
}

/**********************************************************************************************/
inline void skip_whitespace( char*& buf )
{
	while( isspace( *buf ) )
		++buf;	
}

/**********************************************************************************************/
static char* substr_to( char*& buf, char ch )
{
	char* r = buf;
	
	while( *buf && *buf != ch )
		++buf;
		
	if( *buf )
		*buf++ = 0;
	
	return r;
}

/**********************************************************************************************/
static void parse_http_request( cr_connection_data& conn )
{
	char* buf = conn.request_buffer;
	buf[ conn.request_len ] = 0;

	skip_whitespace( buf );

	conn.method_	= substr_to( buf, ' ' );
	conn.uri_		= substr_to( buf, ' ' );

	substr_to( buf, '\n' );
	
	for( int i = 0 ; i < 64 ; ++i )
	{
		char* name  = substr_to( buf, ':' );
		skip_whitespace( buf );
		
		char* value = substr_to( buf, '\r' );
		if( *buf == '\n' )
			++buf;
		
		if( name && *name )
		{
			conn.headers_.add( name, value );
			for( ; *name ; *name = cr_tolower( *name ), ++name );
		}
		else
		{
			break;
		}
	}
}

/**********************************************************************************************/
static void read_http_request( cr_connection_data& conn )
{
	int n = 1;
	int request_len = 0;
	
	while( conn.data_len < MAX_REQUEST_SIZE && !request_len && n > 0 )
	{
		n = pull( conn, conn.request_buffer + conn.data_len, MAX_REQUEST_SIZE - conn.data_len );
		if( n > 0 )
		{
			conn.data_len += n;
			request_len = get_request_len( conn.request_buffer, conn.data_len );
		}
	}

	conn.request_len = n < 0 ? -1 : request_len;
}

/**********************************************************************************************/
static void ssl_locking_callback( int mode, int mutex_num, const char*, int )
{
	( mode & CRYPTO_LOCK ) ?
		g_ssl_mutexes[ mutex_num ]->lock() :
		g_ssl_mutexes[ mutex_num ]->unlock();
}

/**********************************************************************************************/
static unsigned long ssl_id_callback( void )
{
	// TODO:
	thread::id id = this_thread::get_id();
	unsigned long* p = (unsigned long*) &id;
	
	return *p;
}

/**********************************************************************************************/
static int load_dll( const char* dll_name, ssl_func* sw )
{
	void* dll_handle = dlopen( dll_name, RTLD_LAZY );
	if( !dll_handle )
	{
		g_error = "cannot load ssl dll";
		return 0;
	}

	for( ssl_func* fp = sw ; fp->name ; ++fp )
	{
#ifdef _WIN32
		fp->ptr = (void (*)(void)) dlsym( dll_handle, fp->name );
#else // _WIN32
		fp->ptr = (void (*)(void)) dlsym( dll_handle, fp->name );
#endif // _WIN32
		
		if( !fp->ptr )
		{
			g_error = "cannot find ssl symbol";
			return 0;
		}
	}

	return 1;
}

/**********************************************************************************************/
static void close_connection( cr_connection_data& conn )
{
	if( conn.ssl )
		SSL_free( (SSL*) conn.ssl );

	if( conn.client->sock != INVALID_SOCKET )
		close_socket_gracefully( conn.client->sock );
}

/**********************************************************************************************/
static bool set_pem( const string& pem )
{
	if( pem.empty() )
		return true;

	if( !load_dll( SSL_LIB, ssl_sw ) || !load_dll( CRYPTO_LIB, crypto_sw ) )
		return false;

	SSL_library_init();

	if( !( g_ssl_client = SSL_CTX_new( SSLv23_client_method( ) ) ) )
	{
		g_error = "SSL_CTX_new (client) error";
		return false;
	}

	if( !( g_ssl = SSL_CTX_new( SSLv23_server_method( ) ) ) )
	{
		g_error = "SSL_CTX_new (server) error";
		return false;
	}

	// If user callback returned non-NULL, that means that user callback has
	// set up certificate itself. In this case, skip sertificate setting.
	if( !SSL_CTX_use_certificate_file( g_ssl, pem.c_str(), SSL_FILETYPE_PEM ) ||
		!SSL_CTX_use_PrivateKey_file( g_ssl, pem.c_str(), SSL_FILETYPE_PEM ) )
	{
		g_error = "cannot open pem";
		return false;
	}

	size_t count = CRYPTO_num_locks();
	for( size_t i = 0 ; i < count ; ++i )
		g_ssl_mutexes.push_back( new mutex );

	CRYPTO_set_id_callback( &ssl_id_callback );
	CRYPTO_set_locking_callback( &ssl_locking_callback );
	SSL_CTX_use_certificate_chain_file( g_ssl, pem.c_str() );

	return true;
}

/**********************************************************************************************/
static bool cr_connect(
	cr_connection_data&	conn,
	cr_in_socket&		csock,
	const char*			host,
	int					port,
	int					use_ssl )
{
	if( use_ssl && !g_ssl_client )
		return false;
		
	hostent* he = gethostbyname( host );
	if( !he )
		return false;
	
	int sock = socket( PF_INET, SOCK_STREAM, 0 );
	if( sock == INVALID_SOCKET )
		return false;

	sockaddr_in sin;
	sin.sin_family	= AF_INET;
	sin.sin_port	= htons( (uint16_t) port );
	sin.sin_addr	= *(in_addr*) he->h_addr_list[ 0 ];

	if( connect( sock, (sockaddr*) &sin, sizeof( sin ) ) )
	{
		close_socket_gracefully( sock );
	}
	else
	{
		conn.client	  = &csock;
		csock.is_ssl  = use_ssl;
		csock.rsa.sin = sin;
		csock.sock    = sock;
		
		return !use_ssl || sslize( conn, g_ssl_client, SSL_connect );
	}

	return false;
}

/**********************************************************************************************/
bool cr_fetch(
	char*			hbuf,
	char*&			out,
	size_t&			out_size,
	const char*		url,
	cr_string_map*	headers,
	int				redirect_count )
{
	out = NULL;
	
	char	host[ 1025 ];
	int		n;
	int		port = 0;
	char	proto[ 10 ];

	if( sscanf( url, "%9[htps]://%1024[^:]:%d/%n", proto, host, &port, &n ) != 3 &&
		sscanf( url, "%9[htps]://%1024[^/]/%n", proto, host, &n ) != 2 )
	{
		return false;
	}

	bool ssl = !strcmp( proto, "https" );
	if( !port )
		port = ssl ? 443 : 80;
	
	cr_connection_data conn;
	cr_in_socket sock;
	
	if( cr_connect( conn, sock, host, port, ssl ) )
	{
		char* str = hbuf;
		add_string( str, "GET /", 5 );
		add_string( str, url + n, strlen( url + n ) );
		add_string( str, " HTTP/1.0\r\nHost: ", 17 );
		add_string( str, host, strlen( host ) );
		add_string( str, "\r\nUser-Agent: Mozilla/5.0 Gecko Firefox/18\r\n\r\n", 46 );
		cr_write( conn, hbuf, str - hbuf );
		
		read_http_request( conn );
		if( conn.request_len > 0 )
		{
			parse_http_request( conn );
			if( !strncmp( conn.method_, "HTTP/", 5 ) )
			{
				if( headers )
					*headers = conn.headers_;

				// Redirect
				const char* location = conn.headers_[ "location" ];
				if( location && *location && redirect_count < 5 )
				{
					close_connection( conn );
					return cr_fetch( hbuf, out, out_size, location, headers, redirect_count + 1 );
				}

				size_t msize = conn.request_len + conn.data_len + 4;

				const char* content_length = conn.headers_[ "content-length" ];
				msize += content_length ? atoi( content_length ) : 32768;

				str = out = (char*) malloc( msize );
				
				conn.data_len -= conn.request_len + 4;
				if( conn.data_len > 0 )
					add_string( str, conn.request_buffer + conn.request_len + 4, conn.data_len );

				// Read the rest of the response and write it to the file. Do not use
				// mg_read() cause we didn't set newconn->content_len properly.
				char buf[ 65536 ];
				while( ( conn.data_len = pull( conn, buf, sizeof( buf ) ) ) > 0 )
				{
					if( str - out + conn.data_len + 1 > msize )
					{
						msize += conn.data_len + 65536;

						size_t diff = str - out;
						out = (char*) realloc( out, msize );
						str = out + diff;
					}

					add_string( str, buf, conn.data_len );
				}

				out_size = str - out;
			}
		}
		
		close_connection( conn );
	}
	
	return out != NULL;
}

/**********************************************************************************************/
static void loop_finish( void )
{
	g_stop = true;
	close_all_listening_sockets();

	if( g_ssl )
		SSL_CTX_free( g_ssl );

	if( g_ssl_client )
		SSL_CTX_free( g_ssl_client );

	for( mutex* mtx : g_ssl_mutexes )
		delete mtx;

	g_ssl_client = 0;
	g_ssl		 = 0;
	g_ssl_mutexes.clear();

#if defined(_WIN32)
	WSACleanup();
#endif // _WIN32		
}

/**********************************************************************************************/
static void loop_init( void )
{
#ifdef _WIN32
	
	WSADATA data;
	WSAStartup( MAKEWORD( 2, 2 ), &data );
	SetThreadPriority( GetCurrentThread(), THREAD_PRIORITY_ABOVE_NORMAL );
	
#else // _WIN32	

	signal( SIGCHLD, SIG_IGN );
	signal( SIGPIPE, SIG_IGN );
	
#endif // _WIN32	

	g_stop = false;
}

/**********************************************************************************************/
void event_handler( cr_connection_data& conn );

/**********************************************************************************************/
static void process_connection( cr_in_socket& socket )
{
	cr_connection_data conn;
	conn.client		= &socket;
	conn.remote_ip_	= ntohl( conn.client->rsa.sin.sin_addr.s_addr );

	if( !socket.is_ssl || sslize( conn, g_ssl, SSL_accept ) )
	{
		read_http_request( conn );
		if( conn.request_len > 0 )
		{
			parse_http_request( conn );
			if( *conn.uri_ == '/' )
			{
				conn.content_len = atoi( conn.headers_[ "content-length" ] );

				if( ( conn.query_parameters_ = strchr( conn.uri_, '?' ) ) )
					*conn.query_parameters_++ = 0;

				decode_url( conn.uri_ );
				event_handler( conn );
			}
		}
		else
		{
			cr_write( conn, "HTTP/1.1 400 Bad Request\r\nContent-Length: 0\r\n\r\n", 47 );
		}
	}

	close_connection( conn );
}

/**********************************************************************************************/
bool cr_event_loop(
	const vector<cr_port>&	ports,
	const string&			pem,
	size_t					thread_count )
{
	cr_in_socket	accepted;
	fd_set			read_set;
	socklen_t		sock_len	= sizeof( accepted.rsa );
	cr_thread_pool	tpool		( thread_count > 1 ? thread_count : 0 );
	timeval			tv			= { 0, 0 };

	loop_init();

	// BIND SOCKETS
	
	if( !set_pem( pem ) || !set_ports( ports, pem ) )
	{
		loop_finish();
		return false;
	}

	// PRECACHE SOME VALUES
	
	int max_fd = -1;
	for( auto& skt : g_listening_sockets )
	{
		if( skt.sock + 1 > (SOCKET) max_fd )
			max_fd = (int) skt.sock + 1;
	}

	fd_set stat_set;
	FD_ZERO( &stat_set );
	for( auto& skt : g_listening_sockets )
		FD_SET( skt.sock, &stat_set );

	// PROCESS REQUESTS
	
	while( !g_stop )
	{
		tv.tv_usec = 200 * 1000;
		read_set = stat_set;

		if( select( max_fd, &read_set, NULL, NULL, &tv ) >= 0 )
		{
			for( auto& skt : g_listening_sockets )
			{
				if( FD_ISSET( skt.sock, &read_set ) )
				{
					accepted.sock = accept( skt.sock, &accepted.rsa.sa, &sock_len );
					if( accepted.sock != INVALID_SOCKET )
					{
						accepted.is_ssl = skt.is_ssl;

						thread_count < 2 ?
							process_connection( accepted ) :
							tpool.enqueue( accepted );
					}			
				}
			}
		}
	}

	loop_finish();

	return true;
}

/**********************************************************************************************/
void cr_stop( void )
{
	g_stop = true;
}
