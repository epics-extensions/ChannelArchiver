// --------------------------------------------------------
// $Id$
//
// Please refer to NOTICE.txt,
// included as part of this distribution,
// for legal information.
//
// Kay-Uwe Kasemir, kasemir@lanl.gov
// --------------------------------------------------------

// System
#include <fcntl.h>
#include <unistd.h>
// Base
#include <cadef.h>
// Tools
#include "ToolsConfig.h"
#include "AutoPtr.h"
#include "epicsTimeHelper.h"
#include "ArchiveException.h"
#include "MsgLogger.h"
// Storage
#include "IndexFile.h"
#include "DataFile.h"
// Engine
#include "Engine.h"
#include "EngineServer.h"

Engine *theEngine = 0;

static void caException(struct exception_handler_args args)
{
    const char *pName;
    
    if (args.chid)
        pName = ca_name(args.chid);
    else
        pName = "?";

    LOG_MSG("CA Exception %s - with request "
            "chan=%s op=%d type=%s count=%d:\n%s\n", 
            args.ctx, pName, (int)args.op, dbr_type_to_text(args.type),
            (int)args.count,
            ca_message(args.stat));
}

// --------------------------------------------------------------------------
// Engine
// --------------------------------------------------------------------------

Engine::Engine(const stdString &index_name, short port)
  : num_connected(0),
    ca_context(0),
    start_time(epicsTime::getCurrent()),
    RTreeM(50),
    index_name(index_name),
    description("EPICS Channel Archiver Engine"),
    is_writing(false),
    get_threshhold(20.0),
    disconnect_on_disable(false),
    write_period(30),
    buffer_reserve(3),
    process_delay_avg(0.0),
    write_duration(0.0),
    write_count(0),
    next_write_time(roundTimeUp(start_time, write_period)),
    future_secs(6*60*60)
#ifdef USE_PASSWD
    , 
    user(DEFAULT_USER),
    pass(DEFAULT_PASS)
#endif   
{
    // Initialize CA library for multi-treaded use and
    // add exception handler to avoid aborts from CA
    if (ca_context_create(ca_enable_preemptive_callback) != ECA_NORMAL ||
        ca_add_exception_event(caException, 0) != ECA_NORMAL)
        throw GenericException(__FILE__, __LINE__,
                               "CA client initialization failed.");
    ca_context = ca_current_context();
    engine_server = new EngineServer(port, this);
}

Engine::~Engine()
{
    LOG_MSG("Shutdown:\n");
    if (this != theEngine)
        LOG_MSG("CORRUPTION: Not theEngine\n");
    engine_server = 0;
    epicsTime now(epicsTime::getCurrent());
    LOG_MSG("Disconnecting Channels...\n");
    try
    {
        Guard engine_guard(mutex);
        stdList<ArchiveChannel *>::iterator ch;
        for (ch = channels.begin(); ch != channels.end(); ++ch)
        {
            ArchiveChannel *c = *ch;
            Guard guard(c->mutex);
            c->setMechanism(engine_guard, guard, 0, now);
            c->stopCA(engine_guard, guard);
        }
    }
    catch (GenericException &e)
    {
        LOG_MSG("Error while disconnecting:\n%s\n", e.what());
    }
    LOG_MSG("Adding 'Archive_Off' events...\n");
    try
    {
        Guard engine_guard(mutex);
        IndexFile index(RTreeM);
        index.open(index_name.c_str(), false);
        stdList<ArchiveChannel *>::iterator ch;
        for (ch = channels.begin(); ch != channels.end(); ++ch)
        {
            ArchiveChannel *c = *ch;
            Guard guard(c->mutex);
            c->addEvent(guard, 0, ARCH_STOPPED, now);
            c->write(guard, index);
        }
        DataFile::close_all();
    }
    catch (GenericException &e)
    {
        LOG_MSG("Error while writing:\n%s\n", e.what());
    }
    LOG_MSG("Removing memory for channels and groups\n");
    try
    {
        Guard engine_guard(mutex);
        while (! channels.empty())
        {
            ArchiveChannel *c = channels.back();
            channels.pop_back();
            delete c;
        }
        while (! groups.empty())
        {
            delete groups.back();
            groups.pop_back();
        }
    }
    catch (GenericException &e)
    {
        LOG_MSG("Error while deleting channels:\n%s\n", e.what());
    }
    // engine unlocked
    LOG_MSG("Stopping ChannelAccess:\n");
    ca_context_destroy();
    theEngine = 0;
    LOG_MSG("Engine shut down.\n");
}

void Engine::create(const stdString &index_name, short port)
{
    if (theEngine)
        throw GenericException(__FILE__, __LINE__,
                               "Attempted to create multiple engines.");
    theEngine = new Engine(index_name, port);
}

