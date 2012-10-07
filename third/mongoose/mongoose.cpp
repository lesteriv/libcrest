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

// STD
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

// CREST
#include "../../src/utils.h"


#if defined(_WIN32) // Windows specific
#    define _CRT_SECURE_NO_WARNINGS // Disable deprecation warning in VS2005
#    define _WIN32_WINNT 0x0400 // To make it link in VS2005
#    include <windows.h>

#    include <direct.h>
#    include <io.h>
#    include <process.h>

#    define _POSIX_
#    define NO_SOCKLEN_T

#    define ERRNO		GetLastError()
#    define CRYPTO_LIB  "libeay32.dll"
#    define O_NONBLOCK  0
#    define SHUT_WR		1
#    define SSL_LIB		"ssleay32.dll"

#    define mg_sleep_int(x) Sleep(x)

#    define close(x)	_close(x)
#    define dlsym(x,y)	GetProcAddress((HINSTANCE) (x), (y))
#    define RTLD_LAZY	0
#    define write(x, y, z) _write((x), (y), (unsigned) z)
#    define read(x, y, z) _read((x), (y), (unsigned) z)

typedef HANDLE pthread_mutex_t;

typedef struct
{
	HANDLE signal, broadcast;
}
pthread_cond_t;

typedef DWORD pthread_t;
#    define pid_t HANDLE // MINGW typedefs pid_t to int. Using #define here.

static int pthread_mutex_lock( pthread_mutex_t * );
static int pthread_mutex_unlock( pthread_mutex_t * );

// Mark required libraries
#    pragma comment(lib, "Ws2_32.lib")

#else	// UNIX  specific

#    include <dlfcn.h>
#    include <netinet/in.h>
#	 include <netdb.h>
#    include <pthread.h>
#    include <unistd.h>

#    ifdef __MACH__
#        define SSL_LIB "libssl.dylib"
#        define CRYPTO_LIB "libcrypto.dylib"
#    else // __MACH__
#        define SSL_LIB "libssl.so"
#        define CRYPTO_LIB "libcrypto.so"
#    endif // __MACH__

#    define closesocket(a) close(a)
#    define mg_sleep_int(x) usleep((x) * 1000)
#    define ERRNO errno
#    define INVALID_SOCKET (-1)
typedef int SOCKET;

#endif // End of Windows and UNIX specific includes


#include "mongoose.h"

#define MAX_REQUEST_SIZE	16384
#define ARRAY_SIZE(array)	(sizeof(array) / sizeof(array[0]))

#ifdef _WIN32
static pthread_t pthread_self( void )
{
	return GetCurrentThreadId( );
}
#endif // _WIN32

// Darwin prior to 7.0 and Win32 do not have socklen_t
#ifdef NO_SOCKLEN_T
typedef int socklen_t;
#endif // NO_SOCKLEN_T
#define _DARWIN_UNLIMITED_SELECT

#if !defined(MSG_NOSIGNAL)
#    define MSG_NOSIGNAL 0
#endif

#if !defined(SOMAXCONN)
#    define SOMAXCONN 100
#endif

#ifndef NO_SSL
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
	const char* name;   // SSL function name
	void  (*ptr )(void) ; // Function pointer
} ;

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
	{ NULL									, NULL }
};

// Similar array as ssl_sw. These functions could be located in different lib.
static struct ssl_func crypto_sw[] = {
	{ "CRYPTO_num_locks"					, NULL },
	{ "CRYPTO_set_locking_callback"			, NULL },
	{ "CRYPTO_set_id_callback"				, NULL },
	{ NULL									, NULL }
};
#endif // NO_SSL

// Unified socket address. For IPv6 support, add IPv6 address structure
// in the union u.

union usa
{
	struct sockaddr sa;
	struct sockaddr_in sin;
#if defined(USE_IPV6)
	struct sockaddr_in6 sin6;
#endif
} ;

// Describes a string (chunk of memory).

struct vec
{
	const char*	ptr;
	size_t		len;
} ;

// Describes listening socket, or socket which was accept()-ed by the master
// thread and queued for future handling by the worker thread.

struct mg_socket
{
	mg_socket*	next;	// Linkage
	SOCKET		sock;   // Listening socket
	union usa	lsa;	// Local socket address
	union usa	rsa;	// Remote socket address

#ifndef NO_SSL	
	bool		is_ssl; // Is socket SSL-ed
#endif // NO_SSL
};

static const char* error_string;

struct mg_context
{
#ifndef NO_SSL
	SSL_CTX*		client_ssl_ctx;		// Client SSL context
	SSL_CTX*		ssl_ctx;			// SSL context
#endif // NO_SLL
	mg_socket*		listening_sockets;
	volatile int	num_threads;		// Number of threads
	mg_socket		queue[ 64 ];		// Accepted sockets
	pthread_cond_t	sq_empty;			// Signaled when socket is consumed
	pthread_cond_t	sq_full;			// Signaled when socket is produced
	volatile int	sq_head;			// Head of the socket queue
	volatile int	sq_tail;			// Tail of the socket queue
	volatile int	stop_flag;			// Should we stop event loop

	pthread_mutex_t mutex;				// Protects (max|num)_threads
	pthread_cond_t  cond;				// Condvar for tracking workers terminations
} ;

/**********************************************************************************************/
struct mg_connection
{
	time_t			birth_time;				// Time when request was received
	mg_socket		client;					// Connected client
	mg_context*		ctx;
	mg_request_info	request_info;
#ifndef NO_SSL	
	void*			ssl;					// SSL descriptor
#endif // NO_SSL
	int				content_len;			// Content-Length header value
	int				consumed_content;		// How many bytes of content have been read
	char*			buf;					// Buffer for received data
	int				must_close;				// 1 if connection must be closed
	int				request_len;			// Size of the request + headers in a buffer
	int				data_len;				// Total size of data in a buffer
} ;

