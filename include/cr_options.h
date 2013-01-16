/**********************************************************************************************/
/* cr_options.h			  		                                                   			  */
/*                                                                       					  */
/* Igor Nikitin, 2013																		  */
/* MIT license			                                                  					  */
/**********************************************************************************************/

#pragma once

// STD
#include <string>

// CREST
#include "cr_types.h"


/**********************************************************************************************/
struct cr_options
{
	std::string			auth_file;
	cr_http_auth		auth_kind		= CR_AUTH_NONE;
	bool				log_enabled		= false;
	std::string			log_file;
	std::string			pem_file;
	std::string			ports			= "80";				// Comma separated list of [ip_address:]port[s] values
	cr_result_format	result_format	= CR_FORMAT_JSON;	// Default format for cr_result
	size_t				thread_count	= 8;
};
