/**********************************************************************************************/
/* utils.h			  		                                                   				  */
/*                                                                       					  */
/* Igor Nikitin, 2012																		  */
/* MIT license   																		  	  */
/**********************************************************************************************/

#pragma once


/**********************************************************************************************/
// Functions
	
				/** Returns string with status code, 'Content-Length' header and content. */
void			create_responce(
					char*&				out,
					size_t&				out_len,
					crest_http_status	status,
					const char*			content,
					size_t				content_len );

				/** Returns string with status code and 'Content-Length' header. */
void			create_responce_header(
					char*&				out,
					size_t&				out_len,
					crest_http_status	status,
					size_t				content_len );

				/** The same as strdup. */
char*			crest_strdup(
					const char*			str,
					int					len = -1 );

				/** Returns TRUE if this file exists. */
bool			file_exists( const char* path );

				/** Returns file's size. */
size_t			file_size( const char* path );

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

					if( tmp < 0 )
						*ptr1++ = '-';

					*ptr1-- = '\0';

					while( ptr2 < ptr1 )
					{
						ch		= *ptr1;
						*ptr1--	= *ptr2;
						*ptr2++	= ch;
					}
				}
