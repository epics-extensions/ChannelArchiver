// --------------------------------------------------------
// $Id$
//
// Please refer to NOTICE.txt,
// included as part of this distribution,
// for legal information.
//
// Kay-Uwe Kasemir, kasemir@lanl.gov
// --------------------------------------------------------

#include <ToolsConfig.h>
#include <cadef.h>
#include <fdManager.h>
#include <epicsTimeHelper.h>
#include "ArchiveException.h"
#include "Engine.h"
#include "EngineServer.h"
#include "WriteThread.h"

static EngineServer *engine_server = 0;
static WriteThread  write_thread;
Engine *theEngine;

static void caException(struct exception_handler_args args)
{
    const char *pName;
    
    if (args.chid)
        pName = ca_name(args.chid);
    else
        pName = "?";

    LOG_MSG("CA Exception %s - with request chan=%s op=%d type=%s count=%d:\n%s", 
            args.ctx, pName, args.op, dbr_type_to_text(args.type), args.count,
            ca_message(args.stat));
}

// --------------------------------------------------------------------------
// Engine
// --------------------------------------------------------------------------

Engine::Engine(const stdString &directory_file_name)
{
    static bool the_one_and_only = true;

    if (! the_one_and_only)
        throwDetailedArchiveException(
            Unsupported, "Cannot run more than one Engine");
    _start_time = epicsTime::getCurrent();
    _directory = directory_file_name;
    _is_writing = false;
    _description = "EPICS Channel Archiver Engine";
    the_one_and_only = false;

    _get_threshhold = 20.0;
    _write_period = 30;
    _default_period = 1.0;
    _buffer_reserve = 3;
    _next_write_time = roundTimeUp(epicsTime::getCurrent(), _write_period);
    _secs_per_file = BinArchive::SECS_PER_DAY;
    _future_secs = 6*60*60;
    _configuration = 0; // init. so that setSecsPerFile works
    _archive = new Archive(
        new ENGINE_ARCHIVE_TYPE(directory_file_name, true /* for write */));
    setSecsPerFile(_secs_per_file);

    // Initialize CA library for multi-treaded use
    if (ca_context_create(ca_enable_preemptive_callback) != ECA_NORMAL)
        throwDetailedArchiveException(Fail, "ca_context_create");
    
    // Add exception handler to avoid aborts from CA
    if (ca_add_exception_event(caException, 0) != ECA_NORMAL)
        throwDetailedArchiveException(Fail, "ca_add_exception_event");

    engine_server = new EngineServer();

    write_thread.start();

#ifdef USE_PASSWD
    _user = DEFAULT_USER;
    _pass = DEFAULT_PASS;
#endif   
}

void Engine::create(const stdString &directory_file_name)
{
    theEngine = new Engine(directory_file_name);
}

void Engine::shutdown()
{
    LOG_MSG("Shutdown:\n");
    LOG_ASSERT(this == theEngine);

    delete engine_server;
    engine_server = 0;

    LOG_MSG("Waiting for WriteThread to exit...\n");
    write_thread.stop();

    LOG_MSG("Adding 'Archive_Off' events...\n");
    epicsTime now;
    now = epicsTime::getCurrent();

    _archive_lock.lock();
    ChannelIterator channel(*_archive);
    try
    {
        stdList<ChannelInfo *>::iterator channel_info = _channels.begin();
        while (channel_info != _channels.end())
        {
            (*channel_info)->shutdown(*_archive, channel, now);
            ++channel_info;
        }
    }
    catch (ArchiveException &e)
    {
        LOG_MSG("Engine::shutdown caught %s\n", e.what());
    }
    channel->releaseBuffer();
    _archive_lock.unlock();

    LOG_MSG("Done.\n");
    theEngine = 0;
    delete this;
}

Engine::~Engine()
{
    ca_context_destroy();
    delete _archive;

    while (! _channels.empty())
    {
        delete _channels.back();
        _channels.pop_back();
    }
    while (! _groups.empty())
    {
        delete _groups.back();
        _groups.pop_back();
    }
}

#ifdef USE_PASSWD
bool Engine::checkUser(const stdString &user, const stdString &pass)
{
    return user == _user &&  pass == _pass;
}
#endif   

bool Engine::process()
{
    epicsTime now = epicsTime::getCurrent();

    // scan, write or wait?
    double scan_delay  = _scan_list.getDueTime() - now;
    double write_delay = _next_write_time        - now;
    bool   do_wait = true;
    if (scan_delay <= 0.0)
    {
        _scan_list.scan(now);
        do_wait = false;
    }
    if (write_delay <= 0.0)
    {
        write_thread.write();
        // _next_write_time is modified after the archiving is done.
        // If there is slowness in the file writing - we will check
        // it less frequently - thus overwriting the archive circular
        // buffer - thus causing events to be discarded at the monitor
        // receive callback
        _next_write_time = roundTimeUp(epicsTime::getCurrent(), _write_period);
        if (! write_thread.isRunning())
        {
            LOG_MSG("WriteThread stopped. Engine quits, too.\n");
            return false;
        }
        do_wait = false;
    }

    if (do_wait)
    {
        if (write_delay < scan_delay)
            epicsThreadSleep(write_delay);
        else
            epicsThreadSleep(scan_delay);
    }
    
    return true;
}

