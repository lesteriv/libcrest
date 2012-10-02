/**********************************************************************************************/
/* crest.h				  		                                                   			  */
/*                                                                       					  */
/* Igor Nikitin, 2012																		  */
/* MIT license			                                                  					  */
/**********************************************************************************************/

#pragma once

// CREST
#include "internal/crest_handler_register.h"
#include "crest_auth_manager.h"
#include "crest_connection.h"


/**********************************************************************************************/
// Macroses

								/** Register handlers for 'usual' resources. */
#define DELETE( name )			CREST_CPP_HANDLER( CREST_METHOD_DELETE	, false, false, #name )
#define GET( name )				CREST_CPP_HANDLER( CREST_METHOD_GET		, false, false, #name )
#define POST( name )			CREST_CPP_HANDLER( CREST_METHOD_POST	, false, false, #name )
#define PUT( name )				CREST_CPP_HANDLER( CREST_METHOD_PUT		, false, false, #name )

								/** Register handlers for resources for administrators only. */
#define DELETE_ADMIN( name )	CREST_CPP_HANDLER( CREST_METHOD_DELETE	,  true, false, #name )
#define GET_ADMIN( name )		CREST_CPP_HANDLER( CREST_METHOD_GET		,  true, false, #name )
#define POST_ADMIN( name )		CREST_CPP_HANDLER( CREST_METHOD_POST	,  true, false, #name )
#define PUT_ADMIN( name )		CREST_CPP_HANDLER( CREST_METHOD_PUT		,  true, false, #name )

								/** Register handlers for public resources, that's always can
								 *  be accessed without authentification. */
#define DELETE_PUBLIC( name )	CREST_CPP_HANDLER( CREST_METHOD_GET		, false,  true, #name )
#define GET_PUBLIC( name )		CREST_CPP_HANDLER( CREST_METHOD_GET		, false,  true, #name )
#define POST_PUBLIC( name )		CREST_CPP_HANDLER( CREST_METHOD_GET		, false,  true, #name )
#define PUT_PUBLIC( name )		CREST_CPP_HANDLER( CREST_METHOD_GET		, false,  true, #name )


/**********************************************************************************************/
// Functions

				/** Returns error description if crest unable to start,
				 *  empty string otherwise. */
const char*		crest_error_string( void );

				/** Returns kind of authorization is required for each request. */
crest_http_auth	crest_get_auth_kind( void );
void			crest_set_auth_kind( crest_http_auth auth );

				/** Returns TRUE if all requests are logging into file. */
bool			crest_get_log_enabled( void );
void			crest_set_log_enabled( bool value );
	
				/** Returns total count of processed requests. */
size_t			crest_request_count( void );

				/** Starts server, returns FALSE if cannot start,
				 *  @ports is comma separated list of [ip_address:]port[s] values,
				 *  examples: 80, 443s, 127.0.0.1:3128, 1.2.3.4:8080s. */
bool			crest_start(
					const char*		ports			= "8080",
					const char*		auth_file		= "",
					const char*		log_file		= "",
					const char*		pem_file		= "",
					crest_http_auth	auth_kind		= CREST_AUTH_BASIC,
					bool			log_enabled		= true );

				/** Stops running server. */
void			crest_stop( void );

				/** Returns version of library. */
const char*		crest_version( void );
