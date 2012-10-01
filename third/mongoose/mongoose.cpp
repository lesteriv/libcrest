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

#ifdef _WIN32
#define _CRT_SECURE_NO_WARNINGS // Disable deprecation warning in VS2005
#endif

#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>


#if defined(_WIN32) // Windows specific
#define _WIN32_WINNT 0x0400 // To make it link in VS2005
#include <windows.h>

#ifndef PATH_MAX
#define PATH_MAX MAX_PATH
#endif

#include <process.h>
#include <direct.h>
#include <io.h>

#define MAKEUQUAD(lo, hi) ((uint64_t)(((uint32_t)(lo)) | \
	  ((uint64_t)((uint32_t)(hi))) << 32))
#define RATE_DIFF 10000000 // 100 nsecs
#define EPOCH_DIFF MAKEUQUAD(0xd53e8000, 0x019db1de)

#define ERRNO   GetLastError()
#define NO_SOCKLEN_T
#define SSL_LIB   "ssleay32.dll"
#define CRYPTO_LIB  "libeay32.dll"
#define O_NONBLOCK  0
#if !defined(EWOULDBLOCK)
#define EWOULDBLOCK  WSAEWOULDBLOCK
#endif // !EWOULDBLOCK
#define _POSIX_

#define SHUT_WR 1
#define snprintf _snprintf
#define vsnprintf _vsnprintf
#define mg_sleep_int(x) Sleep(x)

#define pipe(x) _pipe(x, MG_BUF_LEN, _O_BINARY)
#define popen(x, y) _popen(x, y)
#define pclose(x) _pclose(x)
#define close(x) _close(x)
#define dlsym(x,y) GetProcAddress((HINSTANCE) (x), (y))
#define RTLD_LAZY  0
#define fseeko(x, y, z) _lseeki64(_fileno(x), (y), (z))
#define fdopen(x, y) _fdopen((x), (y))
#define write(x, y, z) _write((x), (y), (unsigned) z)
#define read(x, y, z) _read((x), (y), (unsigned) z)
#define sleep(x) Sleep((x) * 1000)

#if !defined(fileno)
#define fileno(x) _fileno(x)
#endif // !fileno MINGW #defines fileno

typedef HANDLE pthread_mutex_t;
typedef struct {HANDLE signal, broadcast;} pthread_cond_t;
typedef DWORD pthread_t;
#define pid_t HANDLE // MINGW typedefs pid_t to int. Using #define here.

static int pthread_mutex_lock(pthread_mutex_t *);
static int pthread_mutex_unlock(pthread_mutex_t *);

// Mark required libraries
#pragma comment(lib, "Ws2_32.lib")

#else	// UNIX  specific

#include <dlfcn.h>
#include <netinet/in.h>
#include <pthread.h>
#include <unistd.h>

#ifdef __MACH__
#define SSL_LIB "libssl.dylib"
#define CRYPTO_LIB "libcrypto.dylib"
#else // __MACH__
#define SSL_LIB "libssl.so"
#define CRYPTO_LIB "libcrypto.so"
#endif // __MACH__

#define closesocket(a) close(a)
#define mg_sleep_int(x) usleep((x) * 1000)
#define ERRNO errno
#define INVALID_SOCKET (-1)
typedef int SOCKET;

#endif // End of Windows and UNIX specific includes


#include "mongoose.h"

#define MG_BUF_LEN 8192
#define MAX_REQUEST_SIZE 16384
#define ARRAY_SIZE(array) (sizeof(array) / sizeof(array[0]))

#ifdef _WIN32
static pthread_t pthread_self(void) {
  return GetCurrentThreadId();
}
#endif // _WIN32

// Darwin prior to 7.0 and Win32 do not have socklen_t
#ifdef NO_SOCKLEN_T
typedef int socklen_t;
#endif // NO_SOCKLEN_T
#define _DARWIN_UNLIMITED_SELECT

#if !defined(MSG_NOSIGNAL)
#define MSG_NOSIGNAL 0
#endif

#if !defined(SOMAXCONN)
#define SOMAXCONN 100
#endif

#if !defined(PATH_MAX)
#define PATH_MAX 4096
#endif

// Snatched from OpenSSL includes. I put the prototypes here to be independent
// from the OpenSSL source installation. Having this, mongoose + SSL can be
// built on any system with binary SSL libraries installed.
typedef struct ssl_st SSL;
typedef struct ssl_method_st SSL_METHOD;
typedef struct ssl_ctx_st SSL_CTX;

#define SSL_ERROR_WANT_READ		2
#define SSL_ERROR_WANT_WRITE	3
#define SSL_FILETYPE_PEM		1
#define CRYPTO_LOCK				1


// Dynamically loaded SSL functionality
struct ssl_func
{
  const char* name;   // SSL function name
  void  (*ptr)(void); // Function pointer
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
#define SSL_CTX_set_default_passwd_cb		(* (void (*)(SSL_CTX *, mg_callback_t)) ssl_sw[13].ptr)
#define SSL_CTX_free						(* (void (*)(SSL_CTX *)) ssl_sw[14].ptr)
#define SSL_CTX_use_certificate_chain_file	(* (int (*)(SSL_CTX *, const char *)) ssl_sw[15].ptr)
#define SSLv23_client_method				(* (SSL_METHOD * (*)(void)) ssl_sw[16].ptr)

#define CRYPTO_num_locks					(* (int (*)(void)) crypto_sw[0].ptr)
#define CRYPTO_set_locking_callback			(* (void (*)(void (*)(int, int, const char *, int))) crypto_sw[1].ptr)
#define CRYPTO_set_id_callback				(* (void (*)(unsigned long (*)(void))) crypto_sw[2].ptr)


// set_ssl_option() function updates this array.
// It loads SSL library dynamically and changes NULLs to the actual addresses
// of respective functions. The macros above (like SSL_connect()) are really
// just calling these functions indirectly via the pointer.
static struct ssl_func ssl_sw[] =
{
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
static struct ssl_func crypto_sw[] =
{
	{ "CRYPTO_num_locks"					, NULL },
	{ "CRYPTO_set_locking_callback"			, NULL },
	{ "CRYPTO_set_id_callback"				, NULL },
	{ NULL									, NULL }
};

// Unified socket address. For IPv6 support, add IPv6 address structure
// in the union u.
union usa
{
	struct sockaddr sa;
	struct sockaddr_in sin;
#if defined(USE_IPV6)
	struct sockaddr_in6 sin6;
#endif
};

// Describes a string (chunk of memory).
struct vec
{
	const char*	ptr;
	size_t		len;
};

// Describes listening socket, or socket which was accept()-ed by the master
// thread and queued for future handling by the worker thread.
struct mg_socket
{
	mg_socket*	next;	// Linkage
	SOCKET		sock;   // Listening socket
	union usa	lsa;	// Local socket address
	union usa	rsa;	// Remote socket address
	bool		is_ssl; // Is socket SSL-ed
};

static const char* error_string;

struct mg_context
{
	SSL_CTX*		client_ssl_ctx;		// Client SSL context
	mg_socket*		listening_sockets;
	volatile int	num_threads;		// Number of threads
	mg_socket		queue[ 20 ];		// Accepted sockets
	pthread_cond_t	sq_empty;			// Signaled when socket is consumed
	pthread_cond_t	sq_full;			// Signaled when socket is produced
	volatile int	sq_head;			// Head of the socket queue
	volatile int	sq_tail;			// Tail of the socket queue
	SSL_CTX*		ssl_ctx;			// SSL context
	volatile int	stop_flag;			// Should we stop event loop

