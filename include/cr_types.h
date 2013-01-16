/**********************************************************************************************/
/* cr_types.h			  		                                               				  */
/*                                                                       					  */
/* Igor Nikitin, 2013																		  */
/* MIT license   																		  	  */
/**********************************************************************************************/

#pragma once

// STD
#include <cstdint>
#include <string>
#include <vector>


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

/**********************************************************************************************/
enum cr_result_format
{
	CR_FORMAT_BINARY,
	CR_FORMAT_JSON,
	CR_FORMAT_XML
};

/**********************************************************************************************/
enum cr_value_type
{
	CR_INTEGER		= 1,
	CR_FLOAT		= 2,
	CR_TEXT			= 3,
	CR_BLOB			= 4
};


/**********************************************************************************************/
struct cr_result_field
{
	std::string		name;
	cr_value_type	type;
};


/**********************************************************************************************/
#pragma pack( 2 )


/**********************************************************************************************/
struct cr_result_header
{
	int16_t	protocol;
	int16_t	flags;
	int16_t	field_count;
	int16_t	property_count;
	int32_t	record_count;
};


/**********************************************************************************************/
#pragma pack()


/**********************************************************************************************/
typedef std::vector<cr_result_field> cr_result_fields;
