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

#include "Engine.h"
#include "ArchiveException.h"
#include "EngineServer.h"
#include "HTTPServer.h"
#include "CGIDemangler.h"
#include "NetTools.h"
#include "cvtFast.h"
#include <list>
#include <vector>
#include <strstream>

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
    page.tableLine("Description", theEngine->getDescription().c_str(), 0);
    
    osiTime2string(theEngine->getStartTime(), s);
    page.tableLine("Started", s.c_str(), 0);

    page.tableLine("Archive ", theEngine->getDirectory().c_str(), 0);

    stdList<ChannelInfo *> *channels = theEngine->getChannels();
    if (channels)
    {
        cvtUlongToString(channels->size(), line);
        page.tableLine("Channels", line, 0);
    }
    else
        page.tableLine("Channels", "? not available ?", 0);
    
#ifdef SHOW_DIR
    char dir[100];
    getcwd (dir, sizeof dir);                 
    page.tableLine ("Directory ", dir, 0);
#endif

    osiTime2string(theEngine->getWriteTime (), s);
    page.tableLine("Last write check", s.c_str(), 0);

    page.tableLine("Currently writing",
                   (const char *)
                   (theEngine->isWriting() ? "Yes" : "No"), 0);

    sprintf(line, "%f sec", theEngine->getDefaultPeriod());
    page.tableLine("Default Period", line, 0);

    sprintf(line, "%f sec", theEngine->getWritePeriod());
    page.tableLine("Write Period", line, 0);

    sprintf(line, "%f sec", theEngine->getGetThreshold());
    page.tableLine("Get Threshold", line, 0);

    cvtUlongToString((unsigned long) connection->getTotalClientCount(), line);
    page.tableLine("Web Clients (total)", line, 0);

    cvtUlongToString((unsigned long) connection->getClientCount(), line);
    page.tableLine("Web Clients (current)", line, 0);

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
    LOG_MSG(line);

    HTMLPage page(s, "Archive Engine Stop");

#ifdef USE_PASSWD
    CGIDemangler args;
    args.parse (path.substr(6).c_str());
    stdString user = args.find ("USER");
    stdString pass = args.find ("PASS");

    if (! theEngine->checkUser (user, pass))
    {
        page.line ("<H3><FONT COLOR=#FF0000>Wrong user/password</FONT></H3>");
        LOG_MSG ("USER: '" << user << "', PASS: '" << pass << "' - wrong user/password\n");
        showStopForm (page);
        return;
    }
    LOG_MSG ("user/password accepted\n");
#endif

    page.line ("<H3>Engine Stopped</H3>");

    page.line (line);
    page.line ("<P>");
    page.line ("Engine will quit as soon as possible...");
    page.line ("<P>");
    page.line ("Therefore the web interface stops responding now.");

    extern bool run;
    run = false;
}

static void config(HTTPClientConnection *connection, const stdString &path)
{
    HTMLPage page(connection->getSocket(), "Archive Engine Config.");

#ifdef USE_PASSWD
    showStopForm(page);
#endif

    page.line("<H3>Groups</H3>");
    page.line("<UL>");
    page.line("<LI><A HREF=\"/groups\">List groups</A><br>");
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
}

static void channels(HTTPClientConnection *connection, const stdString &path)
{
    HTMLPage page(connection->getSocket(), "Channels");

    stdList<ChannelInfo *> *channels = theEngine->getChannels();
    if (!channels  ||  channels->empty())
    {
        page.line("<I>no channels</I>");
        return;
    }

    page.openTable(1, "Name", 1, "Status", 0);
    stdList<ChannelInfo *>::iterator channel;
    stdString link;
    link.reserve(80);
    for (channel=channels->begin(); channel!=channels->end(); ++channel)
    {
        link = "<A HREF=\"channel/";
        link += (*channel)->getName();
        link += "\">";
        link += (*channel)->getName();
        link += "</A>";
        page.tableLine(link.c_str(),
                       ((*channel)->isConnected () ?
                        "connected" : "<FONT COLOR=#FF0000>not conn.</FONT>"),
                       0);
    }
    page.closeTable();
}

static void channelInfoTable(HTMLPage &page)
{
    page.openTable (1, "Name", 1, "Status", 1, "CA State", 1, "Period [s]",
        1, "Buffer", 1, "Get Mechanism", 1, "Disabling", 0);
}