	pthread_mutex_t mutex;				// Protects (max|num)_threads
	pthread_cond_t  cond;				// Condvar for tracking workers terminations
};

/**********************************************************************************************/
struct mg_connection
{
	time_t			birth_time;			// Time when request was received
	mg_socket		client;				// Connected client
	mg_context*		ctx;
	mg_request_info	request_info;
	void*			ssl;				// SSL descriptor
	int				content_len;		// Content-Length header value
	int				consumed_content;   // How many bytes of content have been read
	char*			buf;				// Buffer for received data
	int				must_close;			// 1 if connection must be closed
	int				request_len;		// Size of the request + headers in a buffer
	int				data_len;			// Total size of data in a buffer
};

/**********************************************************************************************/
time_t mg_get_birth_time( mg_connection* conn )
{
	return conn->birth_time;
}

/**********************************************************************************************/
int mg_get_content_len( mg_connection* conn )
{
	return conn->content_len;
}

/**********************************************************************************************/
mg_request_info* mg_get_request_info( mg_connection* conn )
{
	return &conn->request_info;
}


// Skip the characters until one of the delimiters characters found.
// 0-terminate resulting word. Skip the delimiter and following whitespaces if any.
// Advance pointer to buffer to the next word. Return found 0-terminated word.
// Delimiters can be quoted with quotechar.
static char *skip_quoted(
	char**		buf,
	const char* delimiters,
	const char* whitespace,
	char		quotechar )
{
	char *p, *begin_word, *end_word, *end_whitespace;

	begin_word = *buf;
	end_word = begin_word + strcspn( begin_word, delimiters );

	// Check for quotechar
	if( end_word > begin_word )
	{
		p = end_word - 1;
		while( *p == quotechar )
		{
			// If there is anything beyond end_word, copy it
			if( *end_word == '\0' )
			{
				*p = '\0';
				break;
			}
			else
			{
				size_t end_off = strcspn(end_word + 1, delimiters);
				memmove (p, end_word, end_off + 1);
				p += end_off; // p must correspond to end_word - 1
				end_word += end_off + 1;
			}
		}
		
		for( p++ ; p < end_word ; p++ )
			*p = '\0';
	}

	if( *end_word == '\0')
	{
		*buf = end_word;
	}
	else
	{
		end_whitespace = end_word + 1 + strspn(end_word + 1, whitespace);
		for( p = end_word ; p < end_whitespace ; p++ )
			*p = '\0';

		*buf = end_whitespace;
	}

	return begin_word;
}

// Simplified version of skip_quoted without quote char
// and whitespace == delimiters
static char* skip( char** buf, const char* delimiters )
{
	return skip_quoted( buf, delimiters, delimiters, 0 );
}

// Return HTTP header value, or NULL if not found.
static const char* get_header(
	const mg_request_info*	ri,
	const char*				name )
{
	for( int i = 0; i < ri->headers_count_ ; i++ )
	{
		if( !strcmp( name, ri->headers_[ i ].name_ ) )
			return ri->headers_[ i ].value_;
	}

	return NULL;
}

const char *mg_get_header( mg_connection* conn, const char* name )
{
	return get_header( &conn->request_info, name );
}

// HTTP 1.1 assumes keep alive if "Connection:" header is not set
// This function must tolerate situations when connection info is not
// set up, for example if request parsing failed.
static int should_keep_alive(mg_connection *conn)
{
	if( conn->must_close )
		return 0;
	
	const char* header = mg_get_header( conn, "Connection" );
	if( conn->must_close || !header || strcmp( header, "Keep-Alive" ) )
		return 0;

	return 1;
}

typedef void * (*mg_thread_func_t)(void *);

#if defined(_WIN32)
static int pthread_mutex_init(pthread_mutex_t *mutex, void *unused) {
  unused = NULL;
  *mutex = CreateMutex(NULL, FALSE, NULL);
  return *mutex == NULL ? -1 : 0;
}

static int pthread_mutex_destroy(pthread_mutex_t *mutex) {
  return CloseHandle(*mutex) == 0 ? -1 : 0;
}

static int pthread_mutex_lock(pthread_mutex_t *mutex) {
  return WaitForSingleObject(*mutex, INFINITE) == WAIT_OBJECT_0? 0 : -1;
}

static int pthread_mutex_unlock(pthread_mutex_t *mutex) {
  return ReleaseMutex(*mutex) == 0 ? -1 : 0;
}

static int pthread_cond_init(pthread_cond_t *cv, const void *unused) {
  unused = NULL;
  cv->signal = CreateEvent(NULL, FALSE, FALSE, NULL);
  cv->broadcast = CreateEvent(NULL, TRUE, FALSE, NULL);
  return cv->signal != NULL && cv->broadcast != NULL ? 0 : -1;
}

static int pthread_cond_wait(pthread_cond_t *cv, pthread_mutex_t *mutex) {
  HANDLE handles[] = {cv->signal, cv->broadcast};
  ReleaseMutex(*mutex);
  WaitForMultipleObjects(2, handles, FALSE, INFINITE);
  return WaitForSingleObject(*mutex, INFINITE) == WAIT_OBJECT_0? 0 : -1;
}

static int pthread_cond_signal(pthread_cond_t *cv) {
  return SetEvent(cv->signal) == 0 ? -1 : 0;
}

static int pthread_cond_broadcast(pthread_cond_t *cv) {
  // Implementation with PulseEvent() has race condition, see
  // http://www.cs.wustl.edu/~schmidt/win32-cv-1.html
  return PulseEvent(cv->broadcast) == 0 ? -1 : 0;
}

static int pthread_cond_destroy(pthread_cond_t *cv) {
  return CloseHandle(cv->signal) && CloseHandle(cv->broadcast) ? 0 : -1;
}

#define set_close_on_exec(fd) // No FD_CLOEXEC on Windows

static int mg_start_thread(mg_thread_func_t f, void *p) {
  return _beginthread((void (__cdecl *)(void *)) f, 0, p) == -1L ? -1 : 0;
}

static HANDLE dlopen(const char *dll_name, int flags) {
  flags = 0; // Unused
  return LoadLibraryA(dll_name);
}

static int set_non_blocking_mode(SOCKET sock) {
  unsigned long on = 1;
  return ioctlsocket(sock, FIONBIO, &on);
}

#else

static void set_close_on_exec(int fd) {
  (void) fcntl(fd, F_SETFD, FD_CLOEXEC);
}

static int mg_start_thread(mg_thread_func_t func, void *param) {
  pthread_t thread_id;
  pthread_attr_t attr;

  (void) pthread_attr_init(&attr);
  (void) pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
  // TODO(lsm): figure out why mongoose dies on Linux if next line is enabled
  // (void) pthread_attr_setstacksize(&attr, sizeof(mg_connection) * 5);

  return pthread_create(&thread_id, &attr, func, param);
}

static int set_non_blocking_mode(SOCKET sock) {
  int flags;

  flags = fcntl(sock, F_GETFL, 0);
  (void) fcntl(sock, F_SETFL, flags | O_NONBLOCK);

  return 0;
}
#endif // _WIN32

// Write data to the IO channel - opened file descriptor, socket or SSL
// descriptor. Return number of bytes written.
static int push(FILE *fp, SOCKET sock, SSL *ssl, const char *buf,
					int len) {
  int sent;
  int n, k;

  sent = 0;
  while (sent < len) {

	// How many bytes we send in this iteration
	k = (int) (len - sent);

	if (ssl != NULL) {
	  n = SSL_write(ssl, buf + sent, k);
	} else if (fp != NULL) {
	  n = (int) fwrite(buf + sent, 1, (size_t) k, fp);
	  if (ferror(fp))
		n = -1;
	} else {
	  n = send(sock, buf + sent, (size_t) k, MSG_NOSIGNAL);
	}

	if (n < 0)
	  break;

	sent += n;
  }

  return sent;
}

// This function is needed to prevent Mongoose to be stuck in a blocking
// socket read when user requested exit. To do that, we sleep in select
// with a timeout, and when returned, check the context for the stop flag.
// If it is set, we return 0, and this means that we must not continue
// reading, must give up and close the connection and exit serving thread.
static int wait_until_socket_is_readable(mg_connection *conn) {
  int result;
  struct timeval tv;
  fd_set set;

  do {
	tv.tv_sec = 0;
	tv.tv_usec = 300 * 1000;
	FD_ZERO(&set);
	FD_SET(conn->client.sock, &set);
	result = select(conn->client.sock + 1, &set, NULL, NULL, &tv);
  } while ((result == 0 || (result < 0 && ERRNO == EINTR)) &&
		   conn->ctx->stop_flag == 0);

  return conn->ctx->stop_flag || result < 0 ? 0 : 1;
}

// Read from IO channel - opened file descriptor, socket, or SSL descriptor.
// Return negative value on error, or number of bytes read on success.
static int pull(FILE *fp, mg_connection *conn, char *buf, int len) {
  int nread;

  if (fp != NULL) {
	// Use read() instead of fread(), because if we're reading from the CGI
	// pipe, fread() may block until IO buffer is filled up. We cannot afford
	// to block and must pass all read bytes immediately to the client.
	nread = read(fileno(fp), buf, (size_t) len);
  } else if (!wait_until_socket_is_readable(conn)) {
	nread = -1;
  } else if (conn->ssl != NULL) {
	nread = SSL_read((SSL*)conn->ssl, buf, len);
  } else {
	nread = recv(conn->client.sock, buf, (size_t) len, 0);
  }

  return conn->ctx->stop_flag ? -1 : nread;
}

int mg_read(mg_connection *conn, void *buf, size_t len) {
  int n, buffered_len, nread;
  const char *body;

  nread = 0;
  if (conn->consumed_content < conn->content_len) {
	// Adjust number of bytes to read.
	int to_read = conn->content_len - conn->consumed_content;
	if (to_read < (int) len) {
	  len = (size_t) to_read;
	}

	// Return buffered data
	body = conn->buf + conn->request_len + conn->consumed_content;
	buffered_len = &conn->buf[conn->data_len] - body;
	if (buffered_len > 0) {
	  if (len < (size_t) buffered_len) {
		buffered_len = (int) len;
	  }
	  memcpy(buf, body, (size_t) buffered_len);
	  len -= buffered_len;
	  conn->consumed_content += buffered_len;
	  nread += buffered_len;
	  buf = (char *) buf + buffered_len;
	}

	// We have returned all buffered data. Read new data from the remote socket.
	while (len > 0) {
	  n = pull(NULL, conn, (char *) buf, (int) len);
	  if (n < 0) {
		nread = n;  // Propagate the error
		break;
	  } else if (n == 0) {
		break;  // No more data to read
	  } else {
		buf = (char *) buf + n;
		conn->consumed_content += n;
		nread += n;
		len -= n;
	  }
	}
  }
  return nread;
}

int mg_write(mg_connection *conn, const void *buf, size_t len) {
  return push(NULL, conn->client.sock, (SSL*)conn->ssl, (const char *) buf,
				 (int) len);
}

// URL-decode input buffer into destination buffer.
// 0-terminate the destination buffer. Return the length of decoded data.
// form-url-encoded data differs from URI encoding in a way that it
// uses '+' as character for space, see RFC 1866 section 8.2.1
// http://ftp.ics.uci.edu/pub/ietf/html/rfc1866.txt
static size_t url_decode(const char *src, size_t src_len, char *dst,
						 size_t dst_len, int is_form_url_encoded) {
  size_t i, j;
  int a, b;
#define HEXTOI(x) (isdigit(x) ? x - '0' : x - 'W')

  for (i = j = 0; i < src_len && j < dst_len - 1; i++, j++) {
	if (src[i] == '%' &&
		isxdigit(* (const unsigned char *) (src + i + 1)) &&
		isxdigit(* (const unsigned char *) (src + i + 2))) {
	  a = tolower(* (const unsigned char *) (src + i + 1));
	  b = tolower(* (const unsigned char *) (src + i + 2));
	  dst[j] = (char) ((HEXTOI(a) << 4) | HEXTOI(b));
	  i += 2;
	} else if (is_form_url_encoded && src[i] == '+') {
	  dst[j] = ' ';
	} else {
	  dst[j] = src[i];
	}
  }

  dst[j] = '\0'; // Null-terminate the destination

  return j;
}

static int sslize(mg_connection *conn, SSL_CTX *s, int (*func)(SSL *)) {
  return (conn->ssl = SSL_new(s)) != NULL &&
	SSL_set_fd((SSL*)conn->ssl, conn->client.sock) == 1 &&
	func((SSL*)conn->ssl) == 1;
}

// Check whether full request is buffered. Return:
//   -1  if request is malformed
//	0  if request is not yet fully buffered
//   >0  actual request length, including last \r\n\r\n
static int get_request_len(const char *buf, int buflen) {
  const char *s, *e;
  int len = 0;

  for (s = buf, e = s + buflen - 1; len <= 0 && s < e; s++)
	// Control characters are not allowed but >=128 is.
	if (!isprint(* (const unsigned char *) s) && *s != '\r' &&
		*s != '\n' && * (const unsigned char *) s < 128) {
	  len = -1;
	  break; // [i_a] abort scan as soon as one malformed character is found; don't let subsequent \r\n\r\n win us over anyhow
	} else if (s[0] == '\n' && s[1] == '\n') {
	  len = (int) (s - buf) + 2;
	} else if (s[0] == '\n' && &s[1] < e &&
		s[1] == '\r' && s[2] == '\n') {
	  len = (int) (s - buf) + 3;
	}

  return len;
}

struct MD5_CTX
{
  uint32_t buf[4];
  uint32_t bits[2];
  unsigned char in[64];
};

#if defined(__BYTE_ORDER) && (__BYTE_ORDER == 1234)
#define byteReverse(buf, len) // Do nothing
#else
static void byteReverse(unsigned char *buf, unsigned longs) {
  uint32_t t;
  do {
	t = (uint32_t) ((unsigned) buf[3] << 8 | buf[2]) << 16 |
	  ((unsigned) buf[1] << 8 | buf[0]);
	*(uint32_t *) buf = t;
	buf += 4;
  } while (--longs);
}
#endif

#define F1(x, y, z) (z ^ (x & (y ^ z)))
#define F2(x, y, z) F1(z, x, y)
#define F3(x, y, z) (x ^ y ^ z)
#define F4(x, y, z) (y ^ (x | ~z))

#define MD5STEP(f, w, x, y, z, data, s) \
  ( w += f(x, y, z) + data,  w = w<<s | w>>(32-s),  w += x )

// Start MD5 accumulation.  Set bit count to 0 and buffer to mysterious
// initialization constants.
static void MD5Init(MD5_CTX *ctx) {
  ctx->buf[0] = 0x67452301;
  ctx->buf[1] = 0xefcdab89;
  ctx->buf[2] = 0x98badcfe;
  ctx->buf[3] = 0x10325476;

  ctx->bits[0] = 0;
  ctx->bits[1] = 0;
}

static void MD5Transform(uint32_t buf[4], uint32_t const in[16]) {
  register uint32_t a, b, c, d;

  a = buf[0];
  b = buf[1];
  c = buf[2];
  d = buf[3];

  MD5STEP(F1, a, b, c, d, in[0] + 0xd76aa478, 7);
  MD5STEP(F1, d, a, b, c, in[1] + 0xe8c7b756, 12);
  MD5STEP(F1, c, d, a, b, in[2] + 0x242070db, 17);
  MD5STEP(F1, b, c, d, a, in[3] + 0xc1bdceee, 22);
  MD5STEP(F1, a, b, c, d, in[4] + 0xf57c0faf, 7);
  MD5STEP(F1, d, a, b, c, in[5] + 0x4787c62a, 12);
  MD5STEP(F1, c, d, a, b, in[6] + 0xa8304613, 17);
  MD5STEP(F1, b, c, d, a, in[7] + 0xfd469501, 22);
  MD5STEP(F1, a, b, c, d, in[8] + 0x698098d8, 7);
  MD5STEP(F1, d, a, b, c, in[9] + 0x8b44f7af, 12);
  MD5STEP(F1, c, d, a, b, in[10] + 0xffff5bb1, 17);
  MD5STEP(F1, b, c, d, a, in[11] + 0x895cd7be, 22);
  MD5STEP(F1, a, b, c, d, in[12] + 0x6b901122, 7);
  MD5STEP(F1, d, a, b, c, in[13] + 0xfd987193, 12);
  MD5STEP(F1, c, d, a, b, in[14] + 0xa679438e, 17);
  MD5STEP(F1, b, c, d, a, in[15] + 0x49b40821, 22);

  MD5STEP(F2, a, b, c, d, in[1] + 0xf61e2562, 5);
  MD5STEP(F2, d, a, b, c, in[6] + 0xc040b340, 9);
  MD5STEP(F2, c, d, a, b, in[11] + 0x265e5a51, 14);
  MD5STEP(F2, b, c, d, a, in[0] + 0xe9b6c7aa, 20);
  MD5STEP(F2, a, b, c, d, in[5] + 0xd62f105d, 5);
  MD5STEP(F2, d, a, b, c, in[10] + 0x02441453, 9);
  MD5STEP(F2, c, d, a, b, in[15] + 0xd8a1e681, 14);
  MD5STEP(F2, b, c, d, a, in[4] + 0xe7d3fbc8, 20);
  MD5STEP(F2, a, b, c, d, in[9] + 0x21e1cde6, 5);
  MD5STEP(F2, d, a, b, c, in[14] + 0xc33707d6, 9);
  MD5STEP(F2, c, d, a, b, in[3] + 0xf4d50d87, 14);
  MD5STEP(F2, b, c, d, a, in[8] + 0x455a14ed, 20);
  MD5STEP(F2, a, b, c, d, in[13] + 0xa9e3e905, 5);
  MD5STEP(F2, d, a, b, c, in[2] + 0xfcefa3f8, 9);
  MD5STEP(F2, c, d, a, b, in[7] + 0x676f02d9, 14);
  MD5STEP(F2, b, c, d, a, in[12] + 0x8d2a4c8a, 20);

  MD5STEP(F3, a, b, c, d, in[5] + 0xfffa3942, 4);
  MD5STEP(F3, d, a, b, c, in[8] + 0x8771f681, 11);
  MD5STEP(F3, c, d, a, b, in[11] + 0x6d9d6122, 16);
  MD5STEP(F3, b, c, d, a, in[14] + 0xfde5380c, 23);
  MD5STEP(F3, a, b, c, d, in[1] + 0xa4beea44, 4);
  MD5STEP(F3, d, a, b, c, in[4] + 0x4bdecfa9, 11);
  MD5STEP(F3, c, d, a, b, in[7] + 0xf6bb4b60, 16);
  MD5STEP(F3, b, c, d, a, in[10] + 0xbebfbc70, 23);
  MD5STEP(F3, a, b, c, d, in[13] + 0x289b7ec6, 4);
  MD5STEP(F3, d, a, b, c, in[0] + 0xeaa127fa, 11);
  MD5STEP(F3, c, d, a, b, in[3] + 0xd4ef3085, 16);
  MD5STEP(F3, b, c, d, a, in[6] + 0x04881d05, 23);
  MD5STEP(F3, a, b, c, d, in[9] + 0xd9d4d039, 4);
  MD5STEP(F3, d, a, b, c, in[12] + 0xe6db99e5, 11);
  MD5STEP(F3, c, d, a, b, in[15] + 0x1fa27cf8, 16);
  MD5STEP(F3, b, c, d, a, in[2] + 0xc4ac5665, 23);

  MD5STEP(F4, a, b, c, d, in[0] + 0xf4292244, 6);
  MD5STEP(F4, d, a, b, c, in[7] + 0x432aff97, 10);
  MD5STEP(F4, c, d, a, b, in[14] + 0xab9423a7, 15);
  MD5STEP(F4, b, c, d, a, in[5] + 0xfc93a039, 21);
  MD5STEP(F4, a, b, c, d, in[12] + 0x655b59c3, 6);
  MD5STEP(F4, d, a, b, c, in[3] + 0x8f0ccc92, 10);
  MD5STEP(F4, c, d, a, b, in[10] + 0xffeff47d, 15);
  MD5STEP(F4, b, c, d, a, in[1] + 0x85845dd1, 21);
  MD5STEP(F4, a, b, c, d, in[8] + 0x6fa87e4f, 6);
  MD5STEP(F4, d, a, b, c, in[15] + 0xfe2ce6e0, 10);
  MD5STEP(F4, c, d, a, b, in[6] + 0xa3014314, 15);
  MD5STEP(F4, b, c, d, a, in[13] + 0x4e0811a1, 21);
  MD5STEP(F4, a, b, c, d, in[4] + 0xf7537e82, 6);
  MD5STEP(F4, d, a, b, c, in[11] + 0xbd3af235, 10);
  MD5STEP(F4, c, d, a, b, in[2] + 0x2ad7d2bb, 15);
  MD5STEP(F4, b, c, d, a, in[9] + 0xeb86d391, 21);

  buf[0] += a;
  buf[1] += b;
  buf[2] += c;
  buf[3] += d;
}

static void MD5Update(MD5_CTX *ctx, unsigned char const *buf, unsigned len) {
  uint32_t t;

  t = ctx->bits[0];
  if ((ctx->bits[0] = t + ((uint32_t) len << 3)) < t)
	ctx->bits[1]++;
  ctx->bits[1] += len >> 29;

  t = (t >> 3) & 0x3f;

  if (t) {
	unsigned char *p = (unsigned char *) ctx->in + t;

	t = 64 - t;
	if (len < t) {
	  memcpy(p, buf, len);
	  return;
	}
	memcpy(p, buf, t);
	byteReverse(ctx->in, 16);
	MD5Transform(ctx->buf, (uint32_t *) ctx->in);
	buf += t;
	len -= t;
  }

  while (len >= 64) {
	memcpy(ctx->in, buf, 64);
	byteReverse(ctx->in, 16);
	MD5Transform(ctx->buf, (uint32_t *) ctx->in);
	buf += 64;
	len -= 64;
  }

  memcpy(ctx->in, buf, len);
}

static void MD5Final(unsigned char digest[16], MD5_CTX *ctx) {
  unsigned count;
  unsigned char *p;

  count = (ctx->bits[0] >> 3) & 0x3F;

  p = ctx->in + count;
  *p++ = 0x80;
  count = 64 - 1 - count;
  if (count < 8) {
	memset(p, 0, count);
	byteReverse(ctx->in, 16);
	MD5Transform(ctx->buf, (uint32_t *) ctx->in);
	memset(ctx->in, 0, 56);
  } else {
	memset(p, 0, count - 8);
  }
  byteReverse(ctx->in, 14);

  ((uint32_t *) ctx->in)[14] = ctx->bits[0];
  ((uint32_t *) ctx->in)[15] = ctx->bits[1];

  MD5Transform(ctx->buf, (uint32_t *) ctx->in);
  byteReverse((unsigned char *) ctx->buf, 4);
  memcpy(digest, ctx->buf, 16);
  memset((char *) ctx, 0, sizeof(*ctx));
}

// Return stringified MD5 hash for list of strings.
void mg_md5(char hash[16], const char* str) {
  MD5_CTX ctx;
  MD5Init(&ctx);
  MD5Update(&ctx, (const unsigned char *) str, (unsigned) strlen(str));
  MD5Update(&ctx, (const unsigned char *) str, (unsigned) strlen(str));
  MD5Final((unsigned char*)hash, &ctx);
}

// Parse HTTP headers from the given buffer, advance buffer to the point
// where parsing stopped.
static void parse_http_headers(char **buf, mg_request_info *ri) {
  int i;

  for (i = 0; i < (int) ARRAY_SIZE(ri->headers_); i++) {
	ri->headers_[i].name_ = skip_quoted(buf, ":", " ", 0);
	ri->headers_[i].value_ = skip(buf, "\r\n");
	if (ri->headers_[i].name_[0] == '\0')
	  break;
	ri->headers_count_ = i + 1;
  }
}

// Parse HTTP request, fill in mg_request_info structure.
// This function modifies the buffer by NUL-terminating
// HTTP request components, header names and header values.
static int parse_http_message(char *buf, int len, mg_request_info *ri) {
  int request_length = get_request_len(buf, len);
  if (request_length > 0) {
	// Reset attributes. DO NOT TOUCH is_ssl, remote_ip, remote_port
	ri->method_ = ri->uri_ = NULL;
	ri->headers_count_ = 0;

	buf[request_length - 1] = '\0';

	// RFC says that all initial whitespaces should be ingored
	while (*buf != '\0' && isspace(* (unsigned char *) buf)) {
	  buf++;
	}
	ri->method_ = skip(&buf, " ");
	ri->uri_ = skip(&buf, " ");
	skip(&buf, "\r\n");
	parse_http_headers(&buf, ri);
  }
  return request_length;
}

// Keep reading the input (either opened file descriptor fd, or socket sock,
// or SSL descriptor ssl) into buffer buf, until \r\n\r\n appears in the
// buffer (which marks the end of HTTP request). Buffer buf may already
// have some data. The length of the data is stored in nread.
// Upon every read operation, increase nread by the number of bytes read.
static int read_request(FILE *fp, mg_connection *conn,
						char *buf, int bufsiz, int *nread) {
  int request_len, n = 1;

  request_len = get_request_len(buf, *nread);
  while (*nread < bufsiz && request_len == 0 && n > 0) {
	n = pull(fp, conn, buf + *nread, bufsiz - *nread);
	if (n > 0) {
	  *nread += n;
	  request_len = get_request_len(buf, *nread);
	}
  }

  if (n < 0) {
	// recv() error -> propagate error; do not process a b0rked-with-very-high-probability request
	return -1;
  }
  return request_len;
}

void event_handler( mg_connection* conn );

// This is the heart of the Mongoose's logic.
// This function is called when the request is read, parsed and validated,
// and Mongoose must decide what action to take: serve a file, or
// a directory, or call embedded function, etcetera.
static void handle_request(mg_connection *conn) {
  mg_request_info *ri = &conn->request_info;
  int uri_len;

  if ((conn->request_info.query_parameters_ = strchr((char*)ri->uri_, '?')) != NULL) {
	* ((char *) conn->request_info.query_parameters_++) = '\0';
  }
  uri_len = (int) strlen(ri->uri_);
  url_decode(ri->uri_, (size_t)uri_len, (char *) ri->uri_,
			 (size_t) (uri_len + 1), 0);

  event_handler(conn);
}

static void close_all_listening_sockets(mg_context *ctx) {
  mg_socket *sp, *tmp;
  for (sp = ctx->listening_sockets; sp != NULL; sp = tmp) {
	tmp = sp->next;
	(void) closesocket(sp->sock);
	free(sp);
  }
}

// Valid listening port specification is: [ip_address:]port[s]
// Examples: 80, 443s, 127.0.0.1:3128, 1.2.3.4:8080s
// TODO(lsm): add parsing of the IPv6 address
static int parse_port_string(const struct vec *vec, mg_socket *so) {
  int a, b, c, d, port, len;

  // MacOS needs that. If we do not zero it, subsequent bind() will fail.
  // Also, all-zeroes in the socket address means binding to all addresses
  // for both IPv4 and IPv6 (INADDR_ANY and IN6ADDR_ANY_INIT).
  memset(so, 0, sizeof(*so));

  if (sscanf(vec->ptr, "%d.%d.%d.%d:%d%n", &a, &b, &c, &d, &port, &len) == 5) {
	// Bind to a specific IPv4 address
	so->lsa.sin.sin_addr.s_addr = htonl((a << 24) | (b << 16) | (c << 8) | d);
  } else if (sscanf(vec->ptr, "%d%n", &port, &len) != 1 ||
			 len <= 0 ||
			 len > (int) vec->len ||
			 (vec->ptr[len] && vec->ptr[len] != 's' && vec->ptr[len] != ',')) {
	return 0;
  }

  so->is_ssl = vec->ptr[len] == 's';
#if defined(USE_IPV6)
  so->lsa.sin6.sin6_family = AF_INET6;
  so->lsa.sin6.sin6_port = htons((uint16_t) port);
#else
  so->lsa.sin.sin_family = AF_INET;
  so->lsa.sin.sin_port = htons((uint16_t) port);
#endif

  return 1;
}

const char* next_option( const char* str, vec& vc )
{
	if( !str || !*str )
		return NULL;
	
	vc.ptr = str;
	const char* sp = strchr( str, ',' );
	return sp ?
		( vc.len = sp - str ), ++sp :
		str + ( vc.len = strlen( str ) );
}

static int set_ports_option(mg_context* ctx, const char* list, const char* pem_file) {
  int on = 1, success = 1;
  SOCKET sock;
  vec vc;
  mg_socket so, *listener;

  while (success && (list = next_option(list, vc)) != NULL) {
	if (!parse_port_string(&vc, &so)) {
	  error_string = "invalid port spec. Expecting list of: [IP_ADDRESS:]PORT[s|p]";
	  success = 0;
	} else if (so.is_ssl &&
			   (ctx->ssl_ctx == NULL || pem_file == NULL)) {
	  error_string = "Cannot add SSL socket, is ssl certificate option set?";
	  success = 0;
	} else if ((sock = socket(so.lsa.sa.sa_family, SOCK_STREAM, 6)) ==
			   INVALID_SOCKET ||
			   // On Windows, SO_REUSEADDR is recommended only for
			   // broadcast UDP sockets
			   setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (const char *) &on,
						  sizeof(on)) != 0 ||
			   // Set TCP keep-alive. This is needed because if HTTP-level
			   // keep-alive is enabled, and client resets the connection,
			   // server won't get TCP FIN or RST and will keep the connection
			   // open forever. With TCP keep-alive, next keep-alive
			   // handshake will figure out that the client is down and
			   // will close the server end.
			   // Thanks to Igor Klopov who suggested the patch.
			   setsockopt(sock, SOL_SOCKET, SO_KEEPALIVE, (char *) &on,
						  sizeof(on)) != 0 ||
			   bind(sock, &so.lsa.sa, sizeof(so.lsa)) != 0 ||
			   listen(sock, SOMAXCONN) != 0) {
	  closesocket(sock);
	  error_string = "cannot bind socket";
	  success = 0;
	} else {
	  listener = (mg_socket *) calloc(1, sizeof(*listener));
	  *listener = so;
	  listener->sock = sock;
	  set_close_on_exec(listener->sock);
	  listener->next = ctx->listening_sockets;
	  ctx->listening_sockets = listener;
	}
  }

