// r_entry.h

#ifndef _R_ENTRY_H_
#define _R_ENTRY_H_

#include <stdio.h>
#include "interval.h"

class r_Entry
{
public:
	r_Entry();
	r_Entry(const interval& i);
	
	
	long				getAddress() const				{return entry_Address;	}
	const interval&		getInterval() const				{return entry_Interval;}
	void				setInterval(const interval& i)	{entry_Interval = i;}

	void				print(FILE * f) const			{entry_Interval.print(f);}

	/**
	*	Only the file pointer is copied into the object; NO memcpy or similar
	*	MUST be called before invoking any i/o methods
	*	Does not check if the file pointer is valid (see archiver_Index::open())	
	*/
	void attach(FILE * f, long entry_Address);
	
	/**
	*	Set the R entry interval according to the values from the file this object was attached to
	*	@see attach()	
	*	@return False if errors occured, or attach() was not called before; true otherwise
	*/
	bool readInterval();

	/**
	*	Determine the index of the next/previous entry, the child or the parent; or the address of the 
	*	key AU pointer
	*	Inside an R tree each entry gets an index from the 
	*	rtree free space manager; pointers to the ouside of the tree are addresses
	*	@param result is a pointer which the values are read to from the file;
	*	a *result < 0 means there is no such construct
	*	@return False if i/o errors occured, or attach() was not called before; true otherwise
	*/
	bool readChildIndex(long * result) const;
	bool readParentIndex(long * result) const;
	bool readNextIndex(long * result) const;
	bool readPreviousIndex(long * result) const;
	bool readKeyPointer(long * result) const;
	
	/**
	*	Write the R entry interval to the file this object was attached to
	*	@see attach()	
	*	@return False if errors occured, or attach() was not called before; true otherwise
	*/
	bool writeInterval() const;
	
	/**
	*	Store the index of the next/previous entry, the child or the parent; or the address of the 
	*	key AU pointer in the file
	*	@return False if i/o errors occured, or attach() was not called before; true otherwise
	*/	
	bool writeChildIndex(long value) const;
	bool writeParentIndex(long value) const;
	bool writeNextIndex(long value) const;
	bool writePreviousIndex(long value) const;
	bool writeKeyPointer(long value) const;
private:
	interval entry_Interval;
	FILE * f;
	long entry_Address;
};

#endif 

