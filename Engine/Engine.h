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

    //* Lock for all the engine info:
    // groups, channels, next write time, ...
    // All but
    // - archive
    // - data within one ChannelInfo
    // General idea:
    // The Engine class doesn't take its own lock,
    // but when you call methods of the Engine
    // you might have to lock/unlock
    void lock()             { _engine_lock.lock();   }
    void unlock()           { _engine_lock.unlock(); }
    
    //* Arb. description string
    const stdString &getDescription() const;
    void setDescription(const stdString &description);

    //* Called by other threads (EngineServer)
    // which need to issue CA calls within the context
    // of the Engine
    bool attachToCAContext();

    //* Called to inform Engine that it needs
    // to flush CA requests in next process call
    void needCAflush()      {_need_CA_flush = true; }
    
    //* Call this in a "main loop" as often as possible
    bool process();

    //* Add/list groups/channels
    const stdList<ChannelInfo *> &getChannels();
    const stdList<GroupInfo *> &getGroups();
    GroupInfo *findGroup(const stdString &name);
    GroupInfo *addGroup(const stdString &name);

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
    const epicsTime &getNextWriteTime() const { return _next_write_time; }
    bool isWriting() const                { return _is_writing; }
    
    // Add channel to ScanList.
    // If result is false,
    // channel has to prepare a monitor.
    bool addToScanList(ChannelInfo *channel);

    ValueI *newValue(DbrType type, DbrCount count);

    void writeArchive();

private:
    Engine(const stdString &directory_file_name);
    ~Engine();
    friend class ToAvoidGNUWarning;

    epicsMutex      _engine_lock;

    struct ca_client_context *_ca_context;
    bool            _need_CA_flush;
    
    epicsTime       _start_time;
    stdString       _directory;
    stdString       _description;
    bool            _is_writing;
    
    stdList<ChannelInfo *> _channels;// all the channels
    stdList<GroupInfo *> _groups;    // scan-groups of channels

    double          _get_threshhold;
    ScanList        _scan_list;      // list of scanned, not monitored channels

    double          _write_period;   // period between writes to archive file
    double          _default_period; // default if not specified by Channel
    int             _buffer_reserve; // 2-> alloc. buffs for 2x expected data
    epicsTime       _next_write_time;// when to write again
    unsigned long   _secs_per_file;  // roughly: data file period
    double          _future_secs;    // now+_future_secs is considered wrong

    Configuration   *_configuration;

    // Archive: Used by WriteThread,
    // sometimes Engine (to get CtrlInfo for new channels)
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


inline const stdList<ChannelInfo *> &Engine::getChannels()
{   return _channels; }

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

inline ValueI *Engine::newValue(DbrType type, DbrCount count)
{   return _archive->newValue(type, count); }

#endif //__ENGINE_H__
 
