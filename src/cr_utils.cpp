/**********************************************************************************************/
/* utils.cpp		  		                                                   				  */
/*                                                                       					  */
/* Igor Nikitin, 2013																		  */
/* MIT license			                                                  					  */
/**********************************************************************************************/

// STD
#include <sys/stat.h>
#include <ctype.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <thread>

// ZLIB
#include "../third/zlib/deflate.h"

// CREST
#include "../include/crest.h"
#include "../include/cr_utils.h"
#include "cr_json.h"
#include "cr_utils_private.h"
#include "cr_xml.h"

/**********************************************************************************************/
#ifdef _MSC_VER
#pragma warning( disable: 4996 )
#define stat _stat
#endif // _WIN32


//////////////////////////////////////////////////////////////////////////
// constants
//////////////////////////////////////////////////////////////////////////


/**********************************************************************************************/
static const unsigned char BASE64_TABLE[] =
{
	64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
	64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
	64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 62, 64, 64, 64, 63,
	52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 64, 64, 64, 64, 64, 64,
	64,  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14,
	15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 64, 64, 64, 64, 64,
	64, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40,
	41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 64, 64, 64, 64, 64,
	64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
	64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
	64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
	64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
	64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
	64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
	64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
	64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64
};

/**********************************************************************************************/
static const char* const RESPONCE_PREFIX[ CR_HTTP_STATUS_COUNT ] =
{
	"HTTP/1.1 200 OK\r\nContent-Length: ",
	"HTTP/1.1 201 Created\r\nContent-Length: ",
	"HTTP/1.1 202 Accepted\r\nContent-Length: ",
	"HTTP/1.1 304 Not Modified\r\nContent-Length: ",
	"HTTP/1.1 400 Bad Request\r\nContent-Length: ",
	"HTTP/1.1 401 Unauthorized\r\nContent-Length: ",
	"HTTP/1.1 403 Forbidden\r\nContent-Length: "
	"HTTP/1.1 404 Not Found\r\nContent-Length: ",
	"HTTP/1.1 405 Method Not Allowed\r\nContent-Length: ",
	"HTTP/1.1 411 Length Required\r\nContent-Length: ",
	"HTTP/1.1 500 Internal Server Error\r\nContent-Length: "
};

/**********************************************************************************************/
static const size_t RESPONCE_PREFIX_SIZE[ CR_HTTP_STATUS_COUNT ] =
{
	33,	38,	39,	43, 42,	43, 40, 40, 49, 46, 51
};


//////////////////////////////////////////////////////////////////////////
// helper functions
//////////////////////////////////////////////////////////////////////////


/**********************************************************************************************/
inline char hex_to_int( char ch )
{
	return ( ch >= '0' && ch <= '9' ) ? ch - '0' : ch - 'W';
}


//////////////////////////////////////////////////////////////////////////
// functions
//////////////////////////////////////////////////////////////////////////


/**********************************************************************************************/
void add_number( char*& buf, int value )
{
	char	ch;
	char*	ptr1 = buf;
	char*	ptr2 = buf;
	int		tmp;

	do
	{
		tmp = value;
		value /= 10;
		*ptr1++ = "9876543210123456789" [ 9 + ( tmp - value * 10 ) ];
	}
	while( value );

	if( tmp < 0 )
		*ptr1++ = '-';

	buf = ptr1;
	*ptr1-- = '\0';

	while( ptr2 < ptr1 )
	{
		ch		= *ptr1;
		*ptr1--	= *ptr2;
		*ptr2++	= ch;
	}
}

