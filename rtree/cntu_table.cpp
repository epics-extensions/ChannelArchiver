#include "cntu_table.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "bin_io.h"
#include "rtree_constants.h"
#include "cntu_address_iterator.h"

cntu_Table::cntu_Table()
:  f(0), fa(0), table_Address(-1), size(-1){}

bool cntu_Table::attach(file_allocator * fa, long pointer_Address)
{
	if(size < 0) 
	{
		printf("The size of the channel names hash table is not known\n");
		return false;
	}
	if(pointer_Address < 0) return false;
	this->fa = fa;
	f = fa->getFile();
	fseek(f, pointer_Address, SEEK_SET);
	if(readLong(f, &table_Address) == false)
	{
		printf("Couldn't read the address of the cntu table\n");
		return false;
	}
	if(table_Address < 0) 
	{
		//the table was not yet initialized
		char * buffer = (char *) calloc(4 * size, 1);
		table_Address = fa->allocate(4 * size);
		fseek(f, table_Address, SEEK_SET);
		if(fwrite(buffer, 4 * size, 1, f) != 1)
		{
			free (buffer);
			return false;
		}
		free (buffer);
		fseek(f, pointer_Address, SEEK_SET);
		if(writeLong(f, table_Address) == false)
		{
			printf("Couldn't write the address of the cntu table\n");
			return false;
		}
	}
	return true;
}

void cntu_Table::detach()
{
	f = 0;
	fa = 0;
	table_Address = -1;
	size = -1;
}

bool cntu_Table::addCNTU(const char * new_Name, long * root_Pointer, long * au_List_Pointer) const
{
	if(new_Name == 0) 
	{
		printf("Tried to add an empty string\n");
		return false;
	}
	long hashed_Address = 4 * hash(new_Name) + table_Address;

	fseek(f, hashed_Address, SEEK_SET);
	long current_CNTU_Address; 
	if(readLong(f, &current_CNTU_Address) == false) return false;
	cntu current_CNTU = cntu();
	if(current_CNTU_Address < 1)
	{
		//nothing at that address yet
		long new_CNTU_Address = fa->allocate(CNTU_SIZE);
		fseek(f, hashed_Address, SEEK_SET);
		if(writeLong(f, new_CNTU_Address) == false)
		{
			printf("Failed to write the new cntu address for the address %ld\n", hashed_Address);
			return false;
		}
		current_CNTU.attach(f, new_CNTU_Address);
		*root_Pointer = new_CNTU_Address + CNTU_ROOT_OFFSET;
		*au_List_Pointer = new_CNTU_Address + CNTU_AU_LIST_OFFSET;
		return
			(
				current_CNTU.writeName(new_Name) &&
				current_CNTU.writeRootPointer(-1) &&
				current_CNTU.writeAUListPointer(-1) &&
				current_CNTU.writeNextCNTUAddress(-1) &&
				current_CNTU.writePreviousCNTUAddress(-1)
			);

	}
	
	current_CNTU.attach(f, current_CNTU_Address);
	long next_CNTU_Address;
	char current_Name[CHANNEL_NAME_LENGTH];

	do
	{
		current_CNTU_Address = current_CNTU.getAddress();

		
		if(current_CNTU.readName(current_Name) == false) return false;
		if(strcmp(current_Name, new_Name) == 0)
		{
			*root_Pointer = current_CNTU_Address + CNTU_ROOT_OFFSET;
			*au_List_Pointer = current_CNTU_Address + CNTU_AU_LIST_OFFSET;
			return true;
		}

		if(current_CNTU.readNextCNTUAddress(&next_CNTU_Address) == false) return false;
		if(next_CNTU_Address < 0)
		{
			long new_CNTU_Address = fa->allocate(CNTU_SIZE);
			//set the next pointer of the previous cntu
			current_CNTU.attach(f, current_CNTU_Address);
			if(current_CNTU.writeNextCNTUAddress(new_CNTU_Address) == false) return false;

			current_CNTU.attach(f, new_CNTU_Address);
			*root_Pointer = new_CNTU_Address + CNTU_ROOT_OFFSET;
			*au_List_Pointer = new_CNTU_Address + CNTU_AU_LIST_OFFSET;
			return
			(
				current_CNTU.writeName(new_Name) &&
				current_CNTU.writeRootPointer(-1) &&
				current_CNTU.writeAUListPointer(-1) &&
				current_CNTU.writeNextCNTUAddress(-1) &&
				current_CNTU.writePreviousCNTUAddress(-1)
			);
		}
		current_CNTU.attach(f, next_CNTU_Address);
	}
	while(true);
}

