// --------------------------------------------------------
// $Id$
//
// Please refer to NOTICE.txt,
// included as part of this distribution,
// for legal information.
//
// Kay-Uwe Kasemir, kasemir@lanl.gov
// --------------------------------------------------------

#ifdef WIN32
#pragma warning (disable: 4786)
#endif

#ifdef solaris
// silly clash with struct map on Solaris
// as long as namespaces are not used by egcs C++ library:
#define _NET_IF_H
#endif

#include <cvtFast.h>
#include <CGIDemangler.h>
#include <NetTools.h>
#include <epicsTimeHelper.h>
#include "Engine.h"
#include "ArchiveException.h"
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

static void engineinfo(HTTPClientConnection *connection,
                       const stdString &path)
{
    HTMLPage page(connection->getSocket(), "Archive Engine");
    stdString s;
    char line[100];

    page.openTable(2, "Engine Info", 0);
    page.tableLine("Name", "ArchiveEngine", 0);
    page.tableLine("Version", VERSION_TXT ", built " __DATE__, 0);

    if (theEngine)
    {
        theEngine->mutex.lock();
        
        page.tableLine("Description", theEngine->getDescription().c_str(), 0);
        
        epicsTime2string(theEngine->getStartTime(), s);
        page.tableLine("Started", s.c_str(), 0);
        
        page.tableLine("Archive ", theEngine->getDirectory().c_str(), 0);
        
        cvtUlongToString(theEngine->channels.size(), line);
        page.tableLine("Channels", line, 0);
        
#ifdef SHOW_DIR
        char dir[100];
        getcwd(dir, sizeof dir);                 
        page.tableLine("Directory ", dir, 0);
#endif
        
        epicsTime2string(theEngine->getNextWriteTime(), s);
        page.tableLine("Next write time", s.c_str(), 0);
        
        page.tableLine("Currently writing",
                       (const char *)
                       (theEngine->isWriting() ? "Yes" : "No"), 0);
        
        sprintf(line, "%f sec", theEngine->getDefaultPeriod());
        page.tableLine("Default Period", line, 0);
        
        sprintf(line, "%f sec", theEngine->getWritePeriod());
        page.tableLine("Write Period", line, 0);
        
        sprintf(line, "%f sec", theEngine->getGetThreshold());
        page.tableLine("Get Threshold", line, 0); 
        theEngine->mutex.unlock();
    }
    else
    {
        page.tableLine("No Engine runnig!?", 0); 
    }
    
    cvtUlongToString(
        (unsigned long) connection->getServer()->getTotalClientCount(), line);
    page.tableLine("Web Clients (total)", line, 0);
    
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
    page.line("<TR><TD></TD><TD><input TYPE=\"submit\" VALUE=\"Stop!\"></TD></TR>");
    page.line("</TABLE>");
    page.line("</FORM>");
}
#endif

