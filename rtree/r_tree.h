// r_Tree.h

#ifndef _R_TREE_H_
#define _R_TREE_H_

#include "archiver_unit.h"
#include "array_list.h"
#include "file_allocator.h"
#include "r_entry.h"
#include "r_tree_free_space_manager.h"


/**
*	For any menaing, see user's manual
*/


class r_Tree
{
public:
	r_Tree();
	r_Tree(short _m);
	bool setM(short _m);

	FILE * getFile() const				{return f;}
	/**
	*	@return the address to where the root address is written
	*/
	long getRootPointer() const			{return root.getRootPointer();}

	/**
	*	@return true if errors occured, or the tree is indeed empty; false otherwise
	*/
	bool isEmpty() const;
	
	/**
	*	Only the file pointer is copied into the object; NO memcpy or similar
	*	MUST be called before invoking any methods!
	*	@param read_Only tells if RTFSM must be instantiated ("false") or not ("true")
	*	Does not check if the file pointer is valid (see archiver_Index::open())	
	*/
	bool attach(file_allocator * fa, long root_Pointer, bool read_Only = false);
	
	/**
	*	Necessary for class archiver_Index in order to be able to reuse the archiver_Index object
	*/
	void detach();
	
	long index2address(long index) const;
	long address2index(long address) const;

	/**
	*	@param result is the pointer to the memory black the index of the first leaf is written to
	*	(-1 means the R tree does not store data from the specified interval)
	*	@return false if errors occured; true otherwise
	*/
	bool findFirstLeaf(const interval * i, long * result) const;

	/**
	*	@param au is the pointer to the AU object which parameters are set according to the values
	*	in the index file
	*	@return false if errors occured, a "latest AU" does not exist; true otherwise
	*/
	bool getLatestAU(archiver_Unit * au) const;
	
	/**
	*	@param i is the pointer to the interval object which parameters are set according to the values
	*	in the index file
	*	@return false if errors occured; true otherwise
	*/
	bool getTreeInterval(interval * i) const;

	/**
	*	The three next methods DO NOT touch the AUs in the AU list; only the entries in the R tree
	*/
	bool addAU(long au_Address);
	bool removeAU(long au_Address);
	
	/**
	*	@return false if the latest leaf's only key is not the AU with the specified address, or
	*	errors occured; true otherwise
	*/
	bool updateLatestLeaf(long au_Address, const epicsTimeStamp& end_Time);
	
	void dump(FILE * text_File) const;
	/**
	*	see archiver_Index::writeDotFile()
	*/
	void writeDotFile(const char * name = "dot.txt") const;
	
	/**
	*	@return true if the tree is consistent; false otherwise
	*/
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