  if (!success) {
	close_all_listening_sockets(ctx);
  }

  return success;
}

static void add_to_set(SOCKET fd, fd_set *set, int *max_fd) {
  FD_SET(fd, set);
  if (fd > (SOCKET) *max_fd) {
	*max_fd = (int) fd;
  }
}

static pthread_mutex_t *ssl_mutexes;

static void ssl_locking_callback(int mode, int mutex_num, const char *file,
								 int line) {
  (void) line;
  (void) file;

  if (mode & CRYPTO_LOCK) {
	(void) pthread_mutex_lock(&ssl_mutexes[mutex_num]);
  } else {
	(void) pthread_mutex_unlock(&ssl_mutexes[mutex_num]);
  }
}

static unsigned long ssl_id_callback(void) {
  return (unsigned long) pthread_self();
}

static int load_dll(const char *dll_name,
					struct ssl_func *sw) {
  union {void *p; void (*fp)(void);} u;
  void  *dll_handle;
  struct ssl_func *fp;

  if ((dll_handle = dlopen(dll_name, RTLD_LAZY)) == NULL) {
	error_string = "cannot load ssl dll";
	return 0;
  }

  for (fp = sw; fp->name != NULL; fp++) {
#ifdef _WIN32
	// GetProcAddress() returns pointer to function
	u.fp = (void (*)(void)) dlsym(dll_handle, fp->name);
#else
	// dlsym() on UNIX returns void *. ISO C forbids casts of data pointers to
	// function pointers. We need to use a union to make a cast.
	u.p = dlsym(dll_handle, fp->name);
#endif // _WIN32
	if (u.fp == NULL) {
	  error_string = "cannot find ssl symbol";
	  return 0;
	} else {
	  fp->ptr = u.fp;
	}
  }

