#include "r_tree.h"
#include <stdlib.h>
#include "rtree_constants.h"

r_Tree::r_Tree()
:	f(0), fa(0), offset(-1), m(0){}

r_Tree::r_Tree(short _m)
:	f(0), fa(0), offset(-1), m(_m){}

bool r_Tree::setM(short _m)
{
	if(f!=0) return false;
	m = _m;
	return true;
}

bool r_Tree::attach(file_allocator * fa, long root_Pointer, bool read_Only)
{
	this->fa = fa;
	if(root.attach(fa, root_Pointer) == false) return false;
	f = fa->getFile();
	if(!read_Only && !rtfsm.attach(f, root)) return false;
	short i;
	if(root.readI(&i) == false) return false;
	offset = root.getAddress() + ROOT_SIZE + RTFSM_HEADER_SIZE + 32 * twoToThePowerOf(i);
	return true;
}

void r_Tree::detach()
{
	f = 0;
	fa = 0;
	offset = -1;
	m = 0;
	root.detach();
	rtfsm.detach();
}

bool r_Tree::isEmpty() const
{
	long root_Child;
	if(root.readChildIndex(&root_Child) == false) return true;
	return (root_Child < 0);
}

bool r_Tree::getLatestAU(archiver_Unit * au) const
{
	long latest_Leaf;
	if(root.readLatestLeafIndex(&latest_Leaf) == false) return false;
	r_Entry entry = r_Entry();
	entry.attach(f, index2address(latest_Leaf));	
	long key_Address;
	if(entry.readKeyAddress(&key_Address) == false) return false;

	if(key_Address < 0) 
	{
		printf("Corrupt key in the entry at the address %ld\n", index2address(latest_Leaf));
		return false;
	}

	au->attach(f, key_Address);
	return au->readAU();
}

bool r_Tree::getTreeInterval(interval * i) const
{
	return root.readTreeInterval(i);
}

