#include "r_tree_root.h"
#include "r_tree_free_space_manager.h"
#include "bin_io.h"

r_Tree_Root::r_Tree_Root()
:	f(0), fa(0), root_Address(-1), root_Pointer(-1){}

//////////
//IO below
//////////

bool r_Tree_Root::attach(file_allocator * fa, long root_Pointer)
{
	if(root_Pointer < 0)
	{
		printf("Wrong root pointer\n");
		return false;
	}
	this->root_Pointer = root_Pointer;
	this->fa = fa;
	f = fa->getFile();
	if(readTreeAddress() == false) return false;
	if(root_Address < 0)
	{
		//there is yet no tree address
		//the 1 is for the rtfsm
		root_Address = fa->allocate(ROOT_SIZE + RTFSM_HEADER_SIZE + (1 + ENTRY_SIZE) * 32);
		return 
			(
				writeNullTreeInterval() &&
				writeChildIndex(-1) &&
				writeLatestLeafIndex(-1) &&
				writeI(0) &&
				writeTreeAddress()
			);
	}
	return true;
}

void r_Tree_Root::detach()
{
	f = 0;
	fa = 0;
	root_Address = root_Pointer = -1;
}

bool r_Tree_Root::readTreeInterval(interval * result) const
{
	result->attach(f, root_Address + ROOT_IV_OFFSET);
	return result->readInterval();
}



bool r_Tree_Root::readChildIndex(long * result) const
{
	const long ADDRESS = root_Address + ROOT_CHILD_OFFSET;
	fseek(f, ADDRESS, SEEK_SET);
	if(readLong(f, result) == false)
	{
		printf("Failed to read the child index of the root from the address %ld \n", ADDRESS);
		return false;
	}
	return true;
}

bool r_Tree_Root::readLatestLeafIndex(long * result) const
{
	const long ADDRESS = root_Address + ROOT_LATEST_OFFSET;
	fseek(f, ADDRESS, SEEK_SET);
	if(readLong(f, result) == false)
	{
		printf("Failed to read the last leaf index of the root from the address %ld \n", ADDRESS);
		return false;
	}
	return true;
}

bool r_Tree_Root::readI(short * result) const
{
	const long ADDRESS = root_Address + ROOT_I_OFFSET;
	fseek(f, ADDRESS, SEEK_SET);
	if(readShort(f, result) == false)
	{
		printf("Failed to read the parameter I of the root at the address %ld \n", ADDRESS);
		return false;
	}
	return true;
}

bool r_Tree_Root::writeTreeInterval(interval & value) const
{
	value.attach(f, root_Address + ROOT_IV_OFFSET);
	return value.writeInterval();
}

bool r_Tree_Root::writeNullTreeInterval() const
{
	interval temp = interval(NULL_INTERVAL);
	temp.attach(f, root_Address + ROOT_IV_OFFSET);
	return temp.writeInterval();
}

bool r_Tree_Root::writeChildIndex(long value) const
{
	const long ADDRESS = root_Address + ROOT_CHILD_OFFSET;
	fseek(f, ADDRESS, SEEK_SET);
	if(writeLong(f, value) == false)
	{
		printf("Failed to write the child index of the root to the address %ld \n", ADDRESS);
		return false;
	}
	return true;
}

bool r_Tree_Root::writeLatestLeafIndex(long value) const
{
	const long ADDRESS = root_Address + ROOT_LATEST_OFFSET;
	fseek(f, ADDRESS, SEEK_SET);
	if(writeLong(f, value) == false)
	{
		printf("Failed to write the last leaf index of the root to the address %ld \n", ADDRESS);
		return false;
	}
	return true;
}

bool r_Tree_Root::writeI(short value) const
{
	const long ADDRESS = root_Address + ROOT_I_OFFSET;
	fseek(f, ADDRESS, SEEK_SET);
	if(writeShort(f, value) == false)
	{
		printf("Failed to write the parameter I of the root to the address %ld \n", ADDRESS);
		return false;
	}
	return true;
}

bool r_Tree_Root::move(long new_Address)
{
	interval tree_Interval;
	long child_Index;
	long latest_Leaf_Index;
	short i;
	if(	!readTreeInterval(&tree_Interval) ||
		!readChildIndex(&child_Index) ||
		!readLatestLeafIndex(&latest_Leaf_Index) ||
		!readI(&i) 
		) return false;
	root_Address = new_Address;
	return
		(
			writeTreeInterval(tree_Interval) &&
			writeChildIndex(child_Index) &&
			writeLatestLeafIndex(latest_Leaf_Index) &&
			writeI(i + 1)&&
			writeTreeAddress()
		);
}

bool r_Tree_Root::readTreeAddress()
{
	fseek(f, root_Pointer, SEEK_SET);
	if(readLong(f, &root_Address) == false)
	{
		printf("Can't read the address of the R tree from the address %ld \n", root_Pointer);
		return false;
	}
	return true;
}

bool r_Tree_Root::writeTreeAddress() const
{
	fseek(f, root_Pointer, SEEK_SET);
	if(writeLong(f, root_Address) == false)
	{
		printf("Can't write the address of the R tree to the address %ld \n", root_Pointer);
		return false;
	}
	return true;
}
