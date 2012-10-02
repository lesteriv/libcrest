/**********************************************************************************************/
/* types.h			  		                                                   				  */
/*                                                                       					  */
/* Igor Nikitin, 2012																		  */
/* MIT license   																		  	  */
/**********************************************************************************************/

#pragma once


/**********************************************************************************************/
enum crest_http_auth
{
	CREST_AUTH_NONE,
	CREST_AUTH_BASIC,
	CREST_AUTH_DIGEST
};

/**********************************************************************************************/
enum crest_http_method
{
	CREST_METHOD_DELETE,
	CREST_METHOD_GET,
	CREST_METHOD_POST,
	CREST_METHOD_PUT,
	CREST_METHOD_COUNT
};

/**********************************************************************************************/
enum crest_http_status
{
	CREST_HTTP_OK,
	CREST_HTTP_CREATED,
	CREST_HTTP_ACCEPTED,
	CREST_HTTP_BAD_REQUEST,
	CREST_HTTP_NON_AUTH,
	CREST_HTTP_NOT_FOUND,
	CREST_HTTP_NOT_ALLOWED,
	CREST_HTTP_LENGTH_REQUIRED,
	CREST_HTTP_INTERNAL_ERROR,
	CREST_HTTP_STATUS_COUNT
};
