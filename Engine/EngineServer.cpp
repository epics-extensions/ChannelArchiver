// Base
#include <cvtFast.h>
// Tools
#include <CGIDemangler.h>
#include <NetTools.h>
#include <epicsTimeHelper.h>
#include <MsgLogger.h>
// Engine
#include "Engine.h"
#include "EngineConfig.h"
#include "EngineServer.h"
#include "HTTPServer.h"

// Excluded because the directory could
// be put in the "Description" field if you care.
#ifdef SHOW_DIR
// getcwd
#ifdef WIN32
#include <direct.h>
#include <process.h>
#else
#include <unistd.h>
#endif
#endif

static void engineinfo(HTTPClientConnection *connection, const stdString &path,
                       void *user_arg)
{
    Engine *engine = (Engine *)user_arg;
    HTMLPage page(connection->getSocket(), "Archive Engine");
    stdString s;
    char line[100];

    page.openTable(2, "Archive Engine Info", 0);
    page.tableLine("Version", ARCH_VERSION_TXT
                   ", built " __DATE__ ", " __TIME__, 0);
    const char *desc, *index;
    size_t num_channels, num_connected;
    epicsTime startTime, nextWrite;
    double proc_dly, write_duration, write_period, get_threshhold;
    unsigned long write_count;
    bool is_writing, disconn;
    {
        
        Guard engine_guard(*engine);
        desc = engine->getDescription(engine_guard).c_str();
        startTime = engine->getStartTime(engine_guard);
        index = engine->getIndexName(engine_guard).c_str();
        num_channels = engine->getChannels(engine_guard).size();
        num_connected = engine->getNumConnected(engine_guard);
        proc_dly = engine->getProcessDelayAvg(engine_guard);
        write_count = engine->getWriteCount(engine_guard);
        write_duration = engine->getWriteDuration(engine_guard);
        nextWrite = engine->getNextWriteTime(engine_guard);
        is_writing = engine->isWriting(engine_guard);
        write_period = engine->getConfig(engine_guard).getWritePeriod();
        get_threshhold = engine->getConfig(engine_guard).getGetThreshold();
        disconn = engine->getConfig(engine_guard).getDisconnectOnDisable();
    }
        
    page.tableLine("Description", desc, 0);
        
    epicsTime2string(startTime, s);
    page.tableLine("Started", s.c_str(), 0);
        
    page.tableLine("Archive Index", index, 0);

    cvtUlongToString(num_channels, line);
    page.tableLine("Channels", line, 0);

    if (num_channels != num_connected)
        sprintf(line,"<FONT COLOR=#FF0000>%d</FONT>",(int)num_connected);
    else
        cvtUlongToString(num_connected, line);
        page.tableLine("Connected", line, 0);
        
#ifdef SHOW_DIR
    getcwd(dir, sizeof line);                 
    page.tableLine("Directory ", line, 0);
#endif
    sprintf(line, "%.3f sec", proc_dly);
    page.tableLine("Avg. Proc. Delay", line, 0);

    sprintf(line, "%lu", write_count);
    page.tableLine("Write Count", line, 0);

    sprintf(line, "%.3f sec", write_duration);
    page.tableLine("Write Duration", line, 0);

    epicsTime2string(nextWrite, s);
    page.tableLine("Next write time", s.c_str(), 0);
        
    page.tableLine("Currently writing",
                   (const char *) (is_writing ? "Yes" : "No"), 0);
        
    sprintf(line, "%.1f sec", write_period);
    page.tableLine("Write Period", line, 0);

    sprintf(line, "%.1f sec", get_threshhold);
    page.tableLine("Get Threshold", line, 0);

    page.tableLine("Disconn. on disable", (disconn ? "Yes" : "No"), 0);
    page.closeTable();
}

