// System
#include <stdio.h>
// Base
#include <epicsVersion.h>
// Tools
#include "ArchiverConfig.h"
#include "ArgParser.h"
#include "epicsTimeHelper.h"

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
    CmdArgFlag debug (parser, "debug", "Enable Debug Info");
    CmdArgFlag redo  (parser, "redo", "Rebuild index from scratch");
    if (! parser.parse())
        return -1;

    if (help)
    {
        printf("This tools reads an archive list file,\n");
        printf("that is a file listing sub-archives,\n");
        printf("and generates a master index for all of them\n");
        printf("in the output index\n");
        printf("\n");
        printf("When invoked with the 'redo' option,\n");
        printf("the output index is created from scratch.\n");
        printf("Otherwise (default), a fast update is attempted\n");
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

    return 0;
}