GroupInfo *Engine::findGroup(const stdString &name)
{
    GroupInfo *found = 0;
    stdList<GroupInfo *>::iterator group = _groups.begin();
    while (group != _groups.end())
    {
        if ((*group)->getName() == name)
        {
            found = *group;
            break;
        }
        ++group;
    }
    return found;
}

GroupInfo *Engine::addGroup(const stdString &name)
{
    if (name.empty())
        throw GenericException(__FILE__, __LINE__);
    GroupInfo *group = findGroup(name);
    if (!group)
    {
        group = new GroupInfo();
        group->setName(name);
        _groups.push_back(group);
    }
    if (_configuration)
        _configuration->saveGroup(group);

    return group;
}

ChannelInfo *Engine::findChannel(const stdString &name)
{
    ChannelInfo *found = 0;

    stdList<ChannelInfo *>::iterator channel = _channels.begin();
    while (channel != _channels.end())
    {
        if ((*channel)->getName() == name)
        {
            found = *channel;
            break;
        }
        ++channel;
    }

    return found;
}

ChannelInfo *Engine::addChannel(GroupInfo *group,
                                const stdString &channel_name,
                                double period, bool disabling, bool monitored)
{
    ChannelInfo *channel_info = findChannel(channel_name);
    bool new_channel;

    if (!channel_info)
    {
        channel_info = new ChannelInfo();
        channel_info->setName(channel_name);
        _channels.push_back(channel_info);
        new_channel = true;
    }
    else
    {
        // For existing channels: minimize period, maximize monitor feature
        if (channel_info->isMonitored())
            monitored = true;
        if (channel_info->getPeriod() < period)
            period = channel_info->getPeriod();
        channel_info->resetBuffers();
        new_channel = false;
    }
    group->addChannel(channel_info);
    channel_info->addToGroup(group, disabling);
    channel_info->setMonitored(monitored);
    channel_info->setPeriod(period);

    if (new_channel)
    {
        _archive_lock.lock();
        try
        {
            // Is channel already in Archive?
            ChannelIterator arch_channel(*_archive);
            if (_archive->findChannelByName(channel_name, arch_channel))
            {   // extract previous knowledge from Archive
                ValueIterator last_value(*_archive);
                if (arch_channel->getLastValue(last_value))
                {
                    // ChannelInfo copies CtrlInfo, so it's still
                    // valid after archive is closed!
                    channel_info->setCtrlInfo(last_value->getCtrlInfo());
                    channel_info->setValueType(last_value->getType(),
                                               last_value->getCount());
                    channel_info->setLastArchiveStamp(last_value->getTime());
                }
            }
            else
                _archive->addChannel(channel_name, arch_channel);
        }
        catch (ArchiveException &e)
        {
            LOG_MSG("Engine::addChannel: caught\n%s\n", e.what());
        }
        _archive_lock.unlock();
    }
    channel_info->startCaConnection(new_channel);

    if (_configuration)
        _configuration->saveChannel(channel_info);

    return channel_info;
}

void Engine::setWritePeriod(double period)
{
    _write_period = period;
    _next_write_time = roundTimeUp(epicsTime::getCurrent(), _write_period);

    stdList<ChannelInfo *>::iterator channel_info = _channels.begin();
    while (channel_info != _channels.end())
    {
        (*channel_info)->checkRingBuffer();
        ++channel_info;
    }

    if (_configuration)
        _configuration->saveEngine();
}

void Engine::setDescription(const stdString &description)
{
    _description = description;
    if (_configuration)
        _configuration->saveEngine();
}

void Engine::setDefaultPeriod(double period)
{
    _default_period = period;
    if (_configuration)
        _configuration->saveEngine();
}

void Engine::setGetThreshold(double get_threshhold)
{
    _get_threshhold = get_threshhold;
    if (_configuration)
        _configuration->saveEngine();
}

void Engine::setBufferReserve(int reserve)
{
    _buffer_reserve = reserve;
    if (_configuration)
        _configuration->saveEngine();
}

void Engine::setSecsPerFile(double secs_per_file)
{
    _secs_per_file = (unsigned long) secs_per_file;
    BinArchive *binarch = dynamic_cast<BinArchive *>(_archive->getI());
    if (binarch)
        binarch->setSecsPerFile(_secs_per_file);
    if (_configuration)
        _configuration->saveEngine();
}

// Called by WriteThread as well as Engine on shutdown!
void Engine::writeArchive()
{
    _is_writing = true;
    _archive_lock.lock();
    ChannelIterator channel(*_archive);
    try
    {
        stdList<ChannelInfo *>::iterator channel_info = _channels.begin();
        while (channel_info != _channels.end() &&
               write_thread.isRunning())
        {
            (*channel_info)->write(*_archive, channel);
            ++channel_info;
        }
    }
    catch (ArchiveException &e)
    {
        LOG_MSG("Engine::writeArchive caught %s\n", e.what());
    }
    channel->releaseBuffer();
    _archive_lock.unlock();
    _is_writing = false;
}

