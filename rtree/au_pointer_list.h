// au_pointer_list.h

#ifndef _AU_POINTER_LIST_H_
#define _AU_POINTER_LIST_H_

#include <stdio.h>
#include "file_allocator.h"
#include "archiver_unit.h"
#include "r_entry.h"

/**
*	Is used inside the class r_Tree only!
*	-------------------------------------
*	Criteria for being the key:
*	- if my priority is the biggest 
*	- OR 
*	- (if my priority is the same as the currently biggest AND "if the current key is NOT the previous key") 
*	- AND 
*	- ("I reach longer into future" OR "I am the key of the previous AU")
*/

class au_Pointer_List 
{
public:
	au_Pointer_List(file_allocator * fa);
    au_Pointer_List(FILE * f); //just for reads
	/**
	*	Loads the key AUP address
	*	@return false if errors occured; true otherwise
	*/
	bool init(long entry_Address);
	bool isEmpty() const	{return is_Empty;}

    /**
    *  needed for getLatestAU() and udateAU()     
    */
    bool onlyOneAU() const {return (current_Size - supply) == 1;}   //needed for getLatestAU()

    /*
    *   @return the address of the key AU   
    */
    long getKeyAUAddress();
	/**
	*	The standard list operations; for insert criteria, see above
    *   Note: Since AUs are added by iterating through the leaves,
    *   a previous iteration can set the previous_Key_AU_Address
	*/
	bool insertAUPointer(long au_Address, long previous_Key_AU_Address); 
	bool deleteAUPointer(long au_Address);		
	
	/**
	*	Copies AU pointers one by one
	*	@return the address of the copied AUP list, or -1 if errors occured
	*/
	long copyList();						
	
	void dump(FILE * text_File, const char * separator = "|");

private:
    bool move();
	bool flushToFile();
    bool writeAUAddress(long value, long aup_Address); //takes care of supply decrease


	FILE * f;				//f & fa are pointers to the outside
	file_allocator * fa;
	long entry_Address;
	long list_Address;
	short current_Size;
    short supply;
	bool is_Empty;
};

#endif // _AU_POINTER_LIST_H_=======


