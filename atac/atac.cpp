// --------------------------------------------------------
// $Id$
//
// Please refer to NOTICE.txt,
// included as part of this distribution,
// for legal information.
//
// Kay-Uwe Kasemir, kasemir@lanl.gov
// --------------------------------------------------------
//
// A T A C . c p p 
//
// "A Tcl Archive Client"
//
// Could be based on swig?
// Almost long enough to be split in several sources?
//

#define DLL_BUILD
#define BUILD_atac
#include "atacInt.h"

#if TCL_MAJOR_VERSION < 8
#error TCL version 8 or better required
#endif

// "Un-const char" cast
// (most tcl routines take a (char *) instead of a (const char *)
#define UCC(c)	const_cast<char *>(c)

// ----------------------------------------------------------------
//
// H a n d l e s
//
// ----------------------------------------------------------------
// All ArchiveI, ChannelIteratorI, ... pointers
// are kept in this table:
static Tcl_HashTable	handles;

class Handle
{
public:
	typedef enum { Undef, Archive, Channel, Value } Type;
	Handle ()
	{
		_type = Undef;
	}

	virtual ~Handle ()
	{}

	Type getType () const
	{	return _type; }

protected:
		Type	_type;
};

class ArchiveHandle : public Handle
{
public:
	ArchiveHandle (ArchiveI *archive)
	{
		_archive = archive;
		_type = Archive;
	}

	~ArchiveHandle ()
	{	delete _archive; }

	ArchiveI *getArchive () const
	{	return _archive; }

private:
	ArchiveI *_archive;
};

// Not based on ArchiveHandle because while
// ChannelHandle has an ArchiveI *, it doesn't delete it!
class ChannelHandle : public Handle
{
public:
	ChannelHandle (ArchiveI *archive, ChannelIteratorI *channel)
	{
		_archive = archive;
		_channel = channel;
		_type = Channel;
	}

	~ChannelHandle ()
	{	delete _channel; }

	ArchiveI *getArchive () const
	{	return _archive; }

	ChannelIteratorI *getChannel () const
	{	return _channel; }

private:
	ArchiveI			*_archive;
	ChannelIteratorI	*_channel;
};

class ValueHandle : public Handle
{
public:
	ValueHandle (ArchiveI *archive, ChannelIteratorI *channel, ValueIteratorI *value)
	{
		_archive = archive;
		_channel = channel;
		//_unexpanded_value = value;
		//_value = new ExpandingValueIteratorI (_unexpanded_value);
		_value = value;
		_type = Value;
	}
	~ValueHandle ()
	{
		delete _value;
		//delete _unexpanded_value;
	}

	ArchiveI *getArchive () const
	{	return _archive; }

	ChannelIteratorI *getChannel () const
	{	return _channel; }

	ValueIteratorI *getValue () const
	{	return _value; }

private:
	ArchiveI			*_archive;
	ChannelIteratorI	*_channel;
	//ValueIteratorI	*_unexpanded_value;
	ValueIteratorI		*_value;
};

// Create new handle in hash table,
// set interp's return value to that handle
static int newHandle (Tcl_Interp *interp, const Handle *handle)
{
	static size_t next_handle = 0;
	char key[40];

	if (! handle)
	{
		Tcl_AddErrorInfo (interp, "Empty Handle, internal ATAC error");
		return TCL_ERROR;
	}

	switch (handle->getType())
	{
	case Handle::Archive:	sprintf (key, "archive%d", ++next_handle); break;
	case Handle::Channel:	sprintf (key, "channel%d", ++next_handle); break;
	case Handle::Value: 	sprintf (key, "value%d",   ++next_handle); break;
	}

	int new_entry;
	Tcl_HashEntry *entry = Tcl_CreateHashEntry (&handles, key, &new_entry);
	Tcl_SetHashValue (entry, handle);

	Tcl_Obj	*result = Tcl_NewStringObj (key, -1);
	Tcl_SetObjResult (interp, result);

	return TCL_OK;
}

// Get a Handle from the "entries" Hash
static ArchiveHandle *getArchiveHandle (const char *key, Tcl_HashEntry **entryPtr = 0)
{
	if (!key)
		return 0;
	Tcl_HashEntry *entry = Tcl_FindHashEntry (&handles, key);
	if (!entry)
		return 0;

	ArchiveHandle * handle =  (ArchiveHandle *) Tcl_GetHashValue (entry);
	if (handle->getType() != Handle::Archive)
		return 0;

	if (entryPtr)
		*entryPtr = entry;
		
	return handle;
}

static ChannelHandle *getChannelHandle (const char *key, Tcl_HashEntry **entryPtr = 0)
{
	if (!key)
		return 0;
	Tcl_HashEntry *entry = Tcl_FindHashEntry (&handles, key);
	if (!entry)
		return 0;

	ChannelHandle * handle =  (ChannelHandle *) Tcl_GetHashValue (entry);
	if (!handle
		|| handle->getType() != Handle::Channel
		|| !handle->getChannel()
		|| !handle->getChannel()
		|| !handle->getChannel()->isValid())
		return 0;

	if (entryPtr)
		*entryPtr = entry;

	return handle;
}

