// array_list.h

#ifndef _ARRAY_LIST_H_
#define _ARRAY_LIST_H_

/**
*	This class stores long values in an array; the long values can, of course, be pointers, too; 
*	hoeevr the class destructor does NOT free the memory blocks the pointers point to!
*	Always check the number of elements before invoking any operations!!!
*/
class ArrayList{
public:
	ArrayList();
	/**
	*	Frees the memory of an internally allocated array only!
	*/
	~ArrayList();	
	void reset()				{number_Of_Elements = 0;}

	int getNumberOfElements()	{return number_Of_Elements;}
	long getElement(int i)		{return elements[i];}
	long getLastElement()		{return elements[number_Of_Elements - 1];}

	void insertElement(long e);	
	void deleteElement(int nr);

	/**
	*	gets and deletes the last element in one function
	*/
	long popLastElement();	
private:
	void increaseMemory();
	long * elements;
	int number_Of_Elements;
	int memory_Space;
};

#endif	//array_list.h


