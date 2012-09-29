/**********************************************************************************************/
/* crest.h				  		                                                   			  */
/*                                                                       					  */
/* Igor Nikitin, 2012																		  */
/* MIT license			                                                  					  */
/**********************************************************************************************/

#pragma once

// CREST
#include "internal/crest_internal.h"


/**********************************************************************************************/
#define DELETE( name )			CREST_API( METHOD_DELETE	, false, false, #name )
#define GET( name )				CREST_API( METHOD_GET		, false, false, #name )
#define POST( name )			CREST_API( METHOD_POST		, false, false, #name )
#define PUT( name )				CREST_API( METHOD_PUT		, false, false, #name )

/**********************************************************************************************/
#define DELETE_ADMIN( name )	CREST_API( METHOD_DELETE	, true, false, #name )
#define GET_ADMIN( name )		CREST_API( METHOD_GET		, true, false, #name )
#define POST_ADMIN( name )		CREST_API( METHOD_POST		, true, false, #name )
#define PUT_ADMIN( name )		CREST_API( METHOD_PUT		, true, false, #name )

/**********************************************************************************************/
#define GET_READONLY( name )	CREST_API( METHOD_GET		, false, true, #name )


/**********************************************************************************************/
CREST_NAMESPACE_START


/**********************************************************************************************/
string crest_error_string( void );

/**********************************************************************************************/
size_t crest_request_count( void );

/**********************************************************************************************/
bool crest_start(
	const char*		ports		= "8080",
	const string&	auth_file	= "",
	const string&	log_file	= "",
	const string&	pem_file	= "" );

/**********************************************************************************************/
void crest_stop( void );

/**********************************************************************************************/
const char* crest_version( void );


/**********************************************************************************************/
CREST_NAMESPACE_END

/**********************************************************************************************/
using namespace crest;