  return 1;
}

// Dynamically load SSL library. Set up ctx->ssl_ctx pointer.
static int set_ssl_option(mg_context *ctx,const char* pem) {
  int i, size;
 
  // If PEM file is not specified, skip SSL initialization.
  if (pem == NULL || !*pem) {
	return 1;
  }

  if (!load_dll(SSL_LIB, ssl_sw) ||
	  !load_dll(CRYPTO_LIB, crypto_sw)) {
	return 0;
  }

  // Initialize SSL crap
  SSL_library_init();

  if ((ctx->client_ssl_ctx = SSL_CTX_new(SSLv23_client_method())) == NULL) {
	error_string = "SSL_CTX_new (client) error";
  }

  if ((ctx->ssl_ctx = SSL_CTX_new(SSLv23_server_method())) == NULL) {
	error_string = "SSL_CTX_new (server) error";
	return 0;
  }
 
  // If user callback returned non-NULL, that means that user callback has
  // set up certificate itself. In this case, skip sertificate setting.
  if ((SSL_CTX_use_certificate_file(ctx->ssl_ctx, pem, SSL_FILETYPE_PEM) == 0 ||
	   SSL_CTX_use_PrivateKey_file(ctx->ssl_ctx, pem, SSL_FILETYPE_PEM) == 0)) {
	error_string = "cannot open pem";
	return 0;
  }

  if (pem != NULL) {
	(void) SSL_CTX_use_certificate_chain_file(ctx->ssl_ctx, pem);
  }

  // Initialize locking callbacks, needed for thread safety.
  // http://www.openssl.org/support/faq.html#PROG1
  size = sizeof(pthread_mutex_t) * CRYPTO_num_locks();
  ssl_mutexes = (pthread_mutex_t*) malloc((size_t)size);

  for (i = 0; i < CRYPTO_num_locks(); i++) {
	pthread_mutex_init(&ssl_mutexes[i], NULL);
  }

  CRYPTO_set_locking_callback(&ssl_locking_callback);
  CRYPTO_set_id_callback(&ssl_id_callback);

  return 1;
}

static void uninitialize_ssl(mg_context *ctx) {
  int i;
  if (ctx->ssl_ctx != NULL) {
	CRYPTO_set_locking_callback(NULL);
	for (i = 0; i < CRYPTO_num_locks(); i++) {
	  pthread_mutex_destroy(&ssl_mutexes[i]);
	}
	CRYPTO_set_locking_callback(NULL);
	CRYPTO_set_id_callback(NULL);
  }
}

static void reset_per_request_attributes(mg_connection *conn) {
  conn->consumed_content = 0;
  conn->must_close = conn->request_len = 0;
}

static void close_socket_gracefully(mg_connection *conn) {
  char buf[MG_BUF_LEN];
  struct linger linger;
  int n, sock = conn->client.sock;

  // Set linger option to avoid socket hanging out after close. This prevent
  // ephemeral port exhaust problem under high QPS.
  linger.l_onoff = 1;
  linger.l_linger = 1;
  setsockopt(sock, SOL_SOCKET, SO_LINGER, (char *) &linger, sizeof(linger));

  // Send FIN to the client
  (void) shutdown(sock, SHUT_WR);
  set_non_blocking_mode(sock);

  // Read and discard pending incoming data. If we do not do that and close the
  // socket, the data in the send buffer may be discarded. This
  // behaviour is seen on Windows, when client keeps sending data
  // when server decides to close the connection; then when client
  // does recv() it gets no data back.
  do {
	n = pull(NULL, conn, buf, sizeof(buf));
  } while (n > 0);

  // Now we know that our FIN is ACK-ed, safe to close
  (void) closesocket(sock);
}

static void close_connection(mg_connection *conn) {
  if (conn->ssl) {
	SSL_free((SSL*)conn->ssl);
	conn->ssl = NULL;
  }

  if (conn->client.sock != INVALID_SOCKET) {
	close_socket_gracefully(conn);
  }
}

static int is_valid_uri(const char *uri) {
  // Conform to http://www.w3.org/Protocols/rfc2616/rfc2616-sec5.html#sec5.1.2
  // URI can be an asterisk (*) or should start with slash.
  return uri[0] == '/' || (uri[0] == '*' && uri[1] == '\0');
}

static void process_new_connection(mg_connection *conn) {
  mg_request_info *ri = &conn->request_info;
  int discard_len;
  const char *cl;

  do {
	reset_per_request_attributes(conn);
	conn->request_len = read_request(NULL, conn, conn->buf, MAX_REQUEST_SIZE,
									 &conn->data_len);
	if (conn->request_len == 0 && conn->data_len == MAX_REQUEST_SIZE) {
	  mg_write( conn, "HTTP/1.1 413 Request Entity Too Large\r\nContent-Length: 0\r\n\r\n", 60 );
	  return;
	} if (conn->request_len <= 0) {
	  return;  // Remote end closed the connection
	}
	if (parse_http_message(conn->buf, MAX_REQUEST_SIZE, ri) <= 0 ||
		!is_valid_uri(ri->uri_)) {
	  // Do not put garbage in the access log, just send it back to the client
	  mg_write( conn, "HTTP/1.1 400 Bad Request Too Large\r\nContent-Length: 0\r\n\r\n", 57 );
	  conn->must_close = 1;
	} else {
	  // Request is valid, handle it
	  if ((cl = get_header(ri, "Content-Length")) != NULL) {
		conn->content_len = strtol(cl, NULL, 10);
	  } else if (!strcmp(ri->method_, "POST") ||
				 !strcmp(ri->method_, "PUT")) {
		conn->content_len = -1;
	  } else {
		conn->content_len = 0;
	  }
	  conn->birth_time = time(NULL);
	  handle_request(conn);
	}

	// Discard all buffered data for this request
	discard_len = conn->content_len >= 0 &&
	  conn->request_len + conn->content_len < (int) conn->data_len ?
	  (int) (conn->request_len + conn->content_len) : conn->data_len;
	memmove(conn->buf, conn->buf + discard_len, conn->data_len - discard_len);
	conn->data_len -= discard_len;
  } while (conn->ctx->stop_flag == 0 &&
		   conn->content_len >= 0 &&
		   should_keep_alive(conn));
}

// Worker threads take accepted socket from the queue
static int consume_socket(mg_context *ctx, mg_socket *sp) {
  (void) pthread_mutex_lock(&ctx->mutex);

  // If the queue is empty, wait. We're idle at this point.
  while (ctx->sq_head == ctx->sq_tail && ctx->stop_flag == 0) {
	pthread_cond_wait(&ctx->sq_full, &ctx->mutex);
  }

  // If we're stopping, sq_head may be equal to sq_tail.
  if (ctx->sq_head > ctx->sq_tail) {
	// Copy socket from the queue and increment tail
	*sp = ctx->queue[ctx->sq_tail % ARRAY_SIZE(ctx->queue)];
	ctx->sq_tail++;

	// Wrap pointers if needed
	while (ctx->sq_tail > (int) ARRAY_SIZE(ctx->queue)) {
	  ctx->sq_tail -= ARRAY_SIZE(ctx->queue);
	  ctx->sq_head -= ARRAY_SIZE(ctx->queue);
	}
  }

  (void) pthread_cond_signal(&ctx->sq_empty);
  (void) pthread_mutex_unlock(&ctx->mutex);

  return !ctx->stop_flag;
}

static void worker_thread(mg_context *ctx) {
  mg_connection conn;
  memset( &conn, 0, sizeof conn );
  
  char buf[ MAX_REQUEST_SIZE ];
  conn.buf = buf;
  memset( buf, 0, MAX_REQUEST_SIZE );

  // Call consume_socket() even when ctx->stop_flag > 0, to let it signal
  // sq_empty condvar to wake up the master waiting in produce_socket()
  while (consume_socket(ctx, &conn.client)) {
	conn.birth_time = time(NULL);
	conn.ctx = ctx;

	// Fill in IP, port info early so even if SSL setup below fails,
	// error handler would have the corresponding info.
	// Thanks to Johannes Winkelmann for the patch.
	// TODO(lsm): Fix IPv6 case
	conn.request_info.remote_port_ = ntohs(conn.client.rsa.sin.sin_port);
	memcpy(&conn.request_info.remote_ip_,
		   &conn.client.rsa.sin.sin_addr.s_addr, 4);
	conn.request_info.remote_ip_ = ntohl(conn.request_info.remote_ip_);
	conn.request_info.is_ssl_ = conn.client.is_ssl;

	if (!conn.client.is_ssl ||
		(conn.client.is_ssl &&
		 sslize(&conn, conn.ctx->ssl_ctx, SSL_accept))) {
	  process_new_connection(&conn);
	}

	close_connection(&conn);
  }

  // Signal master that we're done with connection and exiting
  (void) pthread_mutex_lock(&ctx->mutex);
  ctx->num_threads--;
  (void) pthread_cond_signal(&ctx->cond);
  (void) pthread_mutex_unlock(&ctx->mutex);
}

// Master thread adds accepted socket to a queue
static void produce_socket(mg_context *ctx, const mg_socket *sp) {
  (void) pthread_mutex_lock(&ctx->mutex);

  // If the queue is full, wait
  while (ctx->stop_flag == 0 &&
		 ctx->sq_head - ctx->sq_tail >= (int) ARRAY_SIZE(ctx->queue)) {
	(void) pthread_cond_wait(&ctx->sq_empty, &ctx->mutex);
  }

  if (ctx->sq_head - ctx->sq_tail < (int) ARRAY_SIZE(ctx->queue)) {
	// Copy socket to the queue and increment head
	ctx->queue[ctx->sq_head % ARRAY_SIZE(ctx->queue)] = *sp;
	ctx->sq_head++;
  }

  (void) pthread_cond_signal(&ctx->sq_full);
  (void) pthread_mutex_unlock(&ctx->mutex);
}

static void accept_new_connection(const mg_socket *listener,
								  mg_context *ctx) {
  mg_socket accepted;
  socklen_t len;

  len = sizeof(accepted.rsa);
  accepted.lsa = listener->lsa;
  accepted.sock = accept(listener->sock, &accepted.rsa.sa, &len);
  if (accepted.sock != INVALID_SOCKET) {
	// Put accepted socket structure into the queue
	accepted.is_ssl = listener->is_ssl;
	produce_socket(ctx, &accepted);
  }
}

static void master_thread(mg_context *ctx) {
  fd_set read_set;
  struct timeval tv;
  mg_socket *sp;
  int max_fd;

  // Increase priority of the master thread
#if defined(_WIN32)
  SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_ABOVE_NORMAL);
#endif

