/**********************************************************************************************/
/* crest.h				  		                                                   			  */
/*                                                                       					  */
/* Igor Nikitin, 2012																		  */
/* MIT license			                                                  					  */
/**********************************************************************************************/

#pragma once

// STD
#include <cstring>

// CREST
#include "internal/crest_handler_register.h"
#include "crest_auth_manager.h"
#include "crest_connection.h"


/**********************************************************************************************/
// Macroses

#define DELETE( name )			CREST_CPP_HANDLER( CREST_METHOD_DELETE	, false, false, #name )
#define GET( name )				CREST_CPP_HANDLER( CREST_METHOD_GET		, false, false, #name )
#define POST( name )			CREST_CPP_HANDLER( CREST_METHOD_POST	, false, false, #name )
#define PUT( name )				CREST_CPP_HANDLER( CREST_METHOD_PUT		, false, false, #name )

#define DELETE_ADMIN( name )	CREST_CPP_HANDLER( CREST_METHOD_DELETE	, true, false, #name )
#define GET_ADMIN( name )		CREST_CPP_HANDLER( CREST_METHOD_GET		, true, false, #name )
#define POST_ADMIN( name )		CREST_CPP_HANDLER( CREST_METHOD_POST	, true, false, #name )
#define PUT_ADMIN( name )		CREST_CPP_HANDLER( CREST_METHOD_PUT		, true, false, #name )

#define GET_READONLY( name )	CREST_CPP_HANDLER( CREST_METHOD_GET		, false, true, #name )


/**********************************************************************************************/
// Functions

				/** Returns error description if crest unable to start,
				 *  empty string otherwise. */
const char*		crest_error_string( void );

				/** Returns TRUE if authorization is required for each request. */
bool			crest_get_auth_enabled( void );
void			crest_set_auth_enabled( bool value );

				/** Returns TRUE if all requests are logging into file. */
bool			crest_get_log_enabled( void );
void			crest_set_log_enabled( bool value );
	
				/** Returns total count of processed requests. */
size_t			crest_request_count( void );

				/** Starts server, returns FALSE if cannot start. */
bool			crest_start(
					const char*	ports			= "8080",
					const char*	auth_file		= "",
					const char*	log_file		= "",
					const char*	pem_file		= "",
					bool		auth_enabled	= true,
					bool		log_enabled		= true );

				/** Stops running server. */
void			crest_stop( void );

				/** Returns version of library. */
const char*		crest_version( void );