#ifdef USE_PASSWD
static void showStopForm(HTMLPage &page)
{
    page.line("<H3>Stop Engine</H3>");
    page.line("<FORM METHOD=\"GET\" ACTION=\"/stop\">");
    page.line("<TABLE>");
    page.line("<TR><TD>User Name:</TD>");
    page.line("<TD><input type=\"text\" name=\"USER\" size=20></TD></TR>");
    page.line("<TR><TD>Password:</TD>");
    page.line("<TD><input type=\"text\" name=\"PASS\" size=20></TD></TR>");
    page.line("<TR><TD></TD>"
              "<TD><input TYPE=\"submit\" VALUE=\"Stop!\"></TD></TR>");
    page.line("</TABLE>");
    page.line("</FORM>");
}
#endif

static void stop(HTTPClientConnection *connection, const stdString &path,
                 void *user_arg)
{
    SOCKET s = connection->getSocket();
    HTMLPage page(s, "Archive Engine Stop");
    try
    {
        stdString line, peer;
        GetSocketPeer(s, peer);
        line = "Shutdown initiated via HTTP from ";
        line += peer;
        line += "\n";
        LOG_MSG(line.c_str());
#ifdef USE_PASSWD
        CGIDemangler args;
        args.parse(path.substr(6).c_str());
        stdString user = args.find("USER");
        stdString pass = args.find("PASS");
        if (! engine->checkUser(user, pass))
        {
            page.line("<H3><FONT COLOR=#FF0000>"
                      "Wrong user/password</FONT></H3>");
            LOG_MSG("USER: '%s', PASS: '%s' - wrong user/password\n",
                    user.c_str(), pass.c_str());
            showStopForm(page);
            return;
        }
        LOG_MSG("user/password accepted\n");
#endif
        page.line("<H3>Engine Stopped</H3>");
        page.line(line);
        page.line("<P>");
        page.line("Engine will quit as soon as possible...");
        page.line("<P>");
        page.line("Therefore the web interface stops responding now.");
        extern bool run_main_loop;
        run_main_loop = false;
    }
    catch (GenericException &e)
    {
         page.line("<H3>Error</H3>");
         page.line("<PRE>");
         page.line(e.what());
         page.line("</PRE>");
    }
}

static void config(HTTPClientConnection *connection, const stdString &path,
                   void *user_arg)
{
    HTMLPage page(connection->getSocket(), "Archive Engine Config.");
    if (HTMLPage::with_config == false)
    {
        page.line("Online Config is disabled for this ArchiveEngine!");
        return;
    }
#ifdef USE_PASSWD
    showStopForm(page);
#endif
    page.line("<H3>Groups</H3>");
    page.line("<UL>");
    page.line("<LI><A HREF=\"/groups\">List groups</A>");
    page.line("<LI><FORM METHOD=\"GET\" ACTION=\"/addgroup\">");
    page.line("    Group Name:");
    page.line("    <input type=\"text\" name=\"GROUP\" size=20>");
    page.line("    <input TYPE=\"submit\" VALUE=\"Add Group\">");
    page.line("    </FORM>");
    page.line("<LI><FORM METHOD=\"GET\" ACTION=\"/parseconfig\">");
    page.line("    Config File:");
    page.line("    <input type=\"text\" name=\"CONFIG\" size=20>");
    page.line("    <input TYPE=\"submit\" VALUE=\"Read\">");
    page.line("    </FORM>");
    page.line("</UL>");

    page.line("<H3>Channels</H3>");
    page.line("<UL>");
    page.line("<LI><A HREF=\"/channels\">List channels</A><br>");
    page.line("<LI>Add a new Channel<BR>");
    page.line("    <FORM METHOD=\"GET\" ACTION=\"/addchannel\">");
    page.line("    <TABLE>");
    page.line("    <TR><TD>Group Name:</TD>");
    page.line("        <TD><input type=\"text\" name=\"GROUP\" size=20></TD></TR>");
    page.line("    <TR><TD>Channel Name:</TD>");
    page.line("        <TD><input type=\"text\" name=\"CHANNEL\" size=20></TD></TR>");
    page.line("    <TR><TD>Period:</TD>");
    page.line("        <TD><input type=\"text\" name=\"PERIOD\" size=20></TD></TR>");
    page.line("    <TR><TD>Monitored:<input type=\"checkbox\" name=\"MONITOR\" value=1></TD>");
    page.line("        <TD></TD></TR>");
    page.line("    <TR><TD>Disabling:<input type=\"checkbox\" name=\"DISABLE\" value=1></TD>");
    page.line("        <TD><input TYPE=\"submit\" VALUE=\"Add Channel\"></TD></TR>");
    page.line("    </TABLE>");
    page.line("    </FORM>");
    page.line("<LI>Find group membership for channel<BR>");
    page.line("    <FORM METHOD=\"GET\" ACTION=\"/channelgroups\">");
    page.line("    <TABLE>");
    page.line("    <TR><TD>Channel:</TD>");
    page.line("        <TD><input type=\"text\" name=\"CHANNEL\" size=20>");
    page.line("            <input TYPE=\"submit\" VALUE=\"Find\"></TD></TR>");
    page.line("    </TABLE>");
    page.line("    </FORM>");
    page.line("</UL>");
#   if defined(HTTPD_DEBUG) && HTTPD_DEBUG > 4
    LOG_MSG("EngineServer::config printed to socket %d\n",
            connection->getSocket());
#   endif
}