/**********************************************************************************************/
size_t cr_base64_decode(
	char*		vout,
	const char*	vdata,
	size_t		data_size )
{
	if( !data_size )	
		return 0;
		
	size_t res = ( ( data_size + 3 ) / 4 ) * 3;
    
	unsigned char*       out  = (unsigned char*)		vout;
	const unsigned char* data = (const unsigned char*)	vdata;

    while( data_size && data[ data_size - 1 ] == '=' )
        --data_size;

	while( data_size > 4 )
	{
		*(out++) = BASE64_TABLE[ data[ 0 ] ] << 2 | BASE64_TABLE[ data[ 1 ] ] >> 4;
		*(out++) = BASE64_TABLE[ data[ 1 ] ] << 4 | BASE64_TABLE[ data[ 2 ] ] >> 2;
		*(out++) = BASE64_TABLE[ data[ 2 ] ] << 6 | BASE64_TABLE[ data[ 3 ] ];

		data += 4;
		data_size -= 4;
	}

	if( data_size > 1 )
		*(out++) = BASE64_TABLE[ data[ 0 ] ] << 2 | BASE64_TABLE[ data[ 1 ] ] >> 4;
	if( data_size > 2 )
		*(out++) = BASE64_TABLE[ data[ 1 ] ] << 4 | BASE64_TABLE[ data[ 2 ] ] >> 2;
	if (data_size > 3)
		*(out++) = BASE64_TABLE[ data[ 2 ] ] << 6 | BASE64_TABLE[ data[ 3 ] ];

	res -= ( 4 - data_size ) & 3;
	return res;
}

/**********************************************************************************************/
size_t cr_compress_bound( size_t len )
{
	return len + ( len >> 12 ) + ( len >> 14 ) + ( len >> 25 ) + 13;
}

/**********************************************************************************************/
void cr_create_responce(
	char*&			out,
	size_t&			out_len,
	cr_http_status	status,
	const char*		content,
	size_t			content_len,
	cr_string_map*	headers )
{
	out = (char*) malloc( content_len + 128 + headers->size() );
	
	char* str = out;
	add_string ( str, RESPONCE_PREFIX[ status ], RESPONCE_PREFIX_SIZE[ status ] );
	add_number ( str, (int) content_len );
	add_string ( str, "\r\n", 2 );
	
	if( headers )
	{
		size_t count = headers->size();
		for( size_t i = 0 ; i < count ; ++i )
		{
			add_string( str, headers->name( i ), headers->name_len( i ) );
			add_string( str, ": ", 2 );
			add_string( str, headers->value( i ), headers->value_len( i ) );
			add_string( str, "\r\n", 2 );
		}
	}
	
	add_string( str, "\r\n", 2 );
	add_string( str, content, content_len);
	
	*str = 0;
	out_len = str - out;
}

/**********************************************************************************************/
std::string cr_create_responce(
	cr_http_status		status,
	const std::string&	content,
	cr_string_map*		headers )
{
	char* data;
	size_t len;
	cr_create_responce( data, len, status, content.c_str(), content.length(), headers );
	
	return std::string( data, len );
}

/**********************************************************************************************/
void cr_create_responce_header(
	char*			out,
	size_t&			out_len,
	cr_http_status	status,
	size_t			content_len,
	cr_string_map*	headers )
{
	char* str = out;
	add_string ( str, RESPONCE_PREFIX[ status ], RESPONCE_PREFIX_SIZE[ status ] );
	add_number ( str, (int) content_len );
	add_string ( str, "\r\n", 2 );
	
	if( headers )
	{
		size_t count = headers->size();
		for( size_t i = 0 ; i < count ; ++i )
		{
			add_string( str, headers->name( i ), headers->name_len( i ) );
			add_string( str, ": ", 2 );
			add_string( str, headers->value( i ), headers->value_len( i ) );
			add_string( str, "\r\n", 2 );
		}
	}
	
	add_string( str, "\r\n", 3 );
	
	out_len = str - out - 1;
}

/**********************************************************************************************/
size_t cr_deflate(
	const char*		buf,
	size_t			len,
	char*			out,
	size_t			out_len )
{
	z_stream zstream;
	zstream.avail_in	= (unsigned int) len;
	zstream.next_in		= (byte*) buf;
	zstream.avail_out	= out_len;
	zstream.next_out	= (byte*) out;
	deflate( zstream );

	return (char*) zstream.next_out - out;
}

/**********************************************************************************************/
bool cr_file_exists( const std::string& path )
{
	struct stat st;
	return stat( path.c_str(), &st ) == 0 && ( st.st_mode & S_IFREG );
}