static void stop(HTTPClientConnection *connection, const stdString &path)
{
    SOCKET s = connection->getSocket();
    stdString line, peer;
    GetSocketPeer(s, peer);
    line = "Shutdown initiated via HTTP from ";
    line += peer;
    line += "\n";
    LOG_MSG(line.c_str());

    HTMLPage page(s, "Archive Engine Stop");

#ifdef USE_PASSWD
    CGIDemangler args;
    args.parse(path.substr(6).c_str());
    stdString user = args.find("USER");
    stdString pass = args.find("PASS");

    if (! theEngine->checkUser(user, pass))
    {
        page.line("<H3><FONT COLOR=#FF0000>Wrong user/password</FONT></H3>");
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

    extern bool run;
    run = false;
}

static void config(HTTPClientConnection *connection, const stdString &path)
{
    HTMLPage page(connection->getSocket(), "Archive Engine Config.");

    if(HTMLPage::_nocfg) {
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
    page.line("<LI><FORM METHOD=\"GET\" ACTION=\"/parsegroup\">");
    page.line("    Parse Group File:");
    page.line("    <input type=\"text\" name=\"GROUP\" size=20>");
    page.line("    <input TYPE=\"submit\" VALUE=\"Parse\">");
    page.line("    </FORM>");
    page.line("</UL>");

    page.line("<H3>Channels</H3>");
    page.line("<UL>");
    page.line("<LI><A HREF=\"/channels\">List channels</A><br>");
    page.line("<LI><BR><FORM METHOD=\"GET\" ACTION=\"/addchannel\">");
    page.line("    <TABLE>");
    page.line("    <TR><TD>Group Name:</TD>");
    page.line("        <TD><input type=\"text\" name=\"GROUP\" size=20></TD></TR>");
    page.line("    <TR><TD>Channel Name:</TD>");
    page.line("        <TD><input type=\"text\" name=\"CHANNEL\" size=20></TD></TR>");
    page.line("    <TR><TD>Period:</TD>");
    page.line("        <TD><input type=\"text\" name=\"PERIOD\" size=20></TD></TR>");
    page.line("    <TR><TD>Monitored:<input type=\"checkbox\" name=\"MONITOR\" value=1>");
    page.line("        <TD></TD></TR>");
    page.line("    <TR><TD>Disabling:<input type=\"checkbox\" name=\"DISABLE\" value=1></TD>");
    page.line("        <TD><input TYPE=\"submit\" VALUE=\"Add Channel\"></TD></TR>");
    page.line("    </TABLE>");
    page.line("    </FORM>");
    page.line("<LI><BR><FORM METHOD=\"GET\" ACTION=\"/channelgroups\">");
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

static void channels(HTTPClientConnection *connection, const stdString &path)
{
    HTMLPage page(connection->getSocket(), "Channels");
    theEngine->mutex.lock();
    page.openTable(1, "Name", 1, "Status", 0);
    stdList<ArchiveChannel *>::const_iterator channel;
    stdString link;
    link.reserve(80);
    for (channel=theEngine->channels.begin(); channel != theEngine->channels.end(); ++channel)
    {
        link = "<A HREF=\"channel/";
        link += (*channel)->getName();
        link += "\">";
        link += (*channel)->getName();
        link += "</A>";
        page.tableLine(link.c_str(),
                       ((*channel)->isConnected() ?
                        "connected" : "<FONT COLOR=#FF0000>disconnected</FONT>"),
                       0);
    }
    theEngine->mutex.unlock();
    page.closeTable();
}

static void channelInfoTable(HTMLPage &page)
{
    page.openTable(1, "Name",
                   1, "CA State",
                   1, "Mechanism",
                   1, "Disabling",
                   1, "State",
                   0);
}

static void channelInfoLine(HTMLPage &page, const ArchiveChannel *channel)
{
    stdString channel_link; // link to group list for this channel
    channel_link = "<A HREF=\"/channelgroups?CHANNEL=";
    channel_link += channel->getName();
    channel_link += "\">";
    channel_link += channel->getName();
    channel_link += "</A>";
    
    stdString disabling;
    char num[10];
    const BitSet &bits = channel->getGroupsToDisable();
    if (bits.empty())
        disabling = "-";
    else
    {
        bool at_least_one = false;
        disabling.reserve(bits.capacity() * 4);
        for (size_t i=0; i<bits.capacity(); ++i)
            if (bits.test(i))
            {
                if (at_least_one)
                    disabling += ", ";
                sprintf(num, "%d", i);
                disabling += num;
            }
    }
    
    page.tableLine(
        channel_link.c_str(),
        (channel->isConnected() ? 
         "connected" :
         "<FONT COLOR=#FF0000>NOT CONNECTED</FONT>"),
        channel->getMechanism()->getDescription().c_str(),
        disabling.c_str(),
        (channel->isDisabled() ?
         "<FONT COLOR=#FFFF00>disabled</FONT>" :
         "enabled"),
        0);
}

static void channelInfo(HTTPClientConnection *connection,
                        const stdString &path)
{
    stdString channel_name = path.substr(9);

    theEngine->mutex.lock();
    ArchiveChannel *channel = theEngine->findChannel(channel_name);
    theEngine->mutex.unlock();
    if (! channel)
    {
        connection->error("No such channel: " + channel_name);
        return;
    }

    HTMLPage page(connection->getSocket(), "Channel Info", 30);

    channelInfoTable(page);
    channel->mutex.lock();
    channelInfoLine(page, channel);
    channel->mutex.unlock();
    page.closeTable();
}

void groups(HTTPClientConnection *connection, const stdString &path)
{
    HTMLPage page(connection->getSocket(), "Groups");
    theEngine->mutex.lock();
    if (theEngine->groups.empty())
    {
        theEngine->mutex.unlock();
        page.line("<I>no groups</I>");
        return;
    }

    stdList<GroupInfo *>::const_iterator group;
    size_t  channel_count, connect_count;
    size_t  total_channel_count=0, total_connect_count=0;
    char id[10], channels[50], connected[100];
    stdString name;
    name.reserve(80);
    page.openTable(1, "Name", 1, "ID", 1, "Enabled", 1, "Channels",
                   1, "Connected", 0);
    for (group=theEngine->groups.begin(); group!=theEngine->groups.end(); ++group)
    {
        name = "<A HREF=\"group/";
        name += (*group)->getName();
        name += "\">";
        name += (*group)->getName();
        name += "</A>";
        sprintf(id, "%d", (*group)->getID());
        channel_count = (*group)->getChannels().size();
        connect_count = (*group)->num_connected;
        total_channel_count += channel_count;
        total_connect_count += connect_count;
        sprintf(channels, "%d", channel_count);
        if (channel_count != connect_count)
            sprintf(connected, "<FONT COLOR=#FF0000>%d</FONT>", connect_count);
        else
            sprintf(connected, "%d", connect_count);
        
        page.tableLine(name.c_str(), id,
                        ((*group)->isEnabled() ?
                         "Yes" : "<FONT COLOR=#FF0000>No</FONT>"),
                        channels, connected, 0);
    }

    theEngine->mutex.unlock();
    
    sprintf(channels, "%d", total_channel_count);
    if (total_channel_count != total_connect_count)
        sprintf(connected, "<FONT COLOR=#FF0000>%d</FONT>",
                total_connect_count);
    else
        sprintf(connected, "%d", total_connect_count);
    page.tableLine("Total", " ", " ", channels, connected, 0);
    page.closeTable();
}

static void groupInfo(HTTPClientConnection *connection, const stdString &path)
{
    const stdString group_name = path.substr(7);
    theEngine->mutex.lock();
    const GroupInfo *group = theEngine->findGroup(group_name);
    if (! group)
    {
        theEngine->mutex.unlock();
        connection->error("No such group: " + group_name);
        return;
    }

    HTMLPage page(connection->getSocket(), "Group Info");

    char id[10];
    sprintf(id, "%d", group->getID());
    page.openTable(2, "Group", 0);
    page.tableLine("Name", group_name.c_str(), 0);
    page.tableLine("ID", id, 0);
    page.closeTable();

    if (theEngine->channels.empty())
    {
        theEngine->mutex.unlock();
        page.line("no channels");
        return;
    }

    page.line("<P>");
    page.line("<H2>Channels:</H2>");

    channelInfoTable(page);
    stdList<ArchiveChannel *>::const_iterator channel;
    for (channel = theEngine->channels.begin(); channel != theEngine->channels.end(); ++channel)
    {
        (*channel)->mutex.lock();
        channelInfoLine(page, *channel);
        (*channel)->mutex.unlock();
    }
    theEngine->mutex.unlock();
    page.closeTable();
}

static void addChannel(HTTPClientConnection *connection,
                        const stdString &path)
{
    CGIDemangler args;
    args.parse(path.substr(12).c_str());
    stdString channel_name = args.find("CHANNEL");
    stdString group_name   = args.find("GROUP");

    if (channel_name.empty() || group_name.empty())
    {
        connection->error("Channel and group names must not be empty");
        return;
    }
    theEngine->mutex.lock();
    GroupInfo *group = theEngine->findGroup(group_name);
    if (!group)
    {
        theEngine->mutex.unlock();
        stdString msg = "Cannot find group " + group_name;
        connection->error(msg);
        return;
    }

    double period = atof(args.find("PERIOD").c_str());
    if (period <= 0)
        period = theEngine->getDefaultPeriod();

    bool monitored = false;
    if (atoi(args.find("MONITOR").c_str()) > 0)
        monitored = true;

    bool disabling = false;
    if (atoi(args.find("DISABLE").c_str()) > 0)
        disabling = true;

    HTMLPage page(connection->getSocket(), "Add Channel");
    page.out("Channel <I>");
    page.out(channel_name);
    theEngine->attachToCAContext();
    if (theEngine->addChannel(group, channel_name, period,
                               disabling, monitored))
        page.line("</I> was added to");
    else
        page.line("</I> could not be added");
    theEngine->mutex.unlock();
    page.out(" to group <I>");
    page.out(group_name);
    page.line("</I>.");
}

static void addGroup(HTTPClientConnection *connection, const stdString &path)
{
    CGIDemangler args;
    args.parse(path.substr(10).c_str());
    stdString group_name   = args.find("GROUP");

    if (group_name.empty())
    {
        connection->error("Group name must not be empty");
        return;
    }
    HTMLPage page(connection->getSocket(), "Archiver Engine");
    page.line("<H1>Groups</H1>");
    page.out("Group <I>");
    page.out(group_name);
    theEngine->mutex.lock();
    if (theEngine->addGroup(group_name))
        page.line("</I> was added to the engine.");
    else
        page.line("</I> could not be added to the engine.");
    theEngine->mutex.unlock();
}

static void parseGroup(HTTPClientConnection *connection, const stdString &path)
{
    CGIDemangler args;
    args.parse(path.substr(12).c_str());
    stdString group_name   = args.find("GROUP");

    if (group_name.empty())
    {
        connection->error("Group file name must not be empty");
        return;
    }
    HTMLPage page(connection->getSocket(), "Archiver Engine");
    page.line("<H1>Groups</H1>");
    page.out("Group <I>");
    page.out(group_name);

    theEngine->mutex.lock();
    theEngine->attachToCAContext();
    Configuration *cfg = theEngine->getConfiguration();
    if (cfg->loadGroup(group_name))
    {
        cfg->save();
        page.line("</I> was added to / reloaded into the engine.");
    }
    else
        page.line("</I> could not be added to the engine.<P>");
    theEngine->mutex.unlock();
}

static void channelGroups(HTTPClientConnection *connection, const stdString &path)
{
    CGIDemangler args;
    args.parse(path.substr(15).c_str());
    stdString channel_name = args.find("CHANNEL");

    theEngine->mutex.lock();
    ArchiveChannel *channel = theEngine->findChannel(channel_name);
    if (! channel)
    {
        theEngine->mutex.unlock();
        connection->error("No such channel: " + channel_name);
        return;
    }

    HTMLPage page(connection->getSocket(), "Archiver Engine");
    page.out("<H1>Group membership for channel ");
    page.out(channel_name);
    page.line("</H1>");

    channel->mutex.lock();
    page.openTable(2, "Group", 0);
    stdList<GroupInfo *>::const_iterator group;
    stdString link;
    link.reserve(80);
    for (group=channel->groups.begin(); group!=channel->groups.end(); ++group)
    {
        link = "<A HREF=\"/group/";
        link += (*group)->getName();
        link += "\">";
        link += (*group)->getName();
        link += "</A>";
        page.tableLine(link.c_str(), 0);
    }
    channel->mutex.unlock();
    theEngine->mutex.unlock();
    page.closeTable();
}

static PathHandlerList  handlers[] =
{
#ifdef USE_PASSWD
    { "/stop?", 6, stop },
#endif
    { "/stop", 0, stop  }, // list "old" handler for users who still use it (will give error)
    { "/help", 0, config    },
    { "/config", 0, config  },
    { "/channels", 0, channels  },
    { "/groups", 0, groups },
    { "/channel/", 9, channelInfo },
    { "/group/", 7, groupInfo },
    { "/addchannel?", 12, addChannel },
    { "/addgroup?", 10, addGroup },
    { "/parsegroup?", 12, parseGroup },
    { "/channelgroups?", 15, channelGroups },
    { "/",  0, engineinfo },
    { 0,    0,  },
};

// static member:
short EngineServer::_port = 4812;
bool EngineServer::_nocfg = false;

EngineServer::EngineServer()
{
    HTTPClientConnection::setPathHandlers(handlers);
    _server = HTTPServer::create(_port);
    if (!_server)
    {
        LOG_MSG("Cannot create EngineServer on port %d\n", _port);
        throwDetailedArchiveException(Fail, "HTTPServer::create failed");
    }
#ifdef HTTPD_DEBUG
    LOG_MSG("EngineServer starting HTTPServer 0x%X\n", _server);
#endif
    _server->start();
}

EngineServer::~EngineServer()
{
    delete _server;
#ifdef HTTPD_DEBUG
    LOG_MSG("EngineServer deleted\n");
#endif
}

