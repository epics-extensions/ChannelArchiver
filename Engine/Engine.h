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

// Engine
#include "ArchiverConfig.h"
#include "GroupInfo.h"
#include "ScanList.h"

/// \defgroup Engine
/// Classes related to the ArchiveEngine

/// Engine is the main class of the ArchiveEngine program,
/// the one that contains the lists of all the ArchiveChannel
/// and GroupInfo entries as well as the main loop.
class Engine
{
public:
    /// The Engine is a Singleton,
    /// createable only by calling this method
    /// and from then on accessible via global Engine *theEngine
    static void create(const stdString &index_name);
    void shutdown();
    
#ifdef USE_PASSWD
    /// Check if user/password are valid
    bool checkUser(const stdString &user, const stdString &pass);
#endif

    /// Lock for all the engine info:
    /// groups, channels, next write time, ...
    /// All but
    /// - archive
    /// - data within one ChannelInfo
    /// General idea:
    /// The Engine class doesn't take its own lock,
    /// but when you call methods of the Engine
    /// you might have to lock/unlock
    epicsMutex mutex;
    
    /// Arb. description string
    const stdString &getDescription() const;
    void setDescription(Guard &guard, const stdString &description);

    /// Called by other threads (EngineServer)
    /// which need to issue CA calls within the context
    /// of the Engine
    bool attachToCAContext(Guard &guard);

    /// Set to make the Engine flush CA requests in next process call
    bool need_CA_flush;
    
    /// Call this in a "main loop" as often as possible
    bool process();

    /// Add/list groups/channels
    stdList<ArchiveChannel *> channels;// all the channels
    stdList<GroupInfo *> groups;    // scan-groups of channels
    GroupInfo *findGroup(Guard &guard, const stdString &name);
    GroupInfo *addGroup(Guard &guard, const stdString &name);
    ArchiveChannel *findChannel(Guard &guard, const stdString &name);
    ArchiveChannel *addChannel(Guard &guard, GroupInfo *group,
                               const stdString &channel_name,
                               double period, bool disabling, bool monitored);

    /// Engine configuration
    void   setWritePeriod(Guard &guard, double period);
    double getWritePeriod() const;   
    int    getBufferReserve() const;
    void   setBufferReserve(Guard &guard, int reserve);

    enum {
        SECS_PER_DAY = 60*60*24,
        SECS_PER_MONTH = 60*60*24*31 // depends on month, ... this is a magic number
    };

    /// Determine the suggested buffer size for a value
    /// with given scan period based on how often we write
    /// and the buffer reserve
    size_t suggestedBufferSize(Guard &guard, double period);
    
    /// Engine ignores time stamps which are later than now+future_secs
    double getIgnoredFutureSecs() const;
    void setIgnoredFutureSecs(Guard &guard, double secs);

    /// Channels w/ period > threshold are scanned, not monitored
    void   setGetThreshold(Guard &guard, double get_threshhold);
    double getGetThreshold();

    /// Engine Info: Started, where, info about writes
    const epicsTime &getStartTime() const { return start_time; }
    const stdString &getIndexName() const { return index_name;  }
    const epicsTime &getNextWriteTime(Guard &guard) const
    { return next_write_time; }
    bool isWriting() const        { return is_writing; }
    
    /// Add channel to ScanList.
    /// If result is false,
    /// channel has to prepare a monitor.
    bool addToScanList(Guard &guard, ArchiveChannel *channel);

    stdString makeDataFileName();
    

private:
    Engine(const stdString &index_name);
    void writeArchive();

    struct ca_client_context *ca_context;
    
    epicsTime       start_time;
    int             RTreeM;
    stdString       index_name;
    stdString       description;
    bool            is_writing;
    
    double          get_threshhold;
    ScanList        scan_list;      // list of scanned, not monitored channels

    double          write_period;   // period between writes to archive file
    int             buffer_reserve; // 2-> alloc. buffs for 2x expected data
    epicsTime       next_write_time;// when to write again
    double          secs_per_file;  // roughly: data file period
    double          future_secs;    // now+_future_secs is considered wrong

#ifdef USE_PASSWD
    stdString       user, pass;
#endif
};

// The only, global Engine object:
extern Engine *theEngine;

inline const stdString &Engine::getDescription() const
{   return description; }

inline double Engine::getGetThreshold()
{   return get_threshhold; }

inline double Engine::getWritePeriod() const
{   return write_period; }

inline int Engine::getBufferReserve() const
{   return buffer_reserve; }

inline double Engine::getIgnoredFutureSecs() const
{   return future_secs; }

inline void Engine::setIgnoredFutureSecs(Guard &guard, double secs)
{
    guard.check(mutex);
    future_secs = secs;
}

inline size_t Engine::suggestedBufferSize(Guard &guard, double period)
{
    guard.check(mutex);
    size_t	num;
	if (write_period <= 0)
		return 100;
    num = (size_t)(write_period * buffer_reserve / period);
	if (num < 3)
		num = 3;
    return num;
}

#endif //__ENGINE_H__
 
