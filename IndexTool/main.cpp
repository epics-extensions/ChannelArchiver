// System
#include <stdio.h>
// Base
#include <epicsVersion.h>
// Index
#include <archiver_index.h>
// Tools
#include "ArchiverConfig.h"

#include "ArgParser.h"
#include "epicsTimeHelper.h"

//result must already be allocated 
void  readLine(FILE * f, char * result)
{
    int i = 0;
    do
    {
        result[i] = getc(f);
        i++;
    }
    while(result[i-1]!='\n' &&  result[i-1] != EOF);
    result[i-1] = 0;      //zero termination
}

int main(int argc, const char *argv[])
{
    initEpicsTimeHelper();

    CmdArgParser parser(argc, argv);
    parser.setHeader("Archive Mega Index version " VERSION_TXT ", "
                     EPICS_VERSION_STRING
                      ", built " __DATE__ ", " __TIME__ "\n\n"
                     );
    parser.setArgumentsInfo("<archive list file> <output index>");
    CmdArgFlag help  (parser, "help", "Show Help");
    CmdArgFlag verbose (parser, "verbose", "Show more info");
    if (! parser.parse())
        return -1;

    if (help)
    {
        printf("This tools reads an archive list file,\n");
        printf("that is a file listing sub-archives,\n");
        printf("and generates a master index for all of them\n");
        printf("in the output index\n");
        printf("\n");
        printf("When the master index file already exists,\n");
        printf("a fast update is attempted\n");
        return 0;
    }
    if (parser.getArguments().size() != 2)
    {
        parser.usage();
        return -1;
    }
    stdString archive_list_name = parser.getArgument(0);
    stdString output_name = parser.getArgument(1);

    // read archive_list_name, ...
    if (verbose)
        printf("Reading archive list %s, generating %s\n",
               archive_list_name.c_str(),
               output_name.c_str());
    char line[1000];
    char path[1000];
    char channel_Name[CHANNEL_NAME_LENGTH];
    int priority;
    bool result;
    bool only_New_Data = true;
    archiver_Index ai;
    archiver_Index master_Index;
    FILE * f  = fopen(output_name.c_str(), "r+b");
    //at this point the R tree parameters can be set for user's needs
    if(f == 0)
        {
            master_Index.create(output_name.c_str());
            only_New_Data = false;
        }
    else
        {
            master_Index.open(output_name.c_str(), false);
            fclose(f);
        }

    f = fopen(archive_list_name.c_str(), "r+t");
    while(true)
    {
        readLine(f, line);
        if(feof(f)) break;
        switch(sscanf(line, "%s %d", path, &priority))
        {
            case 2:
                ai.setGlobalPriority(priority);
            case 1:
                ai.open(path);
                break;
            default:
                printf("The configuration file is corrupt\n");
                return 0;
        }
        channel_Name_Iterator * cni = ai.getChannelNameIterator();
        if(cni == 0) return 0;
        result = cni->getFirst(channel_Name);
        while(result)
        {
            if(verbose)
            {
                printf("Processing channel %s from the index file %s\n", channel_Name, path);
            }
            if(master_Index.addDataFromAnotherIndex(channel_Name, ai, only_New_Data ) == false)
            {
                delete cni;
                return 0;
            };
            if(verbose) printf("%s processed\n\n", channel_Name);
            result = cni->getNext(channel_Name);
        }
        delete cni;
        ai.close();
    }
    master_Index.close();
    return 0;
}






