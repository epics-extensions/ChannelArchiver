#include "r_entry.h"
#include "bin_io.h"

r_Entry::r_Entry()
:	f(0), entry_Address(-1){}

r_Entry::r_Entry(const interval & i)
:	f(0), entry_Address(-1)
{
	entry_Interval = i;
}

//////////
//IO BELOW
//////////

void r_Entry::attach(FILE * f, long entry_Address)
{
	this->f = f;
	this->entry_Address = entry_Address;
	entry_Interval.attach(f, entry_Address + ENTRY_IV_OFFSET);
}

bool r_Entry::readInterval()
{
	if(f == 0 || entry_Address < 0) 
	{
		printf("No file specified\n");
		return false;
	}
	return entry_Interval.readInterval();
}

bool r_Entry::readChildIndex(long * result) const
{
	if(f == 0 || entry_Address < 0) 
	{
		printf("No file specified\n");
		return false;
	}
	
	const long ADDRESS = entry_Address + ENTRY_CHILD_OFFSET;
	fseek(f, ADDRESS, SEEK_SET);
	if(readLong(f, result) == false) 
	{
		printf("Failed to read the child index of the entry from the address %ld \n", ADDRESS);
		return false;
	}
	return true;
}

bool r_Entry::readParentIndex(long * result) const
{
	if(f == 0 || entry_Address < 0) 
	{
		printf("No file specified\n");
		return false;
	}

	const long ADDRESS = entry_Address + ENTRY_PARENT_OFFSET;
	fseek(f, ADDRESS, SEEK_SET);
	if(readLong(f, result) == false) 
	{
		printf("Failed to read the parent index of the entry from the address %ld \n", ADDRESS);
		return false;
	}
	return true;
}
	
bool r_Entry::readNextIndex(long * result) const
{
	if(f == 0 || entry_Address < 0) 
	{
		printf("No file specified\n");
		return false;
	}

	const long ADDRESS = entry_Address + ENTRY_NEXT_OFFSET;
	fseek(f, ADDRESS, SEEK_SET);
	if(readLong(f, result) == false) 
	{
		printf("Failed to read the next index of the entry from the address %ld \n", ADDRESS);
		return false;
	}
	return true;
}

bool r_Entry::readPreviousIndex(long * result) const
{
	if(f == 0 || entry_Address < 0) 
	{
		printf("No file specified\n");
		return false;
	}


	const long ADDRESS = entry_Address + ENTRY_PREVIOUS_OFFSET;
	fseek(f, ADDRESS, SEEK_SET);
	if(readLong(f, result) == false) 
	{
		printf("Failed to read the previous index of the entry from the address %ld \n", ADDRESS);
		return false;
	}
	return true;
}

bool r_Entry::readKeyAddress(long * result) const
{
	if(f == 0 || entry_Address < 0) 
	{
		printf("No file specified\n");
		return false;
	}

	const long ADDRESS = entry_Address + ENTRY_KEY_OFFSET;
	fseek(f, ADDRESS, SEEK_SET);
	if(readLong(f, result) == false) 
	{
		printf("Failed to read the key address of the entry from the address %ld \n", ADDRESS);
		return false;
	}
	return true;
}
	
bool r_Entry::writeInterval() const
{
	return entry_Interval.writeInterval();
}

bool r_Entry::writeChildIndex(long value) const
{
	if(f == 0 || entry_Address < 0) 
	{
		printf("No file specified\n");
		return false;
	}

	const long ADDRESS = entry_Address + ENTRY_CHILD_OFFSET;
	fseek(f, ADDRESS, SEEK_SET);
	if(writeLong(f, value) == false) 
	{
		printf("Failed to write the child index of the entry to the address %ld \n", ADDRESS);
		return false;
	}
	return true;
}

bool r_Entry::writeParentIndex(long value) const
{
	if(f == 0 || entry_Address < 0) 
	{
		printf("No file specified\n");
		return false;
	}

	const long ADDRESS = entry_Address + ENTRY_PARENT_OFFSET;
	fseek(f, ADDRESS, SEEK_SET);
	if(writeLong(f, value) == false) 
	{
		printf("Failed to write the parent index of the entry to the address %ld \n", ADDRESS);
		return false;
	}
	return true;
}

bool r_Entry::writeNextIndex(long value) const
{
	if(f == 0 || entry_Address < 0) 
	{
		printf("No file specified\n");
		return false;
	}

	const long ADDRESS = entry_Address + ENTRY_NEXT_OFFSET;
	fseek(f, ADDRESS, SEEK_SET);
	if(writeLong(f, value) == false) 
	{
		printf("Failed to write the next index of the entry to the address %ld \n", ADDRESS);
		return false;
	}
	return true;
}

bool r_Entry::writePreviousIndex(long value) const
{
	if(f == 0 || entry_Address < 0) 
	{
		printf("No file specified\n");
		return false;
	}

	const long ADDRESS = entry_Address + ENTRY_PREVIOUS_OFFSET;
	fseek(f, ADDRESS, SEEK_SET);
	if(writeLong(f, value) == false) 
	{
		printf("Failed to write the previous index of the entry to the address %ld \n", ADDRESS);
		return false;
	}
	return true;	
}

bool r_Entry::writeKeyAddress(long value) const
{
	if(f == 0 || entry_Address < 0) 
	{
		printf("No file specified\n");
		return false;
	}
	
	const long ADDRESS = entry_Address + ENTRY_KEY_OFFSET;
	fseek(f, ADDRESS, SEEK_SET);
	if(writeLong(f, value) == false) 
	{
		printf("Failed to write the key address of the entry to the address %ld \n", ADDRESS);
		return false;
	}
	return true;
}







