/**********************************************************************************************/
/* crest_internal.h																			  */
/*                                                                       					  */
/* Igor Nikitin, 2012																		  */
/* MIT license			                                                  					  */
/**********************************************************************************************/

#pragma once

// STD
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <limits>
#include <list>
#include <map>
#include <stdint.h>
#include <string>
#include <vector>


/**********************************************************************************************/
using std::list;
using std::map;
using std::string;
using std::vector;

/**********************************************************************************************/
struct mg_connection;


// CREST
#include "../macroses.h"
#include "../types.h"
#include "../auth_manager.h"
#include "../connection.h"
#include "../utils.h"


/**********************************************************************************************/
CREST_NAMESPACE_START
	
	
/**********************************************************************************************/
typedef	void(*crest_api_callback_t)( connection& );

/**********************************************************************************************/
struct crest_register_api
{
	crest_register_api(
		http_method			 method,
		bool				 admin,
		bool			 	 readonly,
		const char*			 res,
		crest_api_callback_t func );
};

/**********************************************************************************************/
#define CREST_API( method, admin, ro, name ) \
	static void JOIN( f, __LINE__ )( connection& ); \
	static crest_register_api JOIN( r, __LINE__ )( method, admin, ro, name, JOIN( f, __LINE__ ) ); \
	static void JOIN( f, __LINE__ )


/**********************************************************************************************/
CREST_NAMESPACE_END