/**********************************************************************************************/
time_t mg_get_birth_time( mg_connection* conn )
{
	return conn->birth_time;
}

/**********************************************************************************************/
size_t mg_get_content_len( mg_connection* conn )
{
	return conn->content_len >= 0 ? conn->content_len : 0;
}

/**********************************************************************************************/
mg_request_info* mg_get_request_info( mg_connection* conn )
{
	return &conn->request_info;
}

/**********************************************************************************************/
static char *skip_quoted(
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
static const char* get_header(
	const mg_request_info*	ri,
	const char*				name )
{
	return ri->headers_.value( name );
}

/**********************************************************************************************/
const char* mg_get_header( mg_connection* conn, const char* name )
{
	return get_header( &conn->request_info, name );
}

// HTTP 1.1 assumes keep alive if "Connection:" header is not set
// This function must tolerate situations when connection info is not
// set up, for example if request parsing failed.

/**********************************************************************************************/
static int should_keep_alive( mg_connection *conn )
{
	if( 1 || conn->must_close )
		return 0;

	const char* header = mg_get_header( conn, "connection" );
	if( !header || ( *header != 'k' && *header != 'K' ) )
		return 0;

	return 1;
}

/**********************************************************************************************/
typedef void * ( *mg_thread_func_t )(void *) ;


#if defined(_WIN32)

static int pthread_mutex_init( pthread_mutex_t *mutex, void *unused )
{
	unused = NULL;
	*mutex = CreateMutex( NULL, FALSE, NULL );
	return *mutex == NULL ? -1 : 0;
}

static int pthread_mutex_destroy( pthread_mutex_t *mutex )
{
	return CloseHandle( *mutex ) == 0 ? -1 : 0;
}

static int pthread_mutex_lock( pthread_mutex_t *mutex )
{
	return WaitForSingleObject( *mutex, INFINITE ) == WAIT_OBJECT_0 ? 0 : -1;
}

static int pthread_mutex_unlock( pthread_mutex_t *mutex )
{
	return ReleaseMutex( *mutex ) == 0 ? -1 : 0;
}

static int pthread_cond_init( pthread_cond_t *cv, const void *unused )
{
	unused = NULL;
	cv->signal = CreateEvent( NULL, FALSE, FALSE, NULL );
	cv->broadcast = CreateEvent( NULL, TRUE, FALSE, NULL );
	return cv->signal != NULL && cv->broadcast != NULL ? 0 : -1;
}

static int pthread_cond_wait( pthread_cond_t *cv, pthread_mutex_t *mutex )
{
	HANDLE handles[] = {cv->signal, cv->broadcast};
	ReleaseMutex( *mutex );
	WaitForMultipleObjects( 2, handles, FALSE, INFINITE );
	return WaitForSingleObject( *mutex, INFINITE ) == WAIT_OBJECT_0 ? 0 : -1;
}

static int pthread_cond_signal( pthread_cond_t *cv )
{
	return SetEvent( cv->signal ) == 0 ? -1 : 0;
}

static int pthread_cond_broadcast( pthread_cond_t *cv )
{
	// Implementation with PulseEvent() has race condition, see
	// http://www.cs.wustl.edu/~schmidt/win32-cv-1.html
	return PulseEvent( cv->broadcast ) == 0 ? -1 : 0;
}

static int pthread_cond_destroy( pthread_cond_t *cv )
{
	return CloseHandle( cv->signal ) && CloseHandle( cv->broadcast ) ? 0 : -1;
}

#    define set_close_on_exec(fd) // No FD_CLOEXEC on Windows

static int mg_start_thread( mg_thread_func_t f, void *p )
{
	return _beginthread( (void (__cdecl *) (void *) ) f, 0, p ) == -1L ? -1 : 0;
}

#    ifndef NO_SSL

static HANDLE dlopen( const char *dll_name, int flags )
{
	flags = 0; // Unused
	return LoadLibraryA( dll_name );
}
#    endif // NO_SSL

static int set_non_blocking_mode( SOCKET sock )
{
	unsigned long on = 1;
	return ioctlsocket( sock, FIONBIO, &on );
}

#else

static void set_close_on_exec( int fd )
{
	fcntl( fd, F_SETFD, FD_CLOEXEC );
}

static int mg_start_thread( mg_thread_func_t func, void *param )
{
	pthread_t thread_id;
	pthread_attr_t attr;

	pthread_attr_init( &attr );
	pthread_attr_setdetachstate( &attr, PTHREAD_CREATE_DETACHED );
	// TODO(lsm): figure out why mongoose dies on Linux if next line is enabled
	// pthread_attr_setstacksize(&attr, sizeof(mg_connection) * 5);

	return pthread_create( &thread_id, &attr, func, param );
}

static int set_non_blocking_mode( SOCKET sock )
{
	int flags;

	flags = fcntl( sock, F_GETFL, 0 );
	fcntl( sock, F_SETFL, flags | O_NONBLOCK );

	return 0;
}
#endif // _WIN32

// This function is needed to prevent Mongoose to be stuck in a blocking
// socket read when user requested exit. To do that, we sleep in select
// with a timeout, and when returned, check the context for the stop flag.
// If it is set, we return 0, and this means that we must not continue
// reading, must give up and close the connection and exit serving thread.

static int wait_until_socket_is_readable( mg_connection* conn )
{
	int result;
	struct timeval tv;
	fd_set set;

	do
	{
		tv.tv_sec = 0;
		tv.tv_usec = 300 * 1000;
		FD_ZERO( &set );
		FD_SET( conn->client.sock, &set );
		result = select( conn->client.sock + 1, &set, NULL, NULL, &tv );
	}
	while ( ( result == 0 || ( result < 0 && ERRNO == EINTR ) ) &&
			conn->ctx->stop_flag == 0 );

	return conn->ctx->stop_flag || result < 0 ? 0 : 1;
}

// Read from IO channel - opened file descriptor, socket, or SSL descriptor.
// Return negative value on error, or number of bytes read on success.

static int pull( mg_connection *conn, char *buf, int len )
{
	int nread;

	if ( !wait_until_socket_is_readable( conn ) )
	{
		nread = -1;
#ifndef NO_SSL	
	}
	else if ( conn->ssl != NULL )
	{
		nread = SSL_read( (SSL*) conn->ssl, buf, len );
#endif // NO_SSL	
	}
	else
	{
		nread = recv( conn->client.sock, buf, (size_t) len, 0 );
	}

	return conn->ctx->stop_flag ? -1 : nread;
}

/**********************************************************************************************/
int mg_read( mg_connection *conn, void *buf, size_t len )
{
	int n, buffered_len, nread;
	const char *body;

	nread = 0;
	if ( conn->consumed_content < conn->content_len )
	{
		// Adjust number of bytes to read.
		int to_read = conn->content_len - conn->consumed_content;
		if( to_read < (int) len )
			len = (size_t) to_read;

		// Return buffered data
		body = conn->buf + conn->request_len + conn->consumed_content;
		buffered_len = &conn->buf[conn->data_len] - body;
		if ( buffered_len > 0 )
		{
			if ( len < (size_t) buffered_len )
				buffered_len = (int) len;

			memmove( buf, body, (size_t) buffered_len );
			len -= buffered_len;
			conn->consumed_content += buffered_len;
			nread += buffered_len;
			buf = (char *) buf + buffered_len;
		}

		// We have returned all buffered data. Read new data from the remote socket.
		while( len > 0 )
		{
			n = pull( conn, (char *) buf, (int) len );
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
				buf = (char *) buf + n;
				conn->consumed_content += n;
				nread += n;
				len -= n;
			}
		}
	}
	return nread;
}

