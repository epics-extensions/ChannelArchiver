// --------------------------------------------------------
// $Id$
//
// Please refer to NOTICE.txt,
// included as part of this distribution,
// for legal information.
//
// Kay-Uwe Kasemir, kasemir@lanl.gov
// --------------------------------------------------------

#include "ToolsConfig.h"
#include "NetTools.h"
#include "ArchiveException.h"
#include "Engine.h"
#include "EngineServer.h"
#include "WriteThread.h"
#include <cadef.h>
#include <fdManager.h>
#include <osiTimer.h>

static EngineServer *engine_server = 0;
static WriteThread  write_thread;
static class CaPoller *ca_poller = 0;
Engine *theEngine;

// some time values for timeouts
// Happens to be defined for SGI:
#undef NSEC_PER_SEC
const int NSEC_PER_SEC = 1000000000;    /* nano seconds per second */
// osiTime (sec, nano-sec)
const osiTime one_second(1,0);
const osiTime tenth_second(0,NSEC_PER_SEC/10);
const osiTime twentyth_second(0,NSEC_PER_SEC/20);
const osiTime fiftyth_second(0,NSEC_PER_SEC/50);

// --------------------------------------------------------------------------
// Channel Access poll timer etc.
// --------------------------------------------------------------------------

// CaPoller: started at init, runs again.
// Outgoing channel access events need to be scheduled through a
// periodic excursion to channel access -
// this allows channel access to broadcast channel searches and service puts
class CaPoller : public osiTimer
{
public:
    CaPoller(const osiTime &delay) : osiTimer(delay)  {}

    void expire()
    {   ca_poll();  };

    osiBool again() const
    {   return osiTrue; }

    const osiTime delay() const
    {   return twentyth_second; }
};

// Every task has one fd for the broadcast of channel names
// plus one file descriptor per IOC that it connects to.
// Fd activity will cause the fileDescriptorManager to wake up
// to allow these fd changes to be serviced.
class CAfdReg : public fdReg
{
public:
    CAfdReg(const SOCKET fdIn) : fdReg(fdIn, fdrRead) {}
private:
    void callBack()
    {   ca_poll();  }
};

static void caException(struct exception_handler_args args)
{
    static const char   *severity[] =
    {
        "Warning",
        "Success",
        "Error",
        "Info",
        "Fatal",
        "Fatal",
        "Fatal",
        "Fatal"
    };

    if (CA_EXTRACT_SEVERITY(args.stat) >= 0  &&
        CA_EXTRACT_SEVERITY(args.stat) < 8)
    {
        LOG_MSG("CA Exception received: stat "
                << severity[CA_EXTRACT_SEVERITY(args.stat)]
                << ", op " << args.op << "\n");
    }
    else
        LOG_MSG("CA Exception received: stat " << args.stat
                << ", op " << args.op << "\n");
    if (args.chid)
        LOG_MSG("Channel '" << ca_name(args.chid) << "'\n");
    if (args.ctx)
        LOG_MSG("Context " << args.ctx << "\n");
    if (args.pFile)
        LOG_MSG("File " << args.pFile << " (" << args.lineNo << ")\n");
}

// fd_register is called by the CA client lib.
// for each fd opened/closed
static void fd_register(void *pfdctx, int fd, int opened)
{
    if (opened)
    {
        LOG_MSG("CA registers fd " << fd << "\n");

        // Attempt to make this socket's input buffer as large as possible
        // to have some headroom while the engine is busy writing.
        // This doesn't work because CA's "flow control" kicks in anyway
        // since it's based on "is ca_poll called in vain"
        // instead of "is input buffer getting full above high-water mark".
        //
        // Then there is the system dependency on the type of "len"
        // -> use on known systems only for experimentation
        socklen_t len;
        // Win32-default: 8k
        int bufsize = 0x400000; // 4MB
        while (bufsize > 100)
        {
            len = sizeof(int);
            if (setsockopt(fd, SOL_SOCKET, SO_RCVBUF,
                           (char *)&bufsize, len) == 0)
                break;
            bufsize /= 2;
        }
        
        if (getsockopt(fd, SOL_SOCKET, SO_RCVBUF,
                       (char *)&bufsize, &len) == 0)
            LOG_MSG("Adjusted receive buffer to " << bufsize << "\n");
        
        // Just creating a CAfdReg will register it in fdManager:
        CAfdReg *handler = new CAfdReg(fd);
        handler = 0; // to avoid warnings about "not used"
    }
    else
    {
        fdReg *reg = fileDescriptorManager.lookUpFD(fd, fdrRead);
        if (reg)
            delete reg;
        else
        {
            LOG_MSG("fd_register: cannot remove fd " << fd << "\n");
        }
    }
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
    _start_time = osiTime::getCurrent();
    _directory = directory_file_name;

    _description = "EPICS Channel Archiver Engine";
    the_one_and_only = false;

    _write_period = 30;
    _default_period = 1.0;
    _buffer_reserve = 3;
    _get_threshhold = 20.0;
    _secs_per_file = BinArchive::SECS_PER_DAY;
    _future_secs = 6*60*60;
    _configuration = 0; // init. so that setSecsPerFile works
    _archive = new Archive(
        new ENGINE_ARCHIVE_TYPE(directory_file_name, true /* for write */));
    setSecsPerFile(_secs_per_file);

    if (ca_task_initialize() != ECA_NORMAL)
        throwDetailedArchiveException(Fail, "ca_task_initialize");

    // Add exception handler to avoid aborts from CA
    if (ca_add_exception_event(caException, 0) != ECA_NORMAL)
        throwDetailedArchiveException(Fail, "ca_add_exception_event");

    // Link CA client library to fileDescriptorManager
    if (ca_add_fd_registration(fd_register, 0) != ECA_NORMAL)
        throwDetailedArchiveException(Fail, "ca_add_fd_registration");

    engine_server = new EngineServer();

    // Incoming CA activity will cause the fileDescriptorManager to wake up,
    // outgoing CA events need to be scheduled through a periodic excursion
    // to channel access. The CaPoller timer takes care of that:
    ca_poller = new CaPoller(twentyth_second);
    write_thread.create();

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

    if (ca_poller)
    {
        delete ca_poller;
        ca_poller = 0;
    }
    delete engine_server;
    engine_server = 0;

    LOG_MSG("Waiting for WriteThread to exit...\n");
    int code;
    write_thread.stop();
    write_thread.join(code);
    if (code)
        LOG_MSG("WriteThread exit code: " << code << "\n");

    LOG_MSG("Adding 'Archive_Off' events...\n");
    osiTime now;
    now = osiTime::getCurrent();
    Archive &archive = lockArchive();
    ChannelIterator channel(archive);
    lockChannels();
    try
    {
        stdList<ChannelInfo *>::iterator channel_info = _channels.begin();
        while (channel_info != _channels.end())
        {
            (*channel_info)->shutdown(archive, channel, now);
            ++channel_info;
        }
    }
    catch (ArchiveException &e)
    {
        LOG_MSG(osiTime::getCurrent()
                << ": " << "Engine::shutdown caught"
                << e.what() << "\n");
    }
    unlockChannels();
    channel->releaseBuffer();
    unlockArchive();

    LOG_MSG("Done.\n");
    theEngine = 0;
    delete this;
}

