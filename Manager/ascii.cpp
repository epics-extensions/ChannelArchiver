// --------------------------------------------------------
// $Id$
//
// Please refer to NOTICE.txt,
// included as part of this distribution,
// for legal information.
//
// Kay-Uwe Kasemir, kasemir@lanl.gov
// --------------------------------------------------------

// ASCII read/write routines
//

#include "../ArchiverConfig.h"
#include "BinArchive.h"
#include <ASCIIParser.h>
#include <stdlib.h>

#ifdef MANAGER_USE_MULTI
#   include "MultiArchive.h"
#   define ARCHIVE_TYPE MultiArchive
#else
#   define ARCHIVE_TYPE BinArchive
#endif

// -------------------------------------------------------------------
// write
// -------------------------------------------------------------------

static void output_header (const ValueIterator &value)
{
    stdString type;
    value->getType (type);

    std::cout << "# ------------------------------------------------------\n";
    std::cout << "Header\n";
    std::cout << "{\n";
    std::cout << "\tperiod=" << value.getPeriod () << "\n";
    std::cout << "\ttype="   << type               << "\n";
    std::cout << "\tcount="  << value->getCount () << "\n";
    std::cout << "}\n";
    std::cout << "# ------------------------------------------------------\n";
}

static void output_info (const CtrlInfoI *info)
{
    std::cout << "# ------------------------------------------------------\n";
    std::cout << "CtrlInfo\n";
    std::cout << "{\n";
    if (info->getType() == CtrlInfoI::Numeric)
    {
        std::cout << "\ttype=Numeric\n";
        std::cout << "\tprecision="    << info->getPrecision ()   << "\n";
        std::cout << "\tunits="        << info->getUnits ()       << "\n";
        std::cout << "\tdisplay_high=" << info->getDisplayHigh () << "\n";
        std::cout << "\tdisplay_low="  << info->getDisplayLow ()  << "\n";
        std::cout << "\thigh_alarm="   << info->getHighAlarm ()   << "\n";
        std::cout << "\thigh_warning=" << info->getHighWarning () << "\n";
        std::cout << "\tlow_warning="  << info->getLowWarning ()  << "\n";
        std::cout << "\tlow_alarm="    << info->getLowAlarm ()    << "\n";
    }
    else if (info->getType() == CtrlInfoI::Enumerated)
    {
        std::cout << "\ttype=Enumerated\n";
        std::cout << "\tstates=" << info->getNumStates () << "\n";
        size_t i;
        stdString state;
        for (i=0; i<info->getNumStates (); ++i)
        {
            info->getState (i, state);
            std::cout << "\tstate=" << state << "\n";
        }
    }
    else
    {
        std::cout << "\ttype=Unknown\n";
    }
    std::cout << "}\n";
    std::cout << "# ------------------------------------------------------\n";
}

void output_ascii (const stdString &archive_name,
                   const stdString &channel_name,
                   const osiTime &start, const osiTime &end)
{
    Archive         archive (new ARCHIVE_TYPE (archive_name));
    ChannelIterator channel (archive);
    ValueIterator   value (archive);
    if (! archive.findChannelByName (channel_name, channel))
    {
        std::cout << "# Channel not found: " << channel_name << "\n";
        return;
    }

    std::cout << "channel=" << channel_name << "\n";

    if (! channel->getValueAfterTime (start, value))
    {
        std::cout << "# no values\n";
        return;
        return;
    }

    CtrlInfoI   info;
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
            std::cout << "Error: back in time:\n";
        std::cout << *value << "\n";
        last_time = value->getTime();
        ++value;
    }
}

// -------------------------------------------------------------------
// read
// -------------------------------------------------------------------

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
        std::cout << "Line " << getLineNo() << ": missing start of Header\n";
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
                    std::cout << "Line " << getLineNo() << ": invalid type "
                        << value << "\n";
                    return false;
                }
            }
            else if (parameter == "count")
                count = atoi(value.c_str());
        }
        else
            std::cout << "Line " << getLineNo() << " skipped\n";
    }

    if (_period < 0.0 || type >= LAST_BUFFER_TYPE || count <= 0)
    {
        std::cout << "Line " << getLineNo() << ": incomplete header\n";
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
        std::cout << "Line " << getLineNo() << ": no header, yet\n";
        return false;
    }

    long prec=2;
    stdString units;
    float disp_low=0, disp_high=0;
    float low_alarm=0, low_warn=0, high_warn=0, high_alarm=0;   
    stdString parameter, value;
    CtrlInfoI::Type type;
    size_t states = 0;
    stdVector<stdString> state;
    
    if (!nextLine()  ||
        getLine () != "{")
    {
        std::cout << "Line " << getLineNo() << ": missing start of CtrlInfo\n";
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
                    std::cout << "Line " << getLineNo()
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
            std::cout << "Line " << getLineNo() << " skipped\n";
    }

    if (type == CtrlInfoI::Numeric)
        _info.setNumeric (prec, units, disp_low, disp_high,
            low_alarm, low_warn, high_warn, high_alarm);                     
    else if (type == CtrlInfoI::Enumerated)
    {
        if (state.size() != states)
        {
            std::cout << "Line " << getLineNo()
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
        std::cout << "Line " << getLineNo()
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
        std::cout << "Line " << getLineNo() << ": no header, yet\n";
        return false;
    }
    // Format of line:   time\tvalue\tstatus
    const stdString &line = getLine();
    size_t pos = line.find ('\t');
    if (pos == stdString::npos)
    {
        std::cout << "Line " << getLineNo() << ": expected value\n";
        std::cout << line;
        return false;
    }

    // Time:
    stdString text = line.substr (0, pos);
    osiTime time;
    if (! string2osiTime (text, time))
    {
        std::cout << "Line " << getLineNo() << ": invalid time '" << text << "'\n";
        return false;
    }
    _value->setTime (time);

    text = line.substr (pos+1);
    pos = text.find ('\t');
    if (pos == stdString::npos)
    {
        std::cout << "Line " << getLineNo() << ": expected value\n";
        std::cout << line;
        return false;
    }

    // Value:
    stdString value = text.substr(0, pos);
    if (! _value->parseValue (value))
    {
        std::cout << "Line " << getLineNo() << ": invalid value '" << value << "'\n";
        std::cout << line;
        return false;
    }

    stdString status = text.substr(pos+1);

    // Status:
    if (!_value->parseStatus (status))
    {
        std::cout << "Line " << getLineNo() << ": invalid status '" << status << "'\n";
        std::cout << line;
        return false;
    }

    if (isValidTime (_last_time)  && _value->getTime() < _last_time)
    {
        std::cout << "Line " << getLineNo() << ": back in time\n";
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
                        std::cout << "Cannot add channel " << arg << " to archive\n";
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
    ArchiveParser   parser;
    if (! parser.open (file_name))
    {
        std::cout << "Cannot open " << file_name << "\n";
    }

    Archive archive (new BinArchive (archive_name, true));
    parser.run (archive);
}
