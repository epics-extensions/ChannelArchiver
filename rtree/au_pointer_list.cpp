#include <stdlib.h>
#include "au_pointer_list.h"
#include "bin_io.h"
#include "r_entry.h"

au_Pointer_List::au_Pointer_List(file_allocator * fa)
        :	fa(fa), entry_Address(-1), list_Address(-1), current_Size(-1), supply(-1), is_Empty(true)
{
	f = fa->getFile();
}

au_Pointer_List::au_Pointer_List(FILE * f)
:	f(f), entry_Address(-1), list_Address(-1), current_Size(-1), supply(-1), is_Empty(true)
{
    fa = 0;
}

bool au_Pointer_List::init(long entry_Address)
{	
	if(entry_Address < 0) return false;
    this->entry_Address = entry_Address;
	r_Entry entry;
	entry.attach(f, entry_Address);
	if(entry.readKeyPointer(&list_Address) == false) return false;
	if(list_Address > 0) 
	{	
		is_Empty = false;
        //load object attributes
        long ADDRESS = list_Address + AUP_LIST_SIZE_OFFSET;
        fseek(f, ADDRESS, SEEK_SET);
        if(readShort(f, &current_Size) == false)
        {
            printf("Couldn't read the size of the AU pointer list from the address %ld\n", ADDRESS);
            return false;
        }
        ADDRESS = list_Address + AUP_LIST_SUPPLY_OFFSET;
        fseek(f, ADDRESS, SEEK_SET);
        if(readShort(f, &supply) == false)
        {
            printf("Couldn't read the supply of the AU pointer list from the address %ld\n", ADDRESS);
            return false;
        }        
    }
	return true;
}

long au_Pointer_List::getKeyAUAddress()
{
    long ADDRESS =  list_Address + AUP_LIST_HEADER_SIZE;
    fseek(f, ADDRESS, SEEK_SET);
    long temp;
    if(readLong(f, &temp) == false)
    {
        printf("Couldn't read the key AU address from the address %ld\n", ADDRESS);
        return -1;
    }
    return temp;    
}

bool au_Pointer_List::insertAUPointer(long new_AU_Address, long previous_Key_AU_Address)//SORTED!!!
{	
	if(entry_Address < 0)
	{
		printf("The au pointer list was not initialized\n");
		return false;
	}
    if(fa == 0)
    {
        printf("the file allocator was not passed\n");
        return false;
    }
	if(is_Empty)
	{
		list_Address = fa->allocate(AUP_LIST_HEADER_SIZE + 4 * 8);

        //initialize the object attributes
        current_Size = supply = 8;

        //the list can not be full
        return flushToFile() && writeAUAddress(new_AU_Address, list_Address + AUP_LIST_HEADER_SIZE);       
	}
    //look if the aup list is full
    if(supply == 0 && move() == false) return false;
    
	archiver_Unit new_AU = archiver_Unit();
	new_AU.attach(f, new_AU_Address);
	if(new_AU.readAU() == false) return false;

    archiver_Unit current_AU = archiver_Unit();
    long current_AU_Pointer  = list_Address + AUP_LIST_HEADER_SIZE;
    long current_AU_Address;
    
    while (current_AU_Pointer < list_Address + AUP_LIST_HEADER_SIZE + 4 * (current_Size - supply))
	{
		fseek(f, current_AU_Pointer, SEEK_SET);
        if(readLong(f, &current_AU_Address) == false)
        {
            printf("Couldn't read the address of an AU from the address %ld\n", current_AU_Pointer);
            return false;
        }

        //if the au is already there; shouldn't happen though
        if(current_AU_Address == new_AU_Address) return true;
        
		current_AU.attach(f, current_AU_Address);
		if(current_AU.readAU() == false) return false;
        
		//the criteria for insertion, see also the file au_pointer_list.h
		if(	(new_AU.getPriority() > current_AU.getPriority()) ||
			(	new_AU.getPriority() == current_AU.getPriority() &&
				current_AU_Address != previous_Key_AU_Address	&&
                (
                    (new_AU_Address == previous_Key_AU_Address) ||
                    (compareTimeStamps(new_AU.getInterval().getEnd(), current_AU.getInterval().getEnd()) > 0)
                )
			)
		  )
		{
			return writeAUAddress(new_AU_Address, current_AU_Pointer);
		}
        current_AU_Pointer = current_AU_Pointer + 4;
    };
    //insert at the end of the list
    return writeAUAddress(new_AU_Address, current_AU_Pointer);
}