/**********************************************************************************************/
time_t cr_file_modification_time( const std::string& path )
{
	struct stat st;
	return ( stat( path.c_str(), &st ) == 0 ) ?
		st.st_mtime :
		0;
}

/**********************************************************************************************/
size_t cr_file_size( const std::string& path )
{
	size_t size = 0;
	
	FILE* f = fopen( path.c_str(), "rb" );
	if( f )
	{
		fseek( f, 0, SEEK_END );
		size = ftell( f );
		fclose( f );
	}

	return size;
}

/**********************************************************************************************/
void parse_cookie_header(
	cr_string_map&	cookies,
	char*			header_value,
	size_t			header_len )
{
	bool	in_value = false;
	char*	name;
	size_t	name_len;
	
	for( size_t i = 0 ; i < header_len ; ++i )
	{
		if( in_value )
		{
			// Skip whitespace before value
			if( header_value[ i ] && strchr( "\r\n \t", header_value[ i ] ) )
				continue;
			
			// Find end of value
			char* end = header_value + i;
			while( *end && !strchr( " \r\n\t;=", *end ) ) ++end;
			*end = 0;

			cookies.add( name, header_value + i, name_len, end - header_value - i );
			
			i = end - header_value;
			in_value = false;
		}
		else
		{
			// Skip whitespace before name
			if( !header_value[ i ] || strchr( ";\r\n \t", header_value[ i ] ) )
				continue;
			
			name = header_value + i;
			for( ; i < header_len && header_value[ i ] != '=' ; ++i );
			header_value[ i ] = 0;
			name_len = header_value + i - name;
			
			in_value = true;
		}
	}
}

/**********************************************************************************************/
inline int pop_int( const char*& s )
{
	return strtol( s, (char**) &s, 10 );
}

/**********************************************************************************************/
inline bool skip( const char*&s, char c )
{
	return *s++ == c;
}

/**********************************************************************************************/
vector<cr_port> parse_ports( const string& ports )
{
	vector<cr_port> r;
	
	const char* s = ports.c_str();
	while( *s )
	{
		int n;
		if( !( n = pop_int( s ) ) )
			return vector<cr_port>();
		
		// port(s)
		if( *s == 's' )
		{
			r.push_back( { 0, 0, 0, 0, n, true } );
			++s;
		}
		// a.b.c.d:port and a.b.c.d:port(s)
		else if( *s )
		{
			int b, c, d, port;
			if( skip( s, '.' ) &&
				( b = pop_int( s ) ) && skip( s, '.' ) &&
				( c = pop_int( s ) ) && skip( s, '.' ) &&
				( d = pop_int( s ) ) && skip( s, ':' ) &&
				( port = pop_int( s ) ) )
			{
				if( *s == 's' )
				{
					++s;
					r.push_back( { n, b, c, d, port, true } );
				}
				else
				{
					r.push_back( { n, b, c, d, port, false } );
				}
			}
		}
		// port
		else
		{
			r.push_back( { 0, 0, 0, 0, n, false } );
		}
		
		if( *s && !skip( s, ',' ) )
			return vector<cr_port>();
	}
	
	return r;
}

/**********************************************************************************************/
inline char hex2d( char ch )
{
	return ( ch >= '0' && ch <= '9' ) ?
		ch - '0' :
		( ch >= 'a' && ch <= 'f' ) ?
			ch - 'a' + 10 :
			ch - 'A' + 10;
}

/**********************************************************************************************/
void parse_post_parameters(
	cr_string_map&	out,
	char*			text )
{
	if( !text || !*text )
		return;
	
	switch( *text )
	{
		case '{':
			cr_json().parse( out, text ); 
			break;
		
		case '<':
			cr_xml().parse( out, text ); 
			break;
		
		default :
			parse_query_parameters( out, text );
			break;
	}	
}

