#include <stdio.h>
#include "bin_io.h"
#include <stdlib.h>
#include <string.h>
//#include <time.h>
//#include <sys/types.h> 
//#include <sys/timeb.h>
#include "archiver_index.h"

#define NR_CHANNELS 3

int run()
{
	archiver_Index my_Index;
	my_Index.create("index.dat", 2);
	archiver_Unit au;
	interval iv;
	key_Object ko;
	
	//just add a unit
	ko = key_Object("10.dat", 0);
	iv = interval(10, 0, 12, 0);
	au = archiver_Unit(ko, iv, 0);
	my_Index.addAU("James_Bond", au);
	my_Index.dump("tree.txt", "w");	//tree.txt is always created in the beginning
	
	//update the latest au
	au = archiver_Unit(key_Object("10.dat", 0), interval(10, 0, 13, 1), 0);
	my_Index.addAU("James_Bond", au);
	my_Index.dump("tree.txt", "a");

	//update again: no interval though	
	au= archiver_Unit(key_Object("10.dat", 0), interval(), 2);
	my_Index.addAU("James_Bond", au);
	my_Index.dump("tree.txt", "a");

	//delete
	my_Index.deleteTree("James_Bond");
	my_Index.dump("tree.txt", "a");

	//THE FILE IS EMPTY here
	
	//just add a unit
	au = archiver_Unit(key_Object("10.dat", 0), interval(10, 0, 12, 0), 0);
	my_Index.addAU("James_Bond", au);
	my_Index.dump("tree.txt", "a");
	
	//add a unit that is later than the latest
	au = archiver_Unit(key_Object("10.dat", 1), interval(12, 0, 13, 1), 0);
	my_Index.addAU("James_Bond", au);
	my_Index.dump("tree.txt", "a");
	
	//update a unit that is NOT the latest
	//iv = interval(10, 0, 15, 1);
	au = archiver_Unit(key_Object("10.dat", 0), interval(10, 0, 15, 1), 1);
	my_Index.addAU("James_Bond", au);
	my_Index.dump("tree.txt", "a");

	//add a unit before the current tree interval
	au = archiver_Unit(key_Object("10.dat", 2), interval(2, 0, 3, 0), 0);
	my_Index.addAU("James_Bond", au);
	my_Index.dump("tree.txt", "a");

	//dot File
	my_Index.createDotFile("James_Bond", "dot.txt");

	//add between two intervals
	au = archiver_Unit(key_Object("10.dat", 3), interval(6, 0, 9, 0), 1);
	my_Index.addAU("James_Bond", au);
	my_Index.dump("tree.txt", "a");

	//add after the tree interval
	au = archiver_Unit(key_Object("10.dat", 4), interval(20, 0, 24, 0), 1);
	my_Index.addAU("James_Bond", au);
	my_Index.dump("tree.txt", "a");
		
	//intersect an au with a "larger" interval
	au = archiver_Unit(key_Object("10.dat", 5), interval(14, 0, 17, 0), 1);
	my_Index.addAU("James_Bond", au);
	my_Index.dump("tree.txt", "a");

	//delete 

	my_Index.deleteTree("James_Bond");
	my_Index.dump("tree.txt", "a");

	//add an au with a smaller interval

	au = archiver_Unit(key_Object("10.dat", 0), interval(18, 0, 21, 0), 1);
	my_Index.addAU("James_Bond", au);
	my_Index.dump("tree.txt", "a");

	//add an au with an interval that intersects to leaves
	au = archiver_Unit(key_Object("10.dat", 6), interval(8, 0, 12, 5), 1);
	my_Index.addAU("James_Bond", au);
	my_Index.dump("tree.txt", "a");

	//add an au with an interval that is within ONE leaf's interval
	au = archiver_Unit(key_Object("10.dat", 7), interval(16, 0, 16, 5), 1);
	my_Index.addAU("James_Bond", au);
	my_Index.dump("tree.txt", "a");

	//add an au that stores data from a different channel 
	au = archiver_Unit(key_Object("10.dat", 8), interval(10, 0, 12, 0), 2);
	my_Index.addAU("Sean_Connery", au);
	my_Index.dump("tree.txt", "a");


	//close and open again
	my_Index.close();
	my_Index.open("index.dat", false);

	archiver_Index new_Index; 
	new_Index.create("new_index.dat", 3, 1007);
	channel_Name_Iterator * ci = my_Index.getChannelNameIterator();
    if(ci != 0)
    {
        stdString channel_Name;
        bool result = ci->getFirst(&channel_Name);
        while(result)
        {
            new_Index.setGlobalPriority(2);
            new_Index.addDataFromAnotherIndex(channel_Name.c_str(), my_Index);
            new_Index.dump("new_tree.txt", "w");
            result = ci->getNext(&channel_Name);
        }
        delete ci;
    }

	//add two aus that have the same start time and end time as two other leaves
	au = archiver_Unit(key_Object("10.dat", 8), interval(16, 5, 21, 0), 1);
	my_Index.addAU("James_Bond", au);
	my_Index.dump("tree.txt", "a");
	
	au = archiver_Unit(key_Object("10.dat", 9), interval(12, 0, 14, 0), 0);
	my_Index.addAU("James_Bond", au);
	my_Index.dump("tree.txt", "a");

	//add enough AUs so that the tree is moved
    int i=0;
	for (i=0; i< 32;i++)
	{
		//offset 0..9 is used above
        if(i> 10 && i < 15) continue;
        au = archiver_Unit(key_Object("10.dat", i+10), interval(12 + i, 0, 14 + i, 0), 0);
		my_Index.addAU("James_Bond", au);
	}
	my_Index.dump("tree.txt", "a");

	//add just the new units
	new_Index.addDataFromAnotherIndex("James_Bond", my_Index, true);
	new_Index.dump("new_tree.txt", "a");

	//////////////////////////
	//test the key_au_iterator
	//////////////////////////

	//search_Interval

	interval lookup_Interval;
	FILE * keys = fopen("keys.txt", "w");
	key_AU_Iterator * kai = my_Index.getKeyAUIterator("James_Bond");
	if(kai !=0)
	{
		bool tmp = kai->getFirst(interval(25,1,26,0), &ko, &lookup_Interval);
		while(tmp)
		{
			//epicsTimeStamp start = lookup_Interval.getStart();
			//epicsTimeStamp end = lookup_Interval.getEnd();
			//- go to position 'offset' in the file 'current_File'
			//- read the samples from the time stamp 'start' to 'end'
			ko.print(keys);
			fprintf(keys, "  ");
			lookup_Interval.print(keys);
			fprintf(keys, "\n");
			tmp = kai->getNext(&ko, &lookup_Interval);
		}
		delete kai;
	}
	fclose(keys);

	my_Index.createDotFile("James_Bond", "dot.txt");

	my_Index.close();
	new_Index.close();
	

    //2 AUs per channel
    char channel[100];
    my_Index.create("names_test.dat");
	FILE * names = fopen("names.txt", "r");
    if(names == 0)
    {
        printf("names.txt not in this directory\n");
        return 0;
    }
	while(!feof(names))
	{
		fscanf(names, "%s", channel);
		for (int i=0; i< 2;i++)
		{
			au = archiver_Unit(key_Object("10.dat", i), interval(12 + i, 0, 14 + i, 0), 0);
			my_Index.addAU(channel, au);
		}
	}
	fclose(names);

	FILE * names_File = fopen("names2.txt", "w+");
	my_Index.dumpNames(names_File);
	fclose(names_File);
    my_Index.close();
	////////////////////////////////
	//	The stuff for the documentation
	////////////////////////////////

    ko.setPath("file path");
	my_Index.open("index1.dat", false);
    for(i = 1; i < 7; i++)
    {
        au = archiver_Unit(key_Object("file path A", i), interval(2 * i, 0,  2*i +2, 0), 0);
        if(!my_Index.addAU("channelX", au)) return 0;
    }
    my_Index.createDotFile("channelX", "index1_dot.txt");    
	my_Index.close();

	my_Index.open("index2.dat", false);
    for(i = 1; i < 7; i++)
    {
        au = archiver_Unit(key_Object("file path B", i), interval(3 * i, 0,  3 * i +3, 0), 0);
        if(!my_Index.addAU("channelX", au)) return 0;
    }
    au = archiver_Unit(key_Object("file path B", 7), interval(2, 0, 20, 0), 0);
    if(!my_Index.addAU("channelX", au)) return 0;
    my_Index.createDotFile("channelX", "index2_dot.txt");
    aup_Iterator * ai = my_Index.getAUPIterator("channelX");
   
    if(ai != 0)
    {
        long au_Address;
        bool result = ai->getFirstAUAddress(interval(COMPLETE_TIME_RANGE), &au_Address);
        while(result && (au_Address > 0) && my_Index.readAU(au_Address, &au))
        {
            printf("%s @ %ld\n", au.getKey().getPath(), au.getKey().getOffset());
            result = ai->getNextAUAddress(&au_Address);
        }
    }
	my_Index.close();
	return 0;
}

int main(void *)
{
	run();
	return 0;
}