bool Engine::attachToCAContext(Guard &engine_guard)
{
    engine_guard.check(__FILE__, __LINE__, mutex);
    if (ca_attach_context(ca_context) != ECA_NORMAL)
    {
        LOG_MSG("ca_attach_context failed for thread 0x%08lX (%s)\n",
                (unsigned long)epicsThreadGetIdSelf(),
                epicsThreadGetNameSelf());
        return false;
    }
    return true;
}

#ifdef USE_PASSWD
bool Engine::checkUser(const stdString &user, const stdString &pass)
{
    return user == _user &&  pass == _pass;
}
#endif   

GroupInfo *Engine::findGroup(Guard &engine_guard, const stdString &name)
{
    engine_guard.check(__FILE__, __LINE__, mutex);
    stdList<GroupInfo *>::iterator group = groups.begin();
    while (group != groups.end())
    {
        if ((*group)->getName() == name)
            return *group;
        ++group;
    }
    return 0;
}

GroupInfo *Engine::addGroup(Guard &engine_guard, const stdString &name)
{
    engine_guard.check(__FILE__, __LINE__, mutex);
    if (name.empty())
    {
        LOG_MSG("Engine::addGroup: No name given\n");
        return 0;
    }
    GroupInfo *group = findGroup(engine_guard, name);
    if (!group)
    {
        group = new GroupInfo(name);
        groups.push_back(group);
    }
    return group;
}

ArchiveChannel *Engine::findChannel(Guard &engine_guard, const stdString &name)
{
    engine_guard.check(__FILE__, __LINE__, mutex);
    stdList<ArchiveChannel *>::iterator channel = channels.begin();
    while (channel != channels.end())
    {
        if ((*channel)->getName() == name)
            return *channel;
        ++channel;
    }
    return 0;
}

ArchiveChannel *Engine::addChannel(Guard &engine_guard,
                                   GroupInfo *group,
                                   const stdString &channel_name,
                                   double period, bool disabling,
                                   bool monitored)
{
    engine_guard.check(__FILE__, __LINE__, mutex);
    ArchiveChannel *channel = findChannel(engine_guard, channel_name);
    if (!channel)
    {
        channel = new ArchiveChannel(channel_name, period);
        if (!channel)
        {
            LOG_MSG("Engine::addChannel cannot allocate '%s'\n",
                    channel_name.c_str());
            return 0;
        }
        channels.push_back(channel);
    }
    Guard guard(channel->mutex);
    SampleMechanism *mechanism;
    // For existing channels: maximize monitor feature, minimize period
    if (monitored)
        mechanism = new SampleMechanismMonitored(channel);
    else if (period >= get_threshhold)
        mechanism = new SampleMechanismGet(channel);
    else
        mechanism = new SampleMechanismMonitoredGet(channel);
    if (channel->getPeriod(guard) > period)
        channel->setPeriod(engine_guard, guard, period);
    epicsTime now(epicsTime::getCurrent());
    channel->setMechanism(engine_guard, guard, mechanism, now);
    group->addChannel(engine_guard, guard, channel);
    channel->addToGroup(guard, group, disabling);
    try
    {
        IndexFile index(RTreeM);
        index.open(index_name.c_str(), false);
        // Is channel already in Archive?
        stdString directory;
        AutoPtr<RTree> tree(index.getTree(channel_name, directory));
        if (tree)
        {
            RTree::Datablock block;
            RTree::Node node(tree->getM(), true);
            int idx;
            if (tree->getLastDatablock(node, idx, block))
            {   // extract previous knowledge from Archive
                DataFile *datafile =
                    DataFile::reference(directory,
                                        block.data_filename, false);
                if (datafile)
                {
                    {
                        AutoPtr<DataHeader> header(
                            datafile->getHeader(block.data_offset));
                        if (header)
                        {
                            epicsTime last_stamp(header->data.end_time);
                            CtrlInfo ctrlinfo;
                            ctrlinfo.read(datafile,
                                          header->data.ctrl_info_offset);
                            channel->init(engine_guard, guard,
                                          header->data.dbr_type,
                                          header->data.dbr_count,
                                          &ctrlinfo,
                                          &last_stamp);
                            stdString stamp_txt;
                            epicsTime2string(last_stamp, stamp_txt);
                            /*
                              LOG_MSG("'%s' initialized from storage.\n"
                              "Data file '%s' @ 0x%lX\n"
                              "Last Stamp: %s\n",
                              channel_name.c_str(),
                              block.data_filename.c_str(),
                              block.data_offset,
                              stamp_txt.c_str());
                            */
                            // As long as we don't have a new value,
                            // log as disconnected
                            channel->addEvent(guard, 0, ARCH_DISCONNECT, now);
                        }
                    }
                    datafile->release();
                    DataFile::close_all();
                }
            }
        }
        // else ignore that we can't read data for the channel
        index.close();
    }
    catch (GenericException &e)
    {
        LOG_MSG("No previus archive data for '%s':\n%s\n",
                channel_name.c_str(), e.what());
    }
    // else: Ignore that we can't read existing data
    channel->startCA(guard);
    return channel;
}

