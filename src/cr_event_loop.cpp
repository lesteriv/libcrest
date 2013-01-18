/**********************************************************************************************/
/* mongoose.cpp		  		                                                   				  */
/*                                                                       					  */
/* (c) 2004-2012 Sergey Lyubka																  */
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
#include <time.h>
#include <vector>

// LIBCREST
#include "../include/cr_utils.h"
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

#    define closesocket(a) close(a)
#    define ERRNO errno
#    define INVALID_SOCKET (-1)

typedef int SOCKET;

#endif // _WIN32

// LINUX SPECIFIC
#include <sys/epoll.h>

// MONGOOSE
#include "cr_event_loop.h"

#define ARRAY_SIZE(array)	(sizeof(array) / sizeof(array[0]))

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

// Snatched from OpenSSL includes. I put the prototypes here to be independent
// from the OpenSSL source installation. Having this, mongoose + SSL can be
// built on any system with binary SSL libraries installed.
typedef struct ssl_st SSL;
typedef struct ssl_method_st SSL_METHOD;
typedef struct ssl_ctx_st SSL_CTX;

#    define SSL_ERROR_WANT_READ		2
#    define SSL_ERROR_WANT_WRITE	3
#    define SSL_FILETYPE_PEM		1
#    define CRYPTO_LOCK				1

// Dynamically loaded SSL functionality

struct ssl_func
{
	const char* name;		// SSL function name
	void  (*ptr)( void );	// Function pointer
};

#    define SSL_free							(* (void (*)(SSL *)) ssl_sw[0].ptr)
#    define SSL_accept							(* (int (*)(SSL *)) ssl_sw[1].ptr)
#    define SSL_connect							(* (int (*)(SSL *)) ssl_sw[2].ptr)
#    define SSL_read							(* (int (*)(SSL *, void *, int)) ssl_sw[3].ptr)
#    define SSL_write							(* (int (*)(SSL *, const void *,int)) ssl_sw[4].ptr)
#    define SSL_get_error						(* (int (*)(SSL *, int)) ssl_sw[5].ptr)
#    define SSL_set_fd							(* (int (*)(SSL *, SOCKET)) ssl_sw[6].ptr)
#    define SSL_new								(* (SSL * (*)(SSL_CTX *)) ssl_sw[7].ptr)
#    define SSL_CTX_new							(* (SSL_CTX * (*)(SSL_METHOD *)) ssl_sw[8].ptr)
#    define SSLv23_server_method				(* (SSL_METHOD * (*)(void)) ssl_sw[9].ptr)
#    define SSL_library_init					(* (int (*)(void)) ssl_sw[10].ptr)
#    define SSL_CTX_use_PrivateKey_file			(* (int (*)(SSL_CTX *, const char*, int)) ssl_sw[11].ptr)
#    define SSL_CTX_use_certificate_file		(* (int (*)(SSL_CTX *, const char *, int)) ssl_sw[12].ptr)
#    define SSL_CTX_set_default_passwd_cb		(* (void (*)(SSL_CTX *, mg_callback_t)) ssl_sw[13].ptr)
#    define SSL_CTX_free						(* (void (*)(SSL_CTX *)) ssl_sw[14].ptr)
#    define SSL_CTX_use_certificate_chain_file	(* (int (*)(SSL_CTX *, const char *)) ssl_sw[15].ptr)
#    define SSLv23_client_method				(* (SSL_METHOD * (*)(void)) ssl_sw[16].ptr)
#	 define SSL_pending							(* (int (*)(SSL *)) ssl_sw[17].ptr)

#    define CRYPTO_num_locks					(* (int (*)(void)) crypto_sw[0].ptr)
#    define CRYPTO_set_locking_callback			(* (void (*)(void (*)(int, int, const char *, int))) crypto_sw[1].ptr)
#    define CRYPTO_set_id_callback				(* (void (*)(unsigned long (*)(void))) crypto_sw[2].ptr)


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
	{ "SSL_CTX_set_default_passwd_cb"		, NULL },
	{ "SSL_CTX_free"						, NULL },
	{ "SSL_CTX_use_certificate_chain_file"	, NULL },
	{ "SSLv23_client_method"				, NULL },
	{ "SSL_pending"							, NULL },
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
								~cr_thread_pool( void );

	public://////////////////////////////////////////////////////////////////////////								
						
// This class API:

	// ---------------------
	// Methods

		void					enqueue( const cr_in_socket& skt )
								{
									queue_mutex_.lock();
									tasks_.push( skt );
									queue_mutex_.unlock();

									condition_.notify_one();
								}								

								
	public://////////////////////////////////////////////////////////////////////////

