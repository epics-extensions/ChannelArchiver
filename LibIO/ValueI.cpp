// ValueI.cpp
//////////////////////////////////////////////////////////////////////

#include <alarm.h>
#include <alarmString.h>
#include <ArrayTools.h>
#include <stdlib.h>
#include "ValueI.h"

USE_STD_NAMESPACE
BEGIN_NAMESPACE_CHANARCH

//////////////////////////////////////////////////////////////////////
// RawValueI
//////////////////////////////////////////////////////////////////////

RawValueI::Type * RawValueI::allocate (size_t size)
{
	return reinterpret_cast<RawValueI::Type *> (new char[size]);
}

// allocate space for num samples of type/count
RawValueI::Type * RawValueI::allocate (DbrType type, DbrCount count, size_t num)
{
	return allocate (num * getSize (type, count));
}

void RawValueI::free (Type *value)
{
	delete [] reinterpret_cast<char *>(value);
}

size_t RawValueI::getSize (DbrType type, DbrCount count)
{	// need to make the buffer size be a properly structure aligned number
	size_t buf_size = dbr_size_n(type, count);
	if (buf_size % 8)
		buf_size += 8 - (buf_size % 8);

	return buf_size;
}

bool RawValueI::hasSameValue (DbrType type, DbrCount count, size_t size, const Type *lhs, const Type *rhs)
{
	size_t offset;

	switch (type){
	case DBR_TIME_STRING:	offset = offsetof (dbr_time_string, value);		break;
	case DBR_TIME_SHORT: 	offset = offsetof (dbr_time_short, value);		break;
	case DBR_TIME_FLOAT:	offset = offsetof (dbr_time_float, value);		break;
	case DBR_TIME_ENUM:		offset = offsetof (dbr_time_enum, value);		break;
	case DBR_TIME_CHAR:		offset = offsetof (dbr_time_char, value);		break;
	case DBR_TIME_LONG:		offset = offsetof (dbr_time_long, value);		break;
	case DBR_TIME_DOUBLE:	offset = offsetof (dbr_time_double, value);		break;
	default:
		LOG_MSG ("RawValueI::hasSameValue: cannot decode type " << type << "\n");
		return false;
	}

	return memcmp (((char *)lhs) + offset, ((char *)rhs) + offset, size - offset) == 0;
}

void RawValueI::getStatus (const Type *value, stdString &result)
{
	strstream buf;

	short severity = value->severity & 0xfff;
	switch (severity)
	{
	case NO_ALARM:
		result = '\0';
		return;
	// Archiver specials:
	case ARCH_EST_REPEAT:
		buf << "Est_Repeat " << (unsigned short)value->status << '\0';
		result = buf.str();
		buf.freeze (false);
		return;
	case ARCH_REPEAT:
		buf << "Repeat " << (unsigned short)value->status << '\0';
		result = buf.str();
		buf.freeze (false);
		return;
	case ARCH_DISCONNECT:
		result = "Disconnected";
		return;
	case ARCH_STOPPED:
		result = "Archive_Off";
		return;
	case ARCH_DISABLED:
		result = "Archive_Disabled";
		return;
	case ARCH_CHANGE_PERIOD:
		result = "Change Sampling Period";
		return;
	}

	if (severity < (short)SIZEOF_ARRAY(alarmSeverityString)  &&
		(short)value->status < (short)SIZEOF_ARRAY(alarmStatusString))
	{
		result = alarmSeverityString[severity];
		result += " ";
		result += alarmStatusString[value->status];
	}
	else
	{
		buf << severity << ' ' << value->status << '\0';
		result = buf.str();
	}
	buf.freeze (false);
}

bool RawValueI::parseStatus (const stdString &text, short &stat, short &sevr)
{
	if (text.empty())
	{
		stat = sevr = 0;
		return true;
	}
	if (!strncmp (text.c_str(), "Est_Repeat ", 11))
	{
		sevr = ARCH_EST_REPEAT;
		stat = atoi(text.c_str()+11);
		return true;
	}
	if (!strncmp (text.c_str(), "Repeat ", 7))
	{
		sevr = ARCH_REPEAT;
		stat = atoi(text.c_str()+7);
		return true;
	}
	if (!strcmp (text.c_str(), "Disconnected"))
	{
		sevr = ARCH_DISCONNECT;
		stat = 0;
		return true;
	}
	if (!strcmp (text.c_str(), "Archive_Off"))
	{
		sevr = ARCH_STOPPED;
		stat = 0;
		return true;
	}
	if (!strcmp (text.c_str(), "Archive_Disabled"))
	{
		sevr = ARCH_DISABLED;
		stat = 0;
		return true;
	}
	if (!strcmp (text.c_str(), "Change Sampling Period"))
	{
		sevr = ARCH_CHANGE_PERIOD;
		stat = 0;
		return true;
	}

	short i, j;
	for (i=0; i<(short)SIZEOF_ARRAY(alarmSeverityString); ++i)
	{
		if (!strncmp (text.c_str(), alarmSeverityString[i],
					  strlen(alarmSeverityString[i])))
		{
			sevr = i;
			stdString status = text.substr (strlen(alarmSeverityString[i]));

			for (j=0; j<(short)SIZEOF_ARRAY(alarmStatusString); ++j)
			{
				if (status.find (alarmStatusString[j]) != stdString::npos)
				{
					stat = j;
					return true;
				}
			}
			return false;
		}
	}

	return false;
}

void RawValueI::getTime (const Type *value, stdString &time)
{
	osiTime osi;
	osi = TS_STAMP2osi (value->stamp);
	osiTime2string (osi, time);
}

//////////////////////////////////////////////////////////////////////
// ValueI
//////////////////////////////////////////////////////////////////////

ValueI::ValueI (DbrType type, DbrCount count)
{
	_type  = type;
	_count = count;
	_size  = RawValueI::getSize (type, count);
	_value = RawValueI::allocate (_size);
}

ValueI::~ValueI ()
{	RawValueI::free (_value);	}

void ValueI::setCtrlInfo (const CtrlInfoI *info)
{}

bool ValueI::parseStatus (const stdString &text)
{
	short  stat, sevr;
	if (RawValueI::parseStatus (text, stat, sevr))
	{
		setStatus (stat, sevr);
		return true;
	}
	return false;
}

void ValueI::getType (stdString &text) const
{
	if (getType() < dbr_text_dim)
		text = dbr_text[getType()];
	else
		text = dbr_text_invalid;
}

bool ValueI::parseType (const stdString &text, DbrType &type)
{
	// db_access.h::dbr_text_to_type() doesn't handle invalid types,
	// so it's duplicated here:
    for (type=0; type<dbr_text_dim; ++type)
	{
		if (strcmp(text.c_str(), dbr_text[type]) == 0)
			return true;
	}
	return false;
}

void ValueI::show (ostream &o) const
{
	stdString time_text, stat_text;

	getTime (time_text);
	getStatus (stat_text);

	o << time_text << ' ';
	o << "RawValue (type " << getType() << ", count " << getCount();
	o << ") " << stat_text;
}

END_NAMESPACE_CHANARCH

