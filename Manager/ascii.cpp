// ASCII read/write routines
//

#include "BinArchive.h"
#include <fstream>
#include <vector>
#include <ctype.h>
#include <stdlib.h>

USING_NAMESPACE_CHANARCH
using namespace std;

static void output_header (const ValueIterator &value)
{
	stdString type;
	value->getType (type);

	cout << "# ------------------------------------------------------\n";
	cout << "Header\n";
	cout << "{\n";
	cout << "\tperiod=" << value.getPeriod () << "\n";
	cout << "\ttype="   << type               << "\n";
	cout << "\tcount="  << value->getCount () << "\n";
	cout << "}\n";
	cout << "# ------------------------------------------------------\n";
}

static void output_info (const CtrlInfoI *info)
{
	cout << "# ------------------------------------------------------\n";
	cout << "CtrlInfo\n";
	cout << "{\n";
	if (info->getType() == CtrlInfoI::Numeric)
	{
		cout << "\ttype=Numeric\n";
		cout << "\tprecision="    << info->getPrecision ()   << "\n";
		cout << "\tunits="        << info->getUnits ()       << "\n";
		cout << "\tdisplay_high=" << info->getDisplayHigh () << "\n";
		cout << "\tdisplay_low="  << info->getDisplayLow ()  << "\n";
		cout << "\thigh_alarm="   << info->getHighAlarm ()   << "\n";
		cout << "\thigh_warning=" << info->getHighWarning () << "\n";
		cout << "\tlow_warning="  << info->getLowWarning ()  << "\n";
		cout << "\tlow_alarm="    << info->getLowAlarm ()    << "\n";
	}
	else if (info->getType() == CtrlInfoI::Enumerated)
	{
		cout << "\ttype=Enumerated\n";
		cout << "\tstates=" << info->getNumStates () << "\n";
		size_t i;
		stdString state;
		for (i=0; i<info->getNumStates (); ++i)
		{
			info->getState (i, state);
			cout << "\tstate=" << state << "\n";
		}
	}
	else
	{
		cout << "\ttype=Unknown\n";
	}
	cout << "}\n";
	cout << "# ------------------------------------------------------\n";
}

void output_ascii (const stdString &archive_name, const stdString &channel_name,
	const osiTime &start, const osiTime &end)
{
	Archive			archive (new BinArchive (archive_name));
	ChannelIterator	channel (archive);
	ValueIterator	value (archive);
	if (! archive.findChannelByName (channel_name, channel))
	{
		cout << "# Channel not found: " << channel_name << "\n";
		return;
	}

	cout << "channel=" << channel_name << "\n";

	if (! channel->getValueAfterTime (start, value))
	{
		cout << "# no values\n";
		return;
		return;
	}

	CtrlInfoI	info;
	double period=-1;
	osiTime last_time = nullTime;
	while (value && (!isValidTime(end)  ||  value->getTime() < end))
	{
		if (period != value.getPeriod())
		{
			period = value.getPeriod();
			output_header (value);
		}
		if (info != *value->getCtrlInfo())
		{
			info = *value->getCtrlInfo();
			output_info (&info);
		}
		if (isValidTime (last_time) && value->getTime() < last_time)
			cout << "Error: back in time:\n";
		cout << *value << "\n";
		last_time = value->getTime();
		++value;
	}
}

class ASCIIParser
{
public:
	ASCIIParser ()
	{
		_line_no = 0;
	}
	virtual ~ASCIIParser ()
	{
		if (_file.is_open ())
			_file.close ();
	}

	bool open (const stdString &file_name);

protected:
	// Read next line from file.
	// Result: hit end of file?
	bool nextLine ();

	// Get current line
	const stdString & getLine () const
	{	return _line; }
	size_t getLineNo () const
	{	return _line_no; }

	// Try to extract parameter=value pair
	// from current line.
	// Result: found parameter?
	bool getParameter (stdString &parameter, stdString &value);

private:

	size_t		_line_no;
	stdString	_line;
	ifstream		_file;
};

bool ASCIIParser::open (const stdString &file_name)
{
	_file.open (file_name.c_str());
	if (! _file.is_open ())
		return false;
	_file.unsetf (ios::binary);     
	return true;
}

