#include "archiver_index.h"
#include <Filename.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "bin_io.h"
#include "file_allocator.h"
/**
*   The header files below are for performance tests only  
*/
#include "sys/time.h"
#include "unistd.h"


archiver_Index::archiver_Index()
:f(0), m(0), read_Only(true), global_Priority(-1){}

archiver_Index::~archiver_Index()
{
	if(f!=0) close();
}

bool archiver_Index::open(const char * file_Path, bool read_Only, short _m, short hash_Table_Size)
{
	if(file_Path == 0)
    {
        printf("The path of the index file was not specified\n");
        return false;
    }
    full_Path = stdString(file_Path);
    Filename::getDirname(full_Path, dir);
    Filename::getBasename(full_Path, file_Name);
    this->read_Only = read_Only;
    
	f = fopen(file_Path, "rb");
	if(f==0) 
	{
        if(read_Only)
        {
            printf("Could not open the file %s\n", file_Path);
      		return false;
        }
        else
        {
            f = fopen(file_Path, "w+b");
            return (f!= 0) && create(_m, hash_Table_Size);
        }
	}
    if(!read_Only)
    {
        if((f = freopen(file_Path, "r+b", f)) == 0)
        {
           printf("The file \"%s\" seems to be read- only\n", file_Path);
           return false;
        }
    }
	if(fa.attach(f, HEADER_SIZE) == false)	return false;
	//check if the file is an index file
	char buffer[MAGIC_ID_SIZE + 1];
	fseek(f, MAGIC_ID, SEEK_SET);
	if(fread(buffer, MAGIC_ID_SIZE, 1, f) != 1) 
	{
		printf("Can't read the file %s\n", file_Path);
		return false;
	}
	buffer[MAGIC_ID_SIZE] = 0;
	if(strcmp(buffer, "RTI1") != 0) 
	{
		printf("%s is not a valid index file\n", file_Path);
		return false;
	}

	//check the size of the file
	fseek(f, 0, SEEK_END);
	if(ftell(f) < HEADER_SIZE) 
	{
		printf("%s is corrupt\n", file_Path);
		return false;
	}

	//load the object attributes
	fseek(f, HASH_TABLE_SIZE, SEEK_SET);
	if(readShort(f, &hash_Table_Size) == false)
	{
		printf("Couldn't read the hash table size from the address %d\n", HASH_TABLE_SIZE);
		return false;
	}
	t.setSize(hash_Table_Size);
	this->read_Only = read_Only;
	fseek(f, R_TREE_M, SEEK_SET);
	if(readShort(f, &this->m) == false)
	{
		printf("Couldn't read the r tree parameter from the address %d\n", R_TREE_M);
		return false;
	}
	if(r.setM(this->m) == false) return false;
	return t.attach(&fa, CNTU_TABLE_POINTER);		
}

bool archiver_Index::close()
{
	fa.detach();
	m = 0;
	if(fclose(f) != 0) return false;
	f = 0;
	r.detach();
	t.detach();
	global_Priority = -1;
    dir = 0;
	return true;
}

