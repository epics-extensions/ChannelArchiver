#ifndef _AUP_ITERATOR_H_
#define _AUP_ITERATOR_H_

#include "r_tree.h"
#include "interval.h"

//goes through all aups
class aup_Iterator
{
public:
	aup_Iterator(const r_Tree * r);	//start_Time 0,1 forces to got through all leaves
	bool getFirstAUAddress(const interval& i, long * result);
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