/**********************************************************************************************/
void parse_query_parameters(
	cr_string_map&	out,
	char*			str )
{
	while( str && *str )
	{
		// Name
		char* name = str;

		// Search for '='
		for( ; *str && *str != '=' ; ++str );

		// If found = read value
		if( *str == '=' )
		{
			size_t name_len = str - name;
			*str++ = 0;
			
			char* value = str;
			char* end = str;
			
			for( ; *str && *str != '&' ; ++str, ++end )
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

			char ch = *str++;
			
			*end = 0;
			out.add( name, value, name_len, end - value );
			
			if( !ch || out.size() > 63 )
				break;
		}
	}
}


/**********************************************************************************************/
void cr_set_cookie(
	cr_string_map&	headers,
	const char*		name,
	const char*		value )
{
	// TODO
	(void) headers;
	(void) name;
	(void) value;
}

/**********************************************************************************************/
void cr_sleep( int ms )
{
	std::this_thread::sleep_for( std::chrono::seconds( ms ) );
}

/**********************************************************************************************/
int cr_strcasecmp(
	const char*	str1,
	const char* str2 )
{
	int r;

	do r = tolower( *str1++ ) - tolower( *str2++ );
	while( !r && str1[ -1 ] );

	return r;
}


//////////////////////////////////////////////////////////////////////////
// md5
//////////////////////////////////////////////////////////////////////////


/**********************************************************************************************/
#define F1( x, y, z ) ( z ^ ( x & ( y ^ z ) ) )
#define F2( x, y, z ) ( F1( z, x, y ) )
#define F3( x, y, z ) ( x ^ y ^ z )
#define F4( x, y, z ) ( y ^ ( x | ~z ) )

/**********************************************************************************************/
#define MD5STEP( f, w, x, y, z, data, s ) \
	( w += f( x, y, z ) + data, w = w << s | w >> ( 32 - s ), w += x )

