#include "Engine.h"
#include "ConfigFile.h"
#include <fstream>
#include <Filename.h>
#include <ctype.h>

BEGIN_NAMESPACE_CHANARCH
USE_STD_NAMESPACE

void ConfigFile::setParameter (const stdString &parameter, char *value)
{
	if (parameter == "write_freq"  ||  parameter == "write_period")
	{
		double num = atof (value);
		theEngine->setWritePeriod ((num < 0 ? 0.1 : num));
	}
	else if (parameter == "save_default" || parameter == "default_period")
	{
		double num = atof (value);
		if (num <= 0)
		{
			LOG_MSG ("Config file '" << _file_name << "'(" << _line_no << "): invalid period " << value << "\n");
			return;
		}
		theEngine->setDefaultPeriod (num);
	}
	else if (parameter == "buffer_reserve")
	{
		int num = atoi (value);
		if (num <= 0)
		{
			LOG_MSG ("Config file '" << _file_name << "'(" << _line_no << "): invalid buffer_reserve " << value << "\n");
			return;
		}
		theEngine->setBufferReserve (num);
	}
	else if (parameter == "get_threshold")
		theEngine->setGetThreshold (atof (value));
	else if (parameter == "file_size")
		theEngine->setSecsPerFile (atoi (value) * 3600); // arg: hours per file, i.e. 24
	else if (parameter == "group")
	{
		char *pos = strchr (value, '\r');
		if (pos)
			*pos = '\0';
		loadGroup (value);
	}
	else
		LOG_MSG ("Config file '" << _file_name << "'(" << _line_no << "): parameter '" << parameter << "' is ignored\n");
}

bool ConfigFile::getChannel (ifstream &in, stdString &channel, double &period, bool &monitor, bool &disable)
{
	char line[100];
	char *ch; // current position in line
	stdString parameter;

	channel.assign (0, 0);
	while (true)
	{
		period = theEngine->getDefaultPeriod ();
		monitor = false;
		disable = false;

		in.getline (line, sizeof (line));
		if (in.eof ())
			return false;
		++_line_no;

		ch = line;
		// skip white space
		while (*ch && isspace(*ch)) ++ch;
		if (! *ch) continue; // empty line

		// skip comment lines
		if (*ch == '#')
			continue; // try next line

		// Format: <channel> <period> <Monitor> <Disable> ?
		if (isalpha(*ch))
		{	// get channel
			while (*ch  &&  !isspace(*ch))
				channel += *(ch++);
			if (channel.length() == 0)
				return false;

			// skip space
			while (*ch && isspace(*ch))	++ch;
			if (! *ch) return true;

			// test for period
			if (isdigit (*ch))
				period = strtod (ch, &ch);

			// skip space
			while (*ch && isspace(*ch)) ++ch;
			if (! *ch) return true;

			// test for Monitor
			if (*ch == 'M'  ||  *ch == 'm')
			{
				monitor = true;
				while (*ch && isalpha(*ch))	++ch;
			}

			// skip space
			while (*ch && isspace(*ch))	++ch;
			if (! *ch) return true;

			// test for Disable
			if (*ch == 'D'  ||  *ch == 'd')
				disable = true;

			return true;
		}

		// else: Format !<parameter> <value>
		++ch;
		// get parameter
		parameter.reserve (20);
		while (*ch  &&  !isspace(*ch))
			parameter += *(ch++);
		if (parameter.length() == 0)
			continue;

		// skip space
		while (*ch && isspace(*ch))	++ch;
		if (! *ch) return true;
		setParameter (parameter, ch);
		parameter = '\0';
	}

	return true;
}

ConfigFile::ConfigFile ()
{
	_config_dir = "cfg";
}

bool ConfigFile::loadGroup (const stdString &group_name)
{
	ifstream file;

	file.open (group_name.c_str());
#	ifdef __HP_aCC
	if (file.fail())
#	else
	if (! file.is_open ())
#	endif
	{
		LOG_MSG ("Config file '" << group_name << "': cannot open\n");
		return false;
	}
	file.unsetf (ios::binary);

	// new archive group?
	GroupInfo *group = theEngine->findGroup (group_name);
	if (! group)
	{
		group = theEngine->addGroup (group_name);
		if (! group)
		{
			file.close ();
			LOG_MSG ("Config file '" << group_name << "': cannot add to Engine\n");
			return false;
		}
	}

	_file_name = group_name;
	_line_no = 0;

	stdString channel_name;
	double period;
	bool monitor, disable;
	while (getChannel (file, channel_name, period, monitor, disable))
		theEngine->addChannel (group, channel_name, period, disable, monitor);
	file.close ();

	return true;
}

bool ConfigFile::load (const stdString &config_name)
{
	_config_name = config_name;
	return loadGroup (config_name);
}

bool ConfigFile::saveGroup (const class GroupInfo *group)
{
	stdString filename;
	Filename::build (_config_dir, group->getName(), filename);
	ofstream file;
	file.open (filename.c_str());
#	ifdef __HP_aCC
	if (file.fail())
#	else
	if (! file.is_open())
#	endif
	{
		LOG_MSG ("Config file '" << filename << "': cannot create\n");
		return false;
	}

	file << "# Group: " << 	group->getName() << "\n";
	file << "# This file was auto-created by the ChannelArchiver Engine\n";
	file << "# Instance: " << theEngine->getDescription() << "\n";
	file << "\n";

	file << "!write_period\t" << theEngine->getWritePeriod() << "\n";
	file << "!default_period\t" << theEngine->getDefaultPeriod() << "\n";
	file << "!get_threshold\t" << theEngine->getGetThreshold() << "\n";
	file << "!file_size\t" << theEngine->getSecsPerFile()/3600 << "\n";
	file << "!buffer_reserve\t" << theEngine->getBufferReserve() << "\n";
	file << "\n";

	if (group->getName() == _config_name)
	{
		const list<GroupInfo *> & groups = theEngine->getGroups ();
		list<GroupInfo *>::const_iterator g;
		
		for (g=groups.begin(); g!=groups.end(); ++g)
			if ((*g)->getName ()  != group->getName()) // don't include self
				file << "!group " << (*g)->getName() << "\n";
	}

	const list <ChannelInfo *> & channels = group->getChannels ();
	list <ChannelInfo *>::const_iterator channel;
	for (channel=channels.begin(); channel!=channels.end(); ++channel)
	{
		file << (*channel)->getName() << "\t" << (*channel)->getPeriod();
		if ((*channel)->isMonitored())
			file << "\tMonitor";
		if ((*channel)->isDisabling(group))
			file << "\tDisable";
		file << "\n";
	}
	file << "# EOF\n\n";

	file.close();
	return true;
}
 
bool ConfigFile::save ()
{
	const list<GroupInfo *> & groups = theEngine->getGroups ();
	list<GroupInfo *>::const_iterator group;
	
	for (group=groups.begin(); group!=groups.end(); ++group)
	{
		if (! saveGroup (*group))
			return false;
	}

	return true;
}

END_NAMESPACE_CHANARCH

