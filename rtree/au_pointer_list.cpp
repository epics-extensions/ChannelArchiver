#include <stdlib.h>
#include "au_pointer_list.h"

au_Pointer_List::au_Pointer_List(file_allocator * fa, long r_Tree_Offset)
:	r_Tree_Offset(r_Tree_Offset), entry_Address(-1), key_AUP_Address(-1), is_Empty(true), previous_Key_Compared(false)
{
	this->fa = fa;
	f = fa->getFile();
}

bool au_Pointer_List::init(long entry_Address)
{	
	if(entry_Address < 0) return false;
	this->entry_Address = entry_Address;
	r_Entry entry;
	entry.attach(f, entry_Address);
	if(entry.readKeyPointer(&key_AUP_Address) == false) return false;
	if(key_AUP_Address > 0) 
	{	
		is_Empty = false;
	}
	return true;
}

bool au_Pointer_List::isEmpty() const
{
	return is_Empty;
}


bool au_Pointer_List::insertAUPointer(long new_Archiver_Unit_Address)//SORTED!!!
{	
	if(entry_Address < 0)
	{
		printf("The au pointer list was not initialized\n");
		return false;
	}
	if(is_Empty)
	{
		long aup_Address = fa->allocate(AU_POINTER_SIZE);
		au_Pointer current_AU_Pointer = au_Pointer();
		current_AU_Pointer.attach(f, aup_Address);
		if(	!current_AU_Pointer.writeAUAddress(new_Archiver_Unit_Address) ||
			!current_AU_Pointer.writeNextAUPAddress(-1) ||
			!current_AU_Pointer.writePreviousAUPAddress(-1)
			) return false;

		//write the pointer of the entry
		r_Entry entry = r_Entry();
		entry.attach(f, entry_Address);		
		if(entry.writeKeyPointer(aup_Address) == false) return false;
		is_Empty = false;
		return true;
	}	
	previous_Key_Compared = false;

	au_Pointer current_AU_Pointer = au_Pointer();
	long current_AUP_Address = key_AUP_Address;

	long previous_AUP_Address = -1;	
	long current_Archiver_Unit_Address = -1;	
		
	archiver_Unit new_AU = archiver_Unit();
	new_AU.attach(f, new_Archiver_Unit_Address);
	if(new_AU.readAU() == false) return false;
	archiver_Unit current_AU = archiver_Unit();
	do
	{		
		current_AU_Pointer.attach(f, current_AUP_Address);

		if(current_AU_Pointer.readAUAddress(&current_Archiver_Unit_Address) == false) return false;
		//if the desired au is already there
		if(current_Archiver_Unit_Address == new_Archiver_Unit_Address) return false;
		current_AU.attach(f, current_Archiver_Unit_Address);
		if(current_AU.readAU() == false) return false;

		current_AUP_Address = current_AU_Pointer.getAddress();
		//works also when previous_Key_Compared is true i.e. with non - keys
		if(	(new_AU.getPriority() > current_AU.getPriority()) ||
			(	new_AU.getPriority() == current_AU.getPriority() &&
				(!isTheSameAsPreviousKey(current_Archiver_Unit_Address)	&&
					(	isTheSameAsPreviousKey(new_Archiver_Unit_Address) ||
						compareTimeStamps(new_AU.getInterval().getEnd(), current_AU.getInterval().getEnd()) > 0
					)
				)
			)
		  )
		{
			previous_Key_Compared = true;	
			//if the insert place in the list was found
			//insert before the current_AU_Pointer, and after the previous_AU_Pointer
			
			if(current_AU_Pointer.readPreviousAUPAddress(&previous_AUP_Address) == false) return false;
			
			
			long new_AUP_Address = fa->allocate(AU_POINTER_SIZE);
			
			current_AU_Pointer.attach(f, new_AUP_Address);
			if(	!current_AU_Pointer.writeAUAddress(new_Archiver_Unit_Address) ||
				!current_AU_Pointer.writeNextAUPAddress(current_AUP_Address) ||
				!current_AU_Pointer.writePreviousAUPAddress(previous_AUP_Address)) return false;

			
			//now integrate the new AU pointer into the au_Pointer_List
			if(previous_AUP_Address < 0)
			{
				//if the new au pointer is actually the key of the r_Entry, i.e. the head of the list
				r_Entry entry;
				entry.attach(f, entry_Address);
				if(entry.writeKeyPointer(new_AUP_Address) == false) return false;
			}
			else 
			{
				//if there is a previous pointer
				current_AU_Pointer.attach(f, previous_AUP_Address);
				if(current_AU_Pointer.writeNextAUPAddress(new_AUP_Address) == false) return false;
			}	

			//update the next pointer
			current_AU_Pointer.attach(f, current_AUP_Address);
			if(current_AU_Pointer.writePreviousAUPAddress(new_AUP_Address) == false) return false;
			return true;
		}

		long temp;
		if(current_AU_Pointer.readNextAUPAddress(&temp) == false) return false;
		if(temp < 0) break;
		current_AUP_Address = temp;
	}
	while(true);

	// insert at the end of the list, after the current_AU_Pointer
	long new_AUP_Address = fa->allocate(AU_POINTER_SIZE);

	current_AU_Pointer.attach(f, new_AUP_Address);
	if(	!current_AU_Pointer.writeAUAddress(new_Archiver_Unit_Address) ||
		!current_AU_Pointer.writeNextAUPAddress(-1) ||
		!current_AU_Pointer.writePreviousAUPAddress(current_AUP_Address)
		) return false;
	
	//write the parameter of the previously last au_Pointer
	current_AU_Pointer.attach(f, current_AUP_Address);
	if(current_AU_Pointer.writeNextAUPAddress(new_AUP_Address) == false) return false;
	return true;
}