static ValueHandle *getValueHandle (const char *key, bool check_valid=true, Tcl_HashEntry **entryPtr = 0)
{
	if (!key)
		return 0;
	Tcl_HashEntry *entry = Tcl_FindHashEntry (&handles, key);
	if (!entry)
		return 0;

	ValueHandle * handle =  (ValueHandle *) Tcl_GetHashValue (entry);
	if (! handle
		|| handle->getType() != Handle::Value
		|| !handle->getValue())
		return 0;
		
	if (check_valid && !handle->getValue()->isValid())
		return 0;

	if (entryPtr)
		*entryPtr = entry;

	return handle;
}

// ----------------------------------------------------------------
// o s i T i m e

// Convert into time text format...
//
static const char *osi2txt (const osiTime &osi)
{
	static char txt[80];
	int year, month, day, hour, min, sec;
	unsigned long nano;

	osiTime2vals (osi, year, month, day, hour, min, sec, nano);

	sprintf (txt, "%4d/%02d/%02d %02d:%02d:%02d.%09d",
		year, month, day, hour, min, sec, nano);
	return txt;
}

// Convert from time text in "YYYY/MM/DD hh:mm:ss" format
// as used by the archiver with 24h hours and (maybe) fractional seconds
//
static bool text2osi (const char *text, osiTime &osi)
{
	int	year, month, day, hour, minute;
	double second;

	if (sscanf (text, "%04d/%02d/%02d %02d:%02d:%lf",
		&year, &month, &day,
		&hour, &minute, &second)  != 6)
		return false;

	int secs = (int) second;
	unsigned long nano = (unsigned long) ((second - secs) * 1000000000L);

	vals2osiTime (year, month, day, hour, minute, secs, nano, osi);

	return true;
}

// ----------------------------------------------------------------
//COMMAND archive
//
// <B>Syntax:</B> archive subcommand ?options?
// <P>
// This command provides the starting point for accessing archived data.
//
// See also: CLASS channel.

// --------------------------------------------------------------------------
//*	<B>Syntax:</B> <I>archive open archiveName</I>
//
// <B>Returns:</B> archiveId
//
// When the archive can be opened for read access,
// an archiveId is returned,
// otherwise $errorInfo is set.

static int archive_open (ClientData clientData, Tcl_Interp *interp,
						int objc, struct Tcl_Obj * CONST objv[])
{
	try
	{
		ArchiveI *archive = new ATAC_ARCHIVE_TYPE (Tcl_GetStringFromObj (objv[2], 0));
		return newHandle (interp, new ArchiveHandle (archive));
	}
	catch (ArchiveException &e)
	{
		Tcl_AddErrorInfo (interp, UCC(e.what ()));
		return TCL_ERROR;
	}

}

// --------------------------------------------------------------------------
//*	<B>Syntax:</B> <I>archive findFirstChannel archiveId</I>
//
//	<B>Returns:</B> channelId.
//
// Always returns a channelId as long as the archiveId is valid.
// Use <I>channel valid channelId</I> to check if the channelId
// holds a valid channel.

static int archive_findFirstChannel (ClientData clientData, Tcl_Interp *interp,
						int objc, struct Tcl_Obj * CONST objv[])
{
	const char *key = Tcl_GetStringFromObj (objv[2], 0);
	const ArchiveHandle *handle = getArchiveHandle (key);
	if (!handle)
	{
		Tcl_AddErrorInfo (interp, "invalid archiveId");
		return TCL_ERROR;
	}

	try
	{
		ArchiveI *archive = handle->getArchive ();
		ChannelIteratorI *channel = archive->newChannelIterator ();
		if (archive->findFirstChannel (channel))
			return newHandle (interp, new ChannelHandle (archive, channel));
		delete channel;
		return TCL_OK;
	}
	catch (ArchiveException &e)
	{
		Tcl_AddErrorInfo (interp, UCC(e.what ()));
		return TCL_ERROR;
	}
}

// --------------------------------------------------------------------------
//*	<B>Syntax:</B> <I>archive findChannelByName archiveId channelName</I>
//
//	<B>Returns:</B> channelId.
//
// Similar to <I>findFirstChannel</I>...

static int archive_findChannelByName (ClientData clientData, Tcl_Interp *interp,
						int objc, struct Tcl_Obj * CONST objv[])
{
	const char *key = Tcl_GetStringFromObj (objv[2], 0);
	const ArchiveHandle *handle = getArchiveHandle (key);
	if (!handle)
	{
		Tcl_AddErrorInfo (interp, "invalid archiveId");
		return TCL_ERROR;
	}

	try
	{
		ArchiveI *archive = handle->getArchive ();
		ChannelIteratorI *channel = archive->newChannelIterator ();
		if (archive->findChannelByName (Tcl_GetStringFromObj (objv[3], 0), channel))
			return newHandle (interp, new ChannelHandle (archive, channel));
		delete channel;
		return TCL_OK;
	}
	catch (ArchiveException &e)
	{
		Tcl_AddErrorInfo (interp, UCC(e.what ()));
		return TCL_ERROR;
	}
}

