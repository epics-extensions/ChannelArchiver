// au_pointer_list.h

#ifndef _AU_POINTER_LIST_H_
#define _AU_POINTER_LIST_H_

#include <stdio.h>
#include "file_allocator.h"
#include "archiver_unit.h"
#include "au_pointer.h"
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
	au_Pointer_List(file_allocator * fa, long r_Tree_Offset);

	/**
	*	Loads the key AUP address
	*	@return false if errors occured; true otherwise
	*/
	bool init(long entry_Address);
	bool isEmpty() const	{return is_Empty;}

	/**
	*	The standard list operations; for insert criteria, see above
	*/
	bool insertAUPointer(long au_Address); 
	bool deleteAUPointer(long au_Address);		
	
	/**
	*	Copies AU pointers one by one
	*	@return the address of the copied AUP list, or -1 if errors occured
	*/
	long copyList();						
	
	void dump(FILE * text_File);

private:
	bool isTheSameAsPreviousKey(long au_Address);
	FILE * f;				//f & fa are pointers to the outside
	file_allocator * fa;
	const long r_Tree_Offset;
	long entry_Address;
	long key_AUP_Address;
	bool is_Empty;
	bool previous_Key_Compared;
};

#endif // _AU_POINTER_LIST_H_=======