// Properties		
		
		condition_variable		condition_;
		mutex					queue_mutex_;
		queue<cr_in_socket>		tasks_;
		vector<thread>			workers_;
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

		cr_thread_pool&		pool_;
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
cr_thread_pool::~cr_thread_pool( void )
{
	condition_.notify_all();

	for( auto& it : workers_ )
		it.join();
}

/**********************************************************************************************/
static void close_all_listening_sockets( void )
{
	for( auto& sp : g_listening_sockets )
		closesocket( sp.sock );

	g_listening_sockets.clear();
}

/**********************************************************************************************/
static int set_ports( 
	const vector<cr_port>&	ports,
	const string&			pem )
{
	int on = 1, success = 1;
	SOCKET sock;
	cr_socket so;

	if( ports.empty() )
	{
		g_error = "Invalid port spec. Expecting list of: [IP_ADDRESS:]PORT[s|p]";
		success = 0;
	}

	size_t i = 0;
	while( success && i < ports.size() )
	{
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
			success = 0;
		}
		else if( ( sock = socket( so.lsa.sa.sa_family, SOCK_STREAM, 6 ) ) == INVALID_SOCKET ||

			// On Windows, SO_REUSEADDR is recommended only for
			// broadcast UDP sockets
			setsockopt( sock, SOL_SOCKET, SO_REUSEADDR, (const char *) &on, sizeof( on ) ) != 0 ||

			// Set TCP keep-alive. This is needed because if HTTP-level
			// keep-alive is enabled, and client resets the connection,
			// server won't get TCP FIN or RST and will keep the connection
			// open forever. With TCP keep-alive, next keep-alive
			// handshake will figure out that the client is down and
			// will close the server end.
			// Thanks to Igor Klopov who suggested the patch.
			setsockopt( sock, SOL_SOCKET, SO_KEEPALIVE, (char *) &on, sizeof( on ) ) != 0 ||
			bind( sock, &so.lsa.sa, sizeof(so.lsa ) ) != 0 || listen( sock, SOMAXCONN ) != 0 )
		{
			closesocket( sock );
			g_error = "Cannot bind socket, another socket is already listening on the same port or you must have more privileges";
			success = 0;
		}
		else
		{
#ifndef _WIN32				
			fcntl( sock, F_SETFD, FD_CLOEXEC );
#endif // _WIN32

			cr_socket sk;
			sk.sock = sock;
			sk.is_ssl = so.is_ssl;

			g_listening_sockets.push_back( sk );
		}
	}

	if( !success )
		close_all_listening_sockets();

	return success;
}

/**********************************************************************************************/
static char* skip_quoted(
	char**		buf,
	const char* delimiters,
	const char* whitespace )
{
	char* begin_word = *buf;
	char* end_word = begin_word + strcspn( begin_word, delimiters );

	if( *end_word == '\0' )
	{
		*buf = end_word;
	}
	else
	{
		char* end_whitespace = end_word + 1 + strspn( end_word + 1, whitespace );
		for( char* p = end_word ; p < end_whitespace ; p++ )
			*p = '\0';

		*buf = end_whitespace;
	}

	return begin_word;
}

/**********************************************************************************************/
static char* skip( char** buf, const char* delimiters )
{
	return skip_quoted( buf, delimiters, delimiters );
}

/**********************************************************************************************/
typedef void * ( *mg_thread_func_t )(void *) ;


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
	int flags;

	flags = fcntl( sock, F_GETFL, 0 );
	fcntl( sock, F_SETFL, flags | O_NONBLOCK );

	return 0;
}

#endif // _WIN32

/**********************************************************************************************/
// This function is needed to prevent Mongoose to be stuck in a blocking
// socket read when user requested exit. To do that, we sleep in select
// with a timeout, and when returned, check the context for the stop flag.
// If it is set, we return 0, and this means that we must not continue
// reading, must give up and close the connection and exit serving thread.
//
static int wait_until_socket_is_readable( cr_connection_data& conn )
{
	int result;
	struct timeval tv;
	fd_set set;

	do
	{
		tv.tv_sec = 0;
		tv.tv_usec = 300 * 1000;
		FD_ZERO( &set );
		FD_SET( conn.client->sock, &set );

		result = select( conn.client->sock + 1, &set, NULL, NULL, &tv );
		
		if( !result && conn.ssl )
			result = SSL_pending( (SSL*) conn.ssl );
	}
	while( ( !result || ( result < 0 && ERRNO == EINTR ) ) && !g_stop );

	return g_stop || result < 0 ? 0 : 1;
}

