// au_List.h

#ifndef _AU_LIST_H_
#define _AU_LIST_H_

/*	This class represents a list of archived units
*	that are used to store the archived information 
*	inside the index file
*	SORTED BY START TIMES DESC (
*/

#include "archiver_unit.h"
#include "file_allocator.h"

class au_List
{
public:
	au_List(file_allocator * fa);
	bool init(long au_List_Pointer);
	//returns false if an error or au exists
	//if au exists au_Address > 0; if error au_Address < 0
	bool addAU(archiver_Unit * a, long * au_Address) ;	//return the address of the unit
	//detachUnit only detaches the unit from the list, but does NOT
	//free the space!
	bool detachUnit(const key_Object& au, long * au_Address);
	long findUnit(const key_Object& au_Key);	

private:
	bool updateAUListPointer(long value);	
	//if unit is in the list, its address is returned
	//if not, -1 is returned, and the addresses of the two units between which it can be added are set
	long findUnit(const archiver_Unit& au, long * next_AU_Address, long * previous_AU_Address) const;
	FILE * f;	
	file_allocator * fa;				//pointers to the outside!!!
	long au_List_Pointer;
	long head;
};



#endif // _AU_LIST_H_




