#ifndef _AU_POINTER_H_
#define _AU_POINTER_H_

#include <stdio.h>
#include "rtree_constants.h"

class au_Pointer
{
public:
	au_Pointer();
	long getAddress() const;

	/**
	*	Only the file pointer is copied into the object; NO memcpy or similar
	*	MUST be called before invoking any i/o methods
	*	Does not check if the file pointer is valid (see archiver_Index::open())	
	*/
	void attach(FILE * f, long aup_Address);
	
	/**
	*	Determine the address of the AU or the next/previous AU pointer
	*	AU pointers are stored inside a list
	*	@param result is a pointer which the values are read to from the file;
	*	a *result < 1 means there is no such construct
	*	@return False if i/o errors occured, or attach() was not called before; true otherwise
	*/
	bool readAUAddress(long * result) const;
	bool readNextAUPAddress(long * result) const;
	bool readPreviousAUPAddress(long * result) const;

	/**
	*	Store the address of the AU or the next/previous AU pointer in the index file
	*	@return False if i/o errors occured, or attach() was not called before; true otherwise
	*/	
	bool writeAUAddress(long value) const;
	bool writeNextAUPAddress(long value) const;
	bool writePreviousAUPAddress( long value) const;

private:
	FILE * f;
	long aup_Address;
};

#endif