bool r_Tree::addAU(long au_Address)
{	
	if(au_Address < 0) return false;
	archiver_Unit au;
	au.attach(f, au_Address);
	if(au.readInterval() == false) return false;
	const interval au_Interval = au.getInterval();
	r_Entry entry = r_Entry();

	//first see if the new au is actually the latest for the tree
	long last_Leaf;
	if(root.readLatestLeafIndex(&last_Leaf) == false) return false;
	if(last_Leaf >= 0)
	{
		entry.attach(f, index2address(last_Leaf));
		if(entry.readInterval() == false) return false;
		
		if(compareTimeStamps(au_Interval.getStart(), entry.getInterval().getEnd()) >=0)
		{
			//means: yes, we have the latest leaf
			return (addNewEntry(last_Leaf, -1, -1, au_Address, &au_Interval) >= 0);
		}
	}	

    //the new AU is not the latest...
	long current_Leaf;
	if(findFirstLeaf(&au_Interval, &current_Leaf) == false) return false; //the first leaf that interesects the interval of the new AU
	if(current_Leaf < 0) 
	{
		//no partitioning!
		long root_Child;
		if(root.readChildIndex(&root_Child) == false) return false;
		if(root_Child < 0) 
		{
			//The tree is empty
			return addNewEntry(-1, -1, -1, au_Address, &au_Interval) > -1;
		}
		//search for leaves with "greater" intervals
		interval search_Interval;
		epicsTimeStamp e = {0, 0};
		search_Interval.setStart(au_Interval.getEnd());
		search_Interval.setEnd(e);						//the next interval
		
		if(findFirstLeaf(&search_Interval, &current_Leaf) == false) return false; //find the new AU's future next AU
		if(current_Leaf < 0)
		{
			//The new leaf is at the end of the leaves list
			interval tree_Interval;
			if(root.readTreeInterval(&tree_Interval) == false) return false;

			if(findLastLeaf(&tree_Interval, &current_Leaf) == false) return false;
			if(current_Leaf < 0)
			{
				printf("The root at the address %ld is corrupt \n", root.getAddress());				
				return false; //ERROR because that would mean the tree is empty
			}
			return addNewEntry(current_Leaf, -1, -1, au_Address, &au_Interval) > -1;
		}
		else
		{
			//current_Leaf is the next leaf
			entry.attach(f, index2address(current_Leaf));
			long previous_Leaf;
			if(entry.readPreviousIndex(&previous_Leaf) == false) return false;
			return addNewEntry(previous_Leaf, current_Leaf, -1, au_Address, &au_Interval) > -1;
		}	
	}
	else
	{
		//partitioning might be due: check the priority etc.
		//current_Leaf is the first of the leaves that overlap with the au_Interval
		interval new_Leaf_Interval;
		entry.attach(f, index2address(current_Leaf));
		if(entry.readInterval() == false) return false;
		interval current_Leaf_Interval = entry.getInterval();

		//first: handle the first leaf in order to consider new AU's end time only
		long result = compareTimeStamps(au_Interval.getStart(), current_Leaf_Interval.getStart());
		if(	result < 0)
		{
			//if the new AU starts before current_Leaf
			new_Leaf_Interval.setStart(au_Interval.getStart());
			new_Leaf_Interval.setEnd(current_Leaf_Interval.getStart());
			entry.attach(f, index2address(current_Leaf));
			long previous_Index;
			if(entry.readPreviousIndex(&previous_Index) == false) return false;
			if(addNewEntry(previous_Index, current_Leaf, -1, au_Address, &new_Leaf_Interval) < 0) return false;
		}
		if(	result > 0)
		{
			//if the new AU starts after current_Leaf
			new_Leaf_Interval.setStart(current_Leaf_Interval.getStart());
			new_Leaf_Interval.setEnd(au_Interval.getStart());
			//change the current_Leaf's start time
			if(updateStartTime(&au_Interval, current_Leaf) == false) return false;
            
			//create the new leaf
			entry.attach(f, index2address(current_Leaf));
			long previous_Index;
			if(entry.readPreviousIndex(&previous_Index) == false) return false;
            long key_Address;
            if(entry.readKeyAddress(&key_Address) == false) return false;
			long new_Leaf = addNewEntry(previous_Index, current_Leaf, -1, -1, &new_Leaf_Interval); 
			if(new_Leaf < -1) return false;
			 
			entry.attach(f, index2address(new_Leaf));
			if(entry.writeKeyAddress(key_Address) == false) return false;		

			current_Leaf_Interval.setStart(au_Interval.getStart());
		}
		//at this point we know that the current_Leaf starts at the same time as the not
		//yet treated part of the  au_Interval (new AU's interval)
		//that's why we have to look at the au's END TIME ONLY
        //if the leaf points to an AU with a higher priority (etc., see below), simply go to the next leaf
        //else insert the new key after possibly partitioning it
		epicsTimeStamp previous_Leaf_End_Time;
		bool plet_Set = false;	//is set to true AFTER the first run of the loop, means
        archiver_Unit current_Key_AU;
		do
		{
            entry.attach(f, index2address(current_Leaf));
            long key_Address;
            if(entry.readKeyAddress(&key_Address) == false) return false;
            current_Key_AU.attach(f, key_Address);
            if(current_Key_AU.readAU() == false) return false;
            //go to the next leaf if the current key AU  has a higher priority etc.
            //Criteria for "I" being the new key:
            //I have a bigger priority than the current key OR
            //(my priority is the same) AND (current key is not previous key) AND
            //((
            //
            
            
			if(	plet_Set &&
				compareTimeStamps(current_Leaf_Interval.getStart(), au_Interval.getEnd()) >= 0)
			{
				//we also know that the previous leaf's end time is equal or smaller than current_Leaf's 
				//start time and GENUINELY smaller than au_Interval's end time
				new_Leaf_Interval.setStart(previous_Leaf_End_Time);
				new_Leaf_Interval.setEnd(au_Interval.getEnd());
				entry.attach(f, index2address(current_Leaf));
				long previous_Index;
				if(entry.readPreviousIndex(&previous_Index) == false) return false;
				return addNewEntry(previous_Index, current_Leaf, -1, au_Address, &new_Leaf_Interval) > -1;
			}
			
			if(	plet_Set && 
				compareTimeStamps(current_Leaf_Interval.getStart(), previous_Leaf_End_Time) > 0)
			{
				//gap that must be filled
				new_Leaf_Interval.setStart(previous_Leaf_End_Time);
				new_Leaf_Interval.setEnd(current_Leaf_Interval.getStart());
				entry.attach(f, index2address(current_Leaf));
				long previous_Index;
				if(entry.readPreviousIndex(&previous_Index) == false) return false;
				if(addNewEntry(previous_Index, current_Leaf, -1, au_Address, &new_Leaf_Interval) < 0) return false;
				//don't break
			}
		
			//since we handled the current_Leaf_Interval's start time cases above, only the end time
			//is treated below
			//also previous_Leaf_End_Time is not used any more, however we set it in the last case
			long result = compareTimeStamps(current_Leaf_Interval.getEnd(), au_Interval.getEnd());
			if(result > 0)
			{
				//partition
				new_Leaf_Interval.setStart(current_Leaf_Interval.getStart());
				new_Leaf_Interval.setEnd(au_Interval.getEnd());
				//update the current leaf's end time, get its au_List_Pointer
				interval * temp = new interval();
				temp->setStart(au_Interval.getEnd());
				temp->setEnd(current_Leaf_Interval.getEnd());
				if(updateStartTime(temp, current_Leaf) == false) 
				{
					delete temp;
					return false;
				}
				delete temp;
				
				if(aupl.init(index2address(current_Leaf)) == false) return false;
				long new_Key_Address = aupl.copyList();
				if(new_Key_Address < 0) return false;
				//create the new leaf
				entry.attach(f, index2address(current_Leaf));
				long previous_Index;
				if(entry.readPreviousIndex(&previous_Index) == false) return false;
				long new_Leaf = addNewEntry(previous_Index, current_Leaf, -1, -1, &new_Leaf_Interval);
				if(new_Leaf < 0) return false;
				//copy the pointer list and add the new au_Pointer
				entry.attach(f, index2address(new_Leaf));
				return 
					(
						entry.writeKeyAddress(new_Key_Address) &&
						aupl.init(index2address(new_Leaf)) &&
						aupl.insertAUPointer(au_Address)
					);
			}
			if(result == 0)
			{
				//just add the pointer to the leaf
				return 
					(	aupl.init(index2address(current_Leaf)) &&
						aupl.insertAUPointer(au_Address)
					);
			}
			if(result < 0)
			{
				//add the pointer to the leaf
				if(!aupl.init(index2address(current_Leaf)) ||
				   !aupl.insertAUPointer(au_Address)) return false;

				//check if there is a next leaf
				entry.attach(f, index2address(current_Leaf));
				long next;
				if(entry.readNextIndex(&next) == false) return false;
				if(next < 0)
				{
					//no more leaves
					new_Leaf_Interval.setStart(current_Leaf_Interval.getEnd());
					new_Leaf_Interval.setEnd(au_Interval.getEnd());
					if(addNewEntry(current_Leaf, -1, -1, au_Address, &new_Leaf_Interval) < 0) return false;
					return true;
				}
				else
				{
					//there is a next leaf, re-set all parameters
					current_Leaf = next;
					previous_Leaf_End_Time = current_Leaf_Interval.getEnd();
					plet_Set = true;
					entry.attach(f, index2address(current_Leaf));
					if(entry.readInterval() == false) return false;
					current_Leaf_Interval = entry.getInterval();
					//don't break
				}
			}
		}
		while(true);
	}
	return true;
}

bool r_Tree::removeAU(long au_Address)
{
	if(au_Address < 0) return false;
	archiver_Unit * current_AU = new archiver_Unit();
	current_AU->attach(f, au_Address);
	if(current_AU->readInterval() == false) 
	{
		delete current_AU;
		return false;
	}
	const interval au_Interval = current_AU->getInterval();
	delete current_AU;

	long current_Leaf;
	if(findFirstLeaf(&au_Interval, &current_Leaf) == false) return false;	
	if(current_Leaf < 0) return true;
	au_Pointer_List aupl = au_Pointer_List(fa, offset);
	r_Entry entry = r_Entry();
	do
	{			
		if(	!aupl.init(index2address(current_Leaf)) ||
			!aupl.deleteAUPointer(au_Address)) return false;
		entry.attach(f, index2address(current_Leaf));
		long next_Leaf;	
		if(entry.readNextIndex(&next_Leaf) == false) return false;
		if(aupl.isEmpty() == true)
		{
			if(deleteEntry(current_Leaf) == false) return false;
		}
		if(next_Leaf < 0) return true;
		current_Leaf = next_Leaf;
		entry.attach(f, index2address(current_Leaf));
		if(entry.readInterval() == false) return false;
		if(!entry.getInterval().isIntervalOver(au_Interval)) return true; 		
	}
	while(true);

}