static void channels(HTTPClientConnection *connection, const stdString &path,
                     void *user_arg)
{
    Engine *engine = (Engine *)user_arg;
    HTMLPage page(connection->getSocket(), "Channels");
    page.openTable(1, "Name", 1, "Status", 0);
    stdList<ArchiveChannel *>::const_iterator channel;
    stdString link;
    link.reserve(80);
    Guard engine_guard(*engine);
    const stdList<ArchiveChannel *> &channels = engine->getChannels(engine_guard);
    for (channel = channels.begin(); channel != channels.end(); ++channel)
    {
        link = "<A HREF=\"channel/";
        link += (*channel)->getName();
        link += "\">";
        link += (*channel)->getName();
        link += "</A>";
        Guard guard((*channel)->getMutex());
        page.tableLine(link.c_str(),
                       ((*channel)->isConnected(guard) ?
                        "connected" :
                        "<FONT COLOR=#FF0000>disconnected</FONT>"),
                       0);
    }
    page.closeTable();
}

static void channelInfoTable(HTMLPage &page)
{
    page.openTable(1, "Name",
                   1, "State",
                   1, "Mechanism",
                   1, "Disabling",
                   1, "State",
                   0);
}

static void channelInfoLine(HTMLPage &page, ArchiveChannel *channel)
{
    stdString channel_link; // link to group list for this channel
    channel_link = "<A HREF=\"/channelgroups?CHANNEL=";
    channel_link += channel->getName();
    channel_link += "\">";
    channel_link += channel->getName();
    channel_link += "</A>";
    
    Guard guard(*channel);
    const stdList<class GroupInfo *> groups = channel->getGroupsToDisable(guard);
    stdList<class GroupInfo *>::const_iterator group;
    bool at_least_one = false;
    stdString disabling;
    for (group = groups.begin();  group != groups.end();  ++group)
    {
        if (at_least_one)
            disabling += ", ";
        disabling += (*group)->getName();
    }

    page.tableLine(
        channel_link.c_str(),
        (channel->isConnected(guard) ? 
         "connected" :
         "<FONT COLOR=#FF0000>NOT CONNECTED</FONT>"),
        channel->getSampleInfo(guard).c_str(),
        disabling.c_str(),
        (channel->isDisabled(guard) ?
         "<FONT COLOR=#FFFF00>disabled</FONT>" :
         "enabled"),
        0);
}

