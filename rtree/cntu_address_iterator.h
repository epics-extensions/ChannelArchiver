#ifndef _CNTU_ADDRESS_ITERATOR_H_
#define _CNTU_ADDRESS_ITERATOR_H_

#include "cntu_table.h"

/**
*	For internal use only; serves the channel_Name_Iterator => see there
*/
class cntu_Address_Iterator
{
public:
	cntu_Address_Iterator(const cntu_Table * t);
	bool getFirst(long * result);
	bool getNext(long * result);
private:
	bool getFirstForHashedAddress(long * result, long _hashed_Address);
	FILE * f;
	const cntu_Table * t;
	long hashed_Address;
	long current_CNTU_Address;	
};

#endif //_cntu_address_iterator_h_

