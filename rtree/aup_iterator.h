#ifndef _AUP_ITERATOR_H_
#define _AUP_ITERATOR_H_

#include "r_tree.h"
#include "interval.h"

/**
*	To pass data from one file to another
*	PLEASE TRY NOT TO USE IT!
*/
class aup_Iterator
{
public:
	aup_Iterator(const r_Tree * r);	//start_Time 0,1 forces to got through all leaves

	/**
	*	@param result is the pointer to a memory block to which the address of the AU is written
	*	@return false, if the r Tree doesn't store data from the specified interval, or
	*	errors occured; true otherwise	
	*/
	bool getFirstAUAddress(const interval& i, long * result);
	
	/**
	*	Looks for the next AU address; if the AU has already been retrieved, look for the next, 
	*	and so on.
	*	Note: getFirstAUAddress() must be called before!
	*	@see getFirstAUAddress()
	*	@param result is the pointer to a memory block to which the address of the AU is written
	*	@return false, if there is no more data for the specified interval, or errors occured;
	*	true otherwise
	*/
	bool getNextAUAddress(long * result);
private:
	bool getKeyAUAddressOfTheCurrentLeaf(long * result);	
	const r_Tree * r;
	interval iv;
	long current_Leaf_Address;
	long current_AU_Pointer_Address;
	ArrayList aup_Addresses;
};

#endif //_aup_iterator_h_