  while (ctx->stop_flag == 0) {
	FD_ZERO(&read_set);
	max_fd = -1;

	// Add listening sockets to the read set
	for (sp = ctx->listening_sockets; sp != NULL; sp = sp->next) {
	  add_to_set(sp->sock, &read_set, &max_fd);
	}

	tv.tv_sec = 0;
	tv.tv_usec = 200 * 1000;

	if (select(max_fd + 1, &read_set, NULL, NULL, &tv) < 0) {
#ifdef _WIN32
	  // On windows, if read_set and write_set are empty,
	  // select() returns "Invalid parameter" error
	  // (at least on my Windows XP Pro). So in this case, we sleep here.
	  mg_sleep_int(1000);
#endif // _WIN32
	} else {
	  for (sp = ctx->listening_sockets; sp != NULL; sp = sp->next) {
		if (ctx->stop_flag == 0 && FD_ISSET(sp->sock, &read_set)) {
		  accept_new_connection(sp, ctx);
		}
	  }
	}
  }

  // Stop signal received: somebody called mg_stop. Quit.
  close_all_listening_sockets(ctx);

  // Wakeup workers that are waiting for connections to handle.
  pthread_cond_broadcast(&ctx->sq_full);

  // Wait until all threads finish
  (void) pthread_mutex_lock(&ctx->mutex);
  while (ctx->num_threads > 0) {
	(void) pthread_cond_wait(&ctx->cond, &ctx->mutex);
  }
  (void) pthread_mutex_unlock(&ctx->mutex);

