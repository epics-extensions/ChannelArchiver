#ifndef _CHANNEL_NAME_ITERATOR_H_
#define _CHANNEL_NAME_ITERATOR_H_

#include "cntu_table.h"
#include "cntu_address_iterator.h"

class channel_Name_Iterator
{
public:
	channel_Name_Iterator(const cntu_Table * t);
	//returns 0 if there is none (more)
	//for result, CHANNEL_NAME_LENGTH must be allocated
	//getFirst() must be ALWAYS called before getNext()
	bool getFirst(char * result);
	bool getNext(char * result);
private:
	FILE * f;
	cntu_Address_Iterator cai;
	cntu current_CNTU;
};

#endif //_channel_name_iterator_h_
