#include "channel_name_iterator.h"
#include "bin_io.h"
#include "string.h"

channel_Name_Iterator::channel_Name_Iterator(const cntu_Table * t)
: cai(cntu_Address_Iterator(t))
{
	f = t->getFile();
}

bool channel_Name_Iterator::getFirst(stdString * result)
{
	long cntu_Address;
	if(cai.getFirst(&cntu_Address) == false) return false;
	if(cntu_Address < 0) return false;
	current_CNTU.attach(f, cntu_Address);
	return current_CNTU.readName(result);
}

bool channel_Name_Iterator::getNext(stdString * result)
{
	long cntu_Address;
	if(cai.getNext(&cntu_Address) == false) return false;
	if(cntu_Address < 0) return false;
	current_CNTU.attach(f, cntu_Address);
	return current_CNTU.readName(result);	
}

