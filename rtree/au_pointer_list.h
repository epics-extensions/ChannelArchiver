// au_pointer_list.h

#ifndef _AU_POINTER_LIST_H_
#define _AU_POINTER_LIST_H_

#include <stdio.h>
#include "file_allocator.h"
#include "archiver_unit.h"
#include "au_pointer.h"
#include "r_entry.h"

//criteria for being the key:
//if my priority is the biggest 
//OR 
//if my priority is the same as the currently biggest AND "if the current key is NOT the previous key" 
//AND 
//("I reach longer into future" OR "I am the key of the previous AU")
class au_Pointer_List 
{
public:
	au_Pointer_List(file_allocator * fa, long r_Tree_Offset);
	bool init(long entry_Address);
	bool isEmpty() const;

	//don't forget to set previous_Key_Compared to true, after comparing the keys
	bool insertAUPointer(long au_Address); 
	bool deleteAUPointer(long au_Address);		
	long copyList();						//returns the address of the new list head (= key) or -1
	
	void dump(FILE * text_File);			//writes a line of the au keys 
private:

	bool isTheSameAsPreviousKey(long au_Address);
	FILE * f;				//pointers to the outside
	file_allocator * fa;
	const long r_Tree_Offset;
	long entry_Address;
	long key_AUP_Address;
	bool is_Empty;
	bool previous_Key_Compared;
};

#endif // _AU_POINTER_LIST_H_=======


