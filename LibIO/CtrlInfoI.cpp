#include "ArchiveException.h"
#include "CtrlInfoI.h"

USE_STD_NAMESPACE
BEGIN_NAMESPACE_CHANARCH

CtrlInfoI::CtrlInfoI ()
{
	size_t additional_buffer = 10;

	_infobuf.reserve (sizeof(CtrlInfoData) + additional_buffer);
	CtrlInfoData *info = _infobuf.mem();
	info->type = Invalid;
	info->size = sizeof (DbrCount) + sizeof(DbrType);
}

CtrlInfoI::CtrlInfoI (const CtrlInfoI &rhs)
{
	const CtrlInfoData *rhs_info = rhs._infobuf.mem();
	_infobuf.reserve (rhs_info->size);
	CtrlInfoData *info = _infobuf.mem();
	memcpy (info, rhs_info, rhs_info->size);
}

CtrlInfoI & CtrlInfoI::operator = (const CtrlInfoI &rhs)
{
	const CtrlInfoData *rhs_info = rhs._infobuf.mem();
	_infobuf.reserve (rhs_info->size);
	CtrlInfoData *info = _infobuf.mem();
	memcpy (info, rhs_info, rhs_info->size);
	return *this;
}

CtrlInfoI::~CtrlInfoI ()
{}

bool CtrlInfoI::operator == (const CtrlInfoI &rhs) const
{
	const CtrlInfoData *rhs_info = rhs._infobuf.mem();
	const CtrlInfoData *info = _infobuf.mem();
	
	// will compare "size" element first:
	return memcmp (info, rhs_info, rhs_info->size) == 0;
}

void CtrlInfoI::setNumeric (
	long prec, const stdString &units,
	float disp_low, float disp_high,
	float low_alarm, float low_warn, float high_warn, float high_alarm)
{
	size_t len = units.length();
	size_t size = sizeof (CtrlInfoData) + len;
	_infobuf.reserve (size);
	CtrlInfoData *info = _infobuf.mem();

	info->type = Numeric;
	info->size = size;
	info->value.analog.disp_high  = disp_high;
	info->value.analog.disp_low   = disp_low;
	info->value.analog.low_warn   = low_warn;
	info->value.analog.low_alarm  = low_alarm;
	info->value.analog.high_warn  = high_warn;
	info->value.analog.high_alarm = high_alarm;
	info->value.analog.prec       = prec;
	string2cp (info->value.analog.units, units, len+1);
}

void CtrlInfoI::setEnumerated (size_t num_states, char *strings[])
{
	size_t i, len = 0;
	for (i=0; i<num_states; i++) // calling strlen twice for each string...
		len += strlen(strings[i]) + 1;

	allocEnumerated (num_states, len);
	for (i=0; i<num_states; i++)
		setEnumeratedString (i, strings[i]);
}

void CtrlInfoI::allocEnumerated (size_t num_states, size_t string_len)
{
	size_t size = sizeof (CtrlInfoData) + string_len; // actually this is too big...
	_infobuf.reserve (size);
	CtrlInfoData *info = _infobuf.mem();

	info->type = Enumerated;
	info->size = size;
	info->value.index.num_states = num_states;
	char *enum_string = info->value.index.state_strings;
	*enum_string = '\0';
}

// Must be called after allocEnumerated()
// AND must be called in sequence,
// i.e. setEnumeratedString (0, ..
//      setEnumeratedString (1, ..
void CtrlInfoI::setEnumeratedString (size_t state, const char *string)
{
	CtrlInfoData *info = _infobuf.mem();
	if (info->type != Enumerated  ||
        state >= (size_t)info->value.index.num_states)
		throwArchiveException (Invalid);

	char *enum_string = info->value.index.state_strings;
	size_t i;
	for (i=0; i<state; i++) // find this state string...
		enum_string += strlen (enum_string) + 1;
	strcpy (enum_string, string);
}

// After allocEnumerated() and a sequence of setEnumeratedString ()
// calls, this method recalcs the total size
// and checks if the buffer is sufficient (Debug version only)
void CtrlInfoI::calcEnumeratedSize ()
{
	size_t i, len, total=sizeof (CtrlInfoData);
	CtrlInfoData *info = _infobuf.mem();
	char *enum_string = info->value.index.state_strings;
	for (i=0; i<(size_t)info->value.index.num_states; i++)
	{
		len = strlen (enum_string) + 1;
		enum_string += len;
		total += len;
	}

	info->size = total;
	LOG_ASSERT (total <= _infobuf.getBufferSize ());
}

// Convert a double value into text, formatted according to CtrlInfo
// Throws Invalid if CtrlInfo is not for Numeric
void CtrlInfoI::formatDouble (double value, stdString &result) const
{
	if (getType() != Numeric)
		throwDetailedArchiveException (Invalid, "CtrlInfo::formatDouble: Type not Numeric");

	strstream buf;
	buf.setf(ios::fixed, ios::floatfield);
	buf.precision (getPrecision ());
	buf << value << '\0';
	result = buf.str ();
	buf.rdbuf()->freeze (false);
}

const char *CtrlInfoI::getState (size_t state, size_t &len) const
{
	if (getType() != Enumerated)
		return 0;

	const CtrlInfoData *info = _infobuf.mem();
	const char *enum_string = info->value.index.state_strings;
	size_t i=0;

	do
	{
		len = strlen (enum_string);
		if (i == state)
			return enum_string;
		enum_string += len + 1;
		++i;
	}
	while (i < (size_t)info->value.index.num_states);
	len = 0;
	
	return 0;
}

void CtrlInfoI::getState (size_t state, stdString &result) const
{
	size_t len;
	const char *text = getState (state, len);
	if (text)
	{
		result.assign (text, len);
		return;
	}

	strstream tmp;
	tmp << "<Undef: " << state << '\0';
	result = tmp.str();
	tmp.rdbuf()->freeze (false);
}

bool CtrlInfoI::parseState (const char *text, const char **next, size_t &state) const
{
	const char *state_text;
	size_t	i, len;

	for (i=0; i<getNumStates (); ++i)
	{
		state_text = getState (i, len);
		if (! state_text)
		{
			LOG_MSG ("CtrlInfoI::parseState: missing state " << i);
			return false;
		}
		if (!strncmp (text, state_text, len))
		{
			state = i;
			if (next)
				*next = text + len;
			return true;
		}
	}
	return false;
}

void CtrlInfoI::show (ostream &o) const
{
    if (getType() == Numeric)
    {
        o << "CtrlInfo: Numeric\n";
        o << "Display: " << getDisplayLow () << " ... "
                << getDisplayHigh () << "\n";
        o << "Alarm:   " << getLowAlarm () << " ... "
                << getHighAlarm () << "\n";
        o << "Warning: " << getLowWarning () << " ... "
                << getHighWarning () << "\n";
        o << "Prec:    " << getPrecision () << " '" << getUnits () << "'\n";
    }
    else if (getType() == Enumerated)
    {
        o << "CtrlInfo: Enumerated\n";
        o << "States:\n";
        size_t i, len;
        for (i=0; i<getNumStates (); ++i)
        {
            o << "\tstate='" << getState (i, len) << "'\n";
        }
    }
    else
        o << "CtrlInfo: Unknown\n";
}

END_NAMESPACE_CHANARCH