//if the au is a the only key of the last leaf
 bool r_Tree::updateLatestLeaf(long au_Address, const epicsTimeStamp& end_Time)
{
	if(au_Address < 0) return false;
	archiver_Unit au;
	au.attach(f, au_Address);
	if(au.readKey() == false) return false;
	
	long latest_Leaf;
	if(root.readLatestLeafIndex(&latest_Leaf) == false) return false;
	r_Entry entry = r_Entry();
	entry.attach(f, index2address(latest_Leaf));
	
	au_Pointer aup = au_Pointer(); 
	long key_Address;
	if(entry.readKeyAddress(&key_Address) == false) return false;
	aup.attach(f, key_Pointer);
	long next_AUP;
	if(aup.readNextAUPAddress(&next_AUP) == false) return false;
	//if there is only one "latest" unit
	if(next_AUP < 0)
	{		
		long latest_AU_Address;
		if(aup.readAUAddress(&latest_AU_Address) == false) return false;
		archiver_Unit latest_AU = archiver_Unit();
		latest_AU.attach(f, latest_AU_Address);
		if(latest_AU.readAU() == false) return false;
		
		if(au.getKey() == latest_AU.getKey())
		{
			//if the updated unit is really the latest
			//if the end time wasn't increased
			long temp = compareTimeStamps(latest_AU.getInterval().getEnd(), end_Time);
			if(temp > 0) 
			{
				printf("An existing AU has a later end time than the one that was tried to be added\n");
				return false;
			}
			if(temp == 0) return true;	//nothing to do
			//update the last leaf interval
			if(entry.readInterval() == false) return false;
			interval new_Leaf_Interval;
			new_Leaf_Interval.setStart(entry.getInterval().getStart());
			new_Leaf_Interval.setEnd(end_Time);
			entry.setInterval(new_Leaf_Interval);
			return entry.writeInterval();
		}
	}
	return false;	
}

const long r_Tree::move()
{
	long old_Root_Address = root.getAddress();
	short i;
	if(root.readI(&i) == false) return -1;
	
	long number_Of_Entries = 32 * twoToThePowerOf(i);
	//allocate space for twice as many entries
	//the 1 is for the rtfsm
	long new_Root_Address = fa->allocate(ROOT_SIZE + RTFSM_HEADER_SIZE + (1 + ENTRY_SIZE) * number_Of_Entries * 2); 

	//move the r tree space manager 
	if(!rtfsm.move(new_Root_Address)) return -1;	
	
	//now move the root 
	if(root.move(new_Root_Address) == false) return -1;

	
	//finally move the actual tree data
	long old_Offset = offset;
	long new_Offset = new_Root_Address + ROOT_SIZE + RTFSM_HEADER_SIZE + number_Of_Entries * 2;
	char * buffer = (char *) malloc(number_Of_Entries * ENTRY_SIZE);
	fseek(f, old_Offset, SEEK_SET);
	if (fread(buffer, ENTRY_SIZE * number_Of_Entries, 1, f) != 1)
	{
		printf("Couldn't read the tree from the address %ld \n", old_Root_Address);
		free (buffer);
		return -1;
	}
	fseek(f, new_Offset, SEEK_SET);
	if(fwrite(buffer, ENTRY_SIZE * number_Of_Entries, 1,  f) != 1)
	{
		printf("Couldn't write the tree to the address %ld \n", new_Root_Address);
		free (buffer);
		return -1;
	}
	free (buffer);
	
	//give the old space free
	fa->free(old_Root_Address);

	offset = new_Offset;	
	return new_Root_Address;
}

//the following interrelation is valid:
//offset + entry_Size * index = address

inline long r_Tree::index2address(long index) const
{
	return offset + ENTRY_SIZE * index;
};

inline long r_Tree::address2index(long address) const
{
	return (address - offset)/ENTRY_SIZE;
}


bool r_Tree::findFirstLeaf(const interval * i, long * result) const
{	
	long current_Index;
	if(root.readChildIndex(&current_Index) == false) return false;
	//if there are no leaves
	if(current_Index < 0)
	{
		*result = -1;
		return true;
	}
	
	//if the R tree doesn't contain the desired interval
	interval * tree_Interval = new interval();
	if(root.readTreeInterval(tree_Interval) == false) 
	{
		delete tree_Interval;
		return false;
	}
	if(!(tree_Interval->isIntervalOver(*i))) 
	{
		delete tree_Interval;
		*result = -1;
		return true;
	}
	delete tree_Interval;
	
	//for each r tree level

	r_Entry entry = r_Entry();
	do
	{	
		//find the first r_Entry that contains the interval		
		do
		{	
			entry.attach(f, index2address(current_Index));
			if(entry.readInterval() == false) return false;

			//if there is a gap and the desired interval falls into it
			if(compareTimeStamps(entry.getInterval().getStart(), i->getEnd()) >= 0) 
			{
				*result = -1;
				return true;
			}
			
			//if we are done
			if(entry.getInterval().isIntervalOver(*i)) break;
			
			if(entry.readNextIndex(&current_Index) == false) return false;
			
			//if we reached the end of the level
			if(current_Index < 0) 
			{
				*result = -1;
				return true;
			}
		}
		while(true);
		entry.attach(f, index2address(current_Index));
		
		long child;
		if(entry.readChildIndex(&child) == false) return false;
		
		//we reached the leaves
		if(child < 0) 
		{
			*result = current_Index;
			return true;
		}
		//go one level down
		current_Index = child;
	}
	while(true);
};

bool r_Tree::findLastLeaf(const interval * i, long * result) const
{
	if(i->getEnd().secPastEpoch == 0 && i->getEnd().nsec == 0)
	{
		return root.readLatestLeafIndex(result);
	}

	long current_Index;
	if(findFirstLeaf(i, &current_Index) == false) return false;
	if(current_Index < 0)
	{
		*result = -1;
		return true;
	}

	r_Entry entry = r_Entry();
	do
	{
		entry.attach(f, index2address(current_Index));
		long next_Index;
		if(entry.readNextIndex(&next_Index) == false) return false;
		if(next_Index < 0)
		{
			*result = current_Index; //if we reached the absolutely last leaf on the level
			return true;
		}
		
		entry.attach(f, index2address(next_Index));
		if(entry.readInterval() == false) return false;

		if(compareTimeStamps(entry.getInterval().getStart(), i->getEnd()) >= 0)
		{
			*result = current_Index;
			return true;
		}
		current_Index = next_Index;
	}
	while(true);
}