/**********************************************************************************************/
int mg_write( mg_connection* conn, const char* buf, size_t len )
{
	int sent;
	int n, k;

	sent = 0;
	while( sent < (int) len )
	{
		// How many bytes we send in this iteration
		k = (int) ( len - sent );

#ifndef NO_SSL	
		if ( conn->ssl )
			n = SSL_write( (SSL*) conn->ssl, buf + sent, k );
		else
#endif // NO_SSL		
			n = send( conn->client.sock, buf + sent, (size_t) k, MSG_NOSIGNAL );

		if ( n < 0 )
			break;

		sent += n;
	}

	return sent;
}

// URL-decode input buffer into destination buffer.
// 0-terminate the destination buffer. Return the length of decoded data.
// form-url-encoded data differs from URI encoding in a way that it
// uses '+' as character for space, see RFC 1866 section 8.2.1
// http://ftp.ics.uci.edu/pub/ietf/html/rfc1866.txt

static size_t url_decode( char* buf, size_t len )
{
	size_t i, j;
	int a, b;
#define HEXTOI(x) (isdigit(x) ? x - '0' : x - 'W')

	for ( i = j = 0; i < len; i++, j++ )
	{
		if ( buf[i] == '%' &&
			 isxdigit( * (const unsigned char *) ( buf + i + 1 ) ) &&
			 isxdigit( * (const unsigned char *) ( buf + i + 2 ) ) )
		{
			a = tolower( * (const unsigned char *) ( buf + i + 1 ) );
			b = tolower( * (const unsigned char *) ( buf + i + 2 ) );
			buf[j] = (char) ( ( HEXTOI( a ) << 4 ) | HEXTOI( b ) );
			i += 2;
		}
		else
		{
			if( j != i )
				buf[j] = buf[i];
		}
	}

	buf[j] = '\0'; // Null-terminate the destination
	return j;
}

#ifndef NO_SSL

/**********************************************************************************************/
static int sslize( mg_connection* conn, SSL_CTX* s, int (*func )( SSL * ) )
{
	return
	(conn->ssl = SSL_new( s ) ) != NULL &&
		SSL_set_fd( (SSL*) conn->ssl, conn->client.sock ) == 1 &&
		func( (SSL*) conn->ssl ) == 1;
}

#endif // NO_SSL

// Check whether full request is buffered. Return:
//   -1  if request is malformed
//	0  if request is not yet fully buffered
//   >0  actual request length, including last \r\n\r\n

static int get_request_len( const char *buf, int buflen )
{
	const char *s, *e;
	int len = 0;

	for ( s = buf, e = s + buflen - 1; len <= 0 && s < e; s++ )
		// Control characters are not allowed but >=128 is.
		if ( !isprint( * (const unsigned char *) s ) && *s != '\r' &&
			 *s != '\n' && * (const unsigned char *) s < 128 )
		{
			len = -1;
			break; // [i_a] abort scan as soon as one malformed character is found; don't let subsequent \r\n\r\n win us over anyhow
		}
		else if ( s[0] == '\n' && s[1] == '\n' )
		{
			len = (int) ( s - buf ) + 2;
		}
		else if ( s[0] == '\n' && &s[1] < e &&
		  s[1] == '\r' && s[2] == '\n' )
		{
			len = (int) ( s - buf ) + 3;
		}

	return len;
}

// Parse HTTP headers from the given buffer, advance buffer to the point
// where parsing stopped.

static void parse_http_headers( char **buf, mg_request_info *ri )
{
	for( int i = 0 ; i < 64 ; ++i )
	{
		char* name = skip_quoted( buf, ":", " " );
		char* value = skip( buf, "\r\n" );
		
		if( name && *name )
		{
			ri->headers_.add( name, value );
			while( *name )
			{
				*name = tolower( *name );
				++name;
			}
		}
		else
		{
			break;
		}
	}
}

// Parse HTTP request, fill in mg_request_info structure.
// This function modifies the buffer by NUL-terminating
// HTTP request components, header names and header values.

