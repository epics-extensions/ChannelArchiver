// --------------------------------------------------------
// $Id$
//
// Please refer to NOTICE.txt,
// included as part of this distribution,
// for legal information.
//
// Kay-Uwe Kasemir, kasemir@lanl.gov
// --------------------------------------------------------

#ifndef __CHANNELINFO_H__
#define __CHANNELINFO_H__

#include <list>
#include <cadef.h>
#include "CircularBuffer.h"
#include "Bitset.h"
#include "ArchiveI.h"

BEGIN_NAMESPACE_CHANARCH
#ifdef USE_NAMESPACE_STD
using std::list;
#endif

class GroupInfo; // forward

// For testing only,
// expects all channels to be 0...10 ramps,
// checks for missing values:
// #define CA_STATISTICS

//CLASS ChannelInfo
// Information about an Engine's Channel to be archived
// (class Channel represents a channel already in the Archive)
class ChannelInfo
{
public:
	ChannelInfo ();
	~ChannelInfo ();

	// Archiver configuration for this channel
	// ------------------------------------------------------------
	void setName (const stdString &name)							{ _name = name; }
	const stdString &getName () const								{ return _name; }

	// How is the current value fetched?
	typedef enum { none, use_monitor, use_get } GetMechanism;
	GetMechanism getMechanism () const							{ return _get_mechanism; }

	const list<GroupInfo *> &getGroups () const					{ return _groups; }
	void addToGroup (GroupInfo *group, bool disabling);

	double getPeriod () const									{ return _period; }
	void setPeriod (double sample);

	// Values --------
	// Define the type of value for this ChannelInfo.
	// Result: has the type changed ?
	bool setValueType (DbrType type, DbrCount count);
	void setCtrlInfo (const CtrlInfoI *info);

	const CtrlInfoI &getCtrlInfo() const						{ return _ctrl_info; }
	size_t getValsPerBuffer () const							{ return _vals_per_buffer; }

	// Channel Access
	// ------------------------------------------------------------
	// monitored or scanned?
	bool isMonitored () const									{ return _monitored; }
	void setMonitored (bool monitored)							{ _monitored = monitored; }
	bool isConnected () const									{ return _connected; }
	const osiTime &getConnectTime () const						{ return _connectTime; }
	const char *getHost () const								{ return ca_host_name (_chid); }

	void startCaConnection (bool new_channel);

	// Issue CA get for _value, no ca_pend_io in here!
	void issueCaGet ();

	// Called by Engine
	// ------------------------------------------------------------

	// Called from caEventHandler or ScanList, _value is already set
	void handleNewValue ();

	// For scanned channels,
	// handleNewValue won't put repeated values
	// in the ring buffer unless there's a change.
	// This call will force it to write the
	// repeat count out up to 'now'.
	size_t flushRepeats (const osiTime &now);

	void addEvent (dbr_short_t status, dbr_short_t severity, const osiTime &time);

	// (Try to) enable/disable this channel
	void disable (ChannelInfo *cause);
	void enable (ChannelInfo *cause);
	// Info about disabling behaviour of this channel
	const BitSet &getDisabling() const						{ return _disabling; }
	bool isDisabling(const GroupInfo *group) const;
	bool isDisabled () const								{ return _disabled >= _groups.size(); }

	void write (class Archive &archive, ChannelIterator &channel);

#ifdef CA_STATISTICS
	int _next_CA_value;
	static size_t _missing_CA_values;
#endif

	// Check if Ring buffer is big enough, fits _value etc.
	void checkRingBuffer ();

private:
	enum
	{
	INIT_VALS_PER_BUF = 16,
	BUF_GROWTH_RATE = 4,
	MAX_VALS_PER_BUF = 1000
	};

	stdString			_name;
	list<GroupInfo *>	_groups;			// Groups that this channel belongs to
	BitSet				_disabling;			// bit indicates if Channel could disable that group
	bool				_currently_disabling;// Does this Channel right now disable groups?
	size_t				_disabled;			// disabled by how many groups?
	bool				_monitored;			// monitored or scanned?
	bool				_connected;			// currently connected via CA?
	osiTime				_connectTime;		// when did _connected change?
	GetMechanism		_get_mechanism;		// mechanism actually used internally
	unsigned short		_vals_per_buffer;	// see enum: INIT_VALS_PER_BUF ...
	double				_period;			// (monitor: expected) period in secs
	chid				_chid;
	CtrlInfoI			_ctrl_info;			// has to be copy, not * !

	bool				_new_value_set;
	ValueI				*_new_value;		// New value buffer for ca_get/monitor

	bool				_pending_value_set;
	ValueI				*_pending_value;	// monitor that arrived but wasn't due to be saved

	bool				_previous_value_set;// for change detection in scanned operation,
	ValueI				*_previous_value;

	ValueI				*_tmp_value;		// for constructing values in addEvent

	ThreadSemaphore		_write_lock;
	ValueI				*_write_value;		// tmp value for write routine (different thread!)

	CircularBuffer		_buffer;			// buffer in memory, later written to disk
	osiTime				_expected_next_time;// next time when this Channel has to be put in circ. buffer
	osiTime				_last_buffer_time;	// last time stamp added to Archive so far
	osiTime				_had_null_time;

	ChannelInfo (const ChannelInfo &rhs); // not defined
	ChannelInfo & operator = (const ChannelInfo &rhs); // not defined

	// CA Callback for connections
	static void caLinkConnectionHandler (struct connection_handler_args arg);

	// These are to be called from CA with ar.user == this
	// Callback for control information
	static void caControlHandler (struct event_handler_args arg);

	// Callback for values (monitored)
	static void caEventHandler (struct event_handler_args arg);

	void addToRingBuffer (const ValueI *value);
	void handleNewScannedValue ();
	void handleDisabling ();
};

inline void ChannelInfo::setCtrlInfo (const CtrlInfoI *info)
{
	LOG_ASSERT (info);
	_ctrl_info = *info;
}

END_NAMESPACE_CHANARCH

#endif //__CHANNELINFO_H__


