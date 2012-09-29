/**********************************************************************************************/
/* types.h			  		                                                   				  */
/*                                                                       					  */
/* Igor Nikitin, 2012																		  */
/* MIT license   																		  	  */
/**********************************************************************************************/

#pragma once


/**********************************************************************************************/
enum crest_http_method
{
	METHOD_DELETE,
	METHOD_GET,
	METHOD_POST,
	METHOD_PUT,
	METHOD_COUNT
};

/**********************************************************************************************/
enum crest_http_status
{
	HTTP_OK,
	HTTP_CREATED,
	HTTP_ACCEPTED,
	HTTP_BAD_REQUEST,
	HTTP_NON_AUTH,
	HTTP_NOT_FOUND,
	HTTP_NOT_ALLOWED,
	HTTP_LENGTH_REQUIRED,
	HTTP_INTERNAL_ERROR,
	HTTP_STATUS_COUNT
};

/**********************************************************************************************/
enum crest_user_flags
{
	USER_ADMIN		= 1,
	USER_READONLY	= 2
};
