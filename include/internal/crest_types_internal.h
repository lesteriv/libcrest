/**********************************************************************************************/
/* crest_types_internal.h			                                                   		  */
/*                                                                       					  */
/* Igor Nikitin, 2012																		  */
/* MIT license			                                                  					  */
/**********************************************************************************************/

#pragma once


/**********************************************************************************************/
struct crest_string_array
{
	crest_string_array( void )
	{
		count_ = 0;
		items_ = 0;
	}
	
	size_t	count_;
	char**	items_;
};