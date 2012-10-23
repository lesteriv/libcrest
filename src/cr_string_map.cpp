/**********************************************************************************************/
/* cr_string_map.cpp				                                               			  */
/*                                                                       					  */
/* Igor Nikitin, 2012																		  */
/* MIT license			                                                  					  */
/**********************************************************************************************/

// STD
#include <string.h>

// CREST
#include "../include/cr_string_map.h"
#include "../include/cr_utils.h"
#include "cr_utils_private.h"


//////////////////////////////////////////////////////////////////////////
// construction
//////////////////////////////////////////////////////////////////////////


/**********************************************************************************************/
cr_string_map::cr_string_map( void )
{
	size_ = 0;
}

/**********************************************************************************************/
cr_string_map::cr_string_map(
	const char* name,
	const char* value )
{
	size_ = 0;
	add( name, value );
}


//////////////////////////////////////////////////////////////////////////
// properties
//////////////////////////////////////////////////////////////////////////


/**********************************************************************************************/
const char*	cr_string_map::name( size_t index ) const
{
	return name_[ index ];
}

/**********************************************************************************************/
size_t cr_string_map::name_len( size_t index ) const
{
	return name_len_[ index ];
}

/**********************************************************************************************/
size_t cr_string_map::size( void ) const
{
	return size_;
}

/**********************************************************************************************/
const char*	cr_string_map::value( size_t index ) const
{
	return value_[ index ];
}

/**********************************************************************************************/
size_t cr_string_map::value_len( size_t index ) const
{
	return value_len_[ index ];
}


//////////////////////////////////////////////////////////////////////////
// methods
//////////////////////////////////////////////////////////////////////////


/**********************************************************************************************/
void cr_string_map::add(
	const char*	name,
	const char*	value,
	size_t		name_len,
	size_t		value_len )
{
	if( size_ < CREST_MAP_SIZE )
	{
		if( name_len  == (size_t) -1 ) name_len  = strlen( name );
		if( value_len == (size_t) -1 ) value_len = strlen( value );
		
		name_[ size_ ] = name;
		name_len_[ size_ ] = name_len;

		value_[ size_ ] = value;
		value_len_[ size_ ] = value_len;

		++size_;
	}	
}

/**********************************************************************************************/
void cr_string_map::clear( void )
{
	size_ = 0;
}

/**********************************************************************************************/
int cr_string_map::find(
	const char* name,
	size_t		name_len ) const
{
	if( name_len == (size_t) -1 )
		name_len = strlen( name );
	
	for( size_t i = 0 ; i < size_ ; ++i )
	{
		if( name_len == name_len_[ i ] && !cr_strcasecmp( name, name_[ i ] ) )
			return i;
	}
	
	return -1;
}

/**********************************************************************************************/
const char*	cr_string_map::value(
	const char*	name,
	size_t		name_len ) const
{
	int index = find( name, name_len );
	return index >= 0 ? value_[ index ] : "";
}


//////////////////////////////////////////////////////////////////////////
// operators
//////////////////////////////////////////////////////////////////////////


/**********************************************************************************************/
const char*	cr_string_map::operator[]( const char* name ) const
{
	int index = find( name );
	return index >= 0 ? value_[ index ] : "";
}