  // All threads exited, no sync is needed. Destroy mutex and condvars
  (void) pthread_mutex_destroy(&ctx->mutex);
  (void) pthread_cond_destroy(&ctx->cond);
  (void) pthread_cond_destroy(&ctx->sq_empty);
  (void) pthread_cond_destroy(&ctx->sq_full);

  uninitialize_ssl(ctx);

  // Signal mg_stop() that we're done.
  // WARNING: This must be the very last thing this
  // thread does, as ctx becomes invalid after this line.
  ctx->stop_flag = 2;
}

static void free_context(mg_context *ctx) {
  // Deallocate SSL context
  if (ctx->ssl_ctx != NULL) {
	SSL_CTX_free(ctx->ssl_ctx);
  }
  if (ctx->client_ssl_ctx != NULL) {
	SSL_CTX_free(ctx->client_ssl_ctx);
  }
  if (ssl_mutexes != NULL) {
	free(ssl_mutexes);
	ssl_mutexes = NULL;
  }

  // Deallocate context itself
  free(ctx);
}

void mg_stop(mg_context *ctx) {
  ctx->stop_flag = 1;

  // Wait until mg_fini() stops
  while (ctx->stop_flag != 2) {
	(void) mg_sleep_int(10);
  }
  free_context(ctx);

#if defined(_WIN32)
  (void) WSACleanup();
#endif // _WIN32
}

