// r_Tree.h

#ifndef _R_TREE_H_
#define _R_TREE_H_

#include "array_list.h"
#include "au_pointer_list.h"
#include "file_allocator.h"
#include "r_entry.h"
#include "r_tree_free_space_manager.h"


//r_Entries on each level are sorted in the natural ascending way


class r_Tree
{
public:
	r_Tree();
	r_Tree(short _m);
	bool setM(short _m);

	FILE * getFile() const				{return f;}
	long getRootPointer() const			{return root.getRootPointer();}

	bool isEmpty() const;
	
	bool attach(file_allocator * fa, long root_Pointer, bool read_Only = false);
	void detach();
	
	long index2address(long index) const;
	long address2index(long address) const;

	bool findFirstLeaf(const interval * i, long * result) const;	//returns the index of the leaf

	bool getLatestAU(archiver_Unit * au) const;
	bool getTreeInterval(interval * i) const;

	//these functions DO NOT touch the AUs in the AU list
	
	//pre condition: AU is not in the tree
	bool addAU(long au_Address);
	//does not touch the AU itself only the entries
	bool removeAU(long au_Address);
	//pre condition: AU is in the tree; checks if it is really the latest
	bool updateLatestLeaf(long au_Address, const epicsTimeStamp& end_Time);
	
	//NOT NEEDED, but implemented bool isAUInTheTree(long au_Address);
	void dump(FILE * text_File) const;
	void writeDotFile(const char * name = "dot.txt") const;
	bool test() const;
	

private:


	bool findLastLeaf(const interval * i, long * result) const;	//returns the index of the leaf
	
	bool getEntryInNode(long current_Index, short number, long * result) const;
	//*result can never be negative
	bool getNumberOfEntriesInNode(long current_Index, short * result) const;
	bool getFirstEntryInNode(long current_Index, long * result) const;	//returns the index of the first entry in a node
	bool getLastEntryInNode(long current_Index, long * result) const;	//returns the index of the last entry in a node

	long addNewEntry(long previous_Index, long next_Index, long child_Index, long au_Address, const interval * i); 
	//returns the index of the new entry (caution: the parent index must yet be determined!)
	bool deleteEntry(long current_Index);
	
	// Allows updating the end time & number of samples.
	// Really only works with the last buffer returned by getActiveAUs
	// Otherwise, routine will
	// be the same as deleteAU() && addAU()
	//au_Address should be -1 before caqlling the function
	//if the update was successful true is returned and au_Address
	//will be set to -1; if the function returned true, but the au_Address
	//was set to an actual value- the old unit must be deleted first and then 
	//new one added

	bool splitParent(long parent_Index);
	bool setNewParent(long start_Entry_Index, long end_Entry_Index, long new_Parent_Index) const;
		
	bool tryToUpdateParent(const interval * i, long current_Index);
	bool tryToUpdateStartTimeOfTheParent(const interval * i, long current_Index);
	bool tryToUpdateEndTimeOfTheParent(const interval * i, long current_Index);
	bool tryToUpdateChildIndexOfTheParent(long current_Index);
	bool tryToUpdateLatestLeafIndexOfTheRoot(long current_Index);
	bool updateStartTime(const interval * i, long current_Index);
	bool updateEndTime(const interval * i, long current_Index);

	const long move();			//returns the new root address x
	
	void writeSpace(FILE * text_File, int n) const; //help routine for dump()

	FILE * f;				//f & fa are pointers to the outside
	file_allocator * fa;
	r_Tree_Root root;
	r_Tree_Free_Space_Manager rtfsm;
	long offset;			//address - index
	
	short m;
};

#endif // _R_TREE_H_

