/**********************************************************************************************/
/* cr_parameters.cpp				                                               			  */
/*                                                                       					  */
/* Igor Nikitin, 2012																		  */
/* MIT license			                                                  					  */
/**********************************************************************************************/

// STD
#include <ctype.h>

// CREST
#include "../include/cr_connection.h"
#include "../include/cr_utils.h"


//////////////////////////////////////////////////////////////////////////
// helper functions
//////////////////////////////////////////////////////////////////////////





//////////////////////////////////////////////////////////////////////////
// internal methods
//////////////////////////////////////////////////////////////////////////


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
