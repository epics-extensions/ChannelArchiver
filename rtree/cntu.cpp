#include "cntu.h"
#include "bin_io.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdString.h>

cntu::cntu()
:	f(0), cntu_Address(-1) {}

long cntu::getAddress() const
{
	return cntu_Address;
}

void cntu::print(FILE * text_File)
{
	stdString buffer;
	if(readName(&buffer) == false) return;
	long value;
	if(readRootPointer(&value) == false) return;
	fprintf(text_File, "The tree for channel %s is @ the address %ld ", buffer.c_str(), value);
	return;
}

void cntu::attach(FILE * f, long cntu_Address)
{
	this->f = f;
	this->cntu_Address = cntu_Address;
}


bool cntu::readName(stdString * name) const
{
	const long ADDRESS = cntu_Address + CNTU_NAME_OFFSET;
	fseek(f, ADDRESS, SEEK_SET);
    char temp[CHANNEL_NAME_LENGTH];
	if(fread(temp, CHANNEL_NAME_LENGTH, 1, f)!=1)
	{
		printf("Couldn't read the channel name from the address %ld\n", ADDRESS);
		return false;
	}
    *name = stdString(temp);
	return true;
}

bool cntu::readRootPointer(long * value) const
{    
	const long ADDRESS = cntu_Address + CNTU_ROOT_OFFSET;
	fseek(f, ADDRESS, SEEK_SET);
	if(readLong(f, value) == false)
	{
		printf("Couldn't read the root pointer of the cntu from the address %ld\n", ADDRESS);
		return false;
	}
	return true;
}

bool cntu::readAUListPointer(long * value) const
{
	const long ADDRESS = cntu_Address + CNTU_AU_LIST_OFFSET;
	fseek(f, ADDRESS, SEEK_SET);
	if(readLong(f, value) == false)
	{
		printf("Couldn't read the AU list pointer of the cntu from the address %ld\n", ADDRESS);
		return false;
	}
	return true;
}

bool cntu::readNextCNTUAddress(long * address) const
{
	const long ADDRESS = cntu_Address + CNTU_NEXT_OFFSET;
	fseek(f, ADDRESS, SEEK_SET);
	if(readLong(f, address) == false)
	{
		printf("Couldn't read the next pointer of the cntu from the address %ld\n", ADDRESS);
		return false;
	}
	return true;
}

bool cntu::readPreviousCNTUAddress(long * address) const
{
	const long ADDRESS = cntu_Address + CNTU_PREVIOUS_OFFSET;
	fseek(f, ADDRESS, SEEK_SET);
	if(readLong(f, address) == false)
	{
		printf("Couldn't read the previous pointer of the cntu from the address %ld\n", ADDRESS);
		return false;
	}
	return true;
}

bool cntu::writeName(const char * name) const
{	
	const long ADDRESS = cntu_Address + CNTU_NAME_OFFSET;
	fseek(f, ADDRESS, SEEK_SET);
	if(fwrite(name, CHANNEL_NAME_LENGTH, 1, f)!=1)
	{
		printf("Couldn't write the channel name to the address %ld\n", ADDRESS);
		return false;
	}
	return true;
}

bool cntu::writeRootPointer(long value) const
{
	const long ADDRESS = cntu_Address + CNTU_ROOT_OFFSET;
	fseek(f, ADDRESS, SEEK_SET);
	if(writeLong(f, value) == false)
	{
		printf("Couldn't write the root pointer of the cntu to the address %ld\n", ADDRESS);
		return false;
	}
	return true;
}


bool cntu::writeAUListPointer(long value) const
{
	const long ADDRESS = cntu_Address + CNTU_AU_LIST_OFFSET;
	fseek(f, ADDRESS, SEEK_SET);
	if(writeLong(f, value) == false)
	{
		printf("Couldn't write the AU list pointer of the cntu to the address %ld\n", ADDRESS);
		return false;
	}
	return true;
}

bool cntu::writeNextCNTUAddress(long address) const
{
	const long ADDRESS = cntu_Address + CNTU_NEXT_OFFSET;
	fseek(f, ADDRESS, SEEK_SET);
	if(writeLong(f, address) == false)
	{
		printf("Couldn't write the next pointer of the cntu to the address %ld\n", ADDRESS);
		return false;
	}
	return true;
}

bool cntu::writePreviousCNTUAddress(long address) const
{
	const long ADDRESS = cntu_Address + CNTU_PREVIOUS_OFFSET;
	fseek(f, ADDRESS, SEEK_SET);
	if(writeLong(f, address) == false)
	{
		printf("Couldn't write the previous pointer of the cntu to the address %ld\n", ADDRESS);
		return false;
	}
	return true;
}

