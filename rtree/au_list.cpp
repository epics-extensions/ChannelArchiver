#include "au_list.h"
#include "bin_io.h"

au_List::au_List(file_allocator * fa)
:	au_List_Pointer(-1), head(-1)
{
	this->fa = fa;
	f = fa->getFile();
}

bool au_List::init(long au_List_Pointer)
{
	this->au_List_Pointer = au_List_Pointer;
	fseek(f, au_List_Pointer, SEEK_SET);
	if(readLong(f, &head) == false)
	{
		printf("Couldn't read the head of the au list from the address %ld\n", au_List_Pointer);
		return false;
	}
	return true;
}

bool au_List::addAU(archiver_Unit * au, long * au_Address)
{
	if(au_List_Pointer < 0)
	{
		printf("The archiver units list was not initialized\n");
		*au_Address = -1; 
		return false;
	}

	long next_AU_Address;
	long previous_AU_Address;

	//the unit is already in the list
	//DON'T CONFUSE WITH CLOSING THE LAST UNIT etc.!!!
	if((*au_Address = findUnit(*au, &next_AU_Address, &previous_AU_Address)) > 0) 
	{
		return false;
	}
	else
	{
		//the unit is not in the list, so append it
		*au_Address = fa->allocate(au->getSize());
		au->attach(f, *au_Address);
		//write the previous and next pointers
		if (!au->writeAU() || !au->writePreviousPointer(previous_AU_Address) || !au->writeNextPointer(next_AU_Address)) 
		{
			*au_Address = -1;
			return false;
		}

		//set the next pointer of the previous au
		if(previous_AU_Address>0)
		{
			archiver_Unit tmp = archiver_Unit();
			tmp.attach(f, previous_AU_Address);
			if(tmp.writeNextPointer(*au_Address) == false)
			{
				*au_Address = -1;
				return false;
			}		
		}
		else
		{
			//if we added the first unit
			if(updateAUListPointer(*au_Address) == false) 
			{
				*au_Address = -1; 
				return false;			
			}
		}		
		//set the next pointer of the previous au
		if(next_AU_Address>0)
		{
			archiver_Unit tmp = archiver_Unit();
			tmp.attach(f, next_AU_Address);
			if(tmp.writePreviousPointer(*au_Address) == false)
			{
				*au_Address = -1; 
				return false;
			}
			
		}
		return true;
	}
	
}

bool au_List::detachUnit(const key_Object& au_Key, long * au_Address)
{
	if(au_List_Pointer < 0)
	{
		printf("The archiver units list was not initialized\n");
		return false;
	}


	archiver_Unit au;
	au.setKey(au_Key);
	//search the whole list
	interval temp = interval(COMPLETE_TIME_RANGE);
	au.setInterval(temp);	
	long next;
	long previous;
	*au_Address = findUnit(au, &next, &previous);
	if(*au_Address < 1)
	{
		//there is no such unit in the list
		return false;
	}
	else
	{

		//!!!!inform about the free space AFTER deleting in the tree, NOT here
		if(next > 0)
		{
			//update the next archiver unit
			au.attach(f, next);
			if(au.writePreviousPointer(previous) == false) return false;
		}

		if(previous < 1)
		{
			//update the head
			if(updateAUListPointer(next) == false) return false;
		}
		else
		{
			//update the previous archiver unit
			au.attach(f, previous);
			if(au.writeNextPointer(next) == false) return false;
		}
		return true;
	}
}

long au_List::findUnit(const key_Object& au_Key)
{
	archiver_Unit au;
	au.setKey(au_Key);
	//search the whole list
	interval temp = interval(COMPLETE_TIME_RANGE);
	au.setInterval(temp);
	long xyz;
	return findUnit(au, &xyz, &xyz);
}

long au_List::findUnit(const archiver_Unit& au, long * next_AU_Address, long * previous_AU_Address) const
{
	if(!au.getInterval().isIntervalValid()) return -1;
	if(au_List_Pointer < 0)
	{
		printf("The archiver units list was not initialized\n");
		return -1;
	}
	if(head < 0)
	{
		//no units yet
		*previous_AU_Address = -1;
		*next_AU_Address = -1;
		return -1;
	}

	long current_Address = head;
	archiver_Unit temp;
	while(current_Address >0)
	{
		temp.attach(f, current_Address);
		if(temp.readAU() == false) return -1;
		if(	!au.getInterval().isNull() &&
			compareTimeStamps(temp.getInterval().getStart(), au.getInterval().getStart()) < 0) 
		{
			//sorted by start times descending
			*next_AU_Address = current_Address;
			if(temp.readPreviousPointer(previous_AU_Address) == false) return -1;
			return -1;
		}
		if(temp == au)
		{
			if(	!temp.readNextPointer(next_AU_Address)	||
				!temp.readPreviousPointer(previous_AU_Address)
				) return -1;
			return current_Address;
		}
		if(temp.readNextPointer(&current_Address) == false) 
		{
			return -1;
		}
	}
	//at the end of the list
	*previous_AU_Address = temp.getAddress();
	*next_AU_Address = -1;
	return -1;
}

bool au_List::updateAUListPointer(long value) 
{
	fseek(f, au_List_Pointer, SEEK_SET);
	if(writeLong(f, value) == false) 
	{
		printf("Failed to write the new head of the au list to the address %ld\n", au_List_Pointer);
		return false;
	}
	head = value;
	return true;
}