Engine::~Engine()
{
    ca_task_exit();
    delete _archive;

    lockChannels();
    while (! _channels.empty())
    {
        delete _channels.back();
        _channels.pop_back();
    }
    unlockChannels();

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
    osiTime now = osiTime::getCurrent();

    // FiledescriptorManager: also keeps the osiTimer ticking.
    // - keeps osiTimer ticking
    // - on registered file descriptors, channel access events
    //   result in faster reaction
    // We wait this little so that the ca_get channels are < 50 msec
    // away from the time stamp required for the get
    fileDescriptorManager.process(fiftyth_second);

    // fetch data with ca_gets that are too slow to monitor
    if (_scan_list.isDue(now))
        _scan_list.scan(now);

    ca_poll();
    if ((double(now) - double(_last_written)) >= _write_period)
    {
        write_thread.write();
        // _last_written is modified after the archiving is done.
        // If there is slowness in the file writing - we will check
        // it less frequently - thus overwriting the archive circular
        // buffer - thus causing events to be discarded at the monitor
        // receive callback
        _last_written = osiTime::getCurrent();
        if (! write_thread.isRunning())
        {
            LOG_MSG(osiTime::getCurrent()
                    << ": WriteThread stopped. Engine quits, too.\n");
            return false;
        }
    }

    return true;
}

GroupInfo *Engine::findGroup(const stdString &name)
{
    stdList<GroupInfo *>::iterator group = _groups.begin();
    while (group != _groups.end())
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

    stdList<ChannelInfo *>::iterator channel = lockChannels().begin();
    while (channel != _channels.end())
    {
        if ((*channel)->getName() == name)
        {
            found = *channel;
            break;
        }
        ++channel;
    }
    unlockChannels();

    return found;
}

ChannelInfo *Engine::addChannel(GroupInfo *group, const stdString &channel_name,
    double period, bool disabling, bool monitored)
{
    ChannelInfo *channel_info = findChannel(channel_name);
    bool new_channel;

    lockChannels();
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
        new_channel = false;
    }
    group->addChannel(channel_info);
    channel_info->addToGroup(group, disabling);
    channel_info->setMonitored(monitored);
    channel_info->setPeriod(period);

    if (new_channel)
    {
        Archive &archive = lockArchive();
        try
        {
            // Is channel already in Archive?
            ChannelIterator arch_channel(archive);
            if (archive.findChannelByName(channel_name, arch_channel))
            {   // extract previous knowledge from Archive
                ValueIterator last_value(archive);
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
                archive.addChannel(channel_name, arch_channel);
        }
        catch (ArchiveException &e)
        {
            LOG_MSG("Engine::addChannel: caught\n" << e.what() << "\n");
        }
        unlockArchive();
    }
    channel_info->startCaConnection(new_channel);
    unlockChannels();

    if (_configuration)
        _configuration->saveChannel(channel_info);

    return channel_info;
}

void Engine::setWritePeriod(double period)
{
    _write_period = period;

    lockChannels();
    stdList<ChannelInfo *>::iterator channel_info = channel_info = _channels.begin();
    while (channel_info != _channels.end())
    {
        (*channel_info)->checkRingBuffer();
        ++channel_info;
    }
    unlockChannels();

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
    Archive &archive = lockArchive();
    ChannelIterator channel(archive);
    lockChannels();
    try
    {
        stdList<ChannelInfo *>::iterator channel_info = _channels.begin();
        while (channel_info != _channels.end() &&
               write_thread.isRunning())
        {
            (*channel_info)->write(archive, channel);
            ++channel_info;
        }
    }
    catch (ArchiveException &e)
    {
        LOG_MSG(osiTime::getCurrent()
                 << ": " << "Engine::writeArchive caught"
                 << e.what() << "\n");
    }
    unlockChannels();
    channel->releaseBuffer();
    unlockArchive();
}