bool r_Tree::getFirstEntryInNode(long current_Index, long * result) const
{
	r_Entry entry = r_Entry();
	entry.attach(f, index2address(current_Index));
	long parent_Index;
	if(entry.readParentIndex(&parent_Index) == false) return false;
	do
	{
		entry.attach(f, index2address(current_Index));
		long previous_Index;
		if(entry.readPreviousIndex(&previous_Index) == false) return false;
		if(previous_Index < 0) 
		{
			*result = current_Index;
			return true;
		}
		if(parent_Index >= 0)
		{
			entry.attach(f, index2address(previous_Index));
			long previous_Parent;
			if(entry.readParentIndex(&previous_Parent) == false) return false;
			if(previous_Parent != parent_Index)
			{
				*result = current_Index;
				return true;
			}
		}
		current_Index = previous_Index;
	}
	while(true);
}

bool r_Tree::getNumberOfEntriesInNode(long current_Index, short * result) const
{
	if(getFirstEntryInNode(current_Index, &current_Index) == false) return false;
	if(current_Index < 0)
	{
		*result = -1;
		return true;
	}
	short counter = 1;
	r_Entry entry = r_Entry();
	entry.attach(f, index2address(current_Index));
	long parent_Index;
	if(entry.readParentIndex(&parent_Index) == false) return false;
	
	do
	{
		entry.attach(f, index2address(current_Index));
		long next_Index;
		if(entry.readNextIndex(&next_Index) == false) return false;
		if(next_Index < 0)
		{
			*result = counter;
			return true;
		}
		
		if(parent_Index >=0)
		{
			//check only if we are not on the highest level
			entry.attach(f, index2address(next_Index));
			long next_Parent;
			if(entry.readParentIndex(&next_Parent) == false) return false;
			if(next_Parent != parent_Index)
			{
				*result = counter;
				return true;
			}
		}
		
		
		current_Index = next_Index, counter++;
	}
	while(true);

}

bool r_Tree::getEntryInNode(long current_Index, short number, long * result) const
{
	if(getFirstEntryInNode(current_Index, &current_Index) == false) return false;
	if(number == 0) 
	{
		*result = current_Index;
		return true;
	}
	short counter = 0;
	r_Entry entry = r_Entry();
	entry.attach(f, index2address(current_Index));
	long parent_Index;
	if(entry.readParentIndex(&parent_Index) == false) return false;
	
	do
	{		
		entry.attach(f, index2address(current_Index));
		long next_Index;
		if(entry.readNextIndex(&next_Index) == false) return false;
		if(next_Index < 0) 
		{
			*result = -1;
			return true;
		}
		if(parent_Index >=0)
		{
			//check only if we are not on the highest level
			entry.attach(f, index2address(next_Index));
			long next_Parent;
			if(entry.readParentIndex(&next_Parent) == false) return false;
			if(next_Parent != parent_Index) 
			{
				*result = -1;
				return true;
			}
		}
		current_Index = next_Index, counter++;
		if(counter == number) 
		{
			*result = current_Index;
			return true;
		}
	}
	while(true);
}


bool r_Tree::getLastEntryInNode(long current_Index, long * result) const
{
	if(getFirstEntryInNode(current_Index, &current_Index) == false) return false;

	r_Entry entry = r_Entry();
	entry.attach(f, index2address(current_Index));
	long parent_Index;
	if(entry.readParentIndex(&parent_Index) == false) return false;
	
	do
	{
		entry.attach(f, index2address(current_Index));
		long next_Index;
		if(entry.readNextIndex(&next_Index) == false) return false;
		if(next_Index < 0) 
		{
			*result = current_Index;
			return true;
		}
		
		if(parent_Index >=0)
		{
			//check only if we are not on the highest level
			entry.attach(f, index2address(next_Index));
			long next_Parent;
			if(entry.readParentIndex(&next_Parent) == false) return false;
			if(next_Parent != parent_Index) 
			{
				*result = current_Index;
				return true;
			}
		}
		current_Index = next_Index;
	}
	while(true);
}