static void channelInfoLine (HTMLPage &page, const ChannelInfo *channel)
{
    stdString status, stamp, ca_state;

    status.reserve(150);
    if (channel->isDisabled ())
        status = "disabled";
    else if (channel->isMonitored ())
        status = "monitored";
    else status = "scanned";
    status += "<BR>last: ";
    osiTime2string (channel->getLastArchiveStamp(), stamp);
    status += stamp;
    status += "<BR>next: ";
    osiTime2string (channel->getExpectedNextTime(), stamp);
    status += stamp;
    
    ca_state.reserve(150);
    if (channel->isConnected ())
        ca_state = "connected";
    else
        ca_state = "<FONT COLOR=#FF0000>NOT CONNECTED</FONT>";
    ca_state += "<BR>(";
    osiTime2string (channel->getConnectTime(), stamp);
    ca_state += stamp;
    ca_state += ")";
    ca_state += "<BR>";
    ca_state += channel->getHost ();

    char period[50], bufsize[50];
    sprintf(period, "%f", (double)channel->getPeriod());
    sprintf(bufsize, "%d", (int)channel->getValsPerBuffer());

    const char *get_mechanism;
    switch (channel->getMechanism ())
    {
    case ChannelInfo::use_monitor:  get_mechanism = "monitor"; break;
    case ChannelInfo::use_get:      get_mechanism = "get"; break;
    default:                        get_mechanism = "none";
    }

    std::strstream disabling;
    const BitSet &da_bits = channel->getDisabling();
    if (da_bits.empty ())
        disabling << "-";
    else
    {
        bool empty = true;
        for (size_t i=0; i<da_bits.size(); ++i)
        {
            if (da_bits[i])
            {
                if (empty)
                {
                    disabling << i;
                    empty = false;
                }
                else
                    disabling << ", " << i;
            }
        }
    }
    disabling << '\0';

    stdString channel_link; // link to group list for this channel
    channel_link = "<A HREF=\"/channelgroups?CHANNEL=";
    channel_link += channel->getName();
    channel_link += "\">";
    channel_link += channel->getName();
    channel_link += "</A>";
    page.tableLine (
        channel_link.c_str(),
        status.c_str(),
        ca_state.c_str(),
        period,
        bufsize,
        get_mechanism,
        disabling.str(),
        0);
    disabling.rdbuf()->freeze (false);
}

static void channelInfo (HTTPClientConnection *connection, const stdString &path)
{
    stdString channel_name = path.substr(9);

    ChannelInfo *channel = theEngine->findChannel (channel_name);
    if (! channel)
    {
        connection->error ("No such channel: " + channel_name);
        return;
    }

    HTMLPage page (connection->getSocket(), "Channel Info", 30);

    channelInfoTable (page);
    channelInfoLine (page, channel);
    page.closeTable ();
}

void groups(HTTPClientConnection *connection, const stdString &path)
{
    HTMLPage page(connection->getSocket(), "Groups");
    const stdList<GroupInfo *> &group_list = theEngine->getGroups();
    if (group_list.empty())
    {
        page.line ("<I>no groups</I>");
        return;
    }

    stdList<GroupInfo *>::const_iterator group;
    size_t  channel_count, connect_count;
    size_t  total_channel_count=0, total_connect_count=0;
    stdString name;
    name.reserve(80);
    page.openTable (1, "Name", 1, "ID", 1, "Enabled", 1, "Channels",
                    1, "Connected", 0);
    for (group=group_list.begin(); group!=group_list.end(); ++group)
    {
        name = "<A HREF=\"group/";
        name += (*group)->getName ();
        name += "\">";
        name += (*group)->getName ();
        name += "</A>";
        std::strstream id, channels, connected;
        id << (*group)->getID() << '\0';
        channel_count = (*group)->getChannels().size();
        connect_count = (*group)->getConnectedChannels ();
        total_channel_count += channel_count;
        total_connect_count += connect_count;
        channels << channel_count << '\0';
        if (channel_count != connect_count)
            connected << "<FONT COLOR=#FF0000>"
                      << connect_count << "</FONT>" << '\0';
        else
            connected << connect_count << '\0';
        page.tableLine (name.c_str(),
                        id.str(),
                        ((*group)->isEnabled() ?
                         "Yes" : "<FONT COLOR=#FF0000>No</FONT>"),
                        channels.str(),
                        connected.str(),
                        0);
        id.rdbuf()->freeze(false);
        channels.rdbuf()->freeze(false);
        connected.rdbuf()->freeze(false);
    }

    std::strstream total_channels, total_connected;
    total_channels << total_channel_count << '\0';
    if (total_channel_count != total_connect_count)
        total_connected << "<FONT COLOR=#FF0000>"
                        << total_connect_count << "</FONT>" << '\0';
    else
        total_connected << total_connect_count << '\0';
    page.tableLine("Total", " ", " ",
                   total_channels.str(),
                   total_connected.str(), 0);
    total_channels.rdbuf()->freeze(false);
    total_connected.rdbuf()->freeze(false);
        
    page.closeTable ();
}