// --------------------------------------------------------------------------
//*	<B>Syntax:</B> <I>archive findChannelByPattern archiveId patternString</I>
//
//	<B>Returns:</B> channelId.
//
// Similar to <I>findChannelByName</I>,
// but <I>patternString</I> is a regular expression.
//
// Calling <I>channel next</I> on the resulting channelId will
// try to find the next matching(!) channel.

static int archive_findChannelByPattern (ClientData clientData, Tcl_Interp *interp,
						int objc, struct Tcl_Obj * CONST objv[])
{
	const char *key = Tcl_GetStringFromObj (objv[2], 0);
	const ArchiveHandle *handle = getArchiveHandle (key);
	if (!handle)
	{
		Tcl_AddErrorInfo (interp, "invalid archiveId");
		return TCL_ERROR;
	}

	try
	{
		ArchiveI *archive = handle->getArchive ();
		ChannelIteratorI *channel = archive->newChannelIterator ();
		if (archive->findChannelByPattern (Tcl_GetStringFromObj (objv[3], 0), channel))
			return newHandle (interp, new ChannelHandle (archive, channel));
		delete channel;
		return TCL_OK;
	}
	catch (ArchiveException &e)
	{
		Tcl_AddErrorInfo (interp, UCC(e.what ()));
		return TCL_ERROR;
	}
}

// --------------------------------------------------------------------------
//*	<B>Syntax:</B> <I>archive close archiveId</I>
//
//	Invalidates archiveId as well as all related channelIds etc.<BR>
//	(Does not return error when Id was invalid)

static int archive_close (ClientData clientData, Tcl_Interp *interp,
						int objc, struct Tcl_Obj * CONST objv[])
{
	Tcl_HashEntry *entry;
	const char *key = Tcl_GetStringFromObj (objv[2], 0);
	ArchiveHandle *handle = getArchiveHandle (key, &entry);
	if (handle)
	{
		delete handle;
		Tcl_DeleteHashEntry (entry);
	}
	return TCL_OK;
}

typedef struct
{
	const char		*options;
	int				num_opts;
	Tcl_ObjCmdProc	*proc;
}	CmdInfo;


static char *archive_cmds[] = 
{ "open", "findFirstChannel", "findChannelByName", "findChannelByPattern", "close", 0 };

static CmdInfo archive_cmd_info[] = 
{
	{ "open archiveName", 2, archive_open },
	{ "findFirstChannel archiveId", 2, archive_findFirstChannel },
	{ "findChannelByName archiveId channelName", 3, archive_findChannelByName },
	{ "findChannelByPattern archiveId patternString", 3, archive_findChannelByPattern },
	{ "close archiveId", 2, archive_close }
};

// --------------------------------------------------------------------------
//		archive ...
//
//	checks arguments and dispatches to subcommands, see above
// --------------------------------------------------------------------------
DLLEXPORT int atac_archiveCmd (ClientData clientData,
	Tcl_Interp *interp, int objc, struct Tcl_Obj * CONST objv[])
{
	// Test args.
	if (objc < 2)
	{
		Tcl_WrongNumArgs (interp, 1, objv, "subcommand ?options?");
		return TCL_ERROR;
	}

	int cmd;
	if (Tcl_GetIndexFromObj (interp, objv[1], archive_cmds, "subcommand", 0, &cmd) == TCL_ERROR)
		return TCL_ERROR;

	if (objc-1 != archive_cmd_info[cmd].num_opts)
	{
		Tcl_WrongNumArgs (interp, 1, objv, UCC(archive_cmd_info[cmd].options));
		return TCL_ERROR;
	}

	return archive_cmd_info[cmd].proc (clientData, interp, objc, objv);
}

// ----------------------------------------------------------------
//COMMAND channel
//
// <B>Syntax:</B> channel subcommand ?options?
//
// <P>
// See the documentation for CLASS archive on how to obtain a channelId.
//
// See also: CLASS value.


// --------------------------------------------------------------------------
//*	<B>Syntax:</B> <I>channel valid channelId</I>
//
// <B>Returns:</B> 0 or 1
//
// Use this in loops over channels to check if the channelId
// references a valid entry.
//
// The following subcommands are only defined when a channelId is valid!

static int channel_valid (ClientData clientData, Tcl_Interp *interp,
						int objc, struct Tcl_Obj * CONST objv[])
{
	const char *key = Tcl_GetStringFromObj (objv[2], 0);
	if (getChannelHandle (key))
		Tcl_SetObjResult (interp, Tcl_NewIntObj (1));
	else
		Tcl_SetObjResult (interp, Tcl_NewIntObj (0));

	return TCL_OK;
}

// --------------------------------------------------------------------------
//*	<B>Syntax:</B> <I>channel name channelId</I>
//
// <B>Returns:</B> string

static int channel_name (ClientData clientData, Tcl_Interp *interp,
						int objc, struct Tcl_Obj * CONST objv[])
{
	const char *key = Tcl_GetStringFromObj (objv[2], 0);
	const ChannelHandle * handle = getChannelHandle (key);
	if (handle)
		Tcl_SetObjResult (interp,
			Tcl_NewStringObj (UCC(handle->getChannel()->getChannel()->getName()), -1));
	else
		Tcl_SetObjResult (interp, Tcl_NewStringObj ("<invalid channel>", -1));
	return TCL_OK;
}

