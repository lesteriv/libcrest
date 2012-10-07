/**********************************************************************************************/
/* crest.h				  		                                                   			  */
/*                                                                       					  */
/* Igor Nikitin, 2012																		  */
/* MIT license			                                                  					  */
/**********************************************************************************************/

#pragma once

// CREST
#include "internal/cr_handler_register.h"
#include "cr_connection.h"
#include "cr_mutex.h"
#include "cr_options.h"
#include "cr_user_manager.h"


/**********************************************************************************************/
// Macroses

								/** Register handlers for 'usual' resources. */
#define DELETE( name )			CR_CPP_HANDLER( CR_METHOD_DELETE	, false, false, #name )
#define GET( name )				CR_CPP_HANDLER( CR_METHOD_GET		, false, false, #name )
#define POST( name )			CR_CPP_HANDLER( CR_METHOD_POST		, false, false, #name )
#define PUT( name )				CR_CPP_HANDLER( CR_METHOD_PUT		, false, false, #name )

								/** Register handlers for resources for administrators only. */
#define DELETE_ADMIN( name )	CR_CPP_HANDLER( CR_METHOD_DELETE	,  true, false, #name )
#define GET_ADMIN( name )		CR_CPP_HANDLER( CR_METHOD_GET		,  true, false, #name )
#define POST_ADMIN( name )		CR_CPP_HANDLER( CR_METHOD_POST		,  true, false, #name )
#define PUT_ADMIN( name )		CR_CPP_HANDLER( CR_METHOD_PUT		,  true, false, #name )

								/** Register handlers for public resources, that's always can
								 *  be accessed without authentification. */
#define DELETE_PUBLIC( name )	CR_CPP_HANDLER( CR_METHOD_GET		, false,  true, #name )
#define GET_PUBLIC( name )		CR_CPP_HANDLER( CR_METHOD_GET		, false,  true, #name )
#define POST_PUBLIC( name )		CR_CPP_HANDLER( CR_METHOD_GET		, false,  true, #name )
#define PUT_PUBLIC( name )		CR_CPP_HANDLER( CR_METHOD_GET		, false,  true, #name )


/**********************************************************************************************/
// Functions

				/** Returns error description if crest unable to start,
				 *  empty string otherwise. */
const char*		cr_error_string( void );

				/** Returns kind of authorization is required for each request. */
cr_http_auth	cr_get_auth_kind( void );
void			cr_set_auth_kind( cr_http_auth auth );

				/** Returns TRUE if all requests are logging into file. */
bool			cr_get_log_enabled( void );
void			cr_set_log_enabled( bool value );

				/** Register resource handler. */
void			cr_register_handler(
					cr_http_method		method,
					const char*			resource_name,
					cr_api_callback_t	handler,
					bool				for_admin_only	= false,
					bool			 	public_resource = false );
	
				/** Returns total count of processed requests. */
size_t			cr_request_count( void );

				/** Starts server, returns FALSE if cannot start. */
bool			cr_start( cr_options& opts );

				/** Stops running server. */
void			cr_stop( void );

				/** Returns version of library. */
const char*		cr_version( void );