static int parse_http_message( char *buf, int len, mg_request_info *ri )
{
	int request_length = get_request_len( buf, len );
	if ( request_length > 0 )
	{
		// Reset attributes. DO NOT TOUCH is_ssl, remote_ip, remote_port
		ri->method_ = ri->uri_ = NULL;
		ri->headers_.reset();

		buf[request_length - 1] = '\0';

		// RFC says that all initial whitespaces should be ingored
		while ( *buf != '\0' && isspace( * (unsigned char *) buf ) )
		{
			buf++;
		}
		ri->method_ = skip( &buf, " " );
		ri->uri_ = skip( &buf, " " );
		skip( &buf, "\r\n" );
		parse_http_headers( &buf, ri );
	}
	return request_length;
}

// Keep reading the input (either opened file descriptor fd, or socket sock,
// or SSL descriptor ssl) into buffer buf, until \r\n\r\n appears in the
// buffer (which marks the end of HTTP request). Buffer buf may already
// have some data. The length of the data is stored in nread.
// Upon every read operation, increase nread by the number of bytes read.

static int read_request( 
	mg_connection*	conn,
	char*			buf,
	int				bufsiz,
	int*			nread )
{
	int request_len, n = 1;

	request_len = get_request_len( buf, *nread );
	while ( *nread < bufsiz && request_len == 0 && n > 0 )
	{
		n = pull( conn, buf + *nread, bufsiz - *nread );
		if ( n > 0 )
		{
			*nread += n;
			request_len = get_request_len( buf, *nread );
		}
	}

	if ( n < 0 )
	{
		// recv() error -> propagate error; do not process a b0rked-with-very-high-probability request
		return -1;
	}
	return request_len;
}

/**********************************************************************************************/
void event_handler( mg_connection* conn );

// This is the heart of the Mongoose's logic.
// This function is called when the request is read, parsed and validated,
// and Mongoose must decide what action to take: serve a file, or
// a directory, or call embedded function, etcetera.

static void handle_request( mg_connection *conn )
{
	mg_request_info *ri = &conn->request_info;
	int uri_len;

	if ( ( conn->request_info.query_parameters_ = strchr( (char*) ri->uri_, '?' ) ) != NULL )
	{
		* ( (char *) conn->request_info.query_parameters_++ ) = '\0';
	}
	uri_len = (int) strlen( ri->uri_ );
	url_decode( ri->uri_, uri_len );

	event_handler( conn );
}

static void close_all_listening_sockets( mg_context *ctx )
{
	mg_socket *sp, *tmp;
	for ( sp = ctx->listening_sockets; sp ; sp = tmp )
	{
		tmp = sp->next;
		closesocket( sp->sock );
		free( sp );
	}
}

// Valid listening port specification is: [ip_address:]port[s]
// Examples: 80, 443s, 127.0.0.1:3128, 1.2.3.4:8080s
// TODO(lsm): add parsing of the IPv6 address

static int parse_port_string( const vec *vec, mg_socket *so )
{
	int a, b, c, d, port, len;

	// MacOS needs that. If we do not zero it, subsequent bind() will fail.
	// Also, all-zeroes in the socket address means binding to all addresses
	// for both IPv4 and IPv6 (INADDR_ANY and IN6ADDR_ANY_INIT).
	memset( so, 0, sizeof(*so ) );

	if ( sscanf( vec->ptr, "%d.%d.%d.%d:%d%n", &a, &b, &c, &d, &port, &len ) == 5 )
	{
		// Bind to a specific IPv4 address
		so->lsa.sin.sin_addr.s_addr = htonl( ( a << 24 ) | ( b << 16 ) | ( c << 8 ) | d );
	}
	else if ( sscanf( vec->ptr, "%d%n", &port, &len ) != 1 ||
		  len <= 0 ||
		  len > (int) vec->len ||
		  ( vec->ptr[len] && vec->ptr[len] != 's' && vec->ptr[len] != ',' ) )
	{
		return 0;
	}

#ifndef NO_SSL  
	so->is_ssl = vec->ptr[len] == 's';
#endif // NO_SSL

#if defined(USE_IPV6)
	so->lsa.sin6.sin6_family = AF_INET6;
	so->lsa.sin6.sin6_port = htons( (uint16_t) port );
#else
	so->lsa.sin.sin_family = AF_INET;
	so->lsa.sin.sin_port = htons( (uint16_t) port );
#endif

	return 1;
}

static const char* next_option( const char* str, vec& vc )
{
	if( !str || !*str )
		return NULL;

	vc.ptr = str;
	const char* sp = strchr( str, ',' );
	return sp ?
		( vc.len = sp - str ), ++sp :
		str + ( vc.len = strlen( str ) );
}

static int set_ports_option( mg_context* ctx, const char* list, const char* pem_file )
{
	int on = 1, success = 1;
	SOCKET sock;
	vec vc;
	mg_socket so, *listener;

#ifdef NO_SSL
	(void) pem_file;
#endif // NO_SSL

	while ( success && ( list = next_option( list, vc ) ) != NULL )
	{
		if ( !parse_port_string( &vc, &so ) )
		{
			error_string = "Invalid port spec. Expecting list of: [IP_ADDRESS:]PORT[s|p]";
			success = 0;
#ifndef NO_SSL	  
		}
		else if ( so.is_ssl &&
		  ( ctx->ssl_ctx == NULL || pem_file == NULL ) )
		{
			error_string = "Cannot add SSL socket, is ssl certificate option set?";
			success = 0;
#endif // NO_SSL	  
		}
		else if ( ( sock = socket( so.lsa.sa.sa_family, SOCK_STREAM, 6 ) ) ==
		  INVALID_SOCKET ||
		  // On Windows, SO_REUSEADDR is recommended only for
		  // broadcast UDP sockets
		  setsockopt( sock, SOL_SOCKET, SO_REUSEADDR, (const char *) &on,
					  sizeof(on ) ) != 0 ||
		  // Set TCP keep-alive. This is needed because if HTTP-level
		  // keep-alive is enabled, and client resets the connection,
		  // server won't get TCP FIN or RST and will keep the connection
		  // open forever. With TCP keep-alive, next keep-alive
		  // handshake will figure out that the client is down and
		  // will close the server end.
		  // Thanks to Igor Klopov who suggested the patch.
		  setsockopt( sock, SOL_SOCKET, SO_KEEPALIVE, (char *) &on,
					  sizeof(on ) ) != 0 ||
		  bind( sock, &so.lsa.sa, sizeof(so.lsa ) ) != 0 ||
		  listen( sock, SOMAXCONN ) != 0 )
		{
			closesocket( sock );
			error_string = "cannot bind socket";
			success = 0;
		}
		else
		{
			listener = (mg_socket *) calloc( 1, sizeof(*listener ) );
			*listener = so;
			listener->sock = sock;
			set_close_on_exec( listener->sock );
			listener->next = ctx->listening_sockets;
			ctx->listening_sockets = listener;
		}
	}

	if ( !success )
	{
		close_all_listening_sockets( ctx );
	}

	return success;
}

