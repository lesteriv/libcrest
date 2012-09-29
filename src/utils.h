/**********************************************************************************************/
/* utils.h			  		                                                   				  */
/*                                                                       					  */
/* Igor Nikitin, 2012																		  */
/* MIT license   																		  	  */
/**********************************************************************************************/

#pragma once

// STD
#include <limits>


/**********************************************************************************************/
// Functions
	
				/** Returns TRUE if this file exists. */
bool			file_exists( const char* path );

				/** Returns file's size. */
size_t			file_size( const char* path );

				/** Returns string with status code, 'Content-Length' header and body. */
string			responce(
					crest_http_status		status,
					const string&	content );

				/** The same as strdup. */
char*			crest_strdup( const char* str );

				/** Converts integer value to string. */
				template< typename T >
void			to_string( T value, char* buf )
				{
					char	ch;
					char*	ptr1 = buf;
					char*	ptr2 = buf;
					T		tmp;

					do
					{
						tmp = value;
						value /= 10;
						*ptr1++ = "zyxwvutsrqponmlkjihgfedcba9876543210123456789abcdefghijklmnopqrstuvwxyz" [ 35 + ( tmp - value * 10 ) ];
					}
					while( value );

					// Workaround to fix GCC warning
					if( -1 * ( std::numeric_limits<T>::is_signed ? 1 : 0 ) * tmp > 0 )
						*ptr1++ = '-';

					*ptr1-- = '\0';

					while( ptr2 < ptr1 )
					{
						ch		= *ptr1;
						*ptr1--	= *ptr2;
						*ptr2++	= ch;
					}
				}