/**********************************************************************************************/
// Read from IO channel - opened file descriptor, socket, or SSL descriptor.
// Return negative value on error, or number of bytes read on success.
//
static int pull( cr_connection_data& conn, char* buf, int len )
{
	int nread;

	if( !wait_until_socket_is_readable( conn ) )
	{
		nread = -1;
	}
	else if( conn.ssl )
	{
		nread = SSL_read( (SSL*) conn.ssl, buf, len );
	}
	else
	{
		nread = recv( conn.client->sock, buf, (size_t) len, 0 );
	}

	return g_stop ? -1 : nread;
}

/**********************************************************************************************/
int cr_read( cr_connection_data& conn, void* buf, size_t len )
{
	int n, buffered_len, nread;
	const char *body;

	nread = 0;
	if( conn.consumed_content < conn.content_len )
	{
		// Adjust number of bytes to read.
		int to_read = conn.content_len - conn.consumed_content;
		if( to_read < (int) len )
			len = (size_t) to_read;

		// Return buffered data
		body = conn.request_buffer + conn.request_len + conn.consumed_content;
		buffered_len = &conn.request_buffer[ conn.data_len ] - body;
		
		if( buffered_len > 0 )
		{
			if( len < (size_t) buffered_len )
				buffered_len = (int) len;

			memmove( buf, body, (size_t) buffered_len );
			len -= buffered_len;
			conn.consumed_content += buffered_len;
			nread += buffered_len;
			buf = (char*) buf + buffered_len;
		}

		// We have returned all buffered data. Read new data from the remote socket.
		while( len > 0 )
		{
			n = pull( conn, (char*) buf, (int) len );
			if( n < 0 )
			{
				nread = n;  // Propagate the error
				break;
			}
			else if( n == 0 )
			{
				break;  // No more data to read
			}
			else
			{
				buf = (char*) buf + n;
				conn.consumed_content += n;
				nread += n;
				len -= n;
			}
		}
	}
	
	return nread;
}

/**********************************************************************************************/
int cr_write( cr_connection_data& conn, const char* buf, size_t len )
{
	int n, k;

	int sent = 0;
	while( sent < (int) len )
	{
		// How many bytes we send in this iteration
		k = (int) ( len - sent );

		if( conn.ssl )
			n = SSL_write( (SSL*) conn.ssl, buf + sent, k );
		else
			n = send( conn.client->sock, buf + sent, (size_t) k, MSG_NOSIGNAL );

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
			return s - buf + 2;
	}

	return 0;
}

/**********************************************************************************************/
// Parse HTTP headers from the given buffer, advance buffer to the point
// where parsing stopped.
//
static void parse_http_headers( char** buf, cr_string_map& headers )
{
	for( int i = 0 ; i < 64 ; ++i )
	{
		char* name = skip_quoted( buf, ":", " " );
		char* value = skip( buf, "\r\n" );
		
		if( name && *name )
		{
			headers.add( name, value );
			while( *name )
			{
				*name = cr_tolower( *name );
				++name;
			}
		}
		else
		{
			break;
		}
	}
}

/**********************************************************************************************/
// Parse HTTP request, fill in mg_request_info structure.
// This function modifies the buffer by NUL-terminating
// HTTP request components, header names and header values.
//
static bool parse_http_message( char* buf, int len, cr_connection_data& ri )
{
	int request_length = get_request_len( buf, len );
	if( request_length > 0 )
	{
		// Reset attributes. DO NOT TOUCH is_ssl, remote_ip, remote_port
		ri.method_ = ri.uri_ = NULL;
		ri.headers_.clear();

		buf[ request_length - 1 ] = '\0';

		// RFC says that all initial whitespaces should be ingored
		while( *buf && isspace( *(unsigned char*) buf ) )
			buf++;

		ri.method_ = skip( &buf, " " );
		ri.uri_ = skip( &buf, " " );
		skip( &buf, "\r\n" );
		parse_http_headers( &buf, ri.headers_ );
	}
	
	return request_length > 0;
}

/**********************************************************************************************/
static int read_http_request( 
	cr_connection_data&	conn,
	char*				out_buf,
	int*				readed )
{
	int n = 1;
	int request_len = 0;
	
	while( *readed < MAX_REQUEST_SIZE && !request_len && n > 0 )
	{
		n = pull( conn, out_buf + *readed, MAX_REQUEST_SIZE - *readed );
		if( n > 0 )
		{
			*readed += n;
			request_len = get_request_len( out_buf, *readed );
		}
	}

	return n < 0 ? -1 :	request_len;
}

/**********************************************************************************************/
void event_handler( cr_connection_data& conn );