static void add_to_set( SOCKET fd, fd_set *set, int *max_fd )
{
	FD_SET( fd, set );
	if ( fd > ( SOCKET ) * max_fd )
		*max_fd = (int) fd;
}

#ifndef NO_SSL
static pthread_mutex_t *ssl_mutexes;

static void ssl_locking_callback( int mode, int mutex_num, const char *file,
								  int line )
{
	(void) line;
	(void) file;

	if ( mode & CRYPTO_LOCK )
	{
		pthread_mutex_lock( &ssl_mutexes[mutex_num] );
	}
	else
	{
		pthread_mutex_unlock( &ssl_mutexes[mutex_num] );
	}
}

static unsigned long ssl_id_callback( void )
{
	return (unsigned long) pthread_self( );
}

static int load_dll( const char *dll_name,
					 struct ssl_func *sw )
{

	union
	{
		void *p;
		void (*fp )(void) ;
	} u;
	void  *dll_handle;
	struct ssl_func *fp;

	if ( ( dll_handle = dlopen( dll_name, RTLD_LAZY ) ) == NULL )
	{
		error_string = "cannot load ssl dll";
		return 0;
	}

	for ( fp = sw; fp->name != NULL; fp++ )
	{
#    ifdef _WIN32
		// GetProcAddress() returns pointer to function
		u.fp = (void (* )(void) ) dlsym( dll_handle, fp->name );
#    else
		// dlsym() on UNIX returns void *. ISO C forbids casts of data pointers to
		// function pointers. We need to use a union to make a cast.
		u.p = dlsym( dll_handle, fp->name );
#    endif // _WIN32
		if ( u.fp == NULL )
		{
			error_string = "cannot find ssl symbol";
			return 0;
		}
		else
		{
			fp->ptr = u.fp;
		}
	}

	return 1;
}

// Dynamically load SSL library. Set up ctx->ssl_ctx pointer.

static int set_ssl_option( mg_context *ctx, const char* pem )
{
	int i, size;

	// If PEM file is not specified, skip SSL initialization.
	if ( pem == NULL || !*pem )
	{
		return 1;
	}

	if ( !load_dll( SSL_LIB, ssl_sw ) ||
		 !load_dll( CRYPTO_LIB, crypto_sw ) )
	{
		return 0;
	}

	// Initialize SSL crap
	SSL_library_init( );

	if ( ( ctx->client_ssl_ctx = SSL_CTX_new( SSLv23_client_method( ) ) ) == NULL )
	{
		error_string = "SSL_CTX_new (client) error";
	}

	if ( ( ctx->ssl_ctx = SSL_CTX_new( SSLv23_server_method( ) ) ) == NULL )
	{
		error_string = "SSL_CTX_new (server) error";
		return 0;
	}

	// If user callback returned non-NULL, that means that user callback has
	// set up certificate itself. In this case, skip sertificate setting.
	if ( ( SSL_CTX_use_certificate_file( ctx->ssl_ctx, pem, SSL_FILETYPE_PEM ) == 0 ||
		   SSL_CTX_use_PrivateKey_file( ctx->ssl_ctx, pem, SSL_FILETYPE_PEM ) == 0 ) )
	{
		error_string = "cannot open pem";
		return 0;
	}

	if ( pem != NULL )
	{
		SSL_CTX_use_certificate_chain_file( ctx->ssl_ctx, pem );
	}

	// Initialize locking callbacks, needed for thread safety.
	// http://www.openssl.org/support/faq.html#PROG1
	size = sizeof(pthread_mutex_t ) * CRYPTO_num_locks( );
	ssl_mutexes = (pthread_mutex_t*) malloc( (size_t) size );

	for ( i = 0; i < CRYPTO_num_locks( ); i++ )
	{
		pthread_mutex_init( &ssl_mutexes[i], NULL );
	}

	CRYPTO_set_locking_callback( &ssl_locking_callback );
	CRYPTO_set_id_callback( &ssl_id_callback );

	return 1;
}

static void uninitialize_ssl( mg_context *ctx )
{
	int i;
	if ( ctx->ssl_ctx != NULL )
	{
		CRYPTO_set_locking_callback( NULL );
		for ( i = 0; i < CRYPTO_num_locks( ); i++ )
		{
			pthread_mutex_destroy( &ssl_mutexes[i] );
		}
		CRYPTO_set_locking_callback( NULL );
		CRYPTO_set_id_callback( NULL );
	}
}
#endif // NO_SSL

static void reset_per_request_attributes( mg_connection *conn )
{
	conn->consumed_content = 0;
	conn->must_close = conn->request_len = 0;
}

