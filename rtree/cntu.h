#ifndef _CNTU_H_
#define _CNTU_H_

#include <stdio.h>
#include "constants.h"

class cntu
{
public:
	cntu();
	long getAddress() const;
	void print(FILE * text_File);

	void attach(FILE * f, long cntu_Address);

	bool readName(char * name) const;
	bool readRootPointer(long * value) const;
	bool readAUListPointer(long * value) const;
	bool readNextCNTUAddress(long * address) const;
	bool readPreviousCNTUAddress(long * address) const;
	
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
