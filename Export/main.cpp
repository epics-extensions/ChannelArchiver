// Export/main.cpp

// Base
#include <epicsVersion.h>
// Tools
#include <BinaryTree.h>
#include <RegularExpression.h>
#include <epicsTimeHelper.h>
#include <ArgParser.h>
// Storage
#include <SpreadsheetReader.h>

bool verbose;

// "visitor" for BinaryTree of channel names
static void add_name2vector(const stdString &name, void *arg)
{
    stdVector<stdString> *names = (stdVector<stdString> *)arg;
    if (verbose)
        printf("%s\n", name.c_str());
    names->push_back(name);
}
void get_names_for_pattern(archiver_Index &index,
                           stdVector<stdString> &names,
                           const stdString &pattern)
{
    if (verbose)
        printf("Expanding pattern '%s'\n", pattern.c_str());
    RegularExpression *regex =
        RegularExpression::reference(pattern.c_str());
    if (!regex)
    {
        fprintf(stderr, "Cannot allocate regular expression\n");
        return;
    }     
    channel_Name_Iterator *cni = index.getChannelNameIterator();
    if (!cni)
    {
        fprintf(stderr, "Cannot get channel name iterator\n");
        return;
    }
    // Put all names in binary tree
 	BinaryTree<stdString> channels;
    stdString name;
    bool ok;
    for (ok = cni->getFirst(&name); ok; ok = cni->getNext(&name))
    {
        if (!regex->doesMatch(name.c_str()))
            continue; // skip what doesn't match regex
        channels.add(name);
    }
    delete cni;
    regex->release();
    // Sorted dump of names
    channels.traverse(add_name2vector, (void *)&names);
}

// Used by list_channels & name_printer
class ChannelInfo
{
public:
    stdString name;
    epicsTime start, end;

    bool operator < (const ChannelInfo &rhs)
    { return name < rhs.name; }

    bool operator == (const ChannelInfo &rhs)
    { return name == rhs.name; }
};

// "visitor" for BinaryTree of channel names
static void name_printer(const ChannelInfo &info, void *arg)
{
    bool show_info = (bool) arg;
    stdString s, e;
    if (show_info)
        printf("%s\t%s\t%s\n", info.name.c_str(),
               epicsTimeTxt(info.start, s), epicsTimeTxt(info.end, e));
    else
        printf("%s\n", info.name.c_str());
}

bool list_channels(archiver_Index &index, const stdString &pattern,
                   bool show_info)
{
    RegularExpression *regex = 0;
    if (pattern.length() > 0)
        regex = RegularExpression::reference(pattern.c_str());
    channel_Name_Iterator *cni = index.getChannelNameIterator();
    if (!cni)
    {
        fprintf(stderr, "Cannot get channel name iterator\n");
        return false;
    }
    // Put all names in binary tree
 	BinaryTree<ChannelInfo> channels;
    ChannelInfo info;
    bool ok;
    for (ok = cni->getFirst(&info.name); ok; ok = cni->getNext(&info.name))
    {
        if (regex && !regex->doesMatch(info.name.c_str()))
            continue; // skip what doesn't match regex
        if (show_info)
        {
            interval range;
            if (index.getEntireIndexedInterval(info.name.c_str(), &range))
            {
                info.start = range.getStart();
                info.end = range.getEnd();
            }
        }
        channels.add(info);
    }
    delete cni;
    if (regex)
        regex->release();
    // Sorted dump of names
    channels.traverse(name_printer, (void *)show_info);
    return true;
}

bool dump_spreadsheet(archiver_Index &index,
                      stdVector<stdString> names,
                      epicsTime *start, epicsTime *end,
                      bool status_text)
{
    SpreadsheetReader sheet(index);
    bool ok = sheet.find(names, start);
    size_t i;
    stdString time, stat, val;
    const RawValue::Data *value;
    printf("# Time                       ");
    for (i=0; i<sheet.getNum(); ++i)
    {
        printf("\t%s [%s]", sheet.getName(i).c_str(),
               sheet.getCtrlInfo(i).getUnits());
        if (status_text)
            printf("\t");
    }
    printf("\n");
    while (ok)
    {
        if (end && sheet.getTime() >= *end)
            break;
        epicsTime2string(sheet.getTime(), time);
        printf("%s", time.c_str());
        for (i=0; i<sheet.getNum(); ++i)
        {
            value = sheet.getValue(i);
            if (value)
            {
                RawValue::getStatus(value, stat);
                if (RawValue::isInfo(value))
                {
                    printf("\t#N/A");
                    if (status_text)
                        printf("\t%s", stat.c_str());
                }
                else
                {
                    RawValue::getValueString(
                        val, sheet.getType(i), sheet.getCount(i),
                        value, &sheet.getCtrlInfo(i));
                    printf("\t%s", val.c_str());
                    if (status_text)
                        printf("\t%s", stat.c_str());
                }
            }
            else
            {
                printf("\t#N/A");
                if (status_text)
                    printf("\t");
            }
        }
        printf("\n");
        ok = sheet.next();
    }
    return true;
}

