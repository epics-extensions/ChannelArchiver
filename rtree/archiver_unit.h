// r_tree_types.h

#ifndef _ARCHIVER_UNIT_H_
#define _ARCHIVER_UNIT_H_

#include <stdio.h>
#include "key_object.h"
#include "interval.h"

class archiver_Unit
{
public:
	archiver_Unit();
	archiver_Unit(const key_Object & k, const interval & i, short p);	
	
	long getAddress() const					{return au_Address;}
	long getSize() const					{return AU_SIZE_WITHOUT_KEY + key.getSize();}
	
	const key_Object& getKey() const		{return key;}
	const interval& getInterval() const		{return _interval;}
	short getPriority() const				{return priority;}
	void setKey(const key_Object& k)		{key = k;}
	void setInterval(const interval& i)		{_interval = i;}
	void setPriority(short p)				{priority = p;}
	

	bool operator ==(const archiver_Unit& other) const	{return key == other.getKey();}
	
	/**
	*	@return False, if the key offset or the priority is negative, or interval is not valid;
	*	true otherwise
	*/
	bool isAUValid() const;
	
	void print(FILE * text_File) const;
	
	/**
	*	Only the file pointer is copied into the object; NO memcpy or similar
	*	MUST be called before invoking any i/o methods
	*	Does not check if the file pointer is valid (see archiver_Index::open())	
	*/
	void attach(FILE * f, long au_Address);

	/**
	*	Set the object attributes according to the values from the file this object was attached to
	*	@see attach()	
	*	@return False if errors occured, or attach() was not called before; true otherwise
	*/
	bool readAU();	
	bool readKey();
	bool readInterval();
	bool readPriority();
	
	/**
	*	Determine the address of the next/previous AU
	*	In the index file, AUs are stored inside a list.
	*	@param result is a pointer which the address of the next/previous AU is read to from the file;
	*	a *result < 1 means there is no next/previous AU
	*	@return False if i/o errors occured, or attach() was not called before; true otherwise
	*/
	bool readNextPointer(long * result);
	bool readPreviousPointer(long * result);
	
	/**
	*	Write the object attributes to the file this object was attached to
	*	@see attach()	
	*	@return False if errors occured, or attach() was not called before; true otherwise
	*/
	bool writeAU() const;
	bool writeKey() const;	 
	bool writeInterval() const;
	bool writePriority() const;

	/**
	*	Store the address of the next/previous AU in the index file
	*	@return False if i/o errors occured, or attach() was not called before; true otherwise
	*/
	bool writeNextPointer(long value) const;
	bool writePreviousPointer(long value) const;

private:
	key_Object key;
	interval _interval;
	short priority;
	FILE * f;
	long au_Address;
};

#endif