mg_context *mg_start(
  const char* ports, const char* pem_file) {
  mg_context *ctx;
  int i;

  error_string = "";
  
#if defined(_WIN32)
  WSADATA data;
  WSAStartup(MAKEWORD(2,2), &data);
#endif // _WIN32

  // Allocate context and initialize reasonable general case defaults.
  // TODO(lsm): do proper error handling here.
  ctx = (mg_context *) calloc(1, sizeof(*ctx));

  // NOTE(lsm): order is important here. SSL certificates must
  // be initialized before listening ports. UID must be set last.
  if (
	  !set_ssl_option(ctx,pem_file) ||
	  !set_ports_option(ctx,ports,pem_file)
	  ) {
	free_context(ctx);
	return NULL;
  }

#if !defined(_WIN32)
  // Ignore SIGPIPE signal, so if browser cancels the request, it
  // won't kill the whole process.
  (void) signal(SIGPIPE, SIG_IGN);
  // Also ignoring SIGCHLD to let the OS to reap zombies properly.
  (void) signal(SIGCHLD, SIG_IGN);
#endif // !_WIN32

  (void) pthread_mutex_init(&ctx->mutex, NULL);
  (void) pthread_cond_init(&ctx->cond, NULL);
  (void) pthread_cond_init(&ctx->sq_empty, NULL);
  (void) pthread_cond_init(&ctx->sq_full, NULL);

  // Start master (listening) thread
  mg_start_thread((mg_thread_func_t) master_thread, ctx);

  // Start worker threads
  for (i = 0; i < 20; i++) {
	if (mg_start_thread((mg_thread_func_t) worker_thread, ctx) == 0)
	  ctx->num_threads++;
  }

  return ctx;
}

mg_mutex mg_mutex_create(void)
{
	pthread_mutex_t* m = (pthread_mutex_t*) calloc(sizeof(pthread_mutex_t), 1);
	(void) pthread_mutex_init(m, NULL);
	return m;
}

void mg_mutex_destroy(mg_mutex m)
{
	pthread_mutex_destroy((pthread_mutex_t*) m);
	free(m);
}

void mg_mutex_lock(mg_mutex m)
{
	pthread_mutex_lock((pthread_mutex_t*) m);
}

void mg_mutex_unlock(mg_mutex m)
{
	pthread_mutex_unlock((pthread_mutex_t*) m);
}

void mg_sleep(int ms)
{
	mg_sleep_int(ms);
}

const char* mg_get_error_string(void)
{
	return error_string;
}