int main(int argc, const char *argv[])
{
    initEpicsTimeHelper();
    CmdArgParser parser(argc, argv);
    parser.setHeader("Archive Export version " ARCH_VERSION_TXT ", "
                     EPICS_VERSION_STRING
                     ", built " __DATE__ ", " __TIME__ "\n\n");
    parser.setArgumentsInfo("<index file> {channel}");
    CmdArgFlag   be_verbose (parser, "verbose", "Verbose mode");
    CmdArgFlag   do_list    (parser, "list", "List all channels");
    CmdArgFlag   do_info    (parser, "info", "Time-range info on channels");
    CmdArgString start_time (parser, "start", "<time>",
                             "Format: \"mm/dd/yyyy[ hh:mm:ss[.nano-secs]]\"");
    CmdArgString end_time   (parser, "end", "<time>", "(exclusive)");
    CmdArgFlag   status_text(parser, "text",
                             "Include text column for status/severity");
    CmdArgString pattern    (parser, "match", "<reg. exp.>",
                             "Channel name pattern");
#if 0
    CmdArgDouble interpol   (parser,
                             "interpolate", "<seconds>", "interpolate values");
    CmdArgFlag   GNUPlot    (parser,
                             "gnuplot", "generate GNUPlot output");
    CmdArgInt    reduce     (parser,
                             "reduce", "<# of buckets>", "reduce data to # buckets");
    CmdArgFlag   image      (parser,
                             "Gnuplot", "generate GNUPlot output for Image");
    CmdArgFlag   pipe       (parser,
                             "pipe", "run GNUPlot via pipe");
    CmdArgFlag   MLSheet    (parser,
                             "MLSheet", "generate spreadsheet for Matlab");
    CmdArgFlag   Matlab     (parser,
                             "Matlab", "generate Matlab command-file output");
    CmdArgString output     (parser,
                             "output", "<file>", "output to file");
#endif
    if (! parser.parse())
        return -1;
    if (parser.getArguments().size() < 1)
    {
        parser.usage();
        return -1;
    }
    verbose = be_verbose;

    // Start/end time
    epicsTime *start = 0, *end = 0;
    stdString txt;
	if (start_time.get().length() > 0)
    {
        start = new epicsTime;
        string2epicsTime(start_time.get(), *start);
        if (verbose)
            printf("Using start time %s\n", epicsTimeTxt(*start, txt));
    }
	if (end_time.get().length() > 0)
    {
        end = new epicsTime();
        string2epicsTime(end_time.get(), *end);
        if (verbose)
            printf("Using end time   %s\n", epicsTimeTxt(*end, txt));
    }
    // Index name
    stdString index_name = parser.getArgument(0);
    // Channel names
    stdVector<stdString> names;
    if (parser.getArguments().size() > 1)
    {
        if (! pattern.get().empty())
        {
            fputs("Pattern from '-m' switch is ignored\n"
                  "since a list of channels was also provided.\n", stderr);
        }
        // first argument was directory file name, skip that:
        for (size_t i=1; i<parser.getArguments().size(); ++i)
            names.push_back(parser.getArgument(i));
    }
    // Open index
    archiver_Index index;
    if (!index.open(index_name.c_str()))
    {
        fprintf(stderr, "Cannot open index '%s'\n",
                index_name.c_str());
        return -1;
    }
    if (verbose)
        printf("Opened index '%s'\n", index_name.c_str());
    
    if (names.size() == 0 && pattern.get().length() > 0)
        get_names_for_pattern(index, names, pattern);
    if (do_info)
        return list_channels(index, pattern, true);
    if (do_list)
        return list_channels(index, pattern, false);
    if (names.size() == 0)
        return 0;
    if (!dump_spreadsheet(index, names, start, end, status_text))
        return -1;
 
    return 0;
}
