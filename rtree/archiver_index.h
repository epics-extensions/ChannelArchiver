#ifndef _ARCHIVER_INDEX_H_
#define _ARCHIVER_INDEX_H_

#include <stdString.h>
#include "r_tree.h"
#include "au_list.h"
#include "aup_iterator.h"
#include "channel_name_iterator.h"
#include "cntu_table.h"
#include "rtree_constants.h"
#include "key_au_iterator.h"


/**
*	For each channel name, there is exactly one R tree that stores the AUs that contain the channel data
*/
class archiver_Index
{
public:
	archiver_Index();
	~archiver_Index();

	/**
	*	Is used only inside addDataFromAnotherIndex()
	*	@see addDataFromAnotherIndex()
	*	@param value is read from the master index configuration file
	*/
    short getGlobalPriority() const     {return global_Priority;}
    void setGlobalPriority(short value) {global_Priority = value;}

    /**
    *  A work around  for addDataFromAnotherIndex() if the original index
    *  doesn't store the directory of the AUs
    *  @see addDataFromAnotherindex()
    */
    const stdString&  getDirectory() const {return dir;}
    const stdString& getFileName() const {return file_Name;}
    const stdString& getFullPath() const {return full_Path;}
    
	/**
	*	Open the file with the specified path and check, if it is a valid index file
    *   If the file doesn't exist, but read_Only is set to false, create is called with standard parameters
    *   @param file_Path MUST be an absolute path (the class stores the directory
    *   @see create()
    *   @see getDirectory()
	*	@return false, if the file was tried to be opened in "read only" mode, but it doesn't exist; or if the file is not valid, or errors occured; true otherwise.
	*/
	bool open(const char * file_Path, bool read_Only = true);

	/**
	*	Create a new index file
	*	Caution: If the file with the specified name exists, it is overwritten!
	*	@param m determines the size of the R tree nodes; must be greater than or equal 2
	*	@param hash_Table_Size must be greater than or equal 2; should be a prime number 
	*	@return False if the parameters are illegal, or errors occured; true otherwise.
	*/
	bool create(const char * file_Path, short m=3, short hash_Table_Size=1007);

	/**
	*	Detach the file allocator, close the index file and reset the object attributes.
	*	@return False, if errors occured; true otherwise.
	*/
	bool close();

	FILE * getFile() const			{return f;}

	/**
	*	Find the R tree for the specified channel name;
	*	if the AU is not yet stored in the R tree, insert it into the R tree;
	*	if it is already there- update either its end time or its priority, or both
	*	Note: If you are sure that "au" is in the index file, you can leave some of its parameters 'NULL'
	*	=> they are then not processed
	*	e.g. au = archiver_Unit(key_Object("path", 10), interval(), 2) 
	*	or
	*	au = archiver_Unit(key_Object("path", 10), interval(0, 0, 10000, 3000), -1)
	*	If you however don't set the start time of the au, expect a delay when 
	*	looking it up in the AU list which is sorted by start times
	*	@return False, if errors occured; true otherwise
	*/
	bool addAU(const char * channel_Name, archiver_Unit & au);
	
	/**
	*	Find the R tree for the specified channel name in another index file and
	*	add the archiver units to the corresponding R tree in the file managed by 
	*	<i>this</i> index object.
	*	@param only_New_Data specifies, if only data that was generated after 
	*	the last utilization of <i>this</i> index (true), or actually all data from another index 
	*	should be processed (false).
	*	If the latter option is chosen, but some data already exists in <i>this</i> index, 
	*	it is simply ignored.
	*	Note: If  no tree exists in <i>this</i> index file for the specified channel name, all data 
	*	is processed, regardless of the value of "only_New_Data".
	*	@return False, if errors occured; true otherwise.
	*/
	bool addDataFromAnotherIndex(const char * channel_Name, archiver_Index& other, bool only_New_Data = true);

	/**
	*	The "latest AU" has been defined in the design as the one and only AU referenced
	*	in the AU pointer list of the latest leaf in the corresponding R tree
	*	@param au is the pointer to the AU object which attributes are set according to the values 
	*	of the "latest AU"
	*	@return True, if *au is indeed the "latest"; false if errors occured, or there is no latest AU in the 
	*	tree (beacuse there are references to more than one AU in the AU pointer list of the latest leaf)
	*/
	bool getLatestAU(const char * channel_Name, archiver_Unit * au);
	
	/**
	*	Find the corresponding R tree, release the occupied memory block and delete the entry
	*	in the hash table
	*	NOTE: Due to the dispersion of small AU and AUP blocks of memory (in average just 64 byte each)
	*	all over the file, the deletion of those seems not to make any sense.
	*	For an optimal utilization of file memory, it is recommended to create a new file and with the help
	*	of the "channel name iterator" to use add DataFromAnotherIndex()
	*	@see getChannelNameIterator()
	*	@see addDataFromAnotherIndex()
	*	@return True if successful; false if there is no such AU in the index file, or errors occured
	*/
	bool deleteTree(const char * channel_Name);
	
	/**
	*	@param result is the pointer to the interval object which attributes are set according
	*	to the values of R tree interval
	*	@return False if errors occured or the tree doesn't exist; true otherwise
	*/
	bool getEntireIndexedInterval(const char * channel_Name, interval * result);

	/**
	////
	//	It is imperative for all iterators that
	//	the user take care of the memory, unless 0 is returned!
	//	For detailed description, see the corresponding classes
	////
	*/
	//Iterate through the channel names in the index file
	channel_Name_Iterator * getChannelNameIterator() const;
    
	//Iterate through ALL AUs in the corresponding R tree
	aup_Iterator * getAUPIterator(const char * channel_Name);
    /**
    *   The AUP iterator returns the addresses of the AUs;
    *   <i>this</i> method reads an AU from the index file
    *   @see getAUPIterator()
    *   @param result is the pointer to an au object which attributes
    *   are set according to the values in the index file
    *   return false, if errors occured; true otherwise
    */
    bool readAU(long au_Address, archiver_Unit * result);
    
    //Iterate through the KEY AUs in the corresponding R tree, i.e.
	//the ones that are eventually used to lookup the data!!!
	key_AU_Iterator * getKeyAUIterator(const char * channel_Name);
	
	/**
	*	Print all R trees 
	*	@param mode is the same as in fopen()	
	*/
	void dump(const char * file_Path, const char * mode);

	/**
	*	Creates "dot" input for IBM's program GRAPHVIZ (http://www.graphviz.org) which can draw
	*	the actual tree for the specified channel_Name
	*/
	void createDotFile(const char * channel_Name, const char * file_Name); 
	
	/**
	*	Print all stored channel names in alphabetical order
	*	@param text_File being 0 means: Dump to the standard output
	*/
	void dumpNames(FILE * text_File = 0);
	
	/**
	*	@return True if the index file was accessed using open() or create(); false otherwise
	*/
	bool isInstanceValid() const;	
private:

	FILE * f;						
	file_allocator fa;
	cntu_Table t;
	r_Tree r;	
	short m;	
	bool read_Only;
    stdString dir;
    stdString full_Path;
    stdString file_Name;
	short global_Priority;
};



#endif	//_archiver_index_h_


