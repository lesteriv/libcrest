/**********************************************************************************************/
/* cr_json.h					 	                                                   		  */
/*                                                                       					  */
/* Igor Nikitin, 2012																		  */
/* MIT license   																		  	  */
/**********************************************************************************************/

#pragma once

// CREST
#include "../include/cr_string_map.h"


/**********************************************************************************************/
class cr_json
{
	public://////////////////////////////////////////////////////////////////////////
		
// This class API:

	// ---------------------
	// Methods

		void				parse(
								cr_string_map&	out,
								char*			text );


	protected://////////////////////////////////////////////////////////////////////////
		
	// ---------------------
	// Internal methods
		
		char*					parse_value( char* text );
};

		
/* cJSON Types: */
#define cJSON_False 0
#define cJSON_True 1
#define cJSON_NULL 2
#define cJSON_Number 3
#define cJSON_String 4
#define cJSON_Array 5
#define cJSON_Object 6
	
#define cJSON_IsReference 256

/* The cJSON structure: */
typedef struct cJSON {
	struct cJSON *next,*prev;	/* next/prev allow you to walk array/object chains. Alternatively, use GetArraySize/GetArrayItem/GetObjectItem */
	struct cJSON *child;		/* An array or object item will have a child pointer pointing to a chain of the items in the array/object. */

	int type;					/* The type of the item, as above. */

	char *valuestring;			/* The item's string, if type==cJSON_String */
	int valueint;				/* The item's number, if type==cJSON_Number */
	double valuedouble;			/* The item's number, if type==cJSON_Number */

	char *string;				/* The item's name string, if this item is the child of, or is in the list of subitems of an object. */
} cJSON;

/* Supply a block of JSON, and this returns a cJSON object you can interrogate. Call cJSON_Delete when finished. */
extern cJSON *cJSON_Parse(const char *value);
/* Delete a cJSON entity and all subentities. */
extern void   cJSON_Delete(cJSON *c);