bool cntu_Table::deleteCNTU(long cntu_Address) const
{
	if(cntu_Address < 0) return true;

	cntu current_CNTU = cntu();
	current_CNTU.attach(f, cntu_Address);

	long next_CNTU_Address;
	long previous_CNTU_Address;
	char name[CHANNEL_NAME_LENGTH];
	if(	!current_CNTU.readName(name)						||
		!current_CNTU.readNextCNTUAddress(&next_CNTU_Address)		||
		!current_CNTU.readPreviousCNTUAddress(&previous_CNTU_Address)) return false;
	
	//free the cntu
	fa->free(current_CNTU.getAddress());

	if(previous_CNTU_Address < 0 && next_CNTU_Address < 0)
	{
		//means -1 must be written to the hash table
		long hashed_Address = 4 * hash(name) + table_Address;
		fseek(f, hashed_Address, SEEK_SET);
		return writeLong(f, -1);
	}
	else if(previous_CNTU_Address < 0 && next_CNTU_Address >= 0)
	{
		//means the next unit's address must be written to the table
		//the next unit must be updated
		long hashed_Address = 4 * hash(name) + table_Address;
		fseek(f, hashed_Address, SEEK_SET);
		if(writeLong(f, next_CNTU_Address) == false) return false;
		current_CNTU.attach(f, next_CNTU_Address);
		return current_CNTU.writePreviousCNTUAddress(-1);
	}
	else if(previous_CNTU_Address >=0 && next_CNTU_Address < 0)
	{
		//means only the previous unit must be updated
		current_CNTU.attach(f, previous_CNTU_Address);
		return current_CNTU.writeNextCNTUAddress(-1);
	}
	else
	{
		//means both units must be updated
		current_CNTU.attach(f, next_CNTU_Address);
		if(current_CNTU.writePreviousCNTUAddress(previous_CNTU_Address) == false) return false;
		current_CNTU.attach(f, previous_CNTU_Address);
		return current_CNTU.writeNextCNTUAddress(next_CNTU_Address);
	}
}

long cntu_Table::findCNTU(const char * name, long * root_Pointer, long * au_List_Pointer) const
{
	if(name == 0) 
	{
		printf("Tried to find an empty string\n");
		return -1;
	}
	long hashed_Address = 4 * hash(name) + table_Address;

	fseek(f, hashed_Address, SEEK_SET);
	long current_CNTU_Address; 
	if(readLong(f, &current_CNTU_Address) == false) return -1;
	if(current_CNTU_Address < 1)
	{
		return -1;
	}

	cntu current_CNTU;
	
	char current_Name[CHANNEL_NAME_LENGTH];
	
	while(true)
	{	
		current_CNTU.attach(f, current_CNTU_Address);

		if(current_CNTU.readName(current_Name) == false) return -1;
		if(strcmp(current_Name, name) == 0)
		{
			
			if(root_Pointer != 0) *root_Pointer = current_CNTU.getAddress() + CNTU_ROOT_OFFSET;
			if(au_List_Pointer != 0) *au_List_Pointer = current_CNTU.getAddress() + CNTU_AU_LIST_OFFSET;
			return current_CNTU.getAddress();
		}
		if(current_CNTU.readNextCNTUAddress(&current_CNTU_Address) == false) return -1;
		if(current_CNTU_Address < 0) return -1;		
	};	
}

void cntu_Table::dump(FILE * text_File)
{
	cntu current_CNTU = cntu();
	long address;
	cntu_Address_Iterator cai = cntu_Address_Iterator(this);
	if(cai.getFirst(&address) == false) return;
	int i =0;
	while(true)
	{
		if(address < 0) return;
		current_CNTU.attach(f, address);
		current_CNTU.print(text_File);
		fprintf(text_File, "\n");
		if(cai.getNext(&address) == false) return;
		i++;
	}
}


const short cntu_Table::hash(const char * word) const
{
	short h = 0;
	int i = 0;
    do
	{
		h = (128 * h + (short) word[i]) % size;
		i++;
	}
	while(word[i] != 0);
    return h;
}