bool au_Pointer_List::deleteAUPointer(long au_Address)
{
	if(entry_Address < 0)
	{
		printf("The au pointer list was not initialized\n");
		return false;
	}

	if(is_Empty) return true;

	au_Pointer current_AU_Pointer = au_Pointer();
	current_AU_Pointer.attach(f, key_AUP_Address);
	long current_AU_Address;	
	do
	{
		if(current_AU_Pointer.readAUAddress(&current_AU_Address) == false) return false;
		if(current_AU_Address == au_Address)
		{
			//the au pointer was found
			
			long previous_AUP_Address;
			if(current_AU_Pointer.readPreviousAUPAddress(&previous_AUP_Address) == false) return false;

			long next_AUP_Address;
			if(current_AU_Pointer.readNextAUPAddress(&next_AUP_Address) == false) return false;
						
			//give space free
			fa->free(current_AU_Pointer.getAddress());

			//now look how the delete affects the list
			if((next_AUP_Address < 0) && (previous_AUP_Address < 0))
			{
				//the only au pointer was deleted
				is_Empty = true;
				r_Entry entry;
				entry.attach(f, entry_Address);
				if(entry.writeKeyPointer(-1) == false) return false;
				return true;
			}
			if(previous_AUP_Address >= 0)
			{
				//update the previous au pointer
				current_AU_Pointer.attach(f, previous_AUP_Address);
				if(current_AU_Pointer.writeNextAUPAddress(next_AUP_Address) == false) return false;
			}
			else
			{
				//update the entry
				r_Entry entry;
				entry.attach(f, entry_Address);
				if(entry.writeKeyPointer(next_AUP_Address) == false) return false;
			}
			
			if(next_AUP_Address >=0)
			{
				current_AU_Pointer.attach(f, next_AUP_Address);
				if(current_AU_Pointer.writePreviousAUPAddress(previous_AUP_Address) == false) return false;
			}
			else
			{
				current_AU_Pointer.attach(f, previous_AUP_Address);
				if(current_AU_Pointer.writeNextAUPAddress(-1) == false) return false;
			}
			return true;
		};
		long temp;
		if(current_AU_Pointer.readNextAUPAddress(&temp) == false) return false;
		if(temp < 0) break;
		current_AU_Pointer.attach(f, temp);
	}
	while(true);
	return false;
};

