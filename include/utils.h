/**********************************************************************************************/
/* utils.h			  		                                                   				  */
/*                                                                       					  */
/* Igor Nikitin, 2012																		  */
/* MIT license   																		  	  */
/**********************************************************************************************/

#pragma once

/**********************************************************************************************/
CREST_NAMESPACE_START

	
				/** Returns TRUE if this file exists. */
bool			file_exists( const string& path );

				/** Returns file's size. */
int64_t			file_size( const string& path );

				/** Returns string with status code, 'Content-Length' header and body. */
string			responce(
					http_status		status,
					const string&	str );

				/** Returns substring from the right side of string. */
inline string	right(
					const string&	str,
					size_t			count )
				{
					count = std::min( count, str.length() );
					return str.substr( str.length() - count );
				}


/**********************************************************************************************/
CREST_NAMESPACE_END