static void close_socket_gracefully( mg_connection *conn )
{
	char buf[ 8192 ];
	struct linger linger;
	int n, sock = conn->client.sock;

	// Set linger option to avoid socket hanging out after close. This prevent
	// ephemeral port exhaust problem under high QPS.
	linger.l_onoff = 1;
	linger.l_linger = 1;
	setsockopt( sock, SOL_SOCKET, SO_LINGER, (char *) &linger, sizeof(linger ) );

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
		n = pull( conn, buf, sizeof(buf ) );
	}
	while ( n > 0 );

	// Now we know that our FIN is ACK-ed, safe to close
	closesocket( sock );
}

static void close_connection( mg_connection *conn )
{
#ifndef NO_SSL	
	if ( conn->ssl )
	{
		SSL_free( (SSL*) conn->ssl );
		conn->ssl = NULL;
	}
#endif // NO_SSL

	if ( conn->client.sock != INVALID_SOCKET )
	{
		close_socket_gracefully( conn );
	}
}

static void process_new_connection( mg_connection *conn )
{
	mg_request_info *ri = &conn->request_info;
	int discard_len;
	const char *cl;
	
	do
	{
		reset_per_request_attributes( conn );
		conn->request_len = read_request( conn, conn->buf, MAX_REQUEST_SIZE,
										  &conn->data_len );
		if( conn->request_len == 0 && conn->data_len == MAX_REQUEST_SIZE )
		{
			mg_write( conn, "HTTP/1.1 413 Request Entity Too Large\r\nContent-Length: 0\r\n\r\n", 60 );
			return;
		}
		
		if( conn->request_len <= 0 )
			return;  // Remote end closed the connection
		
		if( parse_http_message( conn->buf, MAX_REQUEST_SIZE, ri ) <= 0 || *ri->uri_ != '/' )
		{
			mg_write( conn, "HTTP/1.1 400 Bad Request Too Large\r\nContent-Length: 0\r\n\r\n", 57 );
			conn->must_close = 1;
		}
		else
		{
			// Request is valid, handle it
			if ( ( cl = get_header( ri, "content-length" ) ) != NULL )
				conn->content_len = strtol( cl, NULL, 10 );
			else if ( !strcmp( ri->method_, "POST" ) || !strcmp( ri->method_, "PUT" ) )
				conn->content_len = -1;
			else
				conn->content_len = 0;
			
			conn->birth_time = time( NULL );
			handle_request( conn );
		}

		// Discard all buffered data for this request
		discard_len = conn->content_len >= 0 &&
			conn->request_len + conn->content_len < (int) conn->data_len ?
			(int) ( conn->request_len + conn->content_len ) : conn->data_len;
		memmove( conn->buf, conn->buf + discard_len, conn->data_len - discard_len );
		conn->data_len -= discard_len;
	}
	while ( conn->ctx->stop_flag == 0 &&
			conn->content_len >= 0 &&
			should_keep_alive( conn ) );
}

// Worker threads take accepted socket from the queue

static int consume_socket( mg_context *ctx, mg_socket *sp )
{
	pthread_mutex_lock( &ctx->mutex );

	// If the queue is empty, wait. We're idle at this point.
	while( ctx->sq_head == ctx->sq_tail && ctx->stop_flag == 0 )
		pthread_cond_wait( &ctx->sq_full, &ctx->mutex );

	// If we're stopping, sq_head may be equal to sq_tail.
	if ( ctx->sq_head > ctx->sq_tail )
	{
		// Copy socket from the queue and increment tail
		*sp = ctx->queue[ctx->sq_tail % ARRAY_SIZE( ctx->queue )];
		ctx->sq_tail++;

		// Wrap pointers if needed
		while ( ctx->sq_tail > (int) ARRAY_SIZE( ctx->queue ) )
		{
			ctx->sq_tail -= ARRAY_SIZE( ctx->queue );
			ctx->sq_head -= ARRAY_SIZE( ctx->queue );
		}
	}

	pthread_cond_signal( &ctx->sq_empty );
	pthread_mutex_unlock( &ctx->mutex );

	return !ctx->stop_flag;
}

static void worker_thread( mg_context *ctx )
{
	mg_connection conn;
	memset( &conn, 0, sizeof conn );

	char buf[ MAX_REQUEST_SIZE ];
	conn.buf = buf;
	memset( buf, 0, MAX_REQUEST_SIZE );

	// Call consume_socket() even when ctx->stop_flag > 0, to let it signal
	// sq_empty condvar to wake up the master waiting in produce_socket()
	while ( consume_socket( ctx, &conn.client ) )
	{
		conn.birth_time = time( NULL );
		conn.ctx = ctx;

		// Fill in IP, port info early so even if SSL setup below fails,
		// error handler would have the corresponding info.
		// Thanks to Johannes Winkelmann for the patch.
		// TODO(lsm): Fix IPv6 case
		conn.request_info.remote_port_ = ntohs( conn.client.rsa.sin.sin_port );
		memmove( &conn.request_info.remote_ip_,
				&conn.client.rsa.sin.sin_addr.s_addr, 4 );
		conn.request_info.remote_ip_ = ntohl( conn.request_info.remote_ip_ );

#ifndef NO_SSL	
		conn.request_info.is_ssl_ = conn.client.is_ssl;

		if ( !conn.client.is_ssl ||
			 ( conn.client.is_ssl &&
			   sslize( &conn, conn.ctx->ssl_ctx, SSL_accept ) ) )
#endif // NO_SSL		
			process_new_connection( &conn );

		close_connection( &conn );
	}

	// Signal master that we're done with connection and exiting
	pthread_mutex_lock( &ctx->mutex );
	ctx->num_threads--;
	pthread_cond_signal( &ctx->cond );
	pthread_mutex_unlock( &ctx->mutex );
}

// Master thread adds accepted socket to a queue

