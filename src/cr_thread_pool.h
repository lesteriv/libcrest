/**********************************************************************************************/
/* cr_thread_pool.h				 	                                                   		  */
/*                                                                       					  */
/* Igor Nikitin, 2013																		  */
/* MIT license   																		  	  */
/**********************************************************************************************/

#pragma once

// STD
#include <condition_variable>
#include <deque>
#include <thread>
#include <vector>

/**********************************************************************************************/
using namespace std;


/**********************************************************************************************/
class cr_thread_pool
{
	public://////////////////////////////////////////////////////////////////////////
	  
								cr_thread_pool( size_t count );
								~cr_thread_pool( void );

	public://////////////////////////////////////////////////////////////////////////								
						
// This class API:

	// ---------------------
	// Methods

								template<class F>
		void					enqueue( F func )
								{
									queue_mutex_.lock();
									tasks_.push_back( function<void()>( func ) );
									queue_mutex_.unlock();

									condition_.notify_one();
								}								

								
	public://////////////////////////////////////////////////////////////////////////

// Properties		
		
		condition_variable		condition_;
		mutex					queue_mutex_;
		bool					stop_;
		deque<function<void()>> tasks_;
		vector<thread>			workers_;
};
