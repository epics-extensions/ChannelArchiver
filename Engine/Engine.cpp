// --------------------------------------------------------
// $Id$
//
// Please refer to NOTICE.txt,
// included as part of this distribution,
// for legal information.
//
// Kay-Uwe Kasemir, kasemir@lanl.gov
// --------------------------------------------------------

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
#include "DirectoryFile.h"
#include "DataFile.h"
// Engine
#include "Engine.h"
#include "EngineServer.h"

static EngineServer *engine_server = 0;
Engine *theEngine;

static void caException(struct exception_handler_args args)
{
    const char *pName;
    
    if (args.chid)
        pName = ca_name(args.chid);
    else
        pName = "?";

    LOG_MSG("CA Exception %s - with request "
            "chan=%s op=%d type=%s count=%d:\n%s", 
            args.ctx, pName, args.op, dbr_type_to_text(args.type), args.count,
            ca_message(args.stat));
}

// --------------------------------------------------------------------------
// Engine
// --------------------------------------------------------------------------

Engine::Engine(const stdString &index_name)
{
    static bool the_one_and_only = true;

    LOG_ASSERT(the_one_and_only);
    start_time = epicsTime::getCurrent();
    RTreeM = 50;
    num_connected = 0;
    this->index_name = index_name;
    is_writing = false;
    description = "EPICS Channel Archiver Engine";
    the_one_and_only = false;

    get_threshhold = 20.0;
    write_period = 30;
    buffer_reserve = 3;
    next_write_time = roundTimeUp(epicsTime::getCurrent(), write_period);
    future_secs = 6*60*60;

    // Initialize CA library for multi-treaded use and
    // add exception handler to avoid aborts from CA
    if (ca_context_create(ca_enable_preemptive_callback) != ECA_NORMAL ||
        ca_add_exception_event(caException, 0) != ECA_NORMAL)
    {
        LOG_MSG("CA client initialization failed\n");
        LOG_ASSERT(0);
    }
    ca_context = ca_current_context();
#ifdef USE_PASSWD
    _user = DEFAULT_USER;
    _pass = DEFAULT_PASS;
#endif   
}

void Engine::create(const stdString &index_name)
{
    theEngine = new Engine(index_name);
    engine_server = new EngineServer();
}

bool Engine::attachToCAContext(Guard &guard)
{
    guard.check(mutex);
    if (ca_attach_context(ca_context) != ECA_NORMAL)
    {
        LOG_MSG("ca_attach_context failed for thread 0x%08X (%s)\n",
                epicsThreadGetIdSelf(), epicsThreadGetNameSelf());
        return false;
    }
    return true;
}

void Engine::shutdown()
{
    LOG_MSG("Shutdown:\n");
    LOG_ASSERT(this == theEngine);
    delete engine_server;
    engine_server = 0;
    LOG_MSG("Adding 'Archive_Off' events...\n");
    epicsTime now = epicsTime::getCurrent();
    mutex.lock(); // - lock
    IndexFile index(RTreeM);
    if (index.open(index_name.c_str(), false))
    {
        stdList<ArchiveChannel *>::iterator ch;
        for (ch = channels.begin(); ch != channels.end(); ++ch)
        {
            Guard guard((*ch)->mutex);
            (*ch)->addEvent(guard, 0, ARCH_STOPPED, now);
            (*ch)->write(guard, index);
        }
        DataFile::close_all();
        index.close();
    }
    else
        LOG_MSG("Engine::shutdown cannot open index %s\n",
                index_name.c_str());
    LOG_MSG("Removing memory for channels and groups\n");
    while (! channels.empty())
    {
        delete channels.back();
        channels.pop_back();
    }
    while (! groups.empty())
    {
        delete groups.back();
        groups.pop_back();
    }
    LOG_MSG("Stopping ChannelAccess:\n");
    ca_context_destroy();
    theEngine = 0;
    mutex.unlock(); // - unlock
    delete this;
    LOG_MSG("Engine shut down.\n");
}

#ifdef USE_PASSWD
bool Engine::checkUser(const stdString &user, const stdString &pass)
{
    return user == _user &&  pass == _pass;
}
#endif   

GroupInfo *Engine::findGroup(Guard &guard, const stdString &name)
{
    guard.check(mutex);
    stdList<GroupInfo *>::iterator group = groups.begin();
    while (group != groups.end())
    {
        if ((*group)->getName() == name)
            return *group;
        ++group;
    }
    return 0;
}