//takes care of [m;2*m )
//does the updates of the neighbors, too
long r_Tree::addNewEntry(long previous_Index, long next_Index, long child_Index, long au_Address, const interval * i)
{
	long new_Index;
	if(rtfsm.getFreeIndex(&new_Index) == false) return -1;
	if(new_Index < 0)
	{
		//if no more space available, move the tree
		if(move() < 0)	return -1;
		if(rtfsm.getFreeIndex(&new_Index) == false)	return -1;
	}	
	
	long parent_Index;
	r_Entry entry = r_Entry();
	//look first if the entry is going to be added between two nodes or inside one node
	//check if the work is done on the upper level!
	if(previous_Index < 0 && next_Index <0) 
	{
		parent_Index = -1;
	}
	else if(previous_Index < 0) 
	{
		//pre condition: next_Index is > 0
		entry.attach(f, index2address(next_Index));
		if(entry.readParentIndex(&parent_Index) == false) return -1;
	}
	else if(next_Index < 0) 
	{
		//pre condition: previous_Index is > 0
		entry.attach(f, index2address(previous_Index));
		if(entry.readParentIndex(&parent_Index) == false) return -1;
	}
	else
	{
		entry.attach(f, index2address(previous_Index));
		long previous_Parent;
		if(entry.readParentIndex(&previous_Parent) == false) return -1;

		entry.attach(f, index2address(next_Index));
		long next_Parent;
		if(entry.readParentIndex(&next_Parent)== false) return -1;
		
		if(previous_Parent != next_Parent)
		{
			//choose the node with the smaller number of entries
			//=> keep it balanced
			short number_Of_Entries_In_Previous_Node;
			if(getNumberOfEntriesInNode(previous_Index, &number_Of_Entries_In_Previous_Node) == false) return -1;

			short number_Of_Entries_In_Next_Node;
			if(getNumberOfEntriesInNode(next_Index, &number_Of_Entries_In_Next_Node) == false) return -1;

			if(number_Of_Entries_In_Previous_Node > number_Of_Entries_In_Next_Node)
			{
				parent_Index = next_Parent;
			}
			else
			{
				parent_Index = previous_Parent;
			}
		}
		else
		{
			//it doesn't matter (inside a node)
			parent_Index = previous_Parent;		
		}
	}
	//first set the links of the previous and next entries
	if(previous_Index >= 0)
	{
		entry.attach(f, index2address(previous_Index));
		if(entry.writeNextIndex(new_Index) == false) return -1;
	}
	if(next_Index >= 0)
	{
		entry.attach(f, index2address(next_Index));
		if(entry.writePreviousIndex(new_Index) == false) return -1;
	}
	
	//now write all the information to the new entry block
	entry.attach(f, index2address(new_Index));
	entry.setInterval(*i);
	
	if(	!entry.writeInterval()						||
		!entry.writeChildIndex(child_Index)			||
		!entry.writeParentIndex(parent_Index)		||
		!entry.writeNextIndex(next_Index)			||
		!entry.writePreviousIndex(previous_Index)	||
		!entry.writeKeyAddress(-1)) return -1;

	if(au_Address > 0) 
	{
		//if we add a leaf AND the au_Address is at this point known, create a new au_Pointer_List
		long key_Address = fa->allocate(AU_POINTER_SIZE);
		au_Pointer * aup = new au_Pointer();
		aup->attach(f, key_Address);
		if(	!aup->writeAUAddress(au_Address) ||
			!aup->writeNextAUPAddress(-1) ||
			!aup->writePreviousAUPAddress(-1)) 
		{
			delete aup;
			return -1;
		}
		entry.attach(f, index2address(new_Index));
		if(entry.writeKeyAddress(key_Address) == false)
		{
			delete aup;
			return -1;
		}
		delete aup;
	}
	
	//at this point, the parent doesn't know a thing about the new entry
	if(tryToUpdateParent(i, new_Index) == false) return -1;

	//so far we added the entry, now we must check if the node is too big and split it, if it is
	short number_Of_Entries_In_Node;
	if(getNumberOfEntriesInNode(new_Index, &number_Of_Entries_In_Node) == false) return -1;
	if(number_Of_Entries_In_Node == 2 * m)
	{
		if(parent_Index < 0)
		{
			//if we are on the highest level, a  new pseudo entry must be created first!
			long first_Child_Index;
			if(getFirstEntryInNode(new_Index, &first_Child_Index) == false) return -1;

			interval * tree_Interval = new interval();
			root.readTreeInterval(tree_Interval);
			parent_Index = addNewEntry(-1, -1, first_Child_Index, -1, tree_Interval);
			delete tree_Interval;
			if(parent_Index < 0) return -1;
			long last_Entry_In_Node;
			if(getLastEntryInNode(first_Child_Index, &last_Entry_In_Node) == false) return -1;
			setNewParent(first_Child_Index, last_Entry_In_Node, parent_Index);
		}
		splitParent(parent_Index);	
		
	}
	return new_Index;
 }

bool r_Tree::deleteEntry(long current_Index)
{
	
	r_Entry entry = r_Entry();
	entry.attach(f, index2address(current_Index));
	long next_Index;
	if(entry.readNextIndex(&next_Index) == false) return false;
	long previous_Index;
	if(entry.readPreviousIndex(&previous_Index) == false) return false;
	long parent_Index;
	if(entry.readParentIndex(&parent_Index) == false) return false;
	//first reset the links of the neighbor leaves
	long next_Parent;		 
	long previous_Parent;	
	if(next_Index >= 0)
	{
		entry.attach(f, index2address(next_Index));
		if(!entry.writePreviousIndex(previous_Index) || !entry.readParentIndex(&next_Parent)) return false;
	}

	if(previous_Index >= 0)
	{
		entry.attach(f, index2address(previous_Index));
		if(!entry.writeNextIndex(next_Index) || !entry.readParentIndex(&previous_Parent)) return false;
	}
	//set the index free
	if( !rtfsm.setIndexFree(current_Index)) return false;

	if(parent_Index < 0)
	{
		//if we are on the highest level
		if(next_Index < 0 && previous_Index < 0)
		{
			//the only entry left in the whole tree
		
			return
				(
						root.writeChildIndex(-1) &&
						root.writeNullTreeInterval()
				);
		}
		if(next_Index < 0)
		{
			//pre condition: there is a previous entry
			entry.attach(f, index2address(previous_Index));
			if(entry.readInterval() == false) return false;
			return tryToUpdateParent(&entry.getInterval(), previous_Index);
		}
		if(previous_Index < 0)
		{
			//pre condition: there is a next entry
			entry.attach(f, index2address(next_Index));
			if(entry.readInterval() == false) return false;
			return tryToUpdateParent(&entry.getInterval(), next_Index);
		}
		//if there are both neighbors
		return true;
	}
	else
	{
		//if we are NOT on the highest level
		if(next_Index < 0 && previous_Index < 0)
		{
			//the only entry left on the level
			return deleteEntry(parent_Index);
		}
		if(next_Index < 0)
		{
			//pre condition: there is a previous entry
			entry.attach(f, index2address(previous_Index));
			if(entry.readInterval() == false) return false;
			
			if(tryToUpdateParent(&entry.getInterval(), previous_Index) == false) return false;
			if(parent_Index != previous_Parent)
			{
				return deleteEntry(parent_Index);
			}
			return true;
		}
		if(previous_Index < 0)
		{
			//pre condition: there is a next entry
			entry.attach(f, index2address(next_Index));
			if(entry.readInterval() == false) return false;
			
			if(tryToUpdateParent(&entry.getInterval(), next_Index) == false) return false;
			if(parent_Index != next_Parent)
			{
				return deleteEntry(parent_Index);
			}
			return true;
		}
		//if there are both neighbors
		if(parent_Index != previous_Parent && parent_Index != next_Parent)
		{
			//the only entry in the node
			return deleteEntry(parent_Index);
		}
		if(parent_Index != previous_Parent)
		{
			//pre condition: the next entry has the same parent
			//means the next entry becomes the child of the parent
			entry.attach(f, index2address(next_Index));
			if(entry.readInterval() == false) return false;
			return tryToUpdateParent(&entry.getInterval(), next_Index);
		}
		if(parent_Index != next_Parent)
		{
			//pre condition: the previous entry has the same parent
			//means the previous entry becomes the last in the node
			entry.attach(f, index2address(previous_Index));
			if(entry.readInterval() == false) return false;
			return tryToUpdateParent(&entry.getInterval(), previous_Index);
		}
		//if both neighbors have the same parent, means the deleted entry is inside a node
	}
	return true;
}

