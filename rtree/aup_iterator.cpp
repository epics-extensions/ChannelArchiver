#include "aup_iterator.h"
#include "r_entry.h"
#include "au_pointer.h"
#include <stdlib.h>

aup_Iterator::aup_Iterator(const r_Tree * r)
:	r(r), current_Leaf_Address(-1), current_AU_Pointer_Address(-1)
{}

bool aup_Iterator::getFirstAUAddress(const interval& i, long * result)
{
	if(i.isIntervalValid() == false) return false;
	if(r->getFile() == 0) 
	{
		printf("The tree to be iterated is not attached to a file\n");
		return false;
	}
	if(i.isIntervalValid() == false) return false;
	iv = i;	
	long first_Leaf;
	if(r->findFirstLeaf(&iv, &first_Leaf) == false) return false;
	if(first_Leaf < 0)
	{
		*result = -1;
		return true;
	}
	current_Leaf_Address = r->index2address(first_Leaf);
	return getKeyAUAddressOfTheCurrentLeaf(result);
}


bool aup_Iterator::getNextAUAddress(long * result)
{
	*result = -1;
	if(current_Leaf_Address < 0)
	{
		printf("You wanted a next aup, but next from WHERE on ?!?! (forgot to call getFirst)\n");
		return false;
	}
	au_Pointer aup;
	aup.attach(r->getFile(), current_AU_Pointer_Address);
	long next;
	if(aup.readNextAUPAddress(&next) == false) return false;

	if(next < 0)
	{
		r_Entry entry;
		entry.attach(r->getFile(), current_Leaf_Address);
		long next_Index;
		if(entry.readNextIndex(&next_Index) == false) return false;
		if(next_Index < 0)
		{
			*result = -1;
			return true;
		}
		current_Leaf_Address = r->index2address(next_Index);
		return getKeyAUAddressOfTheCurrentLeaf(result);
	}
	
	current_AU_Pointer_Address = next;
	aup.attach(r->getFile(), current_AU_Pointer_Address);
	long au_Address;
	if(aup.readAUAddress(&au_Address) == false) 	return false;
	for(int i=0; i < aup_Addresses.getNumberOfElements();i++)
	{
		if(aup_Addresses.getElement(i) == au_Address)
		{
			//if it's already in the list
			return getNextAUAddress(result);
		}
	}
	aup_Addresses.insertElement(au_Address);
	*result = au_Address;
	return true;
}

bool aup_Iterator::getKeyAUAddressOfTheCurrentLeaf(long * result)
{
	*result = -1;
	r_Entry entry;
	entry.attach(r->getFile(), current_Leaf_Address);

	if(entry.readInterval() == false) return false;
	if(compareTimeStamps(entry.getInterval().getEnd(), iv.getStart()) <= 0) return true;

	if(entry.readKeyPointer(&current_AU_Pointer_Address) == false) return false;
	au_Pointer aup;
	aup.attach(r->getFile(), current_AU_Pointer_Address);
	long au_Address;
	if(aup.readAUAddress(&au_Address) == false) return false;
	for(int i=0; i < aup_Addresses.getNumberOfElements();i++)
	{
		if(aup_Addresses.getElement(i) == au_Address)
		{
			//if it's already in the list
			return getNextAUAddress(result);
		}
	}
	aup_Addresses.insertElement(au_Address);
	*result = au_Address;
	return true;
}

