#include "au_list_iterator.h"
#include "archiver_unit.h"
#include "bin_io.h"
#include <stdlib.h>

au_List_Iterator::au_List_Iterator(FILE * f, long au_List_Pointer)
: f(f), current_AU_Address(-1), au_List_Pointer(au_List_Pointer)
{}

bool au_List_Iterator::getFirstAUAddress(const interval& i, long * result)
{
	if(i.isIntervalValid() == false)
    {
        printf("The specified interval is not valid\n");
        return false;
    }
	iv = i;
    fseek(f, au_List_Pointer, SEEK_SET);
    bool tmp = readLong(f, &current_AU_Address);
    archiver_Unit current_AU;
    while(tmp)
    {
        if(current_AU_Address < 0) return false;
        current_AU.attach(f, current_AU_Address);
        if(current_AU.readAU() == false) return false;
        if(iv.isIntervalOver(current_AU.getInterval()) == true)
        {
            *result = current_AU.getAddress();
            return true;
        }
        tmp = current_AU.readNextPointer(&current_AU_Address);
    }
    return false;
}

//current_AU is the one which address was returned last time
bool au_List_Iterator::getNextAUAddress(long * result)
{
	
	if(current_AU_Address < 0)
	{
		printf("You wanted a next AU, but next from WHERE on ?!?! (forgot to call getFirst)\n");
		return false;
	}
    
	archiver_Unit current_AU;
	current_AU.attach(f, current_AU_Address);
    if(current_AU.readNextPointer(&current_AU_Address) == false) return false;
    if(current_AU_Address < 0) return false;
    
    current_AU.attach(f, current_AU_Address);
    if(current_AU.readAU() == false) return false;
    if(iv.isIntervalOver(current_AU.getInterval()) == false) return false;
    *result = current_AU_Address;
    return true;
}





