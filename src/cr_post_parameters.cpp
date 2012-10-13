/**********************************************************************************************/
/* cr_parameters.cpp				                                               			  */
/*                                                                       					  */
/* Igor Nikitin, 2012																		  */
/* MIT license			                                                  					  */
/**********************************************************************************************/

// STD
#include <ctype.h>

// RAPIDXML
#include "../third/rapidxml/rapidxml.hpp"

// CREST
#include "../include/cr_connection.h"
#include "../include/cr_post_parameters.h"
#include "../include/cr_utils.h"


//////////////////////////////////////////////////////////////////////////
// helper functions
//////////////////////////////////////////////////////////////////////////


/**********************************************************************************************/
inline char hex2d( char ch )
{
	return ( ch >= '0' && ch <= '9' ) ?
		ch - '0' :
		( ch >= 'a' && ch <= 'f' ) ?
			ch - 'a' + 10 :
			ch - 'A' + 10;
}


//////////////////////////////////////////////////////////////////////////
// construction
//////////////////////////////////////////////////////////////////////////


/**********************************************************************************************/
cr_post_parameters::cr_post_parameters( char* str )
{
	count_ = 0;
	init( str );
}

/**********************************************************************************************/
cr_post_parameters::cr_post_parameters( cr_connection& conn )
{
	count_ = 0;
	
	size_t len = conn.content_length();
	if( !len )
		return;

	char* buf = (char*) alloca( len + 1 );
	len = conn.read( buf, len );
	if( !len )
		return;

	buf[ len ] = 0;
	init( buf );
}


//////////////////////////////////////////////////////////////////////////
// properties
//////////////////////////////////////////////////////////////////////////


/**********************************************************************************************/
size_t cr_post_parameters::get_parameter_count( void ) const
{
	return count_;
}

/**********************************************************************************************/
const char* cr_post_parameters::get_parameter_name( size_t index ) const
{
	return names_[ index ];
}

/**********************************************************************************************/
const char* cr_post_parameters::get_parameter_value( size_t index ) const
{
	return values_[ index ];
}


//////////////////////////////////////////////////////////////////////////
// internal methods
//////////////////////////////////////////////////////////////////////////


/**********************************************************************************************/
void cr_post_parameters_internal::from_form( char* str )
{
	char* qs = &str[ 0 ];
	while( qs && *qs )
	{
		// Name
		char* name = qs;

		// Search for '='
		for( ; *qs && *qs != '=' ; ++qs );

		// If found = read value
		if( *qs == '=' )
		{
			*qs++ = 0;

			names_[ count_ ] = name;
			values_[ count_ ] = qs;
			char* end = qs;

			for( ; *qs && *qs != '&' ; ++qs, ++end )
			{
				if( *qs == '+' )
				{
					*end = ' ';
				}
				else if( *qs == '%' && isxdigit( qs[ 1 ] ) && isxdigit( qs[ 2 ] ) )
				{
					*end = ( hex2d( qs[ 1 ] ) << 4 ) | hex2d( qs[ 2 ] );
					qs += 2;
				}
				else
				{
					*end = *qs;
				}
			}

			bool finish = !*qs;
			
			*end = 0;
			++count_;
			++qs;
			
			if( finish || count_ >= 64 )
				break;
		}
	}	
}

/**********************************************************************************************/
void cr_post_parameters_internal::from_json( char* str )
{
/*
	cJSON* obj = cJSON_Parse( str );
	if( obj )
	{
		if( obj->type == cJSON_Object )
		{
			cJSON* prop = obj->child;
			while( prop )
			{
				if( prop->string )
				{
//					params_.resize( params_.size() + 1 );
//					params_.back().first = prop->string;
//					string& pm = params_.back().second;
					
					switch( prop->type )
					{
						case cJSON_False	: pm = "0"; break;
						case cJSON_True		: pm = "1"; break;
						case cJSON_NULL		: pm = "NULL"; break;
						case cJSON_String	: pm = prop->valuestring ? prop->valuestring : ""; break;

						case cJSON_Number	:
						{
//							pm = ( (double) prop->valueint == prop->valuedouble ) ?
//								to_string( prop->valueint ) :
//								to_string( prop->valuedouble );
						}
						break;
						
						default:
							break;
					}					
				}
				
				prop = prop->next;
			}
		}
		
		cJSON_Delete( obj );
	}**/
}

/**********************************************************************************************/
void cr_post_parameters_internal::from_xml( char* str )
{
	cr_xml doc( str );
	
/*	
	auto pnode = doc.first_node();
	if( pnode )
	{
		auto node = pnode->first_node();
		while( node )
		{
			params_.resize( params_.size() + 1 );
			params_.back().first = node->name();
			params_.back().second = node->value();
			
			node = node->next_sibling();
		}
	}*/
}

/**********************************************************************************************/
void cr_post_parameters_internal::init( char* str )
{
	if( !str || !*str )
		return;
	
	switch( *str )
	{
		case '{': from_json	( str ); break;
		case '<': from_xml	( str ); break;
		default : from_form	( str ); break;
	}	
}