bool r_Tree::splitParent(long parent_Index)
{
	//write to the same address the earlier of the split entries
	//get all the necessary information
	//interval of E(arly) < interval of L(ate)
	r_Entry entry = r_Entry();
	entry.attach(f, index2address(parent_Index));
	if(entry.readInterval() == false) return false;
	epicsTimeStamp end_Time_L = entry.getInterval().getEnd();
	long child_Index_E;
	if(entry.readChildIndex(&child_Index_E) == false) return false;
	long next_Index_L;
	if(entry.readNextIndex(&next_Index_L) == false) return false;
	//no unit pointer

	long last_Entry_In_Node;
	//needed for resetting the parent indices
	if(getLastEntryInNode(child_Index_E, &last_Entry_In_Node) == false) return false;
	//the first child of the L(ate) entry
	long child_Index_L;
	if(getEntryInNode(child_Index_E, m, &child_Index_L) == false) return false; //m is the constant 

	entry.attach(f, index2address(child_Index_L));
	if(entry.readInterval() == false) return false;;
	epicsTimeStamp start_Time_L = entry.getInterval().getStart();	//the start time of the new leaf (later)

	long previous_Index;
	if(entry.readPreviousIndex(&previous_Index) == false) return false;
	entry.attach(f, index2address(previous_Index));
	if(entry.readInterval() == false) return false;
		
	//update the end time of the old parent with the previous entry from the child_Index_L point of view
	//ignore the start time (instead of creating a new entry)!
	//Note: must also update recursively its parents etc. because when the new ("late") entry is added
	//a split of the parent etc might be due, too
	if(updateEndTime(&entry.getInterval(), parent_Index) == false) return false;
	
	//add the late entry to the tree 
	interval * i = new interval();
	i->setStart(start_Time_L);
	i->setEnd(end_Time_L);
	long new_Parent_Index = addNewEntry(parent_Index, next_Index_L, child_Index_L, -1, i);
	if(new_Parent_Index < 0) 
	{
		delete i;
		return false;
	}
	delete i;
	
	//set the parents for the new children
	return setNewParent(child_Index_L, last_Entry_In_Node, new_Parent_Index);
}

bool r_Tree::setNewParent(long start_Entry_Index, long end_Entry_Index, long new_Parent_Index) const
{
	r_Entry entry = r_Entry();
	long current_Index = start_Entry_Index;
	do
	{
		entry.attach(f, index2address(current_Index));
		if(entry.writeParentIndex(new_Parent_Index) == false) return false;
		if(current_Index == end_Entry_Index) return true;
		long next_Index;
		if(entry.readNextIndex(&next_Index) == false) return false;
		if(next_Index < 0) 
		{
			printf("Broken link at the address %ld\n", index2address(current_Index));
			return false;
		}
		current_Index = next_Index;
	}
	while(true);
}

bool r_Tree::tryToUpdateParent(const interval * i, long current_Index)
{
	return
		(
			tryToUpdateStartTimeOfTheParent(i, current_Index) &&
			tryToUpdateEndTimeOfTheParent(i, current_Index) &&
			tryToUpdateChildIndexOfTheParent(current_Index) &&
			tryToUpdateLatestLeafIndexOfTheRoot(current_Index)
		);
}

bool r_Tree::tryToUpdateStartTimeOfTheParent(const interval * i, long current_Index)
{
	long first_Entry_In_Node;
	if(getFirstEntryInNode(current_Index, &first_Entry_In_Node) == false) return false;
	if(current_Index == first_Entry_In_Node)
	{
		r_Entry entry = r_Entry();
		entry.attach(f, index2address(current_Index));
		long parent_Index;
		if(entry.readParentIndex(&parent_Index) == false) return false;
		if(parent_Index < 0) 
		{
			//update the root
			interval temp = interval();
			if(root.readTreeInterval(&temp) == false) return false;
			temp.setStart(i->getStart());
			return root.writeTreeInterval(temp);
		}
		else
		{
			//when a parent exists
			return
				(
					updateStartTime(i, parent_Index) &&
					tryToUpdateStartTimeOfTheParent(i, parent_Index)
				);
		}
	}
	return true;
}

bool r_Tree::tryToUpdateEndTimeOfTheParent(const interval * i, long current_Index)
{
	long last_Entry_In_Node;
	if(getLastEntryInNode(current_Index, &last_Entry_In_Node) == false) return false;
	if(current_Index == last_Entry_In_Node)
	{
		r_Entry entry = r_Entry();
		entry.attach(f, index2address(current_Index));
		long parent_Index;
		if(entry.readParentIndex(&parent_Index) == false) return false;
		if(parent_Index < 0) 
		{
			//update the root
			interval temp = interval();
			if(root.readTreeInterval(&temp) == false) return false;
			temp.setEnd(i->getEnd());
			return root.writeTreeInterval(temp);
		}
		else
		{
			//when a parent exists
			return
				(
					updateEndTime(i, parent_Index) &&
					tryToUpdateEndTimeOfTheParent(i, parent_Index)
				);
		}
	}
	return true;

}

bool r_Tree::tryToUpdateChildIndexOfTheParent(long current_Index)
{
	long first_Entry_In_Node;
	if(getFirstEntryInNode(current_Index, &first_Entry_In_Node) == false) return false;
	if(current_Index == first_Entry_In_Node)
	{
		r_Entry entry = r_Entry();
		entry.attach(f, index2address(current_Index));
		long parent_Index;
		if(entry.readParentIndex(&parent_Index) == false) return false;
		if(parent_Index < 0) 
		{
			//update the root
			return root.writeChildIndex(current_Index);
		}
		else
		{
			//when a parent exists
			//get the interval of the parent
			entry.attach(f, index2address(parent_Index));
			return entry.writeChildIndex(current_Index);
		}
	}
	return true;
}

bool r_Tree::tryToUpdateLatestLeafIndexOfTheRoot(long current_Index)
{
	r_Entry entry = r_Entry();
	entry.attach(f, index2address(current_Index));
	long next_Index, child_Index;
	if(!entry.readNextIndex(&next_Index) || !entry.readChildIndex(&child_Index)) return false;
	if((next_Index < 0) && (child_Index < 0))
	{
		//update the root
		return root.writeLatestLeafIndex(current_Index);
	}
	return true;
}

bool r_Tree::updateStartTime(const interval * i, long current_Index)
{
	r_Entry entry = r_Entry();
	entry.attach(f, index2address(current_Index));
	if(entry.readInterval() == false) return false;
	//store the current interval in temp
	interval temp = entry.getInterval();
	temp.setStart(i->getStart());
	entry.setInterval(temp);
	return 
		(
			entry.writeInterval() &&
			tryToUpdateStartTimeOfTheParent(i, current_Index)
			);
}