static void channelInfo(HTTPClientConnection *connection,
                        const stdString &path, void *user_arg)
{
    Engine *engine = (Engine *)user_arg;
    stdString channel_name = path.substr(9);
    Guard engine_guard(*engine);
    ArchiveChannel *channel
        = engine->findChannel(engine_guard, channel_name);
    if (! channel)
    {
        connection->error("No such channel: " + channel_name);
        return;
    }
    HTMLPage page(connection->getSocket(), "Channel Info", 30);
    channelInfoTable(page);
    channelInfoLine(page, channel);
    page.closeTable();
}

void groups(HTTPClientConnection *connection, const stdString &path,
            void *user_arg)
{
    Engine *engine = (Engine *)user_arg;
    HTMLPage page(connection->getSocket(), "Groups");
    char channels[50], connected[100];
    size_t  total_channel_count=0, total_connect_count=0;      
    {
        Guard engine_guard(*engine);
        const stdList<GroupInfo *> &groups = engine->getGroups(engine_guard);
        if (groups.empty())
        {
            page.line("<I>no groups</I>");
            return;
        }
        stdList<GroupInfo *>::const_iterator group;
        size_t  channel_count, connect_count;
        stdString name;
        name.reserve(80);
        page.openTable(1, "Name", 1, "Enabled", 1, "Channels",
                       1, "Connected", 0);
        for (group=groups.begin(); group!=groups.end(); ++group)
        {
            name = "<A HREF=\"group/";
            name += (*group)->getName();
            name += "\">";
            name += (*group)->getName();
            name += "</A>";
            Guard group_guard((*group)->getMutex());
            channel_count = (*group)->getChannels(group_guard).size();
            connect_count = (*group)->getNumConnected(group_guard);
            total_channel_count += channel_count;
            total_connect_count += connect_count;
            cvtUlongToString((unsigned long) channel_count, channels);
            if (channel_count != connect_count)
                sprintf(connected, "<FONT COLOR=#FF0000>%u</FONT>",
                        (unsigned int)connect_count);
            else
                cvtUlongToString((unsigned long)connect_count, connected);
            
            page.tableLine(name.c_str(),
                            ((*group)->isEnabled() ?
                             "Yes" : "<FONT COLOR=#FF0000>No</FONT>"),
                            channels, connected, 0);
        }
    } 
    // Engine Unlocked   
    sprintf(channels, "%u", (unsigned int)total_channel_count);
    if (total_channel_count != total_connect_count)
        sprintf(connected, "<FONT COLOR=#FF0000>%u</FONT>",
                (unsigned int)total_connect_count);
    else
        cvtUlongToString((unsigned long) total_connect_count, connected);
    page.tableLine("Total", " ", channels, connected, 0);
    page.closeTable();
}

static void groupInfo(HTTPClientConnection *connection, const stdString &path,
                      void *user_arg)
{
    Engine *engine = (Engine *)user_arg;
    HTMLPage page(connection->getSocket(), "Group Info");
    try
    {
        stdString group_name = path.substr(7);
        CGIDemangler::unescape(group_name);
        Guard engine_guard(*engine);
        GroupInfo *group = engine->findGroup(engine_guard, group_name);
        if (! group)
        {
            page.line("No such group: ");
            page.line(group_name);
            return;
        }
        page.openTable(2, "Group", 0);
        page.tableLine("Name", group_name.c_str(), 0);
        page.closeTable();
        if (engine->getChannels(engine_guard).empty())
        {
            page.line("no channels");
            return;
        }
        page.line("<P>");
        page.line("<H2>Channels</H2>");
        channelInfoTable(page);
        {
            Guard group_guard(*group);
            const stdList<ArchiveChannel *> group_channels
                = group->getChannels(group_guard);
            stdList<ArchiveChannel *>::const_iterator channel;
            for (channel = group_channels.begin();
                 channel != group_channels.end(); ++channel)
                channelInfoLine(page, *channel);
            page.closeTable();
        }
    }
    catch (GenericException &e)
    {
         page.line("<H3>Error</H3>");
         page.line("<PRE>");
         page.line(e.what());
         page.line("</PRE>");
    }
}

