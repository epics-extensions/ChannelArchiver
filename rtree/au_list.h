// au_list.h

#ifndef _AU_LIST_H_
#define _AU_LIST_H_

/**	
*	This class manages the list of archived units; each r Tree has its own AU list
*	SORTED BY START TIMES DESC 
*	P.S. The decision was not to delete the AUs separately, see also archiver_Index::deleteTree()
*/

#include "archiver_unit.h"
#include "file_allocator.h"

class au_List
{
public:
	au_List(file_allocator * fa);

	/**
	*	Loads the address of the head AU of the list
	*	@param au_List_Pointer is the address where the address of the actual head AU can be found
	*	@return false if errors occured; true otherwise
	*	Note: The knowledge of the au_List_Pointer is vital if the head AU of the 
	*	list changes
	*/
	bool init(long au_List_Pointer);

	/*
	*	@param au_Address is the pointer to the memory block to which the address of 
	*	the AU; write -1 if errors occured
	*	@return false, if errors occured, or the AU has already existed; true otherwise
	*/
	bool addAU(archiver_Unit * a, long * au_Address) ;	

	/**
	*	@return the address of the au that has the same key as "au_Key", or -1
	*/
	long findUnit(const key_Object& au_Key);	

private:
	bool updateAUListPointer(long value);	
	long findUnit(const archiver_Unit& au, long * next_AU_Address, long * previous_AU_Address) const;
	FILE * f;	
	file_allocator * fa;				//pointers to the outside!!!
	long au_List_Pointer;
	long head;
};



#endif // _AU_LIST_H_