static void produce_socket( mg_context *ctx, const mg_socket *sp )
{
	pthread_mutex_lock( &ctx->mutex );

	// If the queue is full, wait
	while ( ctx->stop_flag == 0 &&
			ctx->sq_head - ctx->sq_tail >= (int) ARRAY_SIZE( ctx->queue ) )
	{
		pthread_cond_wait( &ctx->sq_empty, &ctx->mutex );
	}

	if ( ctx->sq_head - ctx->sq_tail < (int) ARRAY_SIZE( ctx->queue ) )
	{
		// Copy socket to the queue and increment head
		ctx->queue[ctx->sq_head % ARRAY_SIZE( ctx->queue )] = *sp;
		ctx->sq_head++;
	}

	pthread_cond_signal( &ctx->sq_full );
	pthread_mutex_unlock( &ctx->mutex );
}

static void accept_new_connection( const mg_socket *listener,
								   mg_context *ctx )
{
	mg_socket accepted;
	socklen_t len;

	len = sizeof(accepted.rsa );
	accepted.lsa = listener->lsa;
	accepted.sock = accept( listener->sock, &accepted.rsa.sa, &len );
	if ( accepted.sock != INVALID_SOCKET )
	{
		// Put accepted socket structure into the queue
#ifndef NO_SSL	  
		accepted.is_ssl = listener->is_ssl;
#endif // NO_SSL
		produce_socket( ctx, &accepted );
	}
}

static void master_thread( mg_context *ctx )
{
	fd_set read_set;
	struct timeval tv;
	mg_socket *sp;
	int max_fd;

	// Increase priority of the master thread
#if defined(_WIN32)
	SetThreadPriority( GetCurrentThread( ), THREAD_PRIORITY_ABOVE_NORMAL );
#endif

	while ( ctx->stop_flag == 0 )
	{
		FD_ZERO( &read_set );
		max_fd = -1;

		// Add listening sockets to the read set
		for ( sp = ctx->listening_sockets; sp != NULL; sp = sp->next )
		{
			add_to_set( sp->sock, &read_set, &max_fd );
		}

		tv.tv_sec = 0;
		tv.tv_usec = 200 * 1000;

		if ( select( max_fd + 1, &read_set, NULL, NULL, &tv ) < 0 )
		{
#ifdef _WIN32
			// On windows, if read_set and write_set are empty,
			// select() returns "Invalid parameter" error
			// (at least on my Windows XP Pro). So in this case, we sleep here.
			mg_sleep_int( 1000 );
#endif // _WIN32
		}
		else
		{
			for ( sp = ctx->listening_sockets; sp != NULL; sp = sp->next )
			{
				if ( ctx->stop_flag == 0 && FD_ISSET( sp->sock, &read_set ) )
				{
					accept_new_connection( sp, ctx );
				}
			}
		}
	}

	// Stop signal received: somebody called mg_stop. Quit.
	close_all_listening_sockets( ctx );

	// Wakeup workers that are waiting for connections to handle.
	pthread_cond_broadcast( &ctx->sq_full );

	// Wait until all threads finish
	pthread_mutex_lock( &ctx->mutex );
	while ( ctx->num_threads > 0 )
	{
		pthread_cond_wait( &ctx->cond, &ctx->mutex );
	}
	pthread_mutex_unlock( &ctx->mutex );

	// All threads exited, no sync is needed. Destroy mutex and condvars
	pthread_mutex_destroy( &ctx->mutex );
	pthread_cond_destroy( &ctx->cond );
	pthread_cond_destroy( &ctx->sq_empty );
	pthread_cond_destroy( &ctx->sq_full );

#ifndef NO_SSL
	uninitialize_ssl( ctx );
#endif // NO_SSL

	// Signal mg_stop() that we're done.
	// WARNING: This must be the very last thing this
	// thread does, as ctx becomes invalid after this line.
	ctx->stop_flag = 2;
}

/**********************************************************************************************/
static void free_context( mg_context *ctx )
{
	// Deallocate SSL context
#ifndef NO_SSL	
	if ( ctx->ssl_ctx != NULL )
	{
		SSL_CTX_free( ctx->ssl_ctx );
	}
	if ( ctx->client_ssl_ctx != NULL )
	{
		SSL_CTX_free( ctx->client_ssl_ctx );
	}
	if ( ssl_mutexes != NULL )
	{
		free( ssl_mutexes );
		ssl_mutexes = NULL;
	}
#endif // NO_SSL

	// Deallocate context itself
	free( ctx );
}

/**********************************************************************************************/
void mg_stop( mg_context *ctx )
{
	ctx->stop_flag = 1;

	// Wait until mg_fini() stops
	while ( ctx->stop_flag != 2 )
	{
		mg_sleep_int( 10 );
	}
	free_context( ctx );

#if defined(_WIN32)
	WSACleanup( );
#endif // _WIN32
}

/**********************************************************************************************/
mg_context *mg_start(
	const char* ports,
	const char* pem_file,
	size_t		thread_count )
{
	mg_context *ctx;
	error_string = "";

#if defined(_WIN32)
	WSADATA data;
	WSAStartup( MAKEWORD( 2, 2 ), &data );
#endif // _WIN32

	// Allocate context and initialize reasonable general case defaults.
	// TODO(lsm): do proper error handling here.
	ctx = (mg_context *) calloc( 1, sizeof(*ctx ) );

	// NOTE(lsm): order is important here. SSL certificates must
	// be initialized before listening ports. UID must be set last.
	if (
#ifndef NO_SSL	  
		!set_ssl_option( ctx, pem_file ) ||
#endif // NO_SSL
		!set_ports_option( ctx, ports, pem_file )
		 )
	{
		free_context( ctx );
		return NULL;
	}

#if !defined(_WIN32)
	// Ignore SIGPIPE signal, so if browser cancels the request, it
	// won't kill the whole process.
	signal( SIGPIPE, SIG_IGN );
	// Also ignoring SIGCHLD to let the OS to reap zombies properly.
	signal( SIGCHLD, SIG_IGN );
#endif // !_WIN32

	pthread_mutex_init( &ctx->mutex, NULL );
	pthread_cond_init( &ctx->cond, NULL );
	pthread_cond_init( &ctx->sq_empty, NULL );
	pthread_cond_init( &ctx->sq_full, NULL );

	// Start master (listening) thread
	mg_start_thread( (mg_thread_func_t) master_thread, ctx );

	// Start worker threads
	for( size_t i = 0 ; i < thread_count ; i++ )
	{
		if ( mg_start_thread( (mg_thread_func_t) worker_thread, ctx ) == 0 )
			ctx->num_threads++;
	}

	return ctx;
}