bool archiver_Index::addDataFromAnotherIndex(const char * channel_Name, archiver_Index& other, bool only_New_Data)
{
	//for test purposes only
    struct timeval before, after;
    long length_Of_Time;
    ////////////////////////
	if(!isInstanceValid() || !other.isInstanceValid() || read_Only) return false;
	long root_Pointer;
	long au_List_Pointer;
    ////////////////////
    gettimeofday(&before, 0);
    //
    long cntu_Address = t.findCNTU(channel_Name, &root_Pointer, &au_List_Pointer);
	if(cntu_Address < 0)
	{
		if(t.addCNTU(channel_Name, &root_Pointer, &au_List_Pointer) == false) return false;
	}

    ///////////////////////
    gettimeofday(&after, 0);
    length_Of_Time = (after.tv_sec - before.tv_sec)*1000000 + (after.tv_usec - before.tv_usec);  
    if (verbose)
	    printf("It takes %ld microseconds to find the address of the R tree\n", length_Of_Time);
    //
	if(root_Pointer < 0)
	{
		printf("The cntu at the address %ld is corrupt\n", cntu_Address);
		return false;
	}
	if(	r.getRootPointer() != root_Pointer	&&
		r.attach(&fa, root_Pointer) == false) return 0;

	interval search_Interval = interval(COMPLETE_TIME_RANGE);	
	//makes only sense when the tree exists in this index
	if(!r.isEmpty() && only_New_Data)
	{			
		interval this_Tree_Interval;
		if(r.getTreeInterval(&this_Tree_Interval) == false) return false;
		search_Interval.setStart(this_Tree_Interval.getEnd());
	}
	
	au_List_Iterator * ali = other.getAUListIterator(channel_Name);
	if(ali == 0) return false;
	long au_Address;
	archiver_Unit au;
    ///////////////////
    gettimeofday(&before, 0);
    //
	bool result = ali->getFirstAUAddress(search_Interval, &au_Address);
    ///////////////////////
    gettimeofday(&after, 0);
    length_Of_Time = (after.tv_sec - before.tv_sec)*1000000 + (after.tv_usec - before.tv_usec);  
    if (verbose)
	    printf("It takes %ld microseconds to find the first AU\n", length_Of_Time);
    //
    
	while(result)
	{
		if(au_Address < 0) 
		{
			delete ali;
			return true;
		}
        ///////////////////
        gettimeofday(&before, 0);
        //
		au.attach(other.getFile(), au_Address);
		if(au.readAU() == false) 
		{
			delete ali;
			return false;
		}
        stdString path_Str = stdString(au.getKey().getPath());
        if(!Filename::containsPath(path_Str))
        {
            //append the directory path of the original index
            stdString full_Path_Str;
            Filename::build(other.getDirectory(), path_Str, full_Path_Str);
            key_Object ko = key_Object(full_Path_Str.c_str(), au.getKey().getOffset());
            au.setKey(ko);
        }
		if(other.getGlobalPriority() > -1)	au.setPriority(other.getGlobalPriority());
		//we know its unique
		if(addAU(channel_Name, au) == false) 
		{
			delete ali;
			return false;
		}
         ///////////////////////
        gettimeofday(&after, 0);
        length_Of_Time = (after.tv_sec - before.tv_sec)*1000000 + (after.tv_usec - before.tv_usec);  
        if (verbose)
		printf("It takes %ld microseconds to add an AU\n", length_Of_Time);
        //
        ///////////////////
        gettimeofday(&before, 0);
        //        
      	result = ali->getNextAUAddress(&au_Address);
        ///////////////////////
        gettimeofday(&after, 0);
        length_Of_Time = (after.tv_sec - before.tv_sec)*1000000 + (after.tv_usec - before.tv_usec);  
        if (verbose)
		printf("It takes %ld microseconds to find a next AU\n", length_Of_Time);
        //
	}
	delete ali;
	return false;
}
	
bool archiver_Index::addAU(const char * channel_Name, archiver_Unit & au) 
{
	if(!isInstanceValid() || read_Only) return false;
	if(au.isAUValid() == false) return false;
	long root_Pointer;
	long au_List_Pointer;
	if(t.addCNTU(channel_Name, &root_Pointer, &au_List_Pointer) == false) return false;
	if(root_Pointer < 0 || au_List_Pointer < 0)
	{
		printf("Couldn't add the cntu \"%s\" to the table\n", channel_Name);
		return false;
	}

	au_List l = au_List(&fa);
	l.init(au_List_Pointer);
	long au_Address;
	bool result = l.addAU(&au, &au_Address);

    //both cases possible: AU needs an update, or need to be just inserted
    if(au_Address > 0)
	{	
		if(	r.getRootPointer() != root_Pointer &&
		r.attach(&fa, root_Pointer) == false) return false;
		if(result == true)	
		{
			return r.addAU(au_Address);	
		}
		else
		{
			//means: the AU is already stored in the tree
			if(au.getPriority() < 0 && !au.getInterval().isNull())
            {
                //try a direct update 
				if(r.updateLatestLeaf(au_Address, au.getInterval().getEnd())) return true;
                //if update not successful, go on
            }
			else
			{
                //check if the end time of the stored AU is before of the passed one 
				short new_Priority = au.getPriority(); //works even if it wasn't set!
				interval new_Interval = au.getInterval();
                
                au.attach(f, au_Address);
				if(au.readPriority() == false) return false;

                long temp = 0;
				if(new_Interval.isNull() == false)
				{
					if(au.readInterval() == false) return false;
					temp = compareTimeStamps(new_Interval.getEnd(), au.getInterval().getEnd());
				}

                //if a new priority and/or different end time is set
				if	(	au.getPriority() != new_Priority || temp != 0)
				{
                    //update the AU
					if(temp < 0)
					{
						printf("An existing AU has a later end time than the one that was tried to be added\n");
						return false;
					}
					if(new_Priority > -1)	
					{
						au.setPriority(new_Priority);
						if(au.writePriority() == false) return false;
					}
					if(new_Interval.isNull() == false)
					{
						new_Interval.setStart(au.getInterval().getStart());	//the end time is the one of the new AU
						au.setInterval(new_Interval);
						if(au.writeInterval() == false) return false;
					}

                    //rebuild the tree
                    au_List_Iterator ali = au_List_Iterator(f, au_List_Pointer);
					return r.rebuild(ali);
				}
				//no update needed
				else return true;
			}
		}
	}
	//if l.addAU() produced errors
	return false;
}

