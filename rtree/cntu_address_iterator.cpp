#include "cntu_address_iterator.h"
#include "bin_io.h"
#include "string.h"

cntu_Address_Iterator::cntu_Address_Iterator(const cntu_Table * t)
:	t(t), hashed_Address(-1), current_CNTU_Address(-1)
{
	f = t->getFile();	
}

bool cntu_Address_Iterator::getFirst(long * result)
{
	return getFirstForHashedAddress(result, t->getAddress());	
}

bool cntu_Address_Iterator::getNext(long * result)
{
	if(hashed_Address < 1)
	{
		printf("You want to get the next channel, but: \"next\" counting from where?!?!?\n");
		return false;
	}

	cntu current_CNTU;
	current_CNTU.attach(f, current_CNTU_Address);
	if(current_CNTU.readNextCNTUAddress(&current_CNTU_Address) == false)
	{
		printf("Couldn't read the next pointer of the cntu from the address %ld\n", current_CNTU.getAddress());
		return false;
	}
	if(current_CNTU_Address < 1)
	{
		//if there are no more channels in that list
		return getFirstForHashedAddress(result, hashed_Address + 4);
	}
	else
	{
		*result = current_CNTU_Address;
		return true;
	}
}

bool cntu_Address_Iterator::getFirstForHashedAddress(long * result, long _hashed_Address)
{
	//at this point current_CNTU_Address is -1
	hashed_Address = _hashed_Address;
	do
	{
		if(hashed_Address >= t->getAddress() + 4 * t->getSize()) //started counting with zero (0)
		{
			*result = -1;
			return true;
		}
		fseek(f, hashed_Address, SEEK_SET);
		
		if(readLong(f, &current_CNTU_Address) == false)
		{
			printf("Couldn't read the cntu address from the address %ld\n", hashed_Address);
			return false;
		}
		if(current_CNTU_Address > 0)
		{
			*result = current_CNTU_Address;
			return true;
		}
		hashed_Address = hashed_Address + 4;
	}
	while(true);
	
}


