#include "cntu.h"
#include "file_allocator.h"

#ifndef _cntu_Table_H_
#define _cntu_Table_H_

/**
*	"channel name table unit" 
*	-------------------------
*	A standard hash table- in case of a collision the element is appended at the end of the
*	corresponding list
*/
class cntu_Table
{
public:
	cntu_Table();
	
	void dump(FILE * text_File);
	long getAddress() const		{return table_Address;	}
	FILE * getFile() const		{return f;				}
	
	/**
	*	Baiscally, the modulo parameter of the hash function
	*/
	short getSize() const		{return size;			}
	void setSize(short size)	{this->size = size;		}
		
	/**
	*	Only the file pointer is copied into the object; NO memcpy or similar
	*	MUST be called before invoking any i/o methods
	*	Does not check if the file pointer is valid (see archiver_Index::open())	
	*/
	bool attach(file_allocator * fa, long pointer_Address);
	
	/**
	*	Necessary for class archiver_Index in order to be able to reuse the archiver_Index object
	*/
	void detach();
	
	/**
	*	If the unit is there, the function acts exactly like findCNTU()
	*	@see findCNTU()
    *   @param new_Name MUST be less than CHANNEL_NAME_LENGTH characters
	*	@param root_Pointer is the pointer to the memory block to which the address of the root 
	*	pointer is written
	*	@param au_List_Pointer is the pointer to the memory block to which the address of
	*	the AU list pointer is written
	*	@return false ONLY if errors occured; true otherwise
	*/	
	bool addCNTU(const char * new_Name, long * root_Pointer, long * au_List_Pointer) const;
	
	/**
	*	Deletes the CNTU from the hash table; how to get the address:
	*	@see findCNTU()
	*	@param cntu_Address may be -1; then immediately "true" is returned
	*	@return false ONLY if errors occured; true otherwise
	*/
	bool deleteCNTU(long cntu_Address) const;

	/**
	*	@param root_Pointer is the pointer to the memory block to which the address of the root 
	*	pointer is written
	*	@param au_List_Pointer is the pointer to the memory block to which the address of
	*	the AU list pointer is written
	*	@return the address of the CNTU, or -1 if the "name" is not there
	*/
	long findCNTU(const char * name, long * root_Pointer = 0, long * au_List_Pointer = 0) const;

private:
	const short hash(const char * word) const;
	FILE * f;
	file_allocator * fa;
	long table_Address;	//fixed
	short size;
};
#endif