bool au_Pointer_List::deleteAUPointer(long au_Address)
{
	if(entry_Address < 0)
	{
		printf("The au pointer list was not initialized\n");
		return false;
	}

	if(is_Empty) return true;
    
    long current_AU_Pointer  = list_Address + AUP_LIST_HEADER_SIZE;
    long current_AU_Address;
	while(true)
	{
        fseek(f, current_AU_Pointer, SEEK_SET);
        if(readLong(f, &current_AU_Address) == false)
        {
            printf("Couldn't read the address of an AU from the address %ld\n", current_AU_Pointer);
            return false;
        }
	
		if(current_AU_Address == au_Address)
		{
			//the au pointer was found
            const int BUFFER_SIZE = list_Address + AUP_LIST_HEADER_SIZE + 4 * current_Size - (current_AU_Pointer + 4);
            char buffer[BUFFER_SIZE];
            long ADDRESS = current_AU_Pointer + 4;
            fseek(f, ADDRESS, SEEK_SET);
            if(fread(buffer, BUFFER_SIZE, 1, f) != 1)
            {
                printf("Couldn't read the AU pointers from the address %ld on\n", ADDRESS);
                return false;
            }
            ADDRESS = current_AU_Pointer;
            fseek(f, ADDRESS, SEEK_SET);
            if(fwrite(buffer, BUFFER_SIZE, 1, f) != 1)
            {
                printf("Couldn't write AU pointers to the address %ld and so on\n", ADDRESS);
                return false;
            }

            supply++;
            r_Entry entry;
            if(supply == current_Size)
            {
                is_Empty = true;
                fa->free(list_Address);
                entry.attach(f, entry_Address);
                return entry.writeKeyPointer(-1);
            }
            else
            {
                ADDRESS = list_Address + AUP_LIST_SUPPLY_OFFSET;
                fseek(f, ADDRESS, SEEK_SET);
                if(writeLong(f, supply) == false)
                {
                    printf("Couldn't write the new supply value to the address %ld", ADDRESS);
                }
                return true;
            }
            
        };
		current_AU_Pointer = current_AU_Pointer + 4;
	};
	return true;
};

long au_Pointer_List::copyList() 
{

	if(entry_Address < 0)
	{
		printf("The au pointer list was not initialized \n");
		return -1;
	}
    if(fa == 0)
    {
        printf("The file allocator was not passed\n");
        return -1;
    }
	if(is_Empty) return -1;

    const int BUFFER_SIZE = AUP_LIST_HEADER_SIZE + 4 * current_Size;
    long new_List_Address = fa->allocate(BUFFER_SIZE);
    char buffer[BUFFER_SIZE];
    long ADDRESS = list_Address;
    fseek(f, ADDRESS, SEEK_SET);
    if(fread(buffer, BUFFER_SIZE, 1, f)!= 1)
    {
        printf("Couldn't read the AU pointer list from the address %ld\n", ADDRESS);
        return -1;
    }
    ADDRESS = new_List_Address;
    fseek(f, ADDRESS, SEEK_SET);
    if(fwrite(buffer, BUFFER_SIZE, 1, f) != 1)
    {
        printf("Couldn't write the AU pointer list to the address %ld\n", ADDRESS);
        return -1;
    }
    return new_List_Address;
}

void au_Pointer_List::dump(FILE * text_File, const char * separator) 
{
	if(entry_Address < 0)
	{
		printf("The au pointer list was not initialized\n");
		return;
	}

	if(is_Empty) 
	{
		fprintf(f, "No au pointers found\n");
		return;
	}
	
	long current_AU_Address = list_Address + AUP_LIST_HEADER_SIZE;
	long au_Address;
	archiver_Unit au; 
	while(current_AU_Address < list_Address + AUP_LIST_HEADER_SIZE + 4 * (current_Size - supply))
	{
		fseek(f, current_AU_Address, SEEK_SET);
        if(readLong(f, &au_Address) == false) return;

		au.attach(f, au_Address);
		if(au.readAU() == false) return;
		au.print(text_File);
        current_AU_Address = current_AU_Address + 4;
        fputs(separator, text_File);
	};
    return;
}

