//key_au_iterator.h
#ifndef _KEY_AU_ITERATOR_H_
#define _KEY_AU_ITERATOR_H_

#include <stdio.h>
#include "r_tree.h"
#include "interval.h"
#include "r_entry.h"

/**
*	Returns pointers the key objects for the specific interval 
*/
class key_AU_Iterator
{
public:
	key_AU_Iterator(const r_Tree * source);

	/**
	*	Check if the interval is valid, 
	*	if yes, attach to the first leaf of the tree that intersects the desired interval;
	*	get the key AU of the leaf; also look if it's the key of the next leaf, 
	*	and so on, to "return" the lookup definitive interval
	*	@param result is the pointer to the key object which attributes are set according to the 
	*	stored values
	*	@param lookup_Interval is the pointer to the memory where the interval- that is to be 
	*	processed- is written
	*	@return False, if errors occured or there are no more leaves for the desired interval;
	*	true otherwise
	*/
	bool getFirst(const interval& search_Interval, key_Object * result, interval * lookup_Interval);
	
	/**
	*	Get the key that is the key of the next leaf; also look if it's the key of the next leaf, 
	*	and so on, to "return" the definitive lookup interval
	*	getFirst() must have been called before!
	*	@param result is the pointer to the key object which attributes are set according to the 
	*	stored values
	*	@param lookup_Interval is the pointer to the memory where the interval- that is to be 
	*	processed- is written
	*	@return False, if errors occured or there are no more leaves for the desired interval;
	*	true otherwise
	*/
	bool getNext(key_Object * result, interval * lookup_Interval);

private:
	void determineLookUpInterval(const interval& retrieved_Interval, interval * lookup_Interval) const;	
	bool getInterval(long leaf_Address, interval * leaf_Interval);
	bool getKey(long leaf_Address, key_Object * result);
	FILE * index_File;
	const r_Tree  * source;
	interval iv;
	r_Entry current_Entry;
};

#endif //key_au_iterator.h
