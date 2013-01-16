/**********************************************************************************************/
/* cr_cache_internal.h								                                   		  */
/*                                                                       					  */
/* Igor Nikitin, 2013																		  */
/* MIT license			                                                  					  */
/**********************************************************************************************/

#pragma once

// STD
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>

/**********************************************************************************************/
class cr_connection;


/**********************************************************************************************/
// Internal members for cache
//
class cr_cache_internal
{
	protected://////////////////////////////////////////////////////////////////////////
		
		struct resource
		{
			std::shared_ptr<std::string>	data;
			std::shared_ptr<std::string>	etag;
			time_t							expire;
		};

		typedef std::unordered_map<std::string,resource> cache_entries_t;
		
	
	protected://////////////////////////////////////////////////////////////////////////

// Properties

		int				expire_time_;

		
// Runtime data
		
		cache_entries_t entries_;
		std::mutex		mutex_;
};
