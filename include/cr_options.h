/**********************************************************************************************/
/* cr_options.h			  		                                                   			  */
/*                                                                       					  */
/* Igor Nikitin, 2012																		  */
/* MIT license			                                                  					  */
/**********************************************************************************************/

#pragma once

// CREST
#include "cr_types.h"


/**********************************************************************************************/
struct cr_options
{
		cr_options( void )
		{
			auth_file		= NULL;
			auth_kind		= CR_AUTH_NONE;
			deflate			= true;
			log_enabled		= false;
			log_file		= NULL;
			pem_file		= NULL;
			ports			= "8080";
			thread_count	= 8;
		}

		const char*			auth_file;
		cr_http_auth		auth_kind;
		bool				deflate;
		bool				log_enabled;
		const char*			log_file;
		const char*			pem_file;
		const char*			ports;			// Comma separated list of [ip_address:]port[s] values
		size_t				thread_count;
};
