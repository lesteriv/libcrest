/**********************************************************************************************/
/* cr_types.h			  		                                               				  */
/*                                                                       					  */
/* Igor Nikitin, 2012																		  */
/* MIT license   																		  	  */
/**********************************************************************************************/

#pragma once


/**********************************************************************************************/
enum cr_http_auth
{
	CR_AUTH_NONE,
	CR_AUTH_BASIC,
	CR_AUTH_DIGEST
};

/**********************************************************************************************/
enum cr_http_method
{
	CR_METHOD_DELETE,
	CR_METHOD_GET,
	CR_METHOD_POST,
	CR_METHOD_PUT,
	CR_METHOD_COUNT
};

/**********************************************************************************************/
enum cr_http_status
{
	CR_HTTP_OK,
	CR_HTTP_CREATED,
	CR_HTTP_ACCEPTED,
	CR_HTTP_NOT_MODIFIED,
	CR_HTTP_BAD_REQUEST,
	CR_HTTP_NON_AUTH,
	CR_HTTP_FORBIDDEN,
	CR_HTTP_NOT_FOUND,
	CR_HTTP_NOT_ALLOWED,
	CR_HTTP_LENGTH_REQUIRED,
	CR_HTTP_INTERNAL_ERROR,
	CR_HTTP_STATUS_COUNT
};
