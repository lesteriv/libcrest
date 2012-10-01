/**********************************************************************************************/
/* crest_handler_register.h			                                                   		  */
/*                                                                       					  */
/* Igor Nikitin, 2012																		  */
/* MIT license			                                                  					  */
/**********************************************************************************************/

#pragma once

// CREST
#include "../crest_types.h"

/**********************************************************************************************/
class crest_connection;


/**********************************************************************************************/
typedef	void(*crest_api_callback_t)( crest_connection& );

/**********************************************************************************************/
// Helper class to auto register handlers by macroese.
//
struct crest_handler_register
{
	crest_handler_register(
		crest_http_method	 method,
		const char*			 resource,
		crest_api_callback_t func,
		bool				 admin,
		bool			 	 publ );
};

/**********************************************************************************************/
#define JOIN( x, y )		JOIN_AGAIN( x, y )
#define JOIN_AGAIN( x, y )	x ## y

/**********************************************************************************************/
#define CREST_CPP_HANDLER( method, admin, publ, name ) \
	static void JOIN( f, __LINE__ )( crest_connection& ); \
	static crest_handler_register JOIN( r, __LINE__ )( method, name, JOIN( f, __LINE__ ), admin, publ ); \
	static void JOIN( f, __LINE__ )
