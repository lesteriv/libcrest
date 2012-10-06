/**********************************************************************************************/
/* cr_mutex.cpp						                                                  		  */
/*                                                                       					  */
/* Igor Nikitin, 2012																		  */
/* MIT license			                                                  					  */
/**********************************************************************************************/

// MONGOOSE
#include "../third/mongoose/mongoose.h"

// CREST
#include "../include/cr_mutex.h"


//////////////////////////////////////////////////////////////////////////
// construction
//////////////////////////////////////////////////////////////////////////


/**********************************************************************************************/
cr_mutex::cr_mutex( void )
{
	mutex_ = mg_mutex_create();
}

/**********************************************************************************************/
cr_mutex::~cr_mutex( void )
{
	mg_mutex_destroy( mutex_ );
}


//////////////////////////////////////////////////////////////////////////
// methods
//////////////////////////////////////////////////////////////////////////


/**********************************************************************************************/
void cr_mutex::lock( void )
{
	mg_mutex_lock( mutex_ );
}

/**********************************************************************************************/
void cr_mutex::unlock( void )
{
	mg_mutex_unlock( mutex_ );
}