#include "r_tree_free_space_manager.h"
#include "bin_io.h"
#include <stdlib.h>

r_Tree_Free_Space_Manager::r_Tree_Free_Space_Manager()
:	f(0), start(-1), end(-1)
{}

bool r_Tree_Free_Space_Manager::attach(FILE * f, const r_Tree_Root& root)
{
	this->f = f;
	start = root.getAddress() + ROOT_SIZE + RTFSM_HEADER_SIZE;
	short i;
	if(root.readI(&i) == false) return false;
	long root_Child;
	if(root.readChildIndex(&root_Child) == false) return false;
	//if there is yet no tree
	long ADDRESS;
	if(root_Child < 0)
	{
		ADDRESS = root.getAddress() + ROOT_SIZE + RTFSM_REG_OFFSET;
		fseek(f, ADDRESS, SEEK_SET);
		if(writeLong(f, 0) == false) 
		{
			printf("Couldn't initialize the RTFSM REGISTER at the address %ld\n", ADDRESS);
			return false;
		}
		char * buffer =(char *) calloc(32, 1);
		ADDRESS = start;
		fseek(f, ADDRESS, SEEK_SET);
		if(fwrite(buffer, 32, 1, f) != 1) 
		{
			printf("Couldn't initialize the rtfsm at the address %ld\n", ADDRESS);
			free (buffer);
			return false;
		}
		free (buffer);
	}
	
	end = start + 32 * twoToThePowerOf(i) - 1;
	//initialize the arraylist to prevent having holes in the tree
	if(free_Indices.getNumberOfElements() != 0) 
	{
		free_Indices.reset();
	}

	ADDRESS = root.getAddress() + ROOT_SIZE + RTFSM_REG_OFFSET;
	long rtfsm_Register_Content;
	fseek(f, ADDRESS, SEEK_SET);
	if(readLong(f, &rtfsm_Register_Content) == false) 
	{
		printf("Couldn't read the RTFSM register from the address %ld\n", ADDRESS);
		return false;
	}
	for(long j = start; j < start + rtfsm_Register_Content; j++)
	{
		fseek(f, j, SEEK_SET);
		char buffer;
		if(fread(&buffer, 1, 1, f) != 1) 
		{
			printf("Couldn't read the RTFSM cell from the address %ld\n", j);
			return false;
		}
		if(buffer == 0)
		{
			free_Indices.insertElement(j - start);
		}

	}
	return true;
}

void r_Tree_Free_Space_Manager::detach()
{
	f = 0;
	start = end = -1 ;
	free_Indices.reset();
}

bool r_Tree_Free_Space_Manager::getFreeIndex(long * index)
{
	if(free_Indices.getNumberOfElements() < 1) 
	{
		//read the value of the rtfsm_register
		long ADDRESS = start - RTFSM_HEADER_SIZE + RTFSM_REG_OFFSET;
		fseek(f, ADDRESS, SEEK_SET);
		if(readLong(f, index) == false) 
		{
			printf("Couldn't read the RTFSM_REGISTER from the address %ld\n", ADDRESS);
			return false;
		}
		if(*index > end - start) 
		{
			*index = -1;	//there is no free index
			return true;
		}
		return writeIndexOccupied(true, *index);	
	}
	else
	{
		*index = free_Indices.popLastElement();
		return writeIndexOccupied(false, *index);	
	}
}

bool r_Tree_Free_Space_Manager::setIndexFree(long index) 
{
	//by design the RTFSM_REGISTER is not touched
	fseek(f, start + index, SEEK_SET);
	char buffer = 0;
	if(fwrite(&buffer, 1, 1, f) != 1)
	{
		printf("Couldn't write to a rtfsm cell at the address %ld\n", start + index);
		return false;
	}
	free_Indices.insertElement(index);
	return true;
}

bool r_Tree_Free_Space_Manager::writeIndexOccupied(bool updateRTFSMRegister, long index)
{
	long ADDRESS = start + index;
	fseek(f, ADDRESS, SEEK_SET);
	char buffer = 1;
	if(fwrite(&buffer, 1, 1, f) != 1) 
	{
		printf("Couldn't write to a rtfsm cell at the address %ld\n", ADDRESS);
		return false;
	}
	if(updateRTFSMRegister)
	{
		ADDRESS = start - RTFSM_HEADER_SIZE + RTFSM_REG_OFFSET;
		fseek(f, ADDRESS, SEEK_SET);
		if(writeLong(f, index + 1) == false)
		{
			printf("Couldn't write the RTFSM_REGISTER value to the address %ld\n", ADDRESS);
		}
	}
	return true;
}

bool r_Tree_Free_Space_Manager::move(long new_Root_Address)
{
	//copy the RTFSM header
	//double the size of rtfsm
	//copy the byte array
	long ADDRESS = start - RTFSM_HEADER_SIZE;
	fseek(f, ADDRESS, SEEK_SET);
	char buffer[RTFSM_HEADER_SIZE];
	if(fread(buffer, RTFSM_HEADER_SIZE, 1, f) != 1)
	{
		printf("Couldn't read the RTFSM header from the address %ld\n", ADDRESS);
		return false;
	}
	ADDRESS = new_Root_Address + ROOT_SIZE + RTFSM_HEADER_OFFSET;
	fseek(f, ADDRESS, SEEK_SET);
	if(fwrite(buffer, RTFSM_HEADER_SIZE, 1, f) != 1)
	{
		printf("Couldn't write the RTFSM header to the address %ld\n", ADDRESS);
		return false;
	}

	long number_Of_TFSM_Blocks = end - start + 1;
	char * _buffer = (char *) malloc(number_Of_TFSM_Blocks);
	ADDRESS = start;
	fseek(f, start, SEEK_SET); 
	if(fread(_buffer, number_Of_TFSM_Blocks, 1, f) != 1)
	{
		printf("Failed to read the r tree free space manager from the address %ld\n", ADDRESS);
		free (_buffer);
		return false;
	}
	ADDRESS = new_Root_Address + ROOT_SIZE + RTFSM_HEADER_SIZE;
	fseek(f, ADDRESS, SEEK_SET);
	if(fwrite(_buffer, number_Of_TFSM_Blocks, 1, f) !=1)
	{
		printf("Failed to write the r tree free space manager to the address %ld\n", ADDRESS);
		free(_buffer);
		return false;
	}
	free(_buffer);
	start = new_Root_Address + ROOT_SIZE + RTFSM_HEADER_SIZE;
	end = start + (number_Of_TFSM_Blocks * 2) - 1;	//double the free space
	//initialize the second half of the new Tree Free Space Manager
	_buffer = (char *) calloc(number_Of_TFSM_Blocks, 1);		//all zeroes
	fseek(f, start + number_Of_TFSM_Blocks, SEEK_SET);
	if(fwrite(_buffer, number_Of_TFSM_Blocks, 1, f)!=1)
	{
		printf("Failed to write the r tree free space manager to the address %ld\n", start + number_Of_TFSM_Blocks);
		free (_buffer);
		return false;
	};
	free (_buffer);

	return true;
}

