#include <stdlib.h>
#include <string.h>
#include "key_au_iterator.h"
#include "au_pointer.h"
#include "archiver_unit.h"

key_AU_Iterator::key_AU_Iterator(const r_Tree * source)
:	index_File(0), source(source), no_More_Leaves(false)
{}


/*
*	- Find the first leaf that intersects the desired Interval
*	- Get its key AU
*	If errors occured, return false
*/
bool key_AU_Iterator::getFirst(const interval& search_Interval, key_Object * result, interval * lookup_Interval)
{
	if(search_Interval.isIntervalValid() == false)
	{
		printf("The desired interval for the key AU iterator is not valid\n");
		return false;
	}
	iv = search_Interval;
	index_File = source->getFile();
	long first_Leaf;
	if(source->findFirstLeaf(&iv, &first_Leaf) == false) return false;
	
	interval initial_Interval;
	long first_Address = source->index2address(first_Leaf);
	if(	!getKey(first_Address, result) ||
		!checkInterval(first_Address, &initial_Interval)) return false;

	key_Object temp;
	interval next_Interval = initial_Interval;
	long next_Leaf;
	long next_Address;
	while(true)
	{
		if(current_Entry.readNextIndex(&next_Leaf) == false) return false;
		if(next_Leaf < 0)
		{
			no_More_Leaves = true;
			break;
		}
		next_Address = source->index2address(next_Leaf);
		if(	!getKey(next_Address, &temp)	||
			!(temp == *result)) break;
		if( !checkInterval(next_Address, &next_Interval)) break;
	}
	//now current_Leaf is attached to the next AU!
	
	interval retrieved_Interval;
	retrieved_Interval.setStart(initial_Interval.getStart());
	retrieved_Interval.setEnd(next_Interval.getEnd());
	determineLookUpInterval(retrieved_Interval, lookup_Interval);	
	return true;
}

bool key_AU_Iterator::getNext(key_Object * result, interval * lookup_Interval)
{
	if(index_File == 0)
	{
		printf("You wanted a next AU, but next counting from where?!?! (forgot to call getFirst())\n");
		return false;
	}
	
	if(no_More_Leaves) return false;
	interval initial_Interval;
	if(	!getKey(current_Entry.getAddress(), result) ||
		!checkInterval(current_Entry.getAddress(), &initial_Interval)) return false;

	key_Object temp;
	interval next_Interval = initial_Interval;
	long next_Leaf;
	long next_Address;
	while(true)
	{
		if(current_Entry.readNextIndex(&next_Leaf) == false) return false;
		if(next_Leaf < 0)
		{
			no_More_Leaves = true;
			break;
		}
		next_Address = source->index2address(next_Leaf);
		if(	!getKey(next_Address, &temp)	||
			!(temp == *result)) break;
		if( !checkInterval(next_Address, &next_Interval)) break;
	}
	//now current_Leaf is attached to the next AU!
	
	interval retrieved_Interval;
	retrieved_Interval.setStart(initial_Interval.getStart());
	retrieved_Interval.setEnd(next_Interval.getEnd());
	determineLookUpInterval(retrieved_Interval, lookup_Interval);	
	return true;
}

bool key_AU_Iterator::checkInterval(long leaf_Address, interval * leaf_Interval)
{
	current_Entry.attach(index_File, leaf_Address);
	if(current_Entry.readInterval() == false) return false;
	//check if the leaf contains the AU with data for the desired interval
	//here the intervals are treated as being "inclusive"
	if(compareTimeStamps(current_Entry.getInterval().getStart(), iv.getEnd()) > 0) return false;

	*leaf_Interval = current_Entry.getInterval();

	return true;
}

bool key_AU_Iterator::getKey(long leaf_Address, key_Object * result)
{
	current_Entry.attach(index_File, leaf_Address);
	long key_Pointer_Address;
	if(current_Entry.readKeyPointer(&key_Pointer_Address) == false) return false;

	au_Pointer aup;
	aup.attach(index_File, key_Pointer_Address);
	long key_Address;
	if(aup.readAUAddress(&key_Address) == false) return false;

	archiver_Unit au;
	au.attach(index_File, key_Address);
	if(au.readKey() == false) return false;	

	*result = au.getKey();
	return true;
}

//pre condition: the intervals iv and leaf_Interval intersect
void key_AU_Iterator::determineLookUpInterval(const interval& retrieved_Interval, interval * lookup_Interval) const
{
	if(compareTimeStamps(iv.getStart(), retrieved_Interval.getStart()) >= 0)
	{
		lookup_Interval->setStart(iv.getStart());
	}
	else
	{
		lookup_Interval->setStart(retrieved_Interval.getStart());
	}

	if(compareTimeStamps(iv.getEnd(), retrieved_Interval.getEnd()) >= 0)
	{
		lookup_Interval->setEnd(retrieved_Interval.getEnd());
	}
	else
	{
		lookup_Interval->setEnd(iv.getEnd());
	}
}