/**********************************************************************************************/
static void ssl_locking_callback( int mode, int mutex_num, const char*, int )
{
	if( mode & CRYPTO_LOCK )
		g_ssl_mutexes[ mutex_num ]->lock();
	else
		g_ssl_mutexes[ mutex_num ]->unlock();
}

/**********************************************************************************************/
static unsigned long ssl_id_callback( void )
{
	thread::id id = this_thread::get_id();
	unsigned long* p = (unsigned long*) &id;
	
	return *p;
}

/**********************************************************************************************/
static int load_dll( const char* dll_name, ssl_func* sw )
{
	union
	{
		void *p;
		void (*fp )(void);
	}
	u;
	
	void* dll_handle;
	ssl_func* fp;

	if( !( dll_handle = dlopen( dll_name, RTLD_LAZY ) ) )
	{
		g_error = "cannot load ssl dll";
		return 0;
	}

	for( fp = sw ; fp->name ; fp++ )
	{
#    ifdef _WIN32
		// GetProcAddress() returns pointer to function
		u.fp = (void (*)(void)) dlsym( dll_handle, fp->name );
#    else
		// dlsym() on UNIX returns void *. ISO C forbids casts of data pointers to
		// function pointers. We need to use a union to make a cast.
		u.p = dlsym( dll_handle, fp->name );
#    endif // _WIN32
		
		if( !u.fp )
		{
			g_error = "cannot find ssl symbol";
			return 0;
		}
		else
		{
			fp->ptr = u.fp;
		}
	}

	return 1;
}

/**********************************************************************************************/
static void close_socket_gracefully( cr_connection_data& conn )
{
	char buf[ 8192 ];
	struct linger linger;
	int n, sock = conn.client->sock;

	// Set linger option to avoid socket hanging out after close. This prevent
	// ephemeral port exhaust problem under high QPS.
	linger.l_onoff = 1;
	linger.l_linger = 1;
	setsockopt( sock, SOL_SOCKET, SO_LINGER, (char*) &linger, sizeof(linger) );

	// Send FIN to the client
	shutdown( sock, SHUT_WR );
	set_non_blocking_mode( sock );

	// Read and discard pending incoming data. If we do not do that and close the
	// socket, the data in the send buffer may be discarded. This
	// behaviour is seen on Windows, when client keeps sending data
	// when server decides to close the connection; then when client
	// does recv() it gets no data back.
	do
	{
		n = pull( conn, buf, sizeof(buf) );
	}
	while( n > 0 );

	// Now we know that our FIN is ACK-ed, safe to close
	closesocket( sock );
}

/**********************************************************************************************/
static void close_connection( cr_connection_data& conn )
{
	if( conn.ssl )
		SSL_free( (SSL*) conn.ssl );

	if( conn.client->sock != INVALID_SOCKET )
		close_socket_gracefully( conn );
}

/**********************************************************************************************/
static bool set_pem( const string& pem )
{
	// If PEM file is not specified, skip SSL initialization.
	if( pem.empty() )
		return true;

	if( !load_dll( SSL_LIB, ssl_sw ) || !load_dll( CRYPTO_LIB, crypto_sw ) )
		return false;

	// Initialize SSL crap
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

	SSL_CTX_use_certificate_chain_file( g_ssl, pem.c_str() );

	// Initialize locking callbacks, needed for thread safety.
	// http://www.openssl.org/support/faq.html#PROG1
	size_t n = CRYPTO_num_locks();
	for( size_t i = 0 ; i < n ; ++i )
		g_ssl_mutexes.push_back( new mutex );

	CRYPTO_set_locking_callback( &ssl_locking_callback );
	CRYPTO_set_id_callback( &ssl_id_callback );

	return true;
}

/**********************************************************************************************/
static cr_connection_data* mg_connect(
	cr_in_socket&	csock,
	const char*		host,
	int				port,
	int				use_ssl )
{
	cr_connection_data* newconn = NULL;
	sockaddr_in sin;
	hostent* he;
	int sock;

	if( use_ssl && !g_ssl_client )
	{
	}
	else if( !( he = gethostbyname( host ) ) )
	{
	}
	else if( ( sock = socket( PF_INET, SOCK_STREAM, 0 ) ) == INVALID_SOCKET )
	{
	}
	else
	{
		sin.sin_family	= AF_INET;
		sin.sin_port	= htons( (uint16_t) port );
		sin.sin_addr	= *(in_addr*) he->h_addr_list[ 0 ];
		
		if( connect( sock, (sockaddr*) &sin, sizeof( sin ) ) )
		{
			closesocket( sock );
		}
		else
		{
			newconn = new cr_connection_data;
			newconn->client = &csock;
			newconn->client->sock = sock;
			newconn->client->rsa.sin = sin;
			
			newconn->client->is_ssl = use_ssl;
			if( use_ssl )
				sslize( *newconn, g_ssl_client, SSL_connect );
		}
	}

	return newconn;
}

