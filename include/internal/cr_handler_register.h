/**********************************************************************************************/
/* cr_handler_register.h			                                                   		  */
/*                                                                       					  */
/* Igor Nikitin, 2012																		  */
/* MIT license			                                                  					  */
/**********************************************************************************************/

#pragma once

// CREST
#include "../cr_types.h"

/**********************************************************************************************/
class cr_connection;


/**********************************************************************************************/
typedef	void(*cr_api_callback_t)( cr_connection& );

/**********************************************************************************************/
// Helper class to auto register handlers by macroese.
//
struct cr_auto_handler_register
{
	cr_auto_handler_register(
		cr_http_method	 method,
		const char*			 resource,
		cr_api_callback_t func,
		bool				 admin,
		bool			 	 publ );
};

/**********************************************************************************************/
#define JOIN( x, y )		JOIN_AGAIN( x, y )
#define JOIN_AGAIN( x, y )	x ## y

/**********************************************************************************************/
#define CR_CPP_HANDLER( method, admin, publ, name ) \
	static void JOIN( f, __LINE__ )( cr_connection& ); \
	static cr_auto_handler_register JOIN( r, __LINE__ )( method, name, JOIN( f, __LINE__ ), admin, publ ); \
	static void JOIN( f, __LINE__ )