static void addChannel(HTTPClientConnection *connection,
                       const stdString &path, void *user_arg)
{
    Engine *engine = (Engine *)user_arg;
    HTMLPage page(connection->getSocket(), "Add Channel");


         page.line("<H3>Error</H3>");
         page.line("<PRE>");
         page.line("Not, yet.");
         page.line("</PRE>");

#if 0
    try
    {
        CGIDemangler args;
        args.parse(path.substr(12).c_str());
        stdString channel_name = args.find("CHANNEL");
        stdString group_name   = args.find("GROUP");
        if (channel_name.empty() || group_name.empty())
        {
            page.line("Channel and group names must not be empty");
            return;
        }
        Guard engine_guard(engine->mutex);
        GroupInfo *group = engine->findGroup(engine_guard, group_name);
        if (!group)
        {
            stdString msg = "Cannot find group " + group_name;
            page.line(msg.c_str());
            return;
        }
        double period = atof(args.find("PERIOD").c_str());
        if (period <= 0)
            period = 1.0;
        bool monitored = false;
        if (atoi(args.find("MONITOR").c_str()) > 0)
            monitored = true;
        bool disabling = false;
        if (atoi(args.find("DISABLE").c_str()) > 0)
            disabling = true;
        page.out("Channel <I>");
        page.out(channel_name);
        engine->attachToCAContext(engine_guard);
        if (engine->addChannel(engine_guard, group, channel_name, period,
                                  disabling, monitored))
        {
            page.line("</I> was added");
            EngineConfig config;
            config.write(engine_guard, engine);
        }
        else
            page.line("</I> could not be added");
        page.out(" to group <I>");
        page.out(group_name);
        page.line("</I>.");
    }
    catch (GenericException &e)
    {
         page.line("<H3>Error</H3>");
         page.line("<PRE>");
         page.line(e.what());
         page.line("</PRE>");
    }
#endif
}

static void addGroup(HTTPClientConnection *connection, const stdString &path,
                     void *user_arg)
{
    Engine *engine = (Engine *)user_arg;
    HTMLPage page(connection->getSocket(), "Archiver Engine");

         page.line("<H3>Error</H3>");
         page.line("<PRE>");
         page.line("Not, yet.");
         page.line("</PRE>");

#if 0
    try
    {
        CGIDemangler args;
        args.parse(path.substr(10).c_str());
        stdString group_name   = args.find("GROUP");
        if (group_name.empty())
        {
            page.line("Group name must not be empty");
            return;
        }
        page.line("<H1>Groups</H1>");
        page.out("Group <I>");
        page.out(group_name);
        Guard engine_guard(engine->mutex);
        if (engine->addGroup(engine_guard, group_name))
        {
            page.line("</I> was added to the engine.");
            EngineConfig config;
            config.write(engine_guard, engine);
        }
        else
            page.line("</I> could not be added to the engine.");
    }
    catch (GenericException &e)
    {
         page.line("<H3>Error</H3>");
         page.line("<PRE>");
         page.line(e.what());
         page.line("</PRE>");
    }
#endif
}

    
static void parseConfig(HTTPClientConnection *connection,
                        const stdString &path, void *user_arg)
{
    Engine *engine = (Engine *)user_arg;
    HTMLPage page(connection->getSocket(), "Archiver Engine");

         page.line("<H3>Error</H3>");
         page.line("<PRE>");
         page.line("Not, yet.");
         page.line("</PRE>");
#if 0
   try
    {
        CGIDemangler args;
        args.parse(path.substr(13).c_str());
        stdString config_name   = args.find("CONFIG");
        if (config_name.empty())
        {
            page.line("Config. name must not be empty");
            return;
        }
        page.line("<H1>Configuration</H1>");
        page.out("Configuration <I>");
        page.out(config_name);
        Guard engine_guard(engine->mutex);
        engine->attachToCAContext(engine_guard);
        EngineConfig config;
        if (config.read(engine_guard, engine, config_name))
        {
            page.line("</I> was loaded.");
            EngineConfig config;
            config.write(engine_guard, engine);
        }
        else
            page.line("</I> could not be loaded.<P>");
    }
    catch (GenericException &e)
    {
         page.line("<H3>Error</H3>");
         page.line("<PRE>");
         page.line(e.what());
         page.line("</PRE>");
    }
#endif
}

