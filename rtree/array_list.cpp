#include <stdlib.h>
#include <string.h>
#include "array_list.h"

ArrayList::ArrayList()
		:number_Of_Elements(0), memory_Space(256)
{
	elements = (long *) malloc(memory_Space * 4);
}

ArrayList::~ArrayList()	
{
	free (elements);
}

void ArrayList::reset()
{
	number_Of_Elements = 0;
	memory_Space = 256;
}

void ArrayList::insertElement(long e)
{
	if (number_Of_Elements == memory_Space)
	{
		increaseMemory();
	}
	elements[number_Of_Elements] = e;
	number_Of_Elements++;
}

void ArrayList::deleteElement(int nr)
{
	for(int j=nr;j<number_Of_Elements-1;j++)
	{
		elements[j] = elements[j+1];
	}
	number_Of_Elements--;
}

long ArrayList::popLastElement()
{
	long temp = elements[number_Of_Elements-1];
	number_Of_Elements--;
	return temp;
}

void ArrayList::increaseMemory()
{
	//double the memory
	long * temp = (long *) malloc(2 * memory_Space * 4);
	memcpy(temp, elements, memory_Space * 4);
	free (elements);
	elements = temp;
	memory_Space *= 2;
}
