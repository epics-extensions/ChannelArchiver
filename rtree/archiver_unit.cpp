#include "archiver_unit.h"
#include "bin_io.h"

//archiver_Unit

archiver_Unit::archiver_Unit()
:priority(-1), f(0), au_Address(-1){}


archiver_Unit::archiver_Unit(const key_Object & k, const interval & i, short p)
{
	key = k;
	_interval = i;
	priority = p;
}

void archiver_Unit::print(FILE * text_File) const
{
	key.print(text_File);
	putc(32, text_File);
	_interval.print(text_File);
	putc(32, text_File);
	fprintf(text_File, "%d", priority);
	return;
}

bool archiver_Unit::isAUValid() const
{
	if(key.getOffset() < 0)
	{
		printf("The offset of the archiver unit is negative\n");
		return false;
	}
	if(_interval.isIntervalValid() == false) return false;
	if(priority < 0) 
	{
		printf("The priority of the archiver unit is negative\n");
		return false;
	}
	return true;
}

//////////
//IO BELOW
//////////

void archiver_Unit::attach(FILE * f, long au_Address)
{
	this->f = f;
	this->au_Address = au_Address;
	//attach the private objects
	key.attach(f, au_Address + AU_KEY_OFFSET);
	_interval.attach(f, au_Address + AU_IV_OFFSET);
}

bool archiver_Unit::readAU()
{
	return
		(	readKey()  &&
			readInterval() &&
			readPriority() 
		);	
}

bool archiver_Unit::writeAU() const
{
	return
		(	writeKey()  &&
			writeInterval() &&
			writePriority()
		);		
}

bool archiver_Unit::readKey()
{
	if(f == 0 || au_Address < 0) 
	{
		printf("No file specified\n");
		return false;
	}
	return key.readKey();
}

bool archiver_Unit::readInterval()
{
	if(f == 0 || au_Address < 0) 
	{
		printf("No file specified\n");
		return false;
	}
	return _interval.readInterval();
}

bool archiver_Unit::readPriority()
{
	if(f == 0 || au_Address < 0) 
	{
		printf("No file specified\n");
		return false;
	}
	const long ADDRESS = au_Address + AU_PRIORITY_OFFSET;
	fseek(f, ADDRESS, SEEK_SET);
	if(readShort(f, &priority) == false)
	{
		printf("Failed to read the priority of the archived unit from the address %ld \n", ADDRESS);
		return false;
	}
	return true;
}


bool archiver_Unit::readNextPointer(long * result)
{
	if(f == 0 || au_Address < 0) 
	{
		printf("No file specified\n");
		return false;
	}

	const long ADDRESS = au_Address + AU_NEXT_OFFSET;
	fseek(f, ADDRESS, SEEK_SET);
	if(readLong(f, result) == false)
	{
		printf("Failed to read the next pointer of the archived unit from the address %ld \n", ADDRESS);
		return false;		
	}
	return true;
}

bool archiver_Unit::readPreviousPointer(long * result)
{
	if(f == 0 || au_Address < 0) 
	{
		printf("No file specified\n");
		return false;
	}

	const long ADDRESS = au_Address + AU_PREVIOUS_OFFSET;
	fseek(f, ADDRESS, SEEK_SET);
	if(readLong(f, result) == false)
	{
		printf("Failed to read the previous pointer of the archived unit from the address %ld \n", ADDRESS);
		return false;		
	}
	return true;
}

bool archiver_Unit::writeKey() const
{
	if(f == 0 || au_Address < 0) 
	{
		printf("No file specified\n");
		return false;
	}

	return key.writeKey();
}

bool archiver_Unit::writeInterval() const
{
	if(f == 0 || au_Address < 0) 
	{
		printf("No file specified\n");
		return false;
	}

	return _interval.writeInterval();
}

bool archiver_Unit::writePriority() const
{
	if(f == 0 || au_Address < 0) 
	{
		printf("No file specified\n");
		return false;
	}

	const long ADDRESS = au_Address + AU_PRIORITY_OFFSET;
	fseek(f, ADDRESS, SEEK_SET);
	if(writeShort(f, priority) == false)
	{
		printf("Failed to write the priority of the archived unit to the address %ld \n", ADDRESS);
		return false;
	}
	return true;
}

bool archiver_Unit::writeNextPointer(long value) const
{
	if(f == 0 || au_Address < 0) 
	{
		printf("No file specified\n");
		return false;
	}

	const long ADDRESS = au_Address + AU_NEXT_OFFSET;
	fseek(f, ADDRESS, SEEK_SET);	
	if(writeLong(f, value) == false)
	{
		printf("Failed to write the next pointer of the archived unit to the address %ld \n", ADDRESS);
		return false;
	}
	return true;
}

bool archiver_Unit::writePreviousPointer(long value) const
{
	if(f == 0 || au_Address < 0) 
	{
		printf("No file specified\n");
		return false;
	}

	const long ADDRESS = au_Address + AU_PREVIOUS_OFFSET;
	fseek(f, ADDRESS, SEEK_SET);	
	if(writeLong(f, value) == false)
	{
		printf("Failed to write the previous pointer of the archived unit to the address %ld \n", ADDRESS);
		return false;
	}
	return true;
}