bool r_Tree::updateEndTime(const interval * i, long current_Index)
{
	r_Entry entry = r_Entry();
	entry.attach(f, index2address(current_Index));
	if(entry.readInterval() == false) return false;
	//store the current interval in temp
	interval temp = entry.getInterval();
	temp.setEnd(i->getEnd());
	entry.setInterval(temp);
	return 
		(
			entry.writeInterval() &&
			tryToUpdateEndTimeOfTheParent(i, current_Index));
}


void r_Tree::writeSpace(FILE * text_File, int n) const
{
	for(int i=0;i<n;i++)
	{
		putc(32, text_File);
	}
}



void r_Tree::dump(FILE * text_File) const 
{
	interval * tree_Interval = new interval();
	if(root.readTreeInterval(tree_Interval) == false) 
	{
		delete tree_Interval;
		return;
	}
	long first_Leaf;
	if(findFirstLeaf(tree_Interval, &first_Leaf) == false) 
	{
		delete tree_Interval;
		return;
	}
	delete tree_Interval;
	
	if (first_Leaf < 0) 
	{
		printf("There are no leaves in the tree!\n");
		return;
	}
	long current_Index = first_Leaf;	//first_Leaf will be needed later again, while current_index is changed

	r_Entry entry = r_Entry();
	//dump first the au lists
	au_Pointer_List * aupl = new au_Pointer_List(fa, offset);
	do
	{
		if(!aupl->init(index2address(current_Index))) 
		{
			delete aupl;
			return;
		}
		aupl->dump(text_File);
		putc('\n', text_File);
		
		entry.attach(f, index2address(current_Index));
		if(entry.readNextIndex(&current_Index) == false) 
		{
			delete aupl;
			return;
		}
		if(current_Index < 0) break;
	}
	while(true);
	fputc('\n', text_File);
	delete aupl;
	
	current_Index = first_Leaf;

	long first_Entry_Of_Next_Level = 0;
	
	//the fifo is initialised with 1("one")s, the number of which equals the number of leaves
	//for each inner entry, the first value in the fifo is read, added to a variable next_Fifo_Value
	//and then deleted from the fifo
	//after the whole node is read, the next_Fifo_Value is inserted at the end of fifo
	bool is_Last_Level = true;
	long next_Fifo_Value = 0;

	ArrayList fifo;
	do
	{
		entry.attach(f, index2address(current_Index));
		if(entry.readParentIndex(&first_Entry_Of_Next_Level) == false) return;
		do
		{
			entry.attach(f, index2address(current_Index));
			if(entry.readInterval() == false) return;
			entry.getInterval().print(text_File);
		
			long nr_Leaves_Children = 1;
			if(!is_Last_Level)
			{
				nr_Leaves_Children = fifo.getElement(0);				
				writeSpace(text_File, (IV_STRING_LENGTH) * (nr_Leaves_Children - 1));
				fifo.deleteElement(0);
			}

			next_Fifo_Value = next_Fifo_Value + nr_Leaves_Children;
			long last_Entry_In_Node;
			if(getLastEntryInNode(current_Index, &last_Entry_In_Node) == false) return;
			if(current_Index == last_Entry_In_Node) 
			{
				fifo.insertElement(next_Fifo_Value);
				next_Fifo_Value = 0;
			}
		
			entry.attach(f, index2address(current_Index));
			entry.readNextIndex(&current_Index);
			if(current_Index < 0) break; //we reached the end of the level
		}
		while(true);
			putc('\n', text_File); 
			is_Last_Level = false;
			if (first_Entry_Of_Next_Level<0) break;
			current_Index = first_Entry_Of_Next_Level;
	}
	while(true);
	putc('\n', text_File);
	putc('\n', text_File);
	putc('\n', text_File);
	putc('\n', text_File);
	putc('\n', text_File);
}

void r_Tree::writeDotFile(const char * name) const
{
	FILE * dot_File = fopen(name, "w");
	if(isEmpty())
	{
		fclose(dot_File);
		return;
	}

	fprintf(dot_File, "digraph g{\n");
	fprintf(dot_File, "ranksep =\"%d\";\n", m);
	fprintf(dot_File, "node [shape = record, height=.1];\n");


	interval tree_Interval;
	if(root.readTreeInterval(&tree_Interval) == false)	return;

	long current_Index;
	if(findFirstLeaf(&tree_Interval, &current_Index) == false)	return;
	tree_Interval.~interval();

	r_Entry entry = r_Entry();
	
	int i = 0;	//global node counter
	
	ArrayList node_Numbers;

	long first_Entry_Of_Next_Level;
	bool is_Last_Level = true;
	do
	{
		entry.attach(f, index2address(current_Index));
		if(entry.readParentIndex(&first_Entry_Of_Next_Level) == false) return;
		do
		{
			short number_Of_Entries;
			if(getNumberOfEntriesInNode(current_Index, &number_Of_Entries) == false) return;
			
			if(is_Last_Level)
			{
				//au pointers
				long temp_Index = current_Index;
				for(int k=0;k<number_Of_Entries;k++)
				{
					if(temp_Index< 0)
					{
						//a precaution
						printf("The child at the address %ld has a corrupt parent\n", index2address(temp_Index));
						return;
					}
					fprintf(dot_File, "node%d [label = \"<f0>", i + k);
					entry.attach(f, index2address(temp_Index));
					long current_AU_Address;
					if(entry.readKeyAddress(&current_AU_Address) == false) return;
					archiver_Unit au;
					au_Pointer aup;
					long au_Address;
					while(current_AUP_Address > 0)
					{
						aup.attach(f, current_AUP_Address);
						if(aup.readAUAddress(&au_Address) == false) return;
						au.attach(f, au_Address);
						if(au.readAU() == false) return;
						au.print(dot_File);					
						fprintf(dot_File, "\\n");
						
						if(aup.readNextAUPAddress(&current_AUP_Address) == false) return;
					}
					fprintf(dot_File, "\"];\n");
					//and the connection
					fprintf(dot_File, "\"node%d\":f%d -> \"node%d\":f0;\n", i + number_Of_Entries, k, i + k);
			
					if(entry.readNextIndex(&temp_Index) == false) return;
				}
				i = i + number_Of_Entries;
			}

			//the entries in a node
			fprintf(dot_File, "node%d [label = \"", i);
			for(int j=0;j<number_Of_Entries;j++)
			{
				fprintf(dot_File, "<f%d>", j);
				entry.attach(f, index2address(current_Index));
				if(entry.readInterval() == false) return;
				entry.getInterval().print(dot_File);

				if(j== number_Of_Entries - 1)
				{
					//last record
					fprintf(dot_File, "\"];\n");
					break;
				}
				
				fprintf(dot_File, "|");
				
				if(entry.readNextIndex(&current_Index) == false) return;
				if(current_Index< 0)
				{
					//a precaution
					printf("The child at the address %ld has a corrupt parent\n", index2address(current_Index));
					return;
				}	
			}				
			if(!is_Last_Level)
			{
				for(int l=0;l<number_Of_Entries;l++)
				{
					long child_Number = node_Numbers.getElement(0);
					node_Numbers.deleteElement(0);
					fprintf(dot_File, "\"node%d\":f%d -> \"node%ld\":f0;\n", i, l, child_Number);
				}
			}
			//next node
			node_Numbers.insertElement(i);
			
			if(entry.readNextIndex(&current_Index) == false) return;
			i++;
			if(current_Index < 0) 
			{
				is_Last_Level = false;
				break; //next_Level
			}				
		}
		while(true);
		if(first_Entry_Of_Next_Level < 0) break;//end of the tree
		current_Index = first_Entry_Of_Next_Level;
	}
	while(true);
	//write the root
	interval root_Interval = interval();
	if(root.readTreeInterval(&root_Interval) == false) return;
	fprintf(dot_File, "node%d [label = \"<f0>", i);
	root_Interval.print(dot_File);
	fprintf(dot_File, "\"];\n");

	long child_Number = node_Numbers.getElement(0);
	node_Numbers.deleteElement(0);
	fprintf(dot_File, "\"node%d\":f0 -> \"node%ld\":f0;\n", i, child_Number);
	
	fprintf(dot_File, "}\n");
	fclose(dot_File);

}