bool au_Pointer_List::move()
{
    const long SIZE_IN_BYTES = AUP_LIST_HEADER_SIZE + 4 * current_Size;
    char buffer[SIZE_IN_BYTES];
    fseek(f, list_Address, SEEK_SET);
    if(fread(buffer, SIZE_IN_BYTES, 1, f) != 1)
    {
        printf("Couldn't read the AU pointer list from the address %ld\n", list_Address);
        return false;
    }
    long new_Address = fa->allocate(AUP_LIST_HEADER_SIZE + 4 * current_Size * 2);
    fseek(f, new_Address, SEEK_SET);
    if(fwrite(buffer, SIZE_IN_BYTES, 1, f) != 1)
    {
        printf("Couldn't write the AU pointer list to the address %ld\n", new_Address);
        return false;
    }
    //free the old space
    fa->free(list_Address);
    //reinitialize the parameters
    list_Address = new_Address;
    supply = current_Size;
    current_Size = current_Size * 2;
    return flushToFile();
}

bool au_Pointer_List::flushToFile()
{
//update the aup list header
    long ADDRESS = list_Address + AUP_LIST_SIZE_OFFSET;
    fseek(f, ADDRESS , SEEK_SET);
    if(writeShort(f, current_Size) == false)
    {
        printf("Couldn't write the initial size of the AU pointer list to the address %ld\n", ADDRESS);
        return false;
    }
    ADDRESS =  list_Address + AUP_LIST_SUPPLY_OFFSET;
    fseek(f, ADDRESS , SEEK_SET);
    if(writeShort(f, supply) == false)
    {
        printf("Couldn't write the initial supply of the AU pointer list to the address %ld\n", ADDRESS);
        return false;
    }

    //update the entry
    r_Entry entry;
    entry.attach(f, entry_Address);
    if(entry.writeKeyPointer(list_Address) == false) return false;
    return true;
}

//if unsure, check if the list if full
bool au_Pointer_List::writeAUAddress(long value, long aup_Address)
{
    long ADDRESS = -1;
    //if the au pointer is not the last, move subsequent AU pointers form the aup_Address on
    long first_Free_Address = list_Address + AUP_LIST_HEADER_SIZE + 4 * (current_Size - supply);
    if(aup_Address < first_Free_Address) //the formula calculates the first free address
    {
        const long SUBSEQUENT_BYTES = first_Free_Address - aup_Address;
        char buffer[SUBSEQUENT_BYTES];
        ADDRESS = aup_Address;
        fseek(f, ADDRESS, SEEK_SET);
        if(fread(buffer, SUBSEQUENT_BYTES, 1, f) != 1)
        {
            printf("Couldn't read the AU pointers from the address %ld on\n", ADDRESS);
            return false;
        }
        ADDRESS = aup_Address + 4;
        fseek(f, ADDRESS, SEEK_SET);
        if(fwrite(buffer, SUBSEQUENT_BYTES, 1, f) != 1)
        {
            printf("Couldn't write the AU pointers to the address %ld on\n", ADDRESS);
            return false;
        }        

        
    }
    ADDRESS = aup_Address;
    fseek(f, ADDRESS, SEEK_SET);
    if(writeLong(f, value) == false)
    {
        printf("Couldn't write the address of a new AU to the address %ld\n", ADDRESS);
        return false;
    }

    //update the parameter supply in the file
    supply--;
    ADDRESS = list_Address + AUP_LIST_SUPPLY_OFFSET;
    fseek(f, ADDRESS, SEEK_SET);
    if(writeShort(f, supply) == false)
    {
        printf("Couldn't write the new supply value to the address %ld\n", ADDRESS);
        return false;
    }
    
	return true;
}
