// array_list.h

#ifndef _ARRAY_LIST_H_
#define _ARRAY_LIST_H_

/**
*	This class stores long values in an array; the long values can, of course, be pointer, too
*	Always check the number of elements before invoking any operations!!!
*/
class ArrayList{
public:
	//delete or free issue
	ArrayList();
	~ArrayList();	
	void reset();

	int getNumberOfElements()	{return number_Of_Elements;}
	long getElement(int i)		{return elements[i];}
	long getLastElement()		{return elements[number_Of_Elements - 1];}

	void insertElement(long e);	
	void deleteElement(int nr);
	long popLastElement();	
private:
	void increaseMemory();
	long * elements;
	int number_Of_Elements;
	int memory_Space;
};

#endif	//array_list.h


