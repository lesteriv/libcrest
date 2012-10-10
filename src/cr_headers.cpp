/**********************************************************************************************/
/* cr_heders.cpp						                                               		  */
/*                                                                       					  */
/* Igor Nikitin, 2012																		  */
/* MIT license			                                                  					  */
/**********************************************************************************************/

// CREST
#include <string.h>

#include "../include/cr_headers.h"


//////////////////////////////////////////////////////////////////////////
// construction
//////////////////////////////////////////////////////////////////////////


/**********************************************************************************************/
cr_headers::cr_headers( void )
{
	count_ = 0;
	size_ = 0;
}

/**********************************************************************************************/
cr_headers::cr_headers(
	const char* name,
	const char* value )
{
	count_ = 0;
	add( name, value );
}
							
							
//////////////////////////////////////////////////////////////////////////
// methods
//////////////////////////////////////////////////////////////////////////


/**********************************************************************************************/
void cr_headers::add(
	const char*	name,
	const char*	value,
	size_t		name_len,
	size_t		value_len )
{
	if( count_ < 64 )
	{
		if( name_len  == (size_t) -1 ) name_len  = strlen( name );
		if( value_len == (size_t) -1 ) value_len = strlen( value );
		
		names_[ count_ ] = name;
		names_len_[ count_ ] = name_len;

		values_[ count_ ] = value;
		values_len_[ count_ ] = value_len;

		size_ += name_len + value_len + 4;
		count_++;
	}
}

/**********************************************************************************************/
size_t cr_headers::count( void ) const
{
	return count_;
}

/**********************************************************************************************/
int cr_headers::index( const char* name ) const
{
	size_t name_len = strlen( name );
	
	for( size_t i = 0 ; i < count_ ; ++i )
	{
		if( names_len_[ i ] == name_len && !strcmp( names_[ i ], name ) )
			return i;
	}
	
	return -1;	
}

/**********************************************************************************************/
const char* cr_headers::name( size_t index ) const
{
	return names_[ index ];
}

/**********************************************************************************************/
size_t cr_headers::name_len( size_t index ) const
{
	return names_len_[ index ];
}

/**********************************************************************************************/
void cr_headers::reset( void )
{
	count_ = 0;
}

/**********************************************************************************************/
size_t cr_headers::size( void )
{
	return size_;
}

/**********************************************************************************************/
const char* cr_headers::value( size_t index ) const
{
	return values_[ index ];
}

/**********************************************************************************************/
const char* cr_headers::value( 
	const char*	name,
	size_t		name_len ) const
{
	if( name_len  == (size_t) -1 )
		name_len = strlen( name );
	
	for( size_t i = 0 ; i < count_ ; ++i )
	{
		if( names_len_[ i ] == name_len && !strcmp( names_[ i ], name ) )
			return values_[ i ];
	}
	
	return NULL;
}

/**********************************************************************************************/
size_t cr_headers::value_len( size_t index ) const
{
	return values_len_[ index ];
}