// --------------------------------------------------------------------------
//*	<B>Syntax:</B> <I>channel next channelId</I>
//
// <B>Returns:</B> 0 or 1
//
// Make channelId reference next channel
// (in no specific order).
//
// The return values is the same as <I>channel valid</I>

static int channel_next (ClientData clientData, Tcl_Interp *interp,
						int objc, struct Tcl_Obj * CONST objv[])
{
	const char *key = Tcl_GetStringFromObj (objv[2], 0);
	const ChannelHandle * handle = getChannelHandle (key);
	if (!handle)
	{
		Tcl_AddErrorInfo (interp, "invalid channelId");
		return TCL_ERROR;
	}

	try
	{
		if (handle->getChannel()->next())
			Tcl_SetObjResult (interp, Tcl_NewIntObj (1));
		else
			Tcl_SetObjResult (interp, Tcl_NewIntObj (0));
	}
	catch (ArchiveException &e)
	{
		Tcl_AddErrorInfo (interp, UCC(e.what ()));
		return TCL_ERROR;
	}
	return TCL_OK;
}

// --------------------------------------------------------------------------
//*	<B>Syntax:</B> <I>channel getFirstTime channelId</I>
//
// <B>Returns:</B> timestamp
//
// Get stamp for when this channel was first archived.

static int channel_getFirstTime (ClientData clientData, Tcl_Interp *interp,
						int objc, struct Tcl_Obj * CONST objv[])
{
	const char *key = Tcl_GetStringFromObj (objv[2], 0);
	const ChannelHandle * handle = getChannelHandle (key);
	if (!handle)
	{
		Tcl_AddErrorInfo (interp, "invalid channelId");
		return TCL_ERROR;
	}

	const char *time = osi2txt (handle->getChannel()->getChannel()->getFirstTime());
	Tcl_SetObjResult (interp, Tcl_NewStringObj (UCC(time), -1));
	return TCL_OK;
}

// --------------------------------------------------------------------------
//*	<B>Syntax:</B> <I>channel getLastTime channelId</I>
//
// <B>Returns:</B> timestamp
//
// Get timestamp for when this channel was last archived.

static int channel_getLastTime (ClientData clientData, Tcl_Interp *interp,
						int objc, struct Tcl_Obj * CONST objv[])
{
	const char *key = Tcl_GetStringFromObj (objv[2], 0);
	const ChannelHandle * handle = getChannelHandle (key);
	if (!handle)
	{
		Tcl_AddErrorInfo (interp, "invalid channelId");
		return TCL_ERROR;
	}

	const char *time = osi2txt (handle->getChannel()->getChannel()->getLastTime());
	Tcl_SetObjResult (interp, Tcl_NewStringObj (UCC(time), -1));
	return TCL_OK;
}

// --------------------------------------------------------------------------
//*	<B>Syntax:</B> <I>channel getFirstValue channelId</I>
//
// <B>Returns:</B> valueId
//
// Return first value that's in the archive.

static int channel_getFirstValue (ClientData clientData, Tcl_Interp *interp,
						int objc, struct Tcl_Obj * CONST objv[])
{
	const char *key = Tcl_GetStringFromObj (objv[2], 0);
	const ChannelHandle * handle = getChannelHandle (key);
	if (!handle)
	{
		Tcl_AddErrorInfo (interp, "invalid channelId");
		return TCL_ERROR;
	}
	ArchiveI *archive = handle->getArchive();
	ChannelIteratorI *channel = handle->getChannel();
	try
	{
		ValueIteratorI *value = archive->newValueIterator ();
		if (channel->getChannel()->getFirstValue (value))
			return newHandle (interp, new ValueHandle (archive, channel, value));
		delete value;
	}
	catch (ArchiveException &e)
	{
		Tcl_AddErrorInfo (interp, UCC(e.what ()));
		return TCL_ERROR;
	}
	return TCL_OK;
}

// --------------------------------------------------------------------------
//*	<B>Syntax:</B> <I>channel getLastValue channelId</I>
//
// <B>Returns:</B> valueId
//
// Return last value that's in the archive.

static int channel_getLastValue (ClientData clientData, Tcl_Interp *interp,
						int objc, struct Tcl_Obj * CONST objv[])
{
	const char *key = Tcl_GetStringFromObj (objv[2], 0);
	const ChannelHandle * handle = getChannelHandle (key);
	if (!handle)
	{
		Tcl_AddErrorInfo (interp, "invalid channelId");
		return TCL_ERROR;
	}
	ArchiveI *archive = handle->getArchive();
	ChannelIteratorI *channel = handle->getChannel();
	try
	{
		ValueIteratorI *value = archive->newValueIterator ();
		if (channel->getChannel()->getLastValue (value))
			return newHandle (interp, new ValueHandle (archive, channel, value));
		delete value;
	}
	catch (ArchiveException &e)
	{
		Tcl_AddErrorInfo (interp, UCC(e.what ()));
		return TCL_ERROR;
	}
	return TCL_OK;
}

