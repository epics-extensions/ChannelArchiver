#include "au_pointer.h"
#include "bin_io.h"

au_Pointer::au_Pointer()
:	f(0), aup_Address(-1){}

long au_Pointer::getAddress() const	{return aup_Address;}

void au_Pointer::attach(FILE * f, long aup_Address)
{
	this->f = f;
	this->aup_Address = aup_Address;
}

//////////
//IO below
//////////

bool au_Pointer::readAUAddress(long * result) const
{
	const long ADDRESS = aup_Address + AUP_AU_OFFSET;
	fseek(f, ADDRESS, SEEK_SET);
	if(readLong(f, result) == false)
	{
		printf("Failed to read the address of an archiver unit from the address %ld \n", ADDRESS);
		return false;
	}
	return true;
}

bool au_Pointer::readNextAUPAddress(long * result) const
{
	const long ADDRESS = aup_Address + AUP_NEXT_OFFSET;
	fseek(f, ADDRESS, SEEK_SET);
	if(readLong(f, result) == false)
	{
		printf("Failed to read the next pointer of an archiver unit pointer from the address %ld \n", ADDRESS);
		return false;
	}
	return true;
}

bool au_Pointer::readPreviousAUPAddress(long * result) const
{
	const long ADDRESS = aup_Address + AUP_PREVIOUS_OFFSET;
	fseek(f, ADDRESS, SEEK_SET);
	if(readLong(f, result) == false)
	{
		printf("Failed to read the previous pointer of an archiver unit pointer from the address %ld \n", ADDRESS);
		return false;
	}
	return true;
}

bool au_Pointer::writeAUAddress(long value) const
{
	const long ADDRESS = aup_Address + AUP_AU_OFFSET;
	fseek(f, ADDRESS, SEEK_SET);
	if(writeLong(f, value) == false)
	{
		printf("Failed to write the address of an archiver unit to the address %ld \n", ADDRESS);
		return false;
	}
	return true;
}

bool au_Pointer::writeNextAUPAddress(long value) const
{
	const long ADDRESS = aup_Address + AUP_NEXT_OFFSET;
	fseek(f, ADDRESS, SEEK_SET);
	if(writeLong(f, value) == false)
	{
		printf("Failed to write the next pointer of an archiver unit pointer to the address %ld \n", ADDRESS);
		return false;
	}
	return true;
}

bool au_Pointer::writePreviousAUPAddress(const long value) const
{
	const long ADDRESS = aup_Address + AUP_PREVIOUS_OFFSET;
	fseek(f, ADDRESS, SEEK_SET);
	if(writeLong(f, value) == false)
	{
		printf("Failed to write the previous pointer of an archiver unit pointer to the address %ld \n", ADDRESS);
		return false;
	}
	return true;
}
