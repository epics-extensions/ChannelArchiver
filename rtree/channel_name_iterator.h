#ifndef _CHANNEL_NAME_ITERATOR_H_
#define _CHANNEL_NAME_ITERATOR_H_

#include <stdString.h>
#include "cntu_table.h"
#include "cntu_address_iterator.h"

/**
*	Retrieves all channel names stored in the internal hash table of the index file
*/

class channel_Name_Iterator
{
public:
	channel_Name_Iterator(const cntu_Table * t);
	
	/**
	*	@param result is a pointer to an object which attributes are
    *   set according to the values from the index file
	*	Note: The memory handling is user's problem.
	*	@return false, if there are no channel names, or errors occured; true otherwise
	*/
	bool getFirst(stdString * result);

	/**
	*	getFirst() must be called before
	*	@see getFirst()
	*	@param result is a pointer to an object which attributes are
    *   set according to the values from the index file
	*	Note: The memory handling is user's problem.
	*	@return false, if there are no more channel names, or errors occured; true otherwise
	*/
	bool getNext(stdString * result);
private:
	FILE * f;
	cntu_Address_Iterator cai;
	cntu current_CNTU;
};

#endif //_channel_name_iterator_h_