// --------------------------------------------------------------------------
//*	<B>Syntax:</B> <I>channel getValueAfterTime channelId timestamp</I>
//
// <B>Returns:</B> valueId
//
// Return first value in the archive that's following the given timestamp.

static int channel_getValueAfterTime (ClientData clientData, Tcl_Interp *interp,
						int objc, struct Tcl_Obj * CONST objv[])
{
	const char *key = Tcl_GetStringFromObj (objv[2], 0);
	const ChannelHandle * handle = getChannelHandle (key);
	if (!handle)
	{
		Tcl_AddErrorInfo (interp, "invalid channelId");
		return TCL_ERROR;
	}
	osiTime	time;
	if (! text2osi (Tcl_GetStringFromObj (objv[3], 0), time))
	{
		Tcl_AddErrorInfo (interp, "invalid timestamp");
		return TCL_ERROR;
	}
	ArchiveI *archive = handle->getArchive();
	ChannelIteratorI *channel = handle->getChannel();
	try
	{
		ValueIteratorI *value = archive->newValueIterator ();
		if (channel->getChannel()->getValueAfterTime (time, value))
			return newHandle (interp, new ValueHandle (archive, channel, value));
		delete value;
		return TCL_OK;
	}
	catch (ArchiveException &e)
	{
		Tcl_AddErrorInfo (interp, UCC(e.what ()));
		return TCL_ERROR;
	}
}

// --------------------------------------------------------------------------
//*	<B>Syntax:</B> <I>channel getValueNearTime channelId timestamp</I>
//
// <B>Returns:</B> valueId
//
// Return first value in the archive that's following the given timestamp.

static int channel_getValueNearTime (ClientData clientData, Tcl_Interp *interp,
						int objc, struct Tcl_Obj * CONST objv[])
{
	const char *key = Tcl_GetStringFromObj (objv[2], 0);
	const ChannelHandle * handle = getChannelHandle (key);
	if (!handle)
	{
		Tcl_AddErrorInfo (interp, "invalid channelId");
		return TCL_ERROR;
	}
	osiTime	time;
	if (! text2osi (Tcl_GetStringFromObj (objv[3], 0), time))
	{
		Tcl_AddErrorInfo (interp, "invalid timestamp");
		return TCL_ERROR;
	}

	ArchiveI *archive = handle->getArchive();
	ChannelIteratorI *channel = handle->getChannel();
	try
	{
		ValueIteratorI *value = archive->newValueIterator ();
		if (channel->getChannel()->getValueNearTime (time, value))
			return newHandle (interp, new ValueHandle (archive, channel, value));
		delete value;
		return TCL_OK;
	}
	catch (ArchiveException &e)
	{
		Tcl_AddErrorInfo (interp, UCC(e.what ()));
		return TCL_ERROR;
	}
}

// --------------------------------------------------------------------------
//*	<B>Syntax:</B> <I>channel getValueBeforeTime channelId timestamp</I>
//
// <B>Returns:</B> valueId
//
// Return last value in the archive that's immediately before the given timestamp.

static int channel_getValueBeforeTime (ClientData clientData, Tcl_Interp *interp,
						int objc, struct Tcl_Obj * CONST objv[])
{
	const char *key = Tcl_GetStringFromObj (objv[2], 0);
	const ChannelHandle * handle = getChannelHandle (key);
	if (!handle)
	{
		Tcl_AddErrorInfo (interp, "invalid channelId");
		return TCL_ERROR;
	}
	osiTime	time;
	if (! text2osi (Tcl_GetStringFromObj (objv[3], 0), time))
	{
		Tcl_AddErrorInfo (interp, "invalid timestamp");
		return TCL_ERROR;
	}
	ArchiveI *archive = handle->getArchive();
	ChannelIteratorI *channel = handle->getChannel();
	try
	{
		ValueIteratorI *value = archive->newValueIterator ();
		if (channel->getChannel()->getValueBeforeTime (time, value))
			return newHandle (interp, new ValueHandle (archive, channel, value));
		delete value;
		return TCL_OK;
	}
	catch (ArchiveException &e)
	{
		Tcl_AddErrorInfo (interp, UCC(e.what ()));
		return TCL_ERROR;
	}
}

// --------------------------------------------------------------------------
//*	<B>Syntax:</B> <I>channel close channelId</I>
//
//	Invalidate channelId when no longer used.<BR>
//	(Does not return error when Id was invalid)

static int channel_close (ClientData clientData, Tcl_Interp *interp,
						int objc, struct Tcl_Obj * CONST objv[])
{
	Tcl_HashEntry *entry;
	const char *key = Tcl_GetStringFromObj (objv[2], 0);
	ChannelHandle * handle = getChannelHandle (key, &entry);
	if (handle)
	{
		delete handle;
		Tcl_DeleteHashEntry (entry);
	}
	return TCL_OK;
}