long au_Pointer_List::copyList() 
{

	if(entry_Address < 0)
	{
		printf("The au pointer list was not initialized \n");
		return -1;
	}

	if(is_Empty) return -1;

	au_Pointer current_AU_Pointer = au_Pointer();
	current_AU_Pointer.attach(f, key_AUP_Address);
	long current_AU_Address = -1;
	long next_AUP_Address = -1;
	long previous_AUP_Address = -1;
	long last_Copied_AUP_Address = -1;
	long new_Key_Pointer = -1;
	long copied_AUP_Address = -1;
	do
	{
		if(	!current_AU_Pointer.readAUAddress(&current_AU_Address) ||
			!current_AU_Pointer.readNextAUPAddress(&next_AUP_Address)||
			!current_AU_Pointer.readPreviousAUPAddress(&previous_AUP_Address)
			)return -1;
		
		copied_AUP_Address = fa->allocate(AU_POINTER_SIZE);//allocate space for the new pointer		
		
		if(new_Key_Pointer < 0) 
		{
			new_Key_Pointer = copied_AUP_Address; //set it once only
		}

		current_AU_Pointer.attach(f, copied_AUP_Address);
		
		if(
			!current_AU_Pointer.writeAUAddress(current_AU_Address) ||
			!current_AU_Pointer.writeNextAUPAddress(-1) ||
			!current_AU_Pointer.writePreviousAUPAddress(last_Copied_AUP_Address) 
			) return -1;
		
		//set the next pointer of the previously copied pointer
		if(last_Copied_AUP_Address > 0)
		{
			current_AU_Pointer.attach(f, last_Copied_AUP_Address);
			current_AU_Pointer.writeNextAUPAddress(copied_AUP_Address);
		}
		last_Copied_AUP_Address = copied_AUP_Address;

		if(next_AUP_Address < 0) break; //end of the list
		current_AU_Pointer.attach(f, next_AUP_Address);
	}
	while(true);	

	return new_Key_Pointer;
}

void au_Pointer_List::dump(FILE * text_File) 
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
	
	au_Pointer current_AU_Pointer = au_Pointer();	
	current_AU_Pointer.attach(f, key_AUP_Address);
	long next_AUP_Address;
	long au_Address;
	archiver_Unit au = archiver_Unit(); 
	do
	{
		if(current_AU_Pointer.readAUAddress(&au_Address) == false) return;

		au.attach(f, au_Address);
		if(au.readAU() == false) return;
		au.print(text_File);
		
		
		if(current_AU_Pointer.readNextAUPAddress(&next_AUP_Address) == false) return;
		if(next_AUP_Address < 0) break;		//end of the list
		current_AU_Pointer.attach(f, next_AUP_Address);
		putc('|', text_File);
	}
	while(true);
}

bool au_Pointer_List::isTheSameAsPreviousKey(long au_Address)
{
	if(previous_Key_Compared) return false;	

	r_Entry entry;
	entry.attach(f, entry_Address);
	long previous_Index;
	if(entry.readPreviousIndex(&previous_Index) == false) return false;
	if(previous_Index < 0) return false;
	entry.attach(f, r_Tree_Offset + ENTRY_SIZE * previous_Index);
	long key_AUP_Address;
	if(entry.readKeyPointer(&key_AUP_Address) == false) return false;
	au_Pointer aup;
	aup.attach(f, key_AUP_Address);
	long previous_Key_AU_Address;
	if(aup.readAUAddress(&previous_Key_AU_Address) == false) return false;

	return previous_Key_AU_Address == au_Address;
}



