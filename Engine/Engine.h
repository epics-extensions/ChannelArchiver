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

#undef ENGINE_DEBUG

// The ArchiveEngine uses these locks:
//
// HTTPServer::client_list_mutex
// -----------------------------
// HTTPServer maintains a list of clients for cleanup,
// but HTTPServer::serverinfo could also be invoked
// by HTTPClientConnection (different thread) for the "/server" URL.
// So far not locked elsewhere.
//
// Engine::mutex
// -------------
// Locks lists of channels, groups, ...
// Almost all of the Engine's methods require a Guard with this mutex.
//
// ArchiveChannel::mutex
// ---------------------
// Locks state & values of one channel.
// Almost all of the Channel's methods require a Guard with this mutex.
//
// The following situations created a deadlock:
//
// 1) Main thread was in Engine::process,
//    locked engine->mutex,
//    called writeArchive, trying to lock a channel for writing.
// 2) The same channel received new control info via ChannelAccess:
//    ArchiveChannel::control_callback had locked the channel
//    and was trying to lock the engine to init. the channel.
// ==> Whoever needs Engine & channel locks needs to first lock the
//     engine, then the channel. Never the other way around!
//
// Many CA library calls take a 'callback lock' to prohibit callbacks.
// That same callback lock is taken inside a CA callback.
// This lead to the following deadlock on Engine shutdown:
// Thread 1: Engine::shutdown()->lock engine,channel
//           -> ca_clear_channel(janet998) -> callback lock
// Thread 2: tcpRecvThread::run -> callback lock
//           -> control_info_callback(janet641) -> lock engine
// i.e. lock order engine, callback lock and then the reverse;
// ==> no locks are allowed when adding/removing
//     a channel or a subscription! Use temporary release.

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

    // Ask engine to stop & delete itself.
    // shutdown() will take the engine's mutex.
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
    void setDescription(Guard &engine_guard, const stdString &description);

    /// Called by other threads (EngineServer)
    /// which need to issue CA calls within the context
    /// of the Engine
    bool attachToCAContext(Guard &engine_guard);

    /// Set to make the Engine flush CA requests in next process call
    bool need_CA_flush;
    
    /// Call this in a "main loop" as often as possible

    // Locks mutex while accessing engine's internals
    bool process();

    /// All the channels of this engine.
    stdList<ArchiveChannel *> &getChannels(Guard &engine_guard);

    /// All the groups.
    stdList<GroupInfo *> &getGroups(Guard &engine_guard);
    
    GroupInfo *findGroup(Guard &engine_guard, const stdString &name);
    GroupInfo *addGroup(Guard &engine_guard, const stdString &name);
    ArchiveChannel *findChannel(Guard &engine_guard, const stdString &name);
    ArchiveChannel *addChannel(Guard &engine_guard, GroupInfo *group,
                               const stdString &channel_name,
                               double period, bool disabling, bool monitored);
    void incNumConnected(Guard &engine_guard);
    void decNumConnected(Guard &engine_guard);
    size_t getNumConnected(Guard &engine_guard);
    
    /// Engine configuration
    void   setWritePeriod(Guard &engine_guard, double period);
    double getWritePeriod() const;   
    int    getBufferReserve() const;
    void   setBufferReserve(Guard &engine_guard, int reserve);

    /// Determine the suggested buffer size for a value
    /// with given scan period based on how often we write
    /// and the buffer reserve
    size_t suggestedBufferSize(Guard &engine_guard, double period);
    
    /// Engine ignores time stamps which are later than now+future_secs
    double getIgnoredFutureSecs() const;
    void setIgnoredFutureSecs(Guard &engine_guard, double secs);

    /// Channels w/ period > threshold are scanned, not monitored
    void   setGetThreshold(Guard &engine_guard, double get_threshhold);
    double getGetThreshold();

    /// Engine Info: Started, where, info about writes
    const epicsTime &getStartTime() const { return start_time; }
    const stdString &getIndexName() const { return index_name;  }
    const epicsTime &getNextWriteTime(Guard &engine_guard) const
    {
        engine_guard.check(mutex);
        return next_write_time;
    }
    bool isWriting() const        { return is_writing; }
    
    /// Get Engine's scan list
    ScanList &getScanlist(Guard &engine_guard)
    {
        engine_guard.check(mutex);
        return scan_list;
    }
    
    stdString makeDataFileName();

private:
    Engine(const stdString &index_name); // use create
    ~Engine() {} // Use shutdown
    void writeArchive(Guard &engine_guard);

    stdList<ArchiveChannel *> channels;// all the channels
    stdList<GroupInfo *> groups;    // scan-groups of channels
    size_t num_connected;
    
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
    double          future_secs;    // now+_future_secs is considered wrong

#ifdef USE_PASSWD
    stdString       user, pass;
#endif
};

// The only, global Engine object:
extern Engine *theEngine;

inline stdList<ArchiveChannel *> &Engine::getChannels(Guard &engine_guard)
{
    engine_guard.check(mutex);
    return channels;
}

inline stdList<GroupInfo *> &Engine::getGroups(Guard &engine_guard)
{
    engine_guard.check(mutex);
    return groups;
}

inline const stdString &Engine::getDescription() const
{   return description; }

inline double Engine::getGetThreshold()
{   return get_threshhold; }


inline void Engine::incNumConnected(Guard &engine_guard)
{
    engine_guard.check(mutex);
    ++num_connected;
}

inline void Engine::decNumConnected(Guard &engine_guard)
{
    engine_guard.check(mutex);
    if (num_connected <= 0)
        LOG_MSG("Engine: discrepancy w/ # of connected channels\n");
    else
        --num_connected;
}

inline size_t Engine::getNumConnected(Guard &engine_guard)
{
    engine_guard.check(mutex);
    return num_connected;
}

inline double Engine::getWritePeriod() const
{   return write_period; }

inline int Engine::getBufferReserve() const
{   return buffer_reserve; }

inline double Engine::getIgnoredFutureSecs() const
{   return future_secs; }

inline void Engine::setIgnoredFutureSecs(Guard &engine_guard, double secs)
{
    engine_guard.check(mutex);
    future_secs = secs;
}

inline size_t Engine::suggestedBufferSize(Guard &engine_guard, double period)
{
    engine_guard.check(mutex);
    size_t	num;
	if (write_period <= 0)
		return 100;
    num = (size_t)(write_period * buffer_reserve / period);
	if (num < 3)
		num = 3;
    return num;
}

#endif //__ENGINE_H__
 