static char *channel_cmds[] = 
{	"valid", "name", "next", "getFirstTime", "getLastTime",
	"getFirstValue", "getLastValue",
	"getValueAfterTime", "getValueNearTime", "getValueBeforeTime", "close", 0
};

static CmdInfo channel_cmd_info[] = 
{
	{ "valid channelId", 2, channel_valid },
	{ "name channelId", 2, channel_name },
	{ "next channelId", 2, channel_next },
	{ "getFirstTime channelId", 2, channel_getFirstTime },
	{ "getLastTime channelId", 2, channel_getLastTime },
	{ "getFirstValue channelId", 2, channel_getFirstValue },
	{ "getLastValue channelId", 2, channel_getLastValue },
	{ "getValueAfterTime channelId timestamp",  3, channel_getValueAfterTime },
	{ "getValueNearTime channelId timestamp",   3, channel_getValueNearTime },
	{ "getValueBeforeTime channelId timestamp", 3, channel_getValueBeforeTime },
	{ "close channelId", 2, channel_close },
};

// --------------------------------------------------------------------------
//		channel ...
//
//	checks arguments and dispatches to subcommands, see above
// --------------------------------------------------------------------------
DLLEXPORT int atac_channelCmd (ClientData clientData,
	Tcl_Interp *interp, int objc, struct Tcl_Obj * CONST objv[])
{
	// Test args.
	if (objc < 2)
	{
		Tcl_WrongNumArgs (interp, 1, objv, "subcommand ?options?");
		return TCL_ERROR;
	}

	int cmd;
	if (Tcl_GetIndexFromObj (interp, objv[1], channel_cmds, "subcommand", 0, &cmd) == TCL_ERROR)
		return TCL_ERROR;

	if (objc-1 != channel_cmd_info[cmd].num_opts)
	{
		Tcl_WrongNumArgs (interp, 1, objv, UCC(channel_cmd_info[cmd].options));
		return TCL_ERROR;
	}

	return channel_cmd_info[cmd].proc (clientData, interp, objc, objv);
}

// ----------------------------------------------------------------
//COMMAND value
//
// <B>Syntax:</B> value subcommand ?options?
// <P>
//
// See also: CLASS archive,
// CLASS channel.

//*	<B>Syntax:</B> <I>value valid valueId</I>
//
// <B>Returns:</B> 0 or 1
//
// Use this in loops over values to check if the valueId
// references a valid entry.
//
// The following subcommands are only defined when a valueId is valid!

static int value_valid (ClientData clientData, Tcl_Interp *interp,
						int objc, struct Tcl_Obj * CONST objv[])
{
	const char *key = Tcl_GetStringFromObj (objv[2], 0);
	if (getValueHandle (key))
		Tcl_SetObjResult (interp, Tcl_NewIntObj (1));
	else
		Tcl_SetObjResult (interp, Tcl_NewIntObj (0));

	return TCL_OK;
}

//*	<B>Syntax:</B> <I>value isInfo valueId</I>
//
// <B>Returns:</B> 0 or 1
//
// Some values are not numeric values but purely informational.
//
// In that case <I>value status</I>  returns
//
// <UL>
// <LI>Archive_Off
// <LI>Disconnected
// <LI>Disabled
// <LI>... maybe more
// </UL>
//
// To check for such "info" values, this command can be used.

static int value_isInfo (ClientData clientData, Tcl_Interp *interp,
						int objc, struct Tcl_Obj * CONST objv[])
{
	const char *key = Tcl_GetStringFromObj (objv[2], 0);
	const ValueHandle *handle = getValueHandle (key);
	if (!handle)
	{
		Tcl_SetObjResult (interp, Tcl_NewIntObj (0));
		return TCL_OK;
	}
	if (handle->getValue()->getValue()->isInfo())
		Tcl_SetObjResult (interp, Tcl_NewIntObj (1));
	else
		Tcl_SetObjResult (interp, Tcl_NewIntObj (0));

	return TCL_OK;
}

// --------------------------------------------------------------------------
//*	<B>Syntax:</B> <I>value get valueId</I>
//
// <B>Returns:</B> numeric/double, list if valueId references array
//
// Get numeric value of current valueId.
//
// (arrays/lists not implemented, yet)

static int value_get (ClientData clientData, Tcl_Interp *interp,
						int objc, struct Tcl_Obj * CONST objv[])
{
	const char *key = Tcl_GetStringFromObj (objv[2], 0);
	const ValueHandle *handle = getValueHandle (key);
	if (!handle)
	{
		Tcl_AddErrorInfo (interp, "invalid valueId");
		return TCL_ERROR;
	}

	try
	{
		const ValueI *value = handle->getValue()->getValue();
		size_t count = value->getCount();
		int res;

		if (count == 1)
			Tcl_SetObjResult (interp, Tcl_NewDoubleObj (value->getDouble()));
		else
		{
			Tcl_Obj *list = Tcl_NewListObj (0, 0);

			for (size_t i=0; i<count; ++i)
			{
				res = Tcl_ListObjAppendElement (interp, list, Tcl_NewDoubleObj (value->getDouble(i)));
				if (res != TCL_OK)
					return res;
			}
			Tcl_SetObjResult (interp, list);
		}
	}
	catch (ArchiveException &e)
	{
		Tcl_AddErrorInfo (interp, UCC(e.what ()));
		return TCL_ERROR;
	}
	return TCL_OK;
}