/**********************************************************************************************/
static void md5_transform(
	uint32_t buf[ 4 ],
	uint32_t const in[ 16 ] )
{
	register uint32_t a, b, c, d;

	a = buf[ 0 ];
	b = buf[ 1 ];
	c = buf[ 2 ];
	d = buf[ 3 ];

	MD5STEP( F1, a, b, c, d, in[  0 ] + 0xd76aa478,  7 );
	MD5STEP( F1, d, a, b, c, in[  1 ] + 0xe8c7b756, 12 );
	MD5STEP( F1, c, d, a, b, in[  2 ] + 0x242070db, 17 );
	MD5STEP( F1, b, c, d, a, in[  3 ] + 0xc1bdceee, 22 );
	MD5STEP( F1, a, b, c, d, in[  4 ] + 0xf57c0faf,  7 );
	MD5STEP( F1, d, a, b, c, in[  5 ] + 0x4787c62a, 12 );
	MD5STEP( F1, c, d, a, b, in[  6 ] + 0xa8304613, 17 );
	MD5STEP( F1, b, c, d, a, in[  7 ] + 0xfd469501, 22 );
	MD5STEP( F1, a, b, c, d, in[  8 ] + 0x698098d8,  7 );
	MD5STEP( F1, d, a, b, c, in[  9 ] + 0x8b44f7af, 12 );
	MD5STEP( F1, c, d, a, b, in[ 10 ] + 0xffff5bb1, 17 );
	MD5STEP( F1, b, c, d, a, in[ 11 ] + 0x895cd7be, 22 );
	MD5STEP( F1, a, b, c, d, in[ 12 ] + 0x6b901122,  7 );
	MD5STEP( F1, d, a, b, c, in[ 13 ] + 0xfd987193, 12 );
	MD5STEP( F1, c, d, a, b, in[ 14 ] + 0xa679438e, 17 );
	MD5STEP( F1, b, c, d, a, in[ 15 ] + 0x49b40821, 22 );

	MD5STEP( F2, a, b, c, d, in[  1 ] + 0xf61e2562,  5 );
	MD5STEP( F2, d, a, b, c, in[  6 ] + 0xc040b340,  9 );
	MD5STEP( F2, c, d, a, b, in[ 11 ] + 0x265e5a51, 14 );
	MD5STEP( F2, b, c, d, a, in[  0 ] + 0xe9b6c7aa, 20 );
	MD5STEP( F2, a, b, c, d, in[  5 ] + 0xd62f105d,  5 );
	MD5STEP( F2, d, a, b, c, in[ 10 ] + 0x02441453,  9 );
	MD5STEP( F2, c, d, a, b, in[ 15 ] + 0xd8a1e681, 14 );
	MD5STEP( F2, b, c, d, a, in[  4 ] + 0xe7d3fbc8, 20 );
	MD5STEP( F2, a, b, c, d, in[  9 ] + 0x21e1cde6,  5 );
	MD5STEP( F2, d, a, b, c, in[ 14 ] + 0xc33707d6,  9 );
	MD5STEP( F2, c, d, a, b, in[  3 ] + 0xf4d50d87, 14 );
	MD5STEP( F2, b, c, d, a, in[  8 ] + 0x455a14ed, 20 );
	MD5STEP( F2, a, b, c, d, in[ 13 ] + 0xa9e3e905,  5 );
	MD5STEP( F2, d, a, b, c, in[  2 ] + 0xfcefa3f8,  9 );
	MD5STEP( F2, c, d, a, b, in[  7 ] + 0x676f02d9, 14 );
	MD5STEP( F2, b, c, d, a, in[ 12 ] + 0x8d2a4c8a, 20 );

	MD5STEP( F3, a, b, c, d, in[  5 ] + 0xfffa3942,  4 );
	MD5STEP( F3, d, a, b, c, in[  8 ] + 0x8771f681, 11 );
	MD5STEP( F3, c, d, a, b, in[ 11 ] + 0x6d9d6122, 16 );
	MD5STEP( F3, b, c, d, a, in[ 14 ] + 0xfde5380c, 23 );
	MD5STEP( F3, a, b, c, d, in[  1 ] + 0xa4beea44,  4 );
	MD5STEP( F3, d, a, b, c, in[  4 ] + 0x4bdecfa9, 11 );
	MD5STEP( F3, c, d, a, b, in[  7 ] + 0xf6bb4b60, 16 );
	MD5STEP( F3, b, c, d, a, in[ 10 ] + 0xbebfbc70, 23 );
	MD5STEP( F3, a, b, c, d, in[ 13 ] + 0x289b7ec6,  4 );
	MD5STEP( F3, d, a, b, c, in[  0 ] + 0xeaa127fa, 11 );
	MD5STEP( F3, c, d, a, b, in[  3 ] + 0xd4ef3085, 16 );
	MD5STEP( F3, b, c, d, a, in[  6 ] + 0x04881d05, 23 );
	MD5STEP( F3, a, b, c, d, in[  9 ] + 0xd9d4d039,  4 );
	MD5STEP( F3, d, a, b, c, in[ 12 ] + 0xe6db99e5, 11 );
	MD5STEP( F3, c, d, a, b, in[ 15 ] + 0x1fa27cf8, 16 );
	MD5STEP( F3, b, c, d, a, in[  2 ] + 0xc4ac5665, 23 );

	MD5STEP( F4, a, b, c, d, in[  0 ] + 0xf4292244,  6 );
	MD5STEP( F4, d, a, b, c, in[  7 ] + 0x432aff97, 10 );
	MD5STEP( F4, c, d, a, b, in[ 14 ] + 0xab9423a7, 15 );
	MD5STEP( F4, b, c, d, a, in[  5 ] + 0xfc93a039, 21 );
	MD5STEP( F4, a, b, c, d, in[ 12 ] + 0x655b59c3,  6 );
	MD5STEP( F4, d, a, b, c, in[  3 ] + 0x8f0ccc92, 10 );
	MD5STEP( F4, c, d, a, b, in[ 10 ] + 0xffeff47d, 15 );
	MD5STEP( F4, b, c, d, a, in[  1 ] + 0x85845dd1, 21 );
	MD5STEP( F4, a, b, c, d, in[  8 ] + 0x6fa87e4f,  6 );
	MD5STEP( F4, d, a, b, c, in[ 15 ] + 0xfe2ce6e0, 10 );
	MD5STEP( F4, c, d, a, b, in[  6 ] + 0xa3014314, 15 );
	MD5STEP( F4, b, c, d, a, in[ 13 ] + 0x4e0811a1, 21 );
	MD5STEP( F4, a, b, c, d, in[  4 ] + 0xf7537e82,  6 );
	MD5STEP( F4, d, a, b, c, in[ 11 ] + 0xbd3af235, 10 );
	MD5STEP( F4, c, d, a, b, in[  2 ] + 0x2ad7d2bb, 15 );
	MD5STEP( F4, b, c, d, a, in[  9 ] + 0xeb86d391, 21 );

	buf[ 0 ] += a;
	buf[ 1 ] += b;
	buf[ 2 ] += c;
	buf[ 3 ] += d;
}

