#ifndef _CNTU_H_
#define _CNTU_H_

#include <stdio.h>
#include <stdString.h>
#include "rtree_constants.h"

class cntu
{
public:
	cntu();
	long getAddress() const;
	void print(FILE * text_File);

	/**
	*	Only the file pointer is copied into the object; NO memcpy or similar
	*	MUST be called before invoking any i/o methods
	*	Does not check if the file pointer is valid (see archiver_Index::open())	
	*/
	void attach(FILE * f, long cntu_Address);

	/**
	*	Determine the channel name, root pointer address, au list pointer address, or
	*	the address of the next/previous cntu and store them in the memory blocks
	*	pointed to by corresponding parameters
	*	CNTUs are stored in a hash table and then in a list
	*	name == 0 or *result < 1 means there is no such construct
	*	@return False if i/o errors occured, or attach() was not called before; true otherwise
	*/
	bool readName(stdString * name) const;
	bool readRootPointer(long * value) const;
	bool readAUListPointer(long * value) const;
	bool readNextCNTUAddress(long * address) const;
	bool readPreviousCNTUAddress(long * address) const;
	
	/**
	*	Store the channel name, root pointer address, au list pointer address, or
	*	the address of the next/previous cntu in the index file
	*	@return False if i/o errors occured, or attach() was not called before; true otherwise
	*/	
	bool writeName(const char * name) const;
	bool writeRootPointer(long value) const;
	bool writeAUListPointer(long value) const;
	bool writeNextCNTUAddress(long address) const;
	bool writePreviousCNTUAddress(long address) const;

private:
	FILE * f;
	long cntu_Address;
};


#endif