bool r_Tree::test() const
{
	bool is_Last_Level = true;
	long current_Index;
	interval current_Interval = interval();
	interval next_Interval = interval();
	long first_Entry_On_Next_Level;
	long next_Index;
	if(root.readLatestLeafIndex(&current_Index) == false) return false;
	if(current_Index  < 0)
	{
		printf("The tree at the address %ld contains no leaves!\n", root.getAddress());
		return true;
	}
	r_Entry entry = r_Entry();
	do
	{
		do
		{
			entry.attach(f, index2address(current_Index));
			if(entry.readParentIndex(&first_Entry_On_Next_Level) == false) return false;
			
			if(is_Last_Level == true)
			{
				long child_Index;
				if(entry.readChildIndex(&child_Index) == false) return false;
				if(child_Index > -1)
				{
					printf("The entry at the address %ld has a corrupt child index\n", index2address(current_Index));
					return false;
				}
				long key_Address;
				if(entry.readKeyAddress(&key_Address) == false) return false;
				if(key_Pointer < 0)
				{
					printf("The entry at the address %ld has no key pointer\n", index2address(current_Index));
					return false;
				}
				
			}
			if(entry.readInterval() == false) return false;
			current_Interval = entry.getInterval();
			if(is_Last_Level == false)
			{
				long child_Index;
				if(entry.readChildIndex(&child_Index) == false) return false;
				if(child_Index < 0)
				{
					printf("The entry at the address %ld has no child index\n", index2address(current_Index));
					return false; 
				}
				long key_Address;
				if(entry.readKeyAddress(&key_Address) == false) return false;
				if(key_Pointer > 0)
				{
					printf("The entry at the address %ld has a corrupt key pointer\n", index2address(current_Index));
					return false;
				}

				short number_Of_Entries_In_Child_Node;
				if(getNumberOfEntriesInNode(child_Index, &number_Of_Entries_In_Child_Node) == false) return false;

				if(number_Of_Entries_In_Child_Node >= 2*m)
				{
					printf("The entry at the address %ld has too many children\n", index2address(current_Index));
					return false;
				}
				long last_Child_Index;
				if(getLastEntryInNode(child_Index, &last_Child_Index) == false) return false;
				entry.attach(f, index2address(child_Index));
				if(entry.readInterval() == false) return false;
				interval temp = interval();
				temp.setEnd(entry.getInterval().getEnd());
				entry.attach(f, index2address(last_Child_Index));
				if(entry.readInterval() == false) return false;
				temp.setStart(entry.getInterval().getStart());

				if(temp.compareIntervals(current_Interval) !=0)
				{
					printf("The interval of the entry at the address %ld doesn't match the children\n", index2address(current_Index));
					return false;
				}
			}
			entry.attach(f, index2address(current_Index));
			if(entry.readNextIndex(&next_Index) == false) return false;
			if(next_Index < 0) break;
			entry.attach(f, index2address(next_Index));
			if(entry.readInterval() == false) return false;
			next_Interval = entry.getInterval();
			if(next_Interval.isIntervalOver(current_Interval))
			{
				printf("The entries at the address %ld and %ld intersect\n", index2address(current_Index), index2address(next_Index));
				return false;
			}
			if(compareTimeStamps(next_Interval.getEnd(), current_Interval.getStart()) > 0)
			{
				printf("The entries at the address %ld and %ld are not sorted\n", index2address(current_Index), index2address(next_Index));
				return false;
			}
			current_Index = next_Index;
		}
		while(true);

		if(first_Entry_On_Next_Level < 0) 
		{
			//test the root
			interval temp = interval();
			
			long first_Root_Child;
			if(getFirstEntryInNode(current_Index, &first_Root_Child) == false) return false;
			entry.attach(f, index2address(first_Root_Child));
			if(entry.readInterval() == false) return false;
			temp.setEnd(entry.getInterval().getEnd());			
			long last_Root_Child;
			if(getLastEntryInNode(current_Index, &last_Root_Child) == false) return false;
			entry.attach(f, index2address(last_Root_Child));
			if(entry.readInterval() == false) return false;
			temp.setStart(entry.getInterval().getStart());
			
			interval tree_Interval = interval();
			if(root.readTreeInterval(&tree_Interval) == false) return false;
			
			if(temp.compareIntervals(tree_Interval) !=0)
			{
				printf("The root doesn't hold the proper interval\n");
				return false;
			}
			
			long root_Child;
			root.readChildIndex(&root_Child);
			if(root_Child != first_Root_Child)
			{
				printf("The root doesn't point to the proper child\n");
				return false;
			}
			return true;
		}
		//go to the next level
		current_Index = first_Entry_On_Next_Level;
		is_Last_Level = false;
	}
	while(true);	
}