/**********************************************************************************************/
const char* cr_get_auth_name( cr_http_auth auth )
{
	switch( auth )
	{
		case CR_AUTH_NONE	: return "none";
		case CR_AUTH_BASIC	: return "basic";
		case CR_AUTH_DIGEST	: return "digest";
	}
	
	return "";
}

/**********************************************************************************************/
const char* cr_get_format_name( cr_result_format format )
{
	switch( format )
	{
		case CR_FORMAT_BINARY	: return "binary";
		case CR_FORMAT_HTML		: return "html";
		case CR_FORMAT_JSON		: return "json";
		case CR_FORMAT_XML		: return "xml";
	}
	
	return "";
}

/**********************************************************************************************/
cr_result_format cr_get_result_format( cr_connection& conn )
{
	const char* accept = conn.header( "Accept" );
	if( accept && strstr( accept, "html" ) )
		return CR_FORMAT_HTML;
	else if( accept && strstr( accept, "xml" ) )
		return CR_FORMAT_XML;
	else if( accept && strstr( accept, "json" ) )
		return CR_FORMAT_JSON;
	else if( accept && strstr( accept, "octet" ) )
		return CR_FORMAT_BINARY;
		
	return cr_get_default_result_format();	
}

/**********************************************************************************************/
void cr_md5(
	char			hash[ 16 ],
	size_t			data_count,
	const char**	adata,
	size_t*			alen )
{
	uint32_t		bits [ 2  ] = { 0, 0 };
	uint32_t		buf  [ 4  ] = { 0x67452301, 0xEFCDAB89, 0x98BADCFE, 0x10325476 };
	unsigned char	in	 [ 64 ];
  
	for( size_t i = 0 ; i < data_count ; ++i )
	{
		const char*	data = adata[ i ];
		size_t		len	 = alen[ i ];
		
		uint32_t t = bits[ 0 ];
		if( ( bits[ 0 ] = t + ((uint32_t) len << 3) ) < t )
			bits[ 1 ]++;

		bits[ 1 ] += len >> 29;

		t = ( t >> 3 ) & 0x3F;
		if( t )
		{
			unsigned char* p = (unsigned char*) in + t;

			t = 64 - t;
			if( len < t )
			{
				memmove( p, data, len );
				continue;
			}

			memmove( p, data, t );
			md5_transform( buf, (uint32_t*) in );
			data += t;
			len -= t;
		}

		while( len >= 64 )
		{
			memmove( in, data, 64 );
			md5_transform( buf, (uint32_t*) in );
			data += 64;
			len -= 64;
		}

		memmove( in, data, len );	
	}

	unsigned count = ( bits[ 0 ] >> 3 ) & 0x3F;
	unsigned char* p = in + count;
	
	*p++ = 0x80;
	count = 63 - count;
	
	if( count < 8 )
	{
		memset( p, 0, count );
		md5_transform( buf, (uint32_t*) in );
		memset( in, 0, 56 );
	}
	else
	{
		memset( p, 0, count - 8 );
	}
	
	((uint32_t*) in)[ 14 ] = bits[ 0 ];
	((uint32_t*) in)[ 15 ] = bits[ 1 ];

	md5_transform( buf, (uint32_t*) in);
	memmove( hash, buf, 16 );
}