/**********************************************************************************************/
mg_mutex mg_mutex_create( void )
{
	pthread_mutex_t* m = (pthread_mutex_t*) calloc( sizeof(pthread_mutex_t ), 1 );
	pthread_mutex_init( m, NULL );
	return m;
}

/**********************************************************************************************/
void mg_mutex_destroy( mg_mutex m )
{
	pthread_mutex_destroy( (pthread_mutex_t*) m );
	free( m );
}

/**********************************************************************************************/
void mg_mutex_lock( mg_mutex m )
{
	pthread_mutex_lock( (pthread_mutex_t*) m );
}

/**********************************************************************************************/
void mg_mutex_unlock( mg_mutex m )
{
	pthread_mutex_unlock( (pthread_mutex_t*) m );
}

/**********************************************************************************************/
void mg_sleep( int ms )
{
	mg_sleep_int( ms );
}

/**********************************************************************************************/
mg_context* mg_get_context( mg_connection* conn )
{
	return conn->ctx;
}

/**********************************************************************************************/
const char* mg_get_error_string( void )
{
	return error_string;
}

/**********************************************************************************************/
void mg_close_connection( mg_connection* conn )
{
	close_connection( conn );
	free( conn );
}

/**********************************************************************************************/
mg_connection* mg_connect(
	mg_context*	ctx,
	const char*	host,
	int			port,
	int			use_ssl )
{
	mg_connection* newconn = NULL;
	sockaddr_in sin;
	hostent* he;
	int sock;

#ifndef NO_SSL	
	if( use_ssl && (ctx == NULL || ctx->client_ssl_ctx == NULL) )
	{
	}
	else
#endif // NO_SSL	
	if( !( he = gethostbyname(host) ) )
	{
	}
	else if( ( sock = socket( PF_INET, SOCK_STREAM, 0 ) ) == INVALID_SOCKET )
	{
	}
	else
	{
		sin.sin_family = AF_INET;
		sin.sin_port = htons( (uint16_t) port );
		sin.sin_addr = *(in_addr*) he->h_addr_list[ 0 ];
		
		if( connect( sock, (sockaddr*) &sin, sizeof( sin ) ) )
		{
			closesocket( sock );
		}
		else
		{
			newconn = (mg_connection*) calloc( 1, sizeof(*newconn) );
			newconn->ctx = ctx;
			newconn->client.sock = sock;
			newconn->client.rsa.sin = sin;
			
#ifndef NO_SSL				
			newconn->client.is_ssl = use_ssl;
			if( use_ssl )
				sslize( newconn, ctx->client_ssl_ctx, SSL_connect );
#endif // NO_SSL			
		}
	}

	return newconn;
}

/**********************************************************************************************/
static int parse_http_response( char* buf, int len, mg_request_info* ri )
{
	int result = parse_http_message( buf, len, ri );
	return result > 0 && !strncmp( ri->method_, "HTTP/", 5 ) ? result : -1;
}

/**********************************************************************************************/
bool mg_fetch(
	char*		buf,
	char*&		out,
	size_t&		out_size,
	mg_context*	ctx,
	const char*	url,
	cr_headers*	headers,
	bool		redirected )
{
	out = NULL;
	
	mg_connection* conn;
	mg_request_info ri;
	
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

	if( !( conn = mg_connect( ctx, host, port, !strcmp( proto, "https" ) ) ) )
	{
	}
	else
	{
		char* str = buf;
		str = add_string( str, "GET /", 5 );
		str = add_string( str, url + n, strlen( url + n ) );
		str = add_string( str, " HTTP/1.0\r\nHost: ", 17 );
		str = add_string( str, host, strlen( host ) );
		str = add_string( str, "\r\n\r\n", 4 );
		mg_write( conn, buf, str - buf );
		
		int data_length = 0;
		int req_length = read_request( conn, buf, 8192, &data_length );
		if( req_length <= 0 )
		{
		}
		else if( parse_http_response( buf, req_length, &ri ) <= 0 )
		{
		}
		else
		{
			if( headers )
				*headers = ri.headers_;
			
			const char* location = get_header( &ri, "location" );
			if( location && !redirected )
			{
				mg_close_connection( conn );
				return mg_fetch( buf, out, out_size, ctx, location, headers, true );
			}
		
			// Write chunk of data that may be in the user's buffer
			data_length -= req_length;
			
			size_t msize = req_length + data_length;
			const char* cl = get_header( &ri, "content-length" );
			msize += cl ? strtol( cl, NULL, 10 ) : 32768;
			
			out = (char*) malloc( msize );
			str = out;
			
			if( data_length > 0 )
				str = add_string( str, buf + req_length, data_length );
			
			// Read the rest of the response and write it to the file. Do not use
			// mg_read() cause we didn't set newconn->content_len properly.
			char buf2[ 65536 ];
			while( ( data_length = pull( conn, buf2, sizeof( buf2 ) ) ) > 0 )
			{
				if( str - out + data_length + 1 > (int) msize )
				{
					msize += data_length + 16384;
					
					size_t diff = str - out;
					out = (char*) realloc( out, msize );
					str = out + diff;
				}
				
				str = add_string( str, buf2, data_length );
			}
			
			out_size = str - out;
		}
		
		mg_close_connection( conn );
	}
	
	return out != NULL;
}