bool archiver_Index::getLatestAU(const char * channel_Name, archiver_Unit * au) 
{
	if(isInstanceValid() == false) 
	{
		return 0;
	}
	long root_Pointer;
	long cntu_Address = t.findCNTU(channel_Name, &root_Pointer);
	if(cntu_Address < 0)
	{
		return 0;
	}
	if(root_Pointer < 0)
	{
		printf("The cntu at the address %ld is corrupt\n", cntu_Address);
		return 0;
	}
	if(	r.getRootPointer() != root_Pointer &&
		r.attach(&fa, root_Pointer) == false) return 0;
	return r.getLatestAU(au);
}

bool archiver_Index::deleteTree(const char * channel_Name) 
{
	if(!isInstanceValid() || read_Only) return false;
	if(channel_Name == 0) return false;
	long cntu_Address = t.findCNTU(channel_Name);
	if(cntu_Address < 0) return true;
	
	cntu c;
	c.attach(f, cntu_Address);
	
	//first delete the tree
	long root_Address;
	if(c.readRootPointer(&root_Address) == false) return false;
	r.detach();	//very important
	fa.free(root_Address);

	//then delete the hash table entry
	return t.deleteCNTU(cntu_Address);
}

bool archiver_Index::getEntireIndexedInterval(const char * channel_Name, interval * result)
{
	if(isInstanceValid() == false) return false;
	if(channel_Name == 0) return false;
	long root_Pointer;
	long cntu_Address = t.findCNTU(channel_Name, &root_Pointer);
	if(cntu_Address < 0)
	{
		return false;
	}
	if(root_Pointer < 0)
	{
		printf("The cntu at the address %ld is corrupt\n", cntu_Address);
		return false;
	}
	if(	r.getRootPointer() != root_Pointer &&
		r.attach(&fa, root_Pointer) == false) return false;

	return r.getTreeInterval(result);
}

channel_Name_Iterator * archiver_Index::getChannelNameIterator() const
{
	if(isInstanceValid() == false) return 0;
	return new channel_Name_Iterator(&t);
}

au_List_Iterator * archiver_Index::getAUListIterator(const char * channel_Name)
{
	if(isInstanceValid() == false) return 0;
	if(channel_Name == 0) return false;
	long root_Pointer, au_List_Pointer;
	long cntu_Address = t.findCNTU(channel_Name, &root_Pointer, &au_List_Pointer);
	if(cntu_Address < 0 || root_Pointer < 0 || au_List_Pointer < 0)
	{
		return 0;
	}
	
	return new au_List_Iterator(f, au_List_Pointer);
}

bool archiver_Index::readAU(long au_Address, archiver_Unit * result)
{
    result->attach(f, au_Address);
    return result->readAU();
}

key_AU_Iterator * archiver_Index::getKeyAUIterator(const char * channel_Name)
{
	if(isInstanceValid() == false) return 0;
	if(channel_Name == 0) return false;
	long root_Pointer;
	long cntu_Address = t.findCNTU(channel_Name, &root_Pointer);
	if(cntu_Address < 0)
	{
		return 0;
	}
	if(root_Pointer < 0)
	{
		return 0;
	}
	if(	r.getRootPointer() != root_Pointer &&
		r.attach(&fa, root_Pointer) == false) return 0;
	return new key_AU_Iterator(&r);	
}


void archiver_Index::dump(const char * file_Path, const char * mode = "a")
{
	if(isInstanceValid() == false) return;
	if(file_Path == 0) return;
	FILE * dump_File = fopen(file_Path, mode);
	if(dump_File == 0) return;
	//dump the cntu table first
	t.dump(dump_File);
	fprintf(dump_File, "\n\n");
		
	//dump all the trees next
	cntu current_CNTU = cntu();
	for(int i=0;i<t.getSize();i++)
	{
		long cntu_Address;
		fseek(f, t.getAddress() + 4 *i, SEEK_SET);
		if(readLong(f, &cntu_Address) == false) return;
		do
		{
			if(cntu_Address < 1) break;
			current_CNTU.attach(f, cntu_Address);

			if(r.attach(&fa, current_CNTU.getAddress() + CNTU_ROOT_OFFSET, true) == false) return;
			r.dump(dump_File);
			if(current_CNTU.readNextCNTUAddress(&cntu_Address) == false) return;
			fprintf(dump_File, "\n\n");
		}
		while(true);
		
	}
	fclose(dump_File);
}

void archiver_Index::createDotFile(const char * channel_Name, const char * file_Name)
{
	if(isInstanceValid() == false) return;
	if(channel_Name == 0) return;
	long root_Pointer;
	long cntu_Address = t.findCNTU(channel_Name, &root_Pointer);
	if(cntu_Address < 0) return;
	if(root_Pointer < 0)
	{
		printf("The cntu at the address %ld is corrupt\n", cntu_Address);
		return;
	}
	if(	r.getRootPointer() != root_Pointer &&
		r.attach(&fa, root_Pointer) == false) return;
	r.writeDotFile(file_Name);
}