/**********************************************************************************************/
inline int parse_http_response( char* buf, int len, cr_connection_data& ri )
{
	return parse_http_message( buf, len, ri ) && !strncmp( ri.method_, "HTTP/", 5 );
}

/**********************************************************************************************/
bool mg_fetch(
	char*			buf,
	char*&			out,
	size_t&			out_size,
	const char*		url,
	cr_string_map*	headers,
	int				redirect_count )
{
	// TODO:
	
	out = NULL;
	
	cr_connection_data* conn;
	cr_in_socket sock;
	
	int n, port;
	char host[ 1025 ], proto[ 10 ];

	if( sscanf( url, "%9[htps]://%1024[^:]:%d/%n", proto, host, &port, &n ) == 3 )
	{
	}
	else if( sscanf( url, "%9[htps]://%1024[^/]/%n", proto, host, &n ) == 2 )
	{
		port = !strcmp( proto, "https" ) ? 443 : 80;
	}
	else
	{
		return false;
	}

	if( ( conn = mg_connect( sock, host, port, !strcmp( proto, "https" ) ) ) )
	{
		char* str = buf;
		add_string( str, "GET /", 5 );
		add_string( str, url + n, strlen( url + n ) );
		add_string( str, " HTTP/1.0\r\nHost: ", 17 );
		add_string( str, host, strlen( host ) );
		add_string( str, "\r\nUser-Agent: Mozilla/5.0 Gecko Firefox/18\r\n\r\n", 46 );
		cr_write( *conn, buf, str - buf );
		
		int data_length = 0;
		int req_length = read_http_request( *conn, buf, &data_length );
		if( req_length > 0 && parse_http_message( buf, req_length, *conn ) && !strncmp( conn->method_, "HTTP/", 5 ) )
		{
			if( headers )
				*headers = conn->headers_;
			
			const char* location = conn->headers_[ "location" ];
			if( location && *location && redirect_count < 5 )
			{
				close_connection( *conn );
				free( conn );

				return mg_fetch( buf, out, out_size, location, headers, redirect_count + 1 );
			}
		
			// Write chunk of data that may be in the user's buffer
			data_length -= req_length;
			
			size_t msize = req_length + data_length;
			const char* cl = conn->headers_[ "content-length" ];
			msize += cl ? strtol( cl, NULL, 10 ) : 32768;
			
			out = (char*) malloc( msize );
			str = out;
			
			if( data_length > 0 )
				add_string( str, buf + req_length, data_length );
			
			// Read the rest of the response and write it to the file. Do not use
			// mg_read() cause we didn't set newconn->content_len properly.
			char buf2[ 65536 ];
			while( ( data_length = pull( *conn, buf2, sizeof( buf2 ) ) ) > 0 )
			{
				if( str - out + data_length + 1 > (int) msize )
				{
					msize += data_length + 16384;
					
					size_t diff = str - out;
					out = (char*) realloc( out, msize );
					str = out + diff;
				}
				
				add_string( str, buf2, data_length );
			}
			
			out_size = str - out;
		}
		
		close_connection( *conn );
		delete conn;
	}
	
	return out != NULL;
}

/**********************************************************************************************/
static void loop_finish( void )
{
	close_all_listening_sockets();

	if( g_ssl )
		SSL_CTX_free( g_ssl );

	if( g_ssl_client )
		SSL_CTX_free( g_ssl_client );

	for( mutex* mtx : g_ssl_mutexes )
		delete mtx;

	g_ssl_client = 0;
	g_ssl = 0;
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
static void process_connection( cr_in_socket& socket )
{
	cr_connection_data conn;
	conn.client		= &socket;
	conn.remote_ip_	= ntohl( conn.client->rsa.sin.sin_addr.s_addr );

	if( !socket.is_ssl || sslize( conn, g_ssl, SSL_accept ) )
	{
		conn.request_len = read_http_request( conn, conn.request_buffer, &conn.data_len );
		if( conn.request_len &&
			parse_http_message( conn.request_buffer, MAX_REQUEST_SIZE, conn ) && *conn.uri_ == '/' )
		{
			conn.content_len = atoi( conn.headers_[ "content-length" ] );

			if( ( conn.query_parameters_ = strchr( conn.uri_, '?' ) ) )
				*conn.query_parameters_++ = 0;

			decode_url( conn.uri_ );
			event_handler( conn );
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
