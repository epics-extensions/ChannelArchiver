//key_object.h

#ifndef _KEY_OBJECT_H_
#define _KEY_OBJECT_H_

#include <stdio.h>
#include "rtree_constants.h"

class key_Object
{
public:
	//allocates memory for path
	key_Object();
	key_Object(const char * _path, long _offset);
	
	//deallocates memory; the user does NOT need to take care of memory leaks
	~key_Object();
	
	/**
	*	See file_allocaror::allocate()
	*	@return Number of bytes the object needs to be stored in a file
	*/
	long getSize() const;
	
	const char * getPath() const		{return path;}
	long getOffset() const				{return offset;}
	void setPath(const char * p);		
	void setOffset(const long o)		{offset = o;}
	
	bool operator ==(const key_Object& other) const;
	void operator =(const key_Object& other);

	//text_File should be opened in mode "t"
	void print(FILE * text_File) const	{fprintf(text_File, "%s : %ld", path, offset);	}
	
	/**
	*	Only the file pointer is copied into the object; NO memcpy or similar
	*	MUST be called before invoking any i/o methods
	*	Does not check if the file pointer is valid (see archiver_Index::open())	
	*/
	void attach(FILE * f, long key_Address);

	/**
	*	Set the object attributes according to the values from the file this object was attached to
	*	@see attach()	
	*	@return False if errors occured, or attach() was not called before; true otherwise
	*/
	bool readKey();
	bool readPath();
	bool readOffset();

	/**
	*	Write the object attributes to the file this object was attached to
	*	@see attach()	
	*	@return False if errors occured, or attach() was not called before; true otherise
	*/
	bool writeKey() const;
	bool writePath() const;
	bool writeOffset() const;
	

private:
	//memory management for the attribute "path"
	void initPath(short memory_Block);
	char * path;
	long offset;

	FILE * f;
	long key_Address;
};


#endif //key_object.h

