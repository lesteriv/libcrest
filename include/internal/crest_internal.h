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
#include "../crest_auth_manager.h"
#include "../crest_connection.h"
#include "../utils.h"


/**********************************************************************************************/
typedef	void(*crest_api_callback_t)( crest_connection& );

/**********************************************************************************************/
struct crest_register_api
{
	crest_register_api(
		crest_http_method	 method,
		const char*			 resource,
		crest_api_callback_t func,
		bool				 admin,
		bool			 	 readonly );
};

/**********************************************************************************************/
#define CREST_CPP_API( method, admin, ro, name ) \
	static void JOIN( f, __LINE__ )( crest_connection& ); \
	static crest_register_api JOIN( r, __LINE__ )( method, name, JOIN( f, __LINE__ ), admin, ro ); \
	static void JOIN( f, __LINE__ )
