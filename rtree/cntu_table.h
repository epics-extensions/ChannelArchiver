#include "cntu.h"
#include "file_allocator.h"

#ifndef _cntu_Table_H_
#define _cntu_Table_H_

class cntu_Table
{
public:
	cntu_Table();
	
	void dump(FILE * text_File);
	long getAddress() const		{return table_Address;	}
	FILE * getFile() const		{return f;				}
	short getSize() const		{return size;			}
	void setSize(short size)	{this->size = size;		}

	bool attach(file_allocator * fa, long pointer_Address);
	void detach();
	
	//if the unit is there, it acts like findCNTU
	bool addCNTU(const char * new_Name, long * root_Pointer, long * au_List_Pointer) const;
	bool deleteCNTU(long cntu_Address) const;

	long findCNTU(const char * name, long * root_Pointer = 0, long * au_List_Pointer = 0) const;

private:
	const short hash(const char * word) const;
	FILE * f;
	file_allocator * fa;
	long table_Address;	//fixed
	short size;
};
#endif