static void groupInfo (HTTPClientConnection *connection, const stdString &path)
{
    const stdString group_name = path.substr(7);
    const GroupInfo *group = theEngine->findGroup (group_name);
    if (! group)
    {
        connection->error ("No such group: " + group_name);
        return;
    }

    HTMLPage page (connection->getSocket(), "Group Info");

    std::strstream id;
    id << group->getID() << '\0';
    page.openTable (2, "Group", 0);
    page.tableLine ("Name", group_name.c_str(), 0);
    page.tableLine ("ID", id.str(), 0);
    page.closeTable ();
    id.rdbuf()->freeze (false);

    const stdList<ChannelInfo *>& channels = group->getChannels ();
    if (channels.empty())
    {
        page.line ("no channels");
        return;
    }

    page.line ("<P>");
    page.line ("<H2>Channels:</H2>");

    channelInfoTable (page);
    stdList<ChannelInfo *>::const_iterator channel;
    for (channel = channels.begin(); channel != channels.end(); ++channel)
        channelInfoLine (page, *channel);
    page.closeTable ();
}

static void addChannel (HTTPClientConnection *connection, const stdString &path)
{
    CGIDemangler args;
    args.parse (path.substr(12).c_str());
    stdString channel_name = args.find ("CHANNEL");
    stdString group_name   = args.find ("GROUP");


    if (channel_name.empty() || group_name.empty())
    {
        connection->error ("Channel and group names must not be empty");
        return;
    }
    GroupInfo *group = theEngine->findGroup (group_name);
    if (!group)
    {
        stdString msg = "Cannot find group " + group_name;
        connection->error (msg);
        return;
    }

    double period = atof (args.find ("PERIOD").c_str());
    if (period <= 0)
        period = theEngine->getDefaultPeriod ();

    bool monitored = false;
    if (atoi (args.find ("MONITOR").c_str()) > 0)
        monitored = true;

    bool disabling = false;
    if (atoi (args.find ("DISABLE").c_str()) > 0)
        disabling = true;

    HTMLPage page (connection->getSocket(), "Add Channel");
    page.out ("Channel <I>");
    page.out (channel_name);
    if (theEngine->addChannel (group, channel_name, period, disabling, monitored))
        page.line ("</I> was added to");
    else
        page.line ("</I> could not be added");
    page.out (" to group <I>");
    page.out (group_name);
    page.line ("</I>.");
}

static void addGroup (HTTPClientConnection *connection, const stdString &path)
{
    CGIDemangler args;
    args.parse (path.substr(10).c_str());
    stdString group_name   = args.find ("GROUP");

    if (group_name.empty())
    {
        connection->error ("Group name must not be empty");
        return;
    }
    HTMLPage page (connection->getSocket(), "Archiver Engine");
    page.line ("<H1>Groups</H1>");
    page.out ("Group <I>");
    page.out (group_name);
    if (theEngine->addGroup (group_name))
        page.line ("</I> was added to the engine.");
    else
        page.line ("</I> could not be added to the engine.");
}

static void parseGroup (HTTPClientConnection *connection, const stdString &path)
{
    CGIDemangler args;
    args.parse (path.substr(12).c_str());
    stdString group_name   = args.find ("GROUP");

    if (group_name.empty())
    {
        connection->error ("Group file name must not be empty");
        return;
    }
    HTMLPage page (connection->getSocket(), "Archiver Engine");
    page.line ("<H1>Groups</H1>");
    page.out ("Group <I>");
    page.out (group_name);

    Configuration *cfg = theEngine->getConfiguration ();
    if (cfg->loadGroup (group_name))
    {
        cfg->save ();
        page.line ("</I> was added to / reloaded into the engine.");
    }
    else
        page.line ("</I> could not be added to the engine.<P>");
}

static void channelGroups (HTTPClientConnection *connection,
                           const stdString &path)
{
    CGIDemangler args;
    args.parse(path.substr(15).c_str());
    stdString channel_name = args.find("CHANNEL");

    ChannelInfo *channel = theEngine->findChannel(channel_name);
    if (! channel)
    {
        connection->error("No such channel: " + channel_name);
        return;
    }

    HTMLPage page(connection->getSocket(), "Archiver Engine");
    page.out("<H1>Group membership for channel ");
    page.out(channel_name);
    page.line("</H1>");

    const stdList<GroupInfo *> group_list = channel->getGroups();
    if (group_list.empty())
    {
        page.line("This channel does not belong to any groups.");
        return;
    }

    page.openTable(2, "Group", 0);
    stdList<GroupInfo *>::const_iterator group;
    stdString link;
    link.reserve(80);
    for (group=group_list.begin(); group!=group_list.end(); ++group)
    {
        link = "<A HREF=\"/group/";
        link += (*group)->getName();
        link += "\">";
        link += (*group)->getName();
        link += "</A>";
        page.tableLine (link.c_str(), 0);
    }
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

EngineServer::EngineServer ()
{
    HTTPClientConnection::setPathHandlers (handlers);
    _server = HTTPServer::create (_port);
    if (_server)
    {
        LOG_MSG ("Launched EngineServer on port " << _port << "\n");
    }
    else
    {
        LOG_MSG ("Cannot create EngineServer on port " << _port << "\n");
        throwDetailedArchiveException (Fail, "HTTPServer::create failed");
    }
}

EngineServer::~EngineServer ()
{
    delete _server;
}


