#ifndef _R_TREE_FREE_SPACE_MANAGER_H_
#define _R_TREE_FREE_SPACE_MANAGER_H_
/**
*	RTFSM is essentially an array of bytes. Each byte represents the current state
*	of an R entry cell in the R tree: Zero byte means the cell is free, otherwise- occupied.
*	When attaching the object to the specific address, the "free_Indices" ArrayList is possibly 
*	initialized with free indices in the R tree up to the value stored in the RTFSM_REGISTER.
*	The RTFSM_REGISTER stores the first free index that is guaranteed to be followed by free indices
*	only (the feature is useful after, for instance, moving the R tree)
*	A free index is determined by either "popping" the ArrayList or reading the RTFSM_REGISTER.
*	Of course, the byte array in the file must be held up to date...
*/

#include <stdio.h>
#include "array_list.h"
#include "r_tree_root.h"

class r_Tree_Free_Space_Manager
{
public:
	r_Tree_Free_Space_Manager();

	/**
	*	Read the root parameters and set the object attributes according to them
	*	(if the tree is fresh, initialize the manager i.e. write an array of zero-bytes)
	*	Only the file pointer is copied into the object; NO memcpy or similar
	*	MUST be called before invoking any i/o methods
	*	@return False, if i/o errors occured; true otherwise		
	*/
	bool attach(FILE * f, const r_Tree_Root& root);

	/**
	*	Reset all object parameters
	*/
	void detach();

	/**
	*	Write a zero-byte to the corresponding index byte in the list
	*	Insert the free index into "free_Indices" ArrayList
	*	Is called after an R entry is deleted
	*	@param index is the index of the deleted R entry
	*	@return False if i/o errors occured; true otherwise
	*/
	bool setIndexFree(long index);	

	/**
	*	Find space for an R entry
	*	@param index is the pointer to which the free index is written;
	*	*index < 0 means there is no free space in that memory block, so a new one must be allocated
	*	and the tree must be moved 
	*	@return False, if i/o errors occured; true otherwise.
	*/
	bool getFreeIndex(long * index);

	/**
	*	Copy the byte array to the new address; append an array of zero-bytes and recalculate the 
	*	object attributes
	*	@return False if i/o errors occured; true otherwise
	*/
	bool move(long new_Root_Address);

private:
	bool writeIndexOccupied(bool updateRTFSMRegister, long index);
	FILE * f;	
	long start;
	long end;
	ArrayList free_Indices;
};

#endif