GroupInfo *Engine::addGroup(Guard &guard, const stdString &name)
{
    guard.check(mutex);
    if (name.empty())
    {
        LOG_MSG("Engine::addGroup: No name given\n");
        return 0;
    }
    GroupInfo *group = findGroup(guard, name);
    if (!group)
    {
        group = new GroupInfo(name);
        groups.push_back(group);
    }
    return group;
}

ArchiveChannel *Engine::findChannel(Guard &guard, const stdString &name)
{
    guard.check(mutex);
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
    engine_guard.check(mutex);
    ArchiveChannel *channel = findChannel(engine_guard, channel_name);
    bool new_channel;
    if (!channel)
    {
        channel = new ArchiveChannel(channel_name, period);
        channels.push_back(channel);
        new_channel = true;
    }
    else
        new_channel = false;
    Guard guard(channel->mutex);
    SampleMechanism *mechanism;
    // For existing channels: maximize monitor feature, minimize period
    if (monitored)
        mechanism = new SampleMechanismMonitored(channel);
    else
    {
        // TODO: Pick correct mechanism
        mechanism = new SampleMechanismGet(channel);
    }
    if (channel->getPeriod(guard) > period)
        channel->setPeriod(engine_guard, guard, period);
    channel->setMechanism(guard, mechanism);
    group->addChannel(guard, channel);
    channel->addToGroup(guard, group, disabling);
    if (new_channel)
    {
        IndexFile index(RTreeM);
        if (index.open(index_name.c_str(), false))
        {   // Is channel already in Archive?
            AutoPtr<RTree> tree(index.getTree(channel_name));
            if (tree)
            {
                RTree::Datablock block;
                RTree::Node node(tree->getM(), true);
                int idx;
                if (tree->getLastDatablock(node, idx, block))
                {   // extract previous knowledge from Archive
                    DataFile *datafile =
                        DataFile::reference(index.getDirectory(),
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
                                channel->addEvent(guard, 0, ARCH_DISCONNECT,
                                                  epicsTime::getCurrent());
                            }
                        }
                        datafile->release();
                        DataFile::close_all();
                    }
                }
            }
            index.close();
        }
    }
    channel->startCA(guard);
    return channel;
}

void Engine::setWritePeriod(Guard &guard, double period)
{
    guard.check(mutex);
    write_period = period;
    next_write_time = roundTimeUp(epicsTime::getCurrent(), write_period);
    // Re-set ev'ry channel's period so that they might adjust buffers
    stdList<ArchiveChannel *>::iterator channel;
    for (channel = channels.begin(); channel != channels.end(); ++channel)
    {
        ArchiveChannel *c = *channel;
        Guard channel_guard(c->mutex);
        c->setPeriod(guard, channel_guard, c->getPeriod(channel_guard));
    }
}

void Engine::setDescription(Guard &guard, const stdString &description)
{
    guard.check(mutex);
    this->description = description;
}

void Engine::setGetThreshold(Guard &guard, double get_threshhold)
{
    guard.check(mutex);
    this->get_threshhold = get_threshhold;
}

void Engine::setBufferReserve(Guard &guard, int reserve)
{
    guard.check(mutex);
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

void Engine::writeArchive(Guard &engine_guard)
{
    LOG_MSG("Engine: writing\n");
    is_writing = true;
    IndexFile index(RTreeM);
    if (index.open(index_name.c_str(), false))
    {
        stdList<ArchiveChannel *>::iterator ch;
        for (ch = channels.begin(); ch != channels.end(); ++ch)
        {
            Guard guard((*ch)->mutex);
            (*ch)->write(guard, index);
        }
        index.close();
    }
    else
    {
        LOG_MSG("Engine::writeArchive cannot open index %s\n",
                index_name.c_str());
    }
    DataFile::close_all();
    is_writing = false;
    LOG_MSG("Engine: writing done.\n");
}

bool Engine::process()
{
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
    {
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
            writeArchive(engine_guard);
            next_write_time = roundTimeUp(epicsTime::getCurrent(),
                                          write_period);
            do_wait = false;
        }
        if (need_CA_flush)
        {
            ca_flush_io();
            need_CA_flush = false;
        }
    }
    // Guard released
    if (do_wait)
    {
        if (write_delay < scan_delay)
            epicsThreadSleep(write_delay);
        else
            epicsThreadSleep(scan_delay);
    }
    return true;
}