static void channelGroups(HTTPClientConnection *connection,
                          const stdString &path, void *user_arg)
{
    Engine *engine = (Engine *)user_arg;
    HTMLPage page(connection->getSocket(), "Archiver Engine");
    try
    {
        CGIDemangler args;
        args.parse(path.substr(15).c_str());
        stdString channel_name = args.find("CHANNEL");
        Guard engine_guard(*engine);
        ArchiveChannel *channel =
            engine->findChannel(engine_guard, channel_name);
        if (! channel)
        {
            page.line("No such channel: " + channel_name);
            return;
        }
        page.out("<H2>Channel '");
        page.out(channel_name);
        page.line("'</H2>");
        
        Guard guard(*channel);
        page.out("<H2>Group membership</H2>");
        page.openTable(1, "Group", 1, "Enabled", 0);
        stdList<GroupInfo *>::const_iterator group;
        stdString link;
        link.reserve(80);
        const stdList<class GroupInfo *> &groups = channel->getGroups(guard);
        for (group=groups.begin(); group!=groups.end(); ++group)
        {
            link = "<A HREF=\"/group/";
            link += (*group)->getName();
            link += "\">";
            link += (*group)->getName();
            link += "</A>";
            page.tableLine(link.c_str(),
                           ((*group)->isEnabled() ?
                            "Yes" : "<FONT COLOR=#FF0000>No</FONT>"), 0);
        }
        page.closeTable();
    }
    catch (GenericException &e)
    {   
         page.line("<H3>Error</H3>");
         page.line("<PRE>");
         page.line(e.what());
         page.line("</PRE>");
    }
}

#if 0    
static void caStatus(HTTPClientConnection *connection,
                     const stdString &path, void *user_arg)
{   //   "castatus?name
    Engine *engine = (Engine *)user_arg;
    stdString name = path.substr(10);

    HTMLPage page(connection->getSocket(), "Archiver Engine");
    page.out("<H2>CA Client Status</H2>");
    page.out("Will create ");
    page.out(name.c_str());
    page.out(" at next write cycle.\n");
    engine->setInfoDumpFile(name);
}
#endif

static PathHandlerList engine_handlers[] =
{
    //  URL, substring length?, handler. The order matters!
#ifdef USE_PASSWD
    { "/stop?", 6, stop },
#endif
    { "/stop", 0, stop  },
    { "/help", 0, config    },
    { "/config", 0, config  },
    { "/channels", 0, channels  },
    { "/groups", 0, groups },
    { "/channel/", 9, channelInfo },
    { "/group/", 7, groupInfo },
    { "/addchannel?", 12, addChannel },
    { "/addgroup?", 10, addGroup },
    { "/parseconfig?", 12, parseConfig },
    { "/channelgroups?", 15, channelGroups },
//    { "/castatus?", 10, caStatus },
    { "/",  0, engineinfo },
    { 0,    0,  },
};

EngineServer::EngineServer(short port, Engine *engine)
  : HTTPServer(port, engine_handlers, engine)
{
#ifdef HTTPD_DEBUG
    LOG_MSG("Starting EngineServer\n");
#endif
    start();
}

EngineServer::~EngineServer()
{
#ifdef HTTPD_DEBUG
    LOG_MSG("EngineServer deleted.\n");
#endif
}

