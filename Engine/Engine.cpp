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

static EngineServer *engine_server = 0;
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

    ca_context = ca_current_context();
    
    engine_server = new EngineServer();

#ifdef USE_PASSWD
    _user = DEFAULT_USER;
    _pass = DEFAULT_PASS;
#endif   
}

void Engine::create(const stdString &directory_file_name)
{
    theEngine = new Engine(directory_file_name);
}

bool Engine::attachToCAContext()
{
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

    LOG_MSG("Stopping web server\n");
    delete engine_server;
    engine_server = 0;

    LOG_MSG("Adding 'Archive_Off' events...\n");
    epicsTime now;
    now = epicsTime::getCurrent();
#ifdef TODO
    mutex.lock();
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
    mutex.unlock();
#endif
    LOG_MSG("Closing archive\n");
    delete _archive;

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
    ca_flush_io();
    LOG_MSG("Stopping ChannelAccess:\n");
    ca_context_destroy();
    theEngine = 0;
    delete this;
    LOG_MSG("Engine shut down.\n");
}

#ifdef USE_PASSWD
bool Engine::checkUser(const stdString &user, const stdString &pass)
{
    return user == _user &&  pass == _pass;
}
#endif   

GroupInfo *Engine::findGroup(const stdString &name)
{
    stdList<GroupInfo *>::iterator group = groups.begin();
    while (group != groups.end())
    {
        if ((*group)->getName() == name)
            return *group;
        ++group;
    }
    return 0;
}

GroupInfo *Engine::addGroup(const stdString &name)
{
    if (name.empty())
        throw GenericException(__FILE__, __LINE__);
    GroupInfo *group = findGroup(name);
    if (!group)
    {
        group = new GroupInfo(name);
        groups.push_back(group);
    }
    if (_configuration)
        _configuration->saveGroup(group);
    return group;
}

ArchiveChannel *Engine::findChannel(const stdString &name)
{
    stdList<ArchiveChannel *>::iterator channel = channels.begin();
    while (channel != channels.end())
    {
        if ((*channel)->getName() == name)
            return *channel;
        ++channel;
    }
    return 0;
}

ArchiveChannel *Engine::addChannel(GroupInfo *group,
                                   const stdString &channel_name,
                                   double period, bool disabling, bool monitored)
{
    ArchiveChannel *channel = findChannel(channel_name);
    bool new_channel;

    if (!channel)
    {
        channel = new ArchiveChannel(channel_name, period,
                                     new SampleMechanismMonitored());
        channel->mutex.lock();
        channels.push_back(channel);
        new_channel = true;
    }
    else
    {
#ifdef TODO
        channel_info->lock();
        // For existing channels: minimize period, maximize monitor feature
        if (channel_info->isMonitored())
            monitored = true;
        if (channel_info->getPeriod() < period)
            period = channel_info->getPeriod();
        channel_info->resetBuffers();
        new_channel = false;
 
#endif
    }

    group->addChannel(channel);
    channel->addToGroup(group, disabling);

#ifdef TODO
    if (new_channel)
    {
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
    }
    if (_configuration)
        _configuration->saveChannel(channel_info);
#endif
    channel->startCA();
    channel->mutex.unlock();

    return channel;
}

void Engine::setWritePeriod(double period)
{
    _write_period = period;
    _next_write_time = roundTimeUp(epicsTime::getCurrent(), _write_period);

#ifdef TODO
    stdList<ChannelInfo *>::iterator channel_info = _channels.begin();
    while (channel_info != _channels.end())
    {
        (*channel_info)->lock();
        (*channel_info)->checkRingBuffer();
        (*channel_info)->unlock();
        ++channel_info;
    }

    if (_configuration)
        _configuration->saveEngine();
#endif
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

void Engine::writeArchive()
{
#ifdef
    _is_writing = true;
    ChannelIterator channel(*_archive);
    try
    {
        stdList<ChannelInfo *>::iterator channel_info = _channels.begin();
        while (channel_info != _channels.end())
        {
            (*channel_info)->lock();
            (*channel_info)->write(*_archive, channel);
            (*channel_info)->unlock();
            ++channel_info;
        }
    }
    catch (ArchiveException &e)
    {
        LOG_MSG("Engine::writeArchive caught %s\n", e.what());
    }
    channel->releaseBuffer();
    _is_writing = false;
#endif
}

bool Engine::process()
{
    // scan, write or wait?
    epicsTime now = epicsTime::getCurrent();
    double scan_delay, write_delay;
    bool do_wait = true;

    if (_scan_list.isDueAtAll())
    {
        scan_delay = _scan_list.getDueTime() - now;
        if (scan_delay <= 0.0)
        {
            _scan_list.scan(now);
            do_wait = false;
        }
    }
    else
    {
        // Wait a little while....
        // This determines the _need_CA_flush delay
        scan_delay = 0.5;
    }
    
    write_delay = _next_write_time - now;            
    if (write_delay <= 0.0)
    {
        writeArchive();
        _next_write_time = roundTimeUp(epicsTime::getCurrent(), _write_period);
        do_wait = false;
    }
    if (need_CA_flush)
    {
        ca_flush_io();
        need_CA_flush = false;
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