void Engine::setWritePeriod(Guard &engine_guard, double period)
{
    engine_guard.check(__FILE__, __LINE__, mutex);
    write_period = period;
    next_write_time = roundTimeUp(epicsTime::getCurrent(), write_period);
    // Re-set ev'ry channel's period so that they might adjust buffers
    stdList<ArchiveChannel *>::iterator channel;
    for (channel = channels.begin(); channel != channels.end(); ++channel)
    {
        ArchiveChannel *c = *channel;
        Guard channel_guard(c->mutex);
        c->setPeriod(engine_guard, channel_guard, c->getPeriod(channel_guard));
    }
}

void Engine::setDescription(Guard &engine_guard, const stdString &description)
{
    engine_guard.check(__FILE__, __LINE__, mutex);
    this->description = description;
}

void Engine::setGetThreshold(Guard &engine_guard, double get_threshhold)
{
    engine_guard.check(__FILE__, __LINE__, mutex);
    this->get_threshhold = get_threshhold;
}

void Engine::setBufferReserve(Guard &engine_guard, int reserve)
{
    engine_guard.check(__FILE__, __LINE__, mutex);
    buffer_reserve = reserve;
}

stdString Engine::makeDataFileName()
{
    int year, month, day, hour, min, sec;
    unsigned long nano;
    char buffer[80];
    epicsTime now = epicsTime::getCurrent();
    epicsTime2vals(now, year, month, day, hour, min, sec, nano);
    sprintf(buffer, "%04d%02d%02d-%02d%02d%02d",
            year, month, day, hour, min, sec);
    return stdString(buffer);
}

unsigned long Engine::writeArchive(Guard &engine_guard)
{
    unsigned long count = 0;
    //LOG_MSG("Engine: writing\n");
    is_writing = true;
    try
    {
        IndexFile index(RTreeM);
        index.open(index_name, false);
        stdList<ArchiveChannel *>::iterator ch;
        for (ch = channels.begin(); ch != channels.end(); ++ch)
        {
            Guard guard((*ch)->mutex);
            count += (*ch)->write(guard, index);
        }
    }
    catch (GenericException &e)
    {
        LOG_MSG("Engine write error:\n%s\n", e.what());
    }
    try
    {
        DataFile::close_all();
    }
    catch (GenericException &e)
    {
        LOG_MSG("Engine write error:\n%s\n", e.what());
    }
    is_writing = false;
    //LOG_MSG("Engine: writing done.\n");
    return count;
}

bool Engine::process()
{
    if (info_dump_file.length() > 0)
    {
        int out = open(info_dump_file.c_str(), O_CREAT|O_WRONLY, 0x777);
        info_dump_file.assign(0, 0);
        if (out >= 0)
        {
            int oldout = dup(1);
            dup2(out, 1);
            ca_client_status(10);
            dup2(oldout, 1);
        }
    }

    // When there's nothing to scan or write, we
    // sleep a little. But not too long, because
    // that e.g. determines the max. response time
    // to a shutdown request (via CTRL-C or HTTPD)
    // or a need_CA_flush
#define MAX_DELAY 5.0
    // scan, write or wait?
    epicsTime now = epicsTime::getCurrent();
    double scan_delay, write_delay;
    bool do_wait = true;
    {   // Engine locked
        Guard engine_guard(mutex);
        if (scan_list.isDueAtAll())
        {
            scan_delay = scan_list.getDueTime() - now;
            if (scan_delay <= 0.0)
            {
                scan_list.scan(now);
                do_wait = false;
            }
            if (scan_delay > MAX_DELAY)
                scan_delay = MAX_DELAY;
        }
        else
            scan_delay = MAX_DELAY; 
        write_delay = next_write_time - now;            
        if (write_delay <= 0.0)
        {
            unsigned long count = writeArchive(engine_guard);
            epicsTime end = epicsTime::getCurrent();
            double duration = end - now;
            if (duration < 0.0)
                duration = 0.0;
            write_duration = 0.99*write_duration + 0.01*duration;
            write_count = (99*write_count + count)/100;
            next_write_time = roundTimeUp(end, write_period);
            do_wait = false;
        }
    }
    // Guard released
    if (need_CA_flush)
    {
        ca_flush_io();
        need_CA_flush = false;
    }
    if (do_wait)
    {
        if (write_delay < scan_delay)
        {
            process_delay_avg = 0.99*process_delay_avg + 0.01*write_delay;
            epicsThreadSleep(write_delay);
        }
        else
        {
            process_delay_avg = 0.99*process_delay_avg + 0.01*scan_delay;
            epicsThreadSleep(scan_delay);
        }
    }
    else
        process_delay_avg = 0.99*process_delay_avg;
    return true;
}

