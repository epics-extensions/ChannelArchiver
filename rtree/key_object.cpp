#include "key_object.h"
#include "bin_io.h"
#include <string.h>
#include <stdlib.h>

//key_Object
key_Object::key_Object()
:	path(0), offset(-1), f(0), key_Address(-1){}

key_Object::key_Object(const char * _path, long _offset)
:	path(0), offset(_offset), f(0), key_Address(-1)
{
	initPath(strlen(_path) + 1);
	strcpy(path, _path);
}

key_Object::~key_Object()
{
	free (path);
}

long key_Object::getSize() const
{
	if(path == 0) return -1;
	return KO_SIZE_WITHOUT_PATH + strlen(path) + 1;
}

bool key_Object::operator ==(const key_Object& other) const
{
	return (!strcmp(path, other.path) &&(offset == other.offset));
}

void key_Object::operator =(const key_Object& other)
{
	initPath(strlen(other.getPath()) + 1);
	strcpy(path, other.getPath());
	offset = other.getOffset();
}

void key_Object::setPath(const char * p)	
{
	initPath(strlen(p) + 1);
	strcpy(path, p);
}		

//(re-)allocates 'memory_Block' bytes of memory, sets the last byte to 0
void key_Object::initPath(short memory_Block)
{
	if(path!=0)
	{
		free (path);
	}
	path = (char *) malloc(memory_Block);
	path[memory_Block-1] = 0;
}

////////////////
//IO stuff below
////////////////

void key_Object::attach(FILE * f, long key_Address)
{
	this->f = f;
	this->key_Address = key_Address;
}

bool key_Object::readKey()
{
	return
		(
			readPath() &&
			readOffset()
		);
}

bool key_Object::writeKey() const
{
	return
		(
			writePath() &&
			writeOffset()
		);
}

bool key_Object::readPath()
{
	if(f == 0 || key_Address < 0) 
	{
		printf("No file specified\n");
		return false;
	}

	long ADDRESS = key_Address + KO_PATH_LENGTH_OFFSET;
	fseek(f, ADDRESS , SEEK_SET);
	short PATH_LENGTH;
	if(readShort(f, &PATH_LENGTH) == false)
	{
		printf("Failed to read the path length of the key from the address %ld\n", ADDRESS);
		return false; 
	}
	initPath(PATH_LENGTH + 1);
	ADDRESS = key_Address + KO_PATH_OFFSET;
	fseek(f, ADDRESS, SEEK_SET);
	if(fread(path, PATH_LENGTH, 1, f) != 1)
	{
		printf("Failed to read the path of the key from the address %ld\n", ADDRESS);
		return false; 
	}
	return true;
}

bool key_Object::readOffset()
{
	if(f == 0 || key_Address < 0) 
	{
		printf("No file specified\n");
		return false;
	}
	
	const long ADDRESS = key_Address + KO_OFFSET_OFFSET;
	fseek(f, ADDRESS, SEEK_SET);
	if(readLong(f, &offset) == false)
	{
		printf("Failed to read the offset of the key from the address %ld\n", ADDRESS);
		return false;
	}
	return true;
}

bool key_Object::writePath() const
{
	if(f == 0 || key_Address < 0) 
	{
		printf("No file specified\n");
		return false;
	}
	
	if(path == 0)
	{
		printf("No path specified\n");
		return false;
	}
	const short PATH_LENGTH = strlen(path);
	long ADDRESS = key_Address + KO_PATH_LENGTH_OFFSET;
	fseek(f, ADDRESS, SEEK_SET);
	if(writeShort(f, PATH_LENGTH) == false)
	{
		printf("Failed to write the path length of the key to the address %ld", ADDRESS);
		return false;
	}
	
	ADDRESS = key_Address + KO_PATH_OFFSET;
	fseek(f, ADDRESS, SEEK_SET);
	if(fwrite(path, PATH_LENGTH, 1, f) != 1)
	{
		printf("Failed to write the path of the key to the address %ld", ADDRESS);
		return false;
	}
	return true;	
}

bool key_Object::writeOffset() const
{
	if(f == 0 || key_Address < 0) 
	{
		printf("No file specified\n");
		return false;
	}

	const long ADDRESS = key_Address + KO_OFFSET_OFFSET;
	fseek(f, ADDRESS, SEEK_SET);
	if(writeLong(f, offset) == false) 
	{	
		printf("Failed to write the offset of the key to the address %ld\n", ADDRESS);
		return false;
	}
	return true;
}