bool ASCIIParser::nextLine ()
{
	char line[100];
	char *ch;

	while (true)
	{
		_file.getline (line, 100);
		if (_file.eof ())
		{
			_line.assign (0, 0);
			return false;     
		}
		++_line_no;

		ch = line;
		// skip white space
		while (*ch && isspace(*ch))
			++ch;
		if (! *ch)
			return nextLine (); // empty line

		// skip comment lines
		if (*ch == '#')
			continue; // try next line 

		_line = ch;
		return true;
	}
}

bool ASCIIParser::getParameter (stdString &parameter, stdString &value)
{
	size_t pos = _line.find ('=');
	if (pos == _line.npos)
		return false;

	parameter = _line.substr (0, pos);
	++pos;
	while (_line[pos] && isspace(_line[pos]))
		++pos;
	value = _line.substr (pos);

	return true;
}

class ArchiveParser : public ASCIIParser
{
public:
	ArchiveParser ()
	{
		_value = 0;
		_buffer_alloc = 0;
		_new_ctrl_info = false;
	}
	~ArchiveParser ()
	{
		delete _value;
	}

	void run (Archive &archive);
private:
	bool handleHeader (Archive &archive);
	bool handleCtrlInfo (ChannelIterator &channel);
	bool handleValue (ChannelIterator &channel);

	double _period;
	ValueI *_value;
	osiTime _last_time;
	CtrlInfoI _info;
	bool _new_ctrl_info;
	size_t  _buffer_alloc;
	enum
	{
		INIT_VALS_PER_BUF = 16,
		BUF_GROWTH_RATE = 4,
		MAX_VALS_PER_BUF = 1000
	};          
};

// Implementation:
// all handlers "handle" the current line
// and may move on to digest further lines,
// but will stay on their last line
// -> call nextLine again for following item

bool ArchiveParser::handleHeader (Archive &archive)
{
	_period=-1;
	DbrType type = 999;
	DbrCount count = 0;

	stdString parameter, value;
	
	if (!nextLine()  ||
		getLine () != "{")
	{
		cout << "Line " << getLineNo() << ": missing start of Header\n";
		return false;
	}

	while (nextLine())
	{
		if (getLine() == "}")
			break;
		if (getParameter (parameter, value))
		{
			if (parameter == "period")
				_period = atof(value.c_str());
			else if (parameter == "type")
			{
				if (! ValueI::parseType (value, type))
				{
					cout << "Line " << getLineNo() << ": invalid type "
						<< value << "\n";
					return false;
				}
			}
			else if (parameter == "count")
				count = atoi(value.c_str());
		}
		else
			cout << "Line " << getLineNo() << " skipped\n";
	}

	if (_period < 0.0 || type >= LAST_BUFFER_TYPE || count <= 0)
	{
		cout << "Line " << getLineNo() << ": incomplete header\n";
		return false;
	}
	if (_value)
	{
		if (_value->getType() != type  || _value->getCount() != count)
		{
			delete _value;
			_value = 0;
		}
	}
	if (!_value)
		_value = archive.newValue (type, count);
	_buffer_alloc = INIT_VALS_PER_BUF;
	_new_ctrl_info = false;

	return true;
}

bool ArchiveParser::handleCtrlInfo (ChannelIterator &channel)
{
	if (!_value)
	{
		cout << "Line " << getLineNo() << ": no header, yet\n";
		return false;
	}

    long prec=2;
	stdString units;
	float disp_low=0, disp_high=0;
	float low_alarm=0, low_warn=0, high_warn=0, high_alarm=0;   
	stdString parameter, value;
	CtrlInfoI::Type type;
	size_t states = 0;
	vector<stdString> state;
	
	if (!nextLine()  ||
		getLine () != "{")
	{
		cout << "Line " << getLineNo() << ": missing start of CtrlInfo\n";
		return false;
	}

	while (nextLine())
	{
		if (getLine() == "}")
			break;
		if (getParameter (parameter, value))
		{
			if (parameter == "type")
			{
				if (value == "Numeric")
					type = CtrlInfoI::Numeric;
				else if (value == "Enumerated")
					type = CtrlInfoI::Enumerated;
				else
				{
					cout << "Line " << getLineNo()
						<< ": Unknown type " << value << "\n";
					return false;
				}
			}
			if (parameter == "display_high")
				disp_high = atof(value.c_str());
			else if (parameter == "display_low")
				disp_low = atof(value.c_str());
			else if (parameter == "high_alarm")
				high_alarm = atof(value.c_str());
			else if (parameter == "high_warning")
				high_warn = atof(value.c_str());
			else if (parameter == "low_warning")
				low_warn = atof(value.c_str());
			else if (parameter == "low_alarm")
				low_alarm = atof(value.c_str());
			else if (parameter == "precision")
				prec = atoi(value.c_str());
			else if (parameter == "units")
				units = value;
			else if (parameter == "states")
				states = atoi(value.c_str());
			else if (parameter == "state")
				state.push_back (value);
		}
		else
			cout << "Line " << getLineNo() << " skipped\n";
	}

	if (type == CtrlInfoI::Numeric)
		_info.setNumeric (prec, units, disp_low, disp_high,
			low_alarm, low_warn, high_warn, high_alarm);                     
	else if (type == CtrlInfoI::Enumerated)
	{
		if (state.size() != states)
		{
			cout << "Line " << getLineNo()
				<< ": Asked for " << states
				<< " states but provided " << state.size() << "\n";
				return false;
		}
		size_t i, len = 0;
		for (i=0; i<states; ++i)
			len += state[i].length();
		_info.allocEnumerated (states, len);
		for (i=0; i<states; ++i)
			_info.setEnumeratedString (i, state[i].c_str());
	}
	else
	{
		cout << "Line " << getLineNo()
			<< ": Invalid CtrlInfo\n";
		return false;
	}

	_value->setCtrlInfo (&_info);
	_new_ctrl_info = true;

	return true;
}