// --------------------------------------------------------------------------
//*	<B>Syntax:</B> <I>value text valueId</I>
//
// <B>Returns:</B> text
//
// Get current value of current valueId formatted as text.
// <br>
// For enumerated values this function will return
// the state text assiciated with the value
// while <I>value get</I> would only provide
// the integer code.
//
// (arrays/lists not implemented, yet)

static int value_text (ClientData clientData, Tcl_Interp *interp,
						int objc, struct Tcl_Obj * CONST objv[])
{
	const char *key = Tcl_GetStringFromObj (objv[2], 0);
	const ValueHandle *handle = getValueHandle (key);
	if (!handle)
	{
		Tcl_AddErrorInfo (interp, "invalid valueId");
		return TCL_ERROR;
	}

	try
	{
		ValueIteratorI *value = handle->getValue();
		stdString txt;
		value->getValue()->getValue(txt);
		Tcl_SetObjResult (interp, 
			Tcl_NewStringObj (UCC(txt.c_str()), txt.length()));
	}
	catch (ArchiveException &e)
	{
		Tcl_AddErrorInfo (interp, UCC(e.what ()));
		return TCL_ERROR;
	}
	return TCL_OK;
}

// --------------------------------------------------------------------------
//*	<B>Syntax:</B> <I>value time valueId</I>
//
// <B>Returns:</B> time-string as "yyyy/mm/dd hh:mm:ss.nanosec"
//
// Get time stamp of current valueId.
//
// The time string is in an ASCII-sortable format with 24h hours,
// which is useful for display mostly.
// For further calculations you might have to split it into numbers.

static int value_time (ClientData clientData, Tcl_Interp *interp,
						int objc, struct Tcl_Obj * CONST objv[])
{
	const char *key = Tcl_GetStringFromObj (objv[2], 0);
	const ValueHandle *handle = getValueHandle (key);
	if (!handle)
	{
		Tcl_AddErrorInfo (interp, "invalid valueId");
		return TCL_ERROR;
	}

	try
	{
		ValueIteratorI *value = handle->getValue();
		Tcl_SetObjResult (interp,
			Tcl_NewStringObj (UCC(osi2txt (value->getValue()->getTime())), -1));
	}
	catch (ArchiveException &e)
	{
		Tcl_AddErrorInfo (interp, UCC(e.what ()));
		return TCL_ERROR;
	}
	return TCL_OK;
}

// --------------------------------------------------------------------------
//*	<B>Syntax:</B> <I>value status valueId</I>
//
// <B>Returns:</B> status-string
//
// Get status of current valueId.

static int value_status (ClientData clientData, Tcl_Interp *interp,
						int objc, struct Tcl_Obj * CONST objv[])
{
	const char *key = Tcl_GetStringFromObj (objv[2], 0);
	const ValueHandle *handle = getValueHandle (key);
	if (!handle)
	{
		Tcl_AddErrorInfo (interp, "invalid valueId");
		return TCL_ERROR;
	}

	try
	{
		ValueIteratorI *value = handle->getValue();
		stdString txt;
		value->getValue()->getStatus(txt);
		Tcl_SetObjResult (interp, 
			Tcl_NewStringObj (UCC(txt.c_str()), txt.length()));
	}
	catch (ArchiveException &e)
	{
		Tcl_AddErrorInfo (interp, UCC(e.what ()));
		return TCL_ERROR;
	}
	return TCL_OK;
}

// --------------------------------------------------------------------------
//*	<B>Syntax:</B> <I>value next valueId</I>
//
// <B>Returns:</B> 0 or 1
//
// Make valueId reference next value
//
// The return values is the same as <I>value valid</I>

static int value_next (ClientData clientData, Tcl_Interp *interp,
						int objc, struct Tcl_Obj * CONST objv[])
{
	const char *key = Tcl_GetStringFromObj (objv[2], 0);
	const ValueHandle *handle = getValueHandle (key, false);
	if (!handle)
	{
		Tcl_AddErrorInfo (interp, "invalid valueId");
		return TCL_ERROR;
	}

	try
	{
		ValueIteratorI *value = handle->getValue();
		if (value->next())
			Tcl_SetObjResult (interp, Tcl_NewIntObj (1));
		else
			Tcl_SetObjResult (interp, Tcl_NewIntObj (0));
	}
	catch (ArchiveException &e)
	{
		Tcl_AddErrorInfo (interp, UCC(e.what ()));
		return TCL_ERROR;
	}
	return TCL_OK;
}

// --------------------------------------------------------------------------
//*	<B>Syntax:</B> <I>value prev valueId</I>
//
// <B>Returns:</B> 0 or 1
//
// Make valueId reference previous value
//
// The return values is the same as <I>value valid</I>

