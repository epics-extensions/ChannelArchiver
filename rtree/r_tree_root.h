// r_tree_root.h

#ifndef _R_TREE_ROOT_H_
#define _R_TREE_ROOT_H_

#include <stdio.h> 
#include "interval.h"
#include "file_allocator.h"

class r_Tree_Root
{
public:
	r_Tree_Root();
	long getAddress() const		{return root_Address;	}
	long getRootPointer() const	{return root_Pointer;	}
	
	/**
	*	Only the file pointer is copied into the object; NO memcpy or similar
	*	MUST be called before invoking any i/o methods
	*	Does not check if the file pointer is valid (see archiver_Index::open())	
	*/
	bool attach(file_allocator * fa, long root_Pointer);

	void detach();
	
	/**
	*	Set the object attributes according to the values from the file this object was attached to
	*	@see attach()	
	*	@return False if errors occured, or attach() was not called before; true otherwise
	*/
	bool readTreeInterval(interval * result) const;
	bool readChildIndex(long * result) const;
	bool readLatestLeafIndex(long * result) const;
	bool readI(short * result) const;
	
	/**
	*	Write the object attributes to the file this object was attached to
	*	@see attach()	
	*	@return False if errors occured, or attach() was not called before; true otherwise
	*/
	bool writeNullTreeInterval() const;	//writes the NULL_INTERVAL defined in constants.h
	bool writeTreeInterval(interval & value) const;	
	bool writeChildIndex(long value) const;
	bool writeLatestLeafIndex(long value) const;
	bool writeI(short value) const;

	/**
	*	Copy the adjusted root values to the specified address	
	*	@return False if errors occured; true otherwise.
	*/
	bool move(long new_Address);

private:
	bool readTreeAddress();
	bool writeTreeAddress() const;

	FILE * f;				//no memory allocated
	file_allocator * fa;	//no memory allocated
	long root_Address;
	long root_Pointer;
};

#endif