bool ArchiveParser::handleValue (ChannelIterator &channel)
{
	if (!_value)
	{
		cout << "Line " << getLineNo() << ": no header, yet\n";
		return false;
	}
	// Format of line:   time\tvalue\tstatus
	const stdString &line = getLine();
	size_t pos = line.find ('\t');
	if (pos == stdString::npos)
	{
		cout << "Line " << getLineNo() << ": expected value\n";
		cout << line;
		return false;
	}

	// Time:
	stdString text = line.substr (0, pos);
	osiTime time;
	if (! string2osiTime (text, time))
	{
		cout << "Line " << getLineNo() << ": invalid time '" << text << "'\n";
		return false;
	}
	_value->setTime (time);

	text = line.substr (pos+1);
	pos = text.find ('\t');
	if (pos == stdString::npos)
	{
		cout << "Line " << getLineNo() << ": expected value\n";
		cout << line;
		return false;
	}

	// Value:
	stdString value = text.substr(0, pos);
	if (! _value->parseValue (value))
	{
		cout << "Line " << getLineNo() << ": invalid value '" << value << "'\n";
		cout << line;
		return false;
	}

	stdString status = text.substr(pos+1);

	// Status:
	if (!_value->parseStatus (status))
	{
		cout << "Line " << getLineNo() << ": invalid status '" << status << "'\n";
		cout << line;
		return false;
	}

	if (isValidTime (_last_time)  && _value->getTime() < _last_time)
	{
		cout << "Line " << getLineNo() << ": back in time\n";
		return false;
	}

	size_t avail = channel->lockBuffer (*_value, _period);
	if (avail <= 0 || _new_ctrl_info)
	{
		channel->addBuffer (*_value, _period, _buffer_alloc);
		if (_buffer_alloc < MAX_VALS_PER_BUF)
			_buffer_alloc *= BUF_GROWTH_RATE;
		_new_ctrl_info = false;
	}
	channel->addValue (*_value);
	channel->releaseBuffer ();

	_last_time = _value->getTime();

	return true;
}

void ArchiveParser::run (Archive &archive)
{
	ChannelIterator channel (archive);
	ValueIterator value (archive);
	stdString parameter, arg;
	bool go = true;

	while (go && nextLine ())
	{
		if (getParameter (parameter, arg))
		{
			if (parameter == "channel")
			{
				if (archive.findChannelByName (arg, channel))
				{
					_last_time = channel->getLastTime ();
				}
				else
				{
					if (! archive.addChannel (arg, channel))
					{
						cout << "Cannot add channel " << arg << " to archive\n";
						return;
					}
					_last_time = nullTime;
				}
			}
		}
		else
		{
			if (getLine() == "Header")
				go = handleHeader (archive);
			else if (getLine() == "CtrlInfo")
				go = handleCtrlInfo (channel);
			else
				go = handleValue (channel);
		}
	}
}

void input_ascii (const stdString &archive_name, const stdString &file_name)
{
	ArchiveParser	parser;
	if (! parser.open (file_name))
	{
		cout << "Cannot open " << file_name << "\n";
	}

	Archive archive (new BinArchive (archive_name, true));
	parser.run (archive);
}