static int value_prev (ClientData clientData, Tcl_Interp *interp,
						int objc, struct Tcl_Obj * CONST objv[])
{
	const char *key = Tcl_GetStringFromObj (objv[2], 0);
	const ValueHandle *handle = getValueHandle (key, false);
	if (!handle)
	{
		Tcl_AddErrorInfo (interp, "invalid valueId");
		return TCL_ERROR;
	}

	try
	{
		ValueIteratorI *value = handle->getValue();
		if (value->prev())
			Tcl_SetObjResult (interp, Tcl_NewIntObj (1));
		else
			Tcl_SetObjResult (interp, Tcl_NewIntObj (0));
	}
	catch (ArchiveException &e)
	{
		Tcl_AddErrorInfo (interp, UCC(e.what ()));
		return TCL_ERROR;
	}
	return TCL_OK;
}

// --------------------------------------------------------------------------
//*	<B>Syntax:</B> <I>value type valueId</I>
//
// <B>Returns:</B> string describing type
//
// Get type of current value for valueId.

static int value_type (ClientData clientData, Tcl_Interp *interp,
						int objc, struct Tcl_Obj * CONST objv[])
{
	const char *key = Tcl_GetStringFromObj (objv[2], 0);
	const ValueHandle *handle = getValueHandle (key);
	if (!handle)
	{
		Tcl_AddErrorInfo (interp, "invalid valueId");
		return TCL_ERROR;
	}

	try
	{
		ValueIteratorI *value = handle->getValue();
		DbrType type = value->getValue()->getType ();
		if (type < dbr_text_dim)
			Tcl_SetObjResult (interp, Tcl_NewStringObj (UCC(dbr_text[type]), -1) );
		else
		{
			char txt[20];
			sprintf (txt, "unknown: 0x%X", type);
			Tcl_SetObjResult (interp, Tcl_NewStringObj (txt, -1));
		}
	}
	catch (ArchiveException &e)
	{
		Tcl_AddErrorInfo (interp, UCC(e.what ()));
		return TCL_ERROR;
	}
	return TCL_OK;
}

// --------------------------------------------------------------------------
//*	<B>Syntax:</B> <I>value close valueId</I>
//
//	Invalidate valueId when no longer used.<BR>
//	(Does not return error when Id was invalid)

static int value_close (ClientData clientData, Tcl_Interp *interp,
				int objc, struct Tcl_Obj * CONST objv[])
{
	Tcl_HashEntry *entry;
	const char *key = Tcl_GetStringFromObj (objv[2], 0);
	ValueHandle *handle = getValueHandle (key, false, &entry);
	if (handle)
	{
		delete handle;
		Tcl_DeleteHashEntry (entry);
	}
	return TCL_OK;
}

static char *value_cmds[] = 
{ "valid", "isInfo", "get", "text", "time", "status", "next", "prev", "type", "close", 0 };

static CmdInfo value_cmd_info[] = 
{
	{ "valid valueId", 2, value_valid },
	{ "isInfo valueId", 2, value_isInfo },
	{ "get valueId", 2, value_get },
	{ "text valueId", 2, value_text },
	{ "time valueId", 2, value_time },
	{ "status valueId", 2, value_status },
	{ "next valueId", 2, value_next },
	{ "prev valueId", 2, value_prev },
	{ "type valueId", 2, value_type },
	{ "close valueId", 2, value_close },
};

// --------------------------------------------------------------------------
//		channel ...
//
//	checks arguments and dispatches to subcommands, see above
// --------------------------------------------------------------------------
DLLEXPORT int atac_valueCmd (ClientData clientData,
	Tcl_Interp *interp, int objc, struct Tcl_Obj * CONST objv[])
{
	// Test args.
	if (objc < 2)
	{
		Tcl_WrongNumArgs (interp, 1, objv, "subcommand ?options?");
		return TCL_ERROR;
	}

	int cmd;
	if (Tcl_GetIndexFromObj (interp, objv[1], value_cmds, "subcommand", 0, &cmd) == TCL_ERROR)
		return TCL_ERROR;

	if (objc-1 != value_cmd_info[cmd].num_opts)
	{
		Tcl_WrongNumArgs (interp, 1, objv, UCC(value_cmd_info[cmd].options));
		return TCL_ERROR;
	}

	return value_cmd_info[cmd].proc (clientData, interp, objc, objv);
}

// --------------------------------------------------------------------------
//		Init
//
//	Called once to initialize the atac extension,
//	registers new commands to TCL.
// --------------------------------------------------------------------------
DLLEXPORT int Atac_Init (Tcl_Interp *interp)
{
	// Create atac hash-table
	Tcl_InitHashTable (&handles, TCL_STRING_KEYS);

	// register commands
	Tcl_CreateObjCommand (interp, "archive", atac_archiveCmd, 0, 0);
	Tcl_CreateObjCommand (interp, "channel", atac_channelCmd, 0, 0);
	Tcl_CreateObjCommand (interp, "value", atac_valueCmd, 0, 0);

	// define package for pkg_mkIndex
	Tcl_PkgProvide (interp, "atac", VERSION_TXT);

	// make version information available as global variable
	Tcl_SetVar (interp, "atac_version", VERSION_TXT, TCL_GLOBAL_ONLY);

	return TCL_OK;
}


