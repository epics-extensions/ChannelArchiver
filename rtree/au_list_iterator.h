#ifndef _AU_LIST_ITERATOR_H_
#define _AU_LIST_ITERATOR_H_

#include "interval.h"
#include <stdio.h>

/**
*	To pass data from one file to another
*/

class au_List_Iterator
{
public:
    
	au_List_Iterator(FILE * f, long au_List_Pointer);	

	/**
	*	@param result is the pointer to a memory block to which the address of the AU is written
	*	@return false, if errors occured, or there is no AU for the specified interval; true otherwise	
	*/
	bool getFirstAUAddress(const interval& i, long * result);
	
	/**
	*	Looks for the next AU address
	*	Note: getFirstAUAddress() must be called before!
	*	@see getFirstAUAddress()
	*	@param result is the pointer to a memory block to which the address of the AU is written
	*	@return false, if errors occured, or there are no more AUs for the specified interval; true otherwise	
	*/
	bool getNextAUAddress(long * result);
private:
	interval iv;
    FILE * f;
	long current_AU_Address;
    long au_List_Pointer;
};

#endif //_au_list_iterator_h_
