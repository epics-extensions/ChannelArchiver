// -------------------- -*- c++ -*- -----------------------
// $Id$
//
// Please refer to NOTICE.txt,
// included as part of this distribution,
// for legal information.
//
// Kay-Uwe Kasemir, kasemir@lanl.gov
// --------------------------------------------------------

#ifndef __ENGINE_H__
#define __ENGINE_H__

#include "../ArchiverConfig.h"
#include "GroupInfo.h"
#include "ScanList.h"
#include "Configuration.h"
#include "BinArchive.h"

//CLASS Engine
class Engine
{
public:
    //* The Engine is a Singleton,
    // createable only by calling this method
    // and from then on accessible via global Engine *theEngine
    static void create(const stdString &directory_file_name);
    void shutdown();

    //* Engine will tell this Configuration class about changes,
    // so that they can be made persistent
    void setConfiguration(Configuration *c);
    Configuration *getConfiguration();

#ifdef USE_PASSWD
    // Check if user/password are valid
    bool checkUser(const stdString &user, const stdString &pass);
#endif

    //* Arb. description string
    const stdString &getDescription() const;
    void setDescription(const stdString &description);

    //* Call this in a "main loop" as often as possible
    bool process();

    //* Add/list groups/channels
    const stdList<GroupInfo *> &getGroups();
    GroupInfo *findGroup(const stdString &name);
    GroupInfo *addGroup(const stdString &name);

    // Get current channel list, locked and unlocked
    stdList<ChannelInfo *> *getChannels();
    stdList<ChannelInfo *> *lockChannels();
    // has to be called after lockChannels
    void unlockChannels();

    ChannelInfo *findChannel(const stdString &name);
    ChannelInfo *addChannel(GroupInfo *group, const stdString &channel_name,
                            double period, bool disabling, bool monitored);

    //* Engine configuration
    void   setWritePeriod(double period);
    double getWritePeriod() const;
    void   setDefaultPeriod(double period);
    double getDefaultPeriod();
    int    getBufferReserve() const;
    void   setBufferReserve(int reserve);
    void   setSecsPerFile(double secs_per_file);
    double getSecsPerFile() const;

    // Engine ignores time stamps which are later than now+future_secs
    double getIgnoredFutureSecs() const;
    void setIgnoredFutureSecs(double secs);

    //* Channels w/ period > threshold are scanned, not monitored
    void   setGetThreshold(double get_threshhold);
    double getGetThreshold();

    // Engine Info: Started, where, info about write thread
    const epicsTime &getStartTime() const { return _start_time; }
    const stdString &getDirectory() const { return _directory;  }
    const epicsTime &getWriteTime() const { return _last_written; }
    bool isWriting() const                { return _is_writing; }
    
    // Add channel to ScanList.
    // If result is false,
    // channel has to prepare a monitor.
    bool addToScanList(ChannelInfo *channel);

    Archive &lockArchive();
    void unlockArchive();
    ValueI *newValue(DbrType type, DbrCount count);

    void writeArchive();

private:
    Engine(const stdString &directory_file_name);
    ~Engine();
    friend class ToAvoidGNUWarning;

    epicsTime       _start_time;
    stdString       _directory;
    stdString       _description;
    bool            _is_writing;
    
    
    epicsMutex      _channels_lock;
    stdList<ChannelInfo *> _channels;// all the channels
    stdList<GroupInfo *> _groups;    // scan-groups of channels

    double          _get_threshhold;
    ScanList        _scan_list;      // list of scanned, not monitored channels

    double          _write_period;   // period between writes to archive file
    double          _default_period; // default if not specified by Channel
    int             _buffer_reserve; // 2-> alloc. buffs for 2x expected data
    epicsTime       _last_written;   // time this took place
    unsigned long   _secs_per_file;  // roughly: data file period
    double          _future_secs;    // now+_future_secs is considered wrong

    Configuration   *_configuration;
    epicsMutex      _archive_lock;
    Archive         *_archive;

#ifdef USE_PASSWD
    stdString       _user, _pass;
#endif
};

// The only, global Engine object:
extern Engine *theEngine;

inline const stdString &Engine::getDescription() const
{   return _description; }

inline void Engine::setConfiguration(Configuration *c)
{   _configuration = c; }

inline Configuration *Engine::getConfiguration()
{   return _configuration;  }

inline stdList<ChannelInfo *> *Engine::getChannels()
{   return &_channels; }

inline stdList<ChannelInfo *> *Engine::lockChannels()
{
    _channels_lock.lock();
    return &_channels;
}

inline void Engine::unlockChannels()
{   _channels_lock.unlock(); }

inline const stdList<GroupInfo *> &Engine::getGroups()
{   return _groups; }

inline double Engine::getGetThreshold()
{   return _get_threshhold; }

inline double Engine::getWritePeriod() const
{   return _write_period; }

inline double Engine::getDefaultPeriod()
{   return _default_period; }

inline int Engine::getBufferReserve() const
{   return _buffer_reserve; }

inline double Engine::getSecsPerFile() const
{   return _secs_per_file; }

inline double Engine::getIgnoredFutureSecs() const
{   return _future_secs; }

inline void Engine::setIgnoredFutureSecs(double secs)
{   _future_secs = secs; }

inline bool Engine::addToScanList(ChannelInfo *channel)
{
    if (channel->getPeriod() >= _get_threshhold)
    {
        _scan_list.addChannel(channel);
        return true;
    }
    return false;
}

inline Archive &Engine::lockArchive()
{
    _archive_lock.lock();
    return *_archive;
}

inline void Engine::unlockArchive()
{   _archive_lock.unlock(); }

inline ValueI *Engine::newValue(DbrType type, DbrCount count)
{   return _archive->newValue(type, count); }

#endif //__ENGINE_H__
 