void archiver_Index::dumpNames(FILE * text_File)
{
	if(text_File == 0) return;
	class bin_Tree_Item
	{
    public:
		stdString name;
		bin_Tree_Item * left;	//<
		bin_Tree_Item * right;  //>=
	};

	channel_Name_Iterator cni = channel_Name_Iterator(&t);
	bin_Tree_Item * bti_Root = new bin_Tree_Item();
	if(cni.getFirst(&(bti_Root->name)) == false)
	{
		delete bti_Root;
		return;
	}
	bti_Root->left = 0;
	bti_Root->right = 0;
	
	bin_Tree_Item ** root = &bti_Root;	//root is the pointer to the root node
	bin_Tree_Item * current_Node;
    do
	{
		bin_Tree_Item * bti = new bin_Tree_Item();
		if(cni.getNext(&bti->name) == false)
            {
                delete bti;
                break;
            }
		bti->left = 0;
		bti->right = 0;
		current_Node = *root;
		do
		{
			if(bti->name >= current_Node->name)
			{
				if(current_Node->right == 0)	
				{
					current_Node->right = bti;
					break;
				}
				else
				{
					current_Node = current_Node->right;
				}
			}
			else
			{
				if(current_Node->left == 0)	
				{
					current_Node->left = bti;
					break;
				}
				else
				{
					current_Node = current_Node->left;
				}
			}
		}
		while(true);
	}
	while(true);

	//traversing the binary tree 
	//we descend to the left until we find a node that has NO left child (always save the previous node)
	//this node is written to the text file
	//if the node has no parent (is the "root"),
	//	set its right child as the root
	//if the node with no left child has no right child //the two cases below are essentially the same	
	//	set its parent's left pointer to 0
	//if the node with no left child has a right child
	//	set its parent's left pointer to the right child

	//in any way the node is FREED (-> no memory leaks)
    bin_Tree_Item * parent_Node;
	while(true)
	{
		parent_Node	= 0;
		current_Node = *root;
		while(true)
		{
			if(current_Node->left == 0)
			{
				if(parent_Node == 0)
				{
					*root = current_Node->right;					
				}
				else 
				{
					parent_Node->left = current_Node->right;
				}
				if(text_File == 0) 
				{
					printf("%s\n", current_Node->name.c_str());
				}
				else 
				{
					fprintf(text_File, "%s\n", current_Node->name.c_str());
				}

				delete current_Node;
				break;
			}
			parent_Node = current_Node;
			current_Node = current_Node->left;
		}
		if(*root == 0) return;
	}
}	

bool archiver_Index::isInstanceValid() const
{
	if	(	(f==0) ||
			(t.getSize() < 2) ||
			(m < 2)
		) 
	{
		printf("The archiver_Index class is not used correctly\n");
		return false;
	}
	return true;
}

//the file must have been opened i.e. use only inside open()!!!
bool archiver_Index::create(short m, short hash_Table_Size)
{
	if(m < 2)
	{
		printf("The R tree parameter m must be greater than or equal 2\n");
		return false;
	}
	if(hash_Table_Size < 2)
	{
		printf("The size of the hash table must be greater than or equal 2\n");
		return false;
	}
	this->m = m;
	t.setSize(hash_Table_Size);

	if(fa.attach(f, HEADER_SIZE) == false) return false;	
	
	fseek(f, MAGIC_ID, SEEK_SET);
	if(fwrite("RTI1", 4, 1, f) != 1)
	{
		printf("Couldn't write to the address %d of the file %s\n", MAGIC_ID, full_Path.c_str());
		return false;
	}

	fseek(f, CNTU_TABLE_POINTER, SEEK_SET);
	if(writeLong(f, -1) == false)
	{
		printf("Couldn't write to the address %d of the file %s\n", CNTU_TABLE_POINTER, full_Path.c_str());
		return false;
	}

	fseek(f, HASH_TABLE_SIZE, SEEK_SET);
	if(writeShort(f, hash_Table_Size) == false)
	{
		printf("Couldn't write to the address %d of the file %s\n", HASH_TABLE_SIZE, full_Path.c_str());
		return false;
	}

	fseek(f, R_TREE_M, SEEK_SET);
	if(writeShort(f, m) == false)
	{
		printf("Couldn't write to the address %d of the file %s\n", R_TREE_M, full_Path.c_str());
		return false;
	}
	if(r.setM(m) == false) return false;
	
	return t.attach(&fa, CNTU_TABLE_POINTER);
}


