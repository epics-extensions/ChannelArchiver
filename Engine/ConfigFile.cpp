#include "Engine.h"
#include "ConfigFile.h"
#include <Filename.h>
#include <ctype.h>

ConfigFile::ConfigFile()
{
    config_dir = "cfg";
}

void ConfigFile::setParameter(const ASCIIParser &parser,
                              const stdString &parameter, const char *value)
{
    if (parameter == "write_freq"  ||  parameter == "write_period")
    {
        double num = atof(value);
        theEngine->setWritePeriod((num < 0 ? 0.1 : num));
    }
    else if (parameter == "save_default" || parameter == "default_period")
    {
        double num = atof(value);
        if (num <= 0)
        {
            LOG_MSG("Config file '%s'(line %d): invalid period %s\n",
                    file_name.c_str(), parser.getLineNo(), value);
            return;
        }
        theEngine->setDefaultPeriod(num);
    }
    else if (parameter == "buffer_reserve")
    {
        int num = atoi(value);
        if (num <= 0)
        {
            LOG_MSG("Config file '%s'(line %d): invalid buffer_reserve %s\n",
                    file_name.c_str(), parser.getLineNo());
            return;
        }
        theEngine->setBufferReserve(num);
    }
    else if (parameter == "get_threshold")
        theEngine->setGetThreshold(atof(value));
    else if (parameter == "file_size")// arg: hours per file, i.e. 24
        theEngine->setSecsPerFile(atoi(value) * 3600);
    else if (parameter == "ignored_future") // arg: hours
    {
        double num = atof(value);
        if (num < 0)
        {
            LOG_MSG("Config file '%s'(line %d): invalid hour specific. %s\n",
                    file_name.c_str(), parser.getLineNo(), value);
            return;
        }
        theEngine->setIgnoredFutureSecs(num*60*60);
    }
    else if (parameter == "group")
        loadGroup(value);
    else
        LOG_MSG("Config file '%s'(line %d): parameter '%s' is ignored\n",
                file_name.c_str(), parser.getLineNo(), parameter.c_str());
}

// This one checks e.g. channel name characters
// to avoid crashes when we are fed a binary file
// for a config file.
// The definition of "good" might be too tight.
static inline bool good_character(char ch)
{
    return isalpha(ch) || isdigit(ch) || strchr("_:()-.", ch);
}

bool ConfigFile::getChannel(ASCIIParser &parser, stdString &channel,
                            double &period, bool &monitor, bool &disable)
{
    const char *ch; // current position in line
    stdString parameter;

    channel.assign(0, 0);
    while (true)
    {
        period = theEngine->getDefaultPeriod();
        monitor = false;
        disable = false;

        if (!parser.nextLine())
            return false;
        ch = parser.getLine().c_str();

        // Format: <channel> <period> <Monitor> <Disable> ?
        // How does a <channel> name start?
        // '!' would start an archive engine parameter...
        if (*ch != '!')
        {   // get channel
            while (*ch  &&  !isspace(*ch))
            {
                if (! good_character(*ch))
                {
                    LOG_MSG("Config file '%s'(line %d): "
                            "suspicious channel name\n",
                            file_name.c_str(), parser.getLineNo());
                    return false;
                }
                channel += *(ch++);
            }
            if (channel.length() == 0)
                return false;

            // skip space
            while (*ch && isspace(*ch)) ++ch;
            if (! *ch) return true;

            // test for period
            if (isdigit (*ch))
                period = strtod(ch, (char **)&ch);

            // skip space
            while (*ch && isspace(*ch)) ++ch;
            if (! *ch) return true;

            // test for Monitor
            if (*ch == 'M'  ||  *ch == 'm')
            {
                monitor = true;
                while (*ch && isalpha(*ch)) ++ch;
            }

            // skip space
            while (*ch && isspace(*ch)) ++ch;
            if (! *ch) return true;

            // test for Disable
            if (*ch == 'D'  ||  *ch == 'd')
                disable = true;

            return true;
        }

        // else: Format !<parameter> <value>
        ++ch;
        // get parameter
        parameter.reserve(20);
        while (*ch  &&  !isspace(*ch))
            parameter += *(ch++);
        if (parameter.length() == 0)
            continue;

        // skip space
        while (*ch && isspace(*ch)) ++ch;
        if (! *ch) return true;
        setParameter(parser, parameter, ch);
        parameter = '\0';
    }

    return true;
}

bool ConfigFile::loadGroup(const stdString &group_name)
{
    ASCIIParser parser;
    
    if (! parser.open(group_name))
    {
        LOG_MSG("Config file '%s': cannot open\n", group_name.c_str());
        return false;
    }

    // new archive group?
    GroupInfo *group = theEngine->findGroup(group_name);
    if (! group)
    {
        group = theEngine->addGroup(group_name);
        if (! group)
        {
            LOG_MSG("Config file '%s': cannot add to Engine\n",
                    group_name.c_str());
            return false;
        }
    }

    file_name = group_name;
    stdString channel_name;
    double period;
    bool monitor, disable;
    while (getChannel(parser, channel_name, period, monitor, disable))
        theEngine->addChannel(group, channel_name, period, disable, monitor);

    return true;
}

bool ConfigFile::load(const stdString &config_name)
{
    this->config_name = config_name;
    return loadGroup (config_name);
}

bool ConfigFile::saveGroup(const class GroupInfo *group)
{
#ifdef TODO
    stdString filename;
    Filename::build(config_dir, group->getName(), filename);
    FILE *f = fopen(filename.c_str(), "wt");
    if (! f)
    {
        LOG_MSG("Config file '%s': cannot create\n"
                "(No severe error. Create a '%s' subdirectory\n"
                " if you want to save updated configuration files).\n",
                filename.c_str(), config_dir.c_str());
        return false;
    }
    
    fprintf(f, "# Group: %s\n", group->getName().c_str());
    fprintf(f, "# This file was auto-created by the ChannelArchiver Engine\n");
    fprintf(f, "# Instance: %s\n", theEngine->getDescription().c_str());
    fprintf(f, "\n");
    fprintf(f, "!write_period\t%g\n", theEngine->getWritePeriod());
    fprintf(f, "!default_period\t%g\n", theEngine->getDefaultPeriod());
    fprintf(f, "!get_threshold\t%g\n", theEngine->getGetThreshold());
    fprintf(f, "!file_size\t%g\n", theEngine->getSecsPerFile()/3600);
    fprintf(f, "!ignored_future\t%g\n",
            theEngine->getIgnoredFutureSecs()/3600);
    fprintf(f, "!buffer_reserve\t%d", theEngine->getBufferReserve());
    fprintf(f, "\n");

    if (group->getName() == config_name)
    {
        const stdList<GroupInfo *> & groups = theEngine->getGroups ();
        stdList<GroupInfo *>::const_iterator g;
        
        for (g=groups.begin(); g!=groups.end(); ++g)
            if ((*g)->getName ()  != group->getName()) // don't include self
                fprintf(f, "!group %s\n", (*g)->getName().c_str());
    }

    const stdList <ChannelInfo *> & channels = group->getChannels();
    stdList <ChannelInfo *>::const_iterator channel;
    for (channel=channels.begin(); channel!=channels.end(); ++channel)
    {
        fprintf(f, "%s\t%g",
                (*channel)->getName().c_str(), (*channel)->getPeriod());
        if ((*channel)->isMonitored())
            fprintf(f, "\tMonitor");
        if ((*channel)->isDisabling(group))
            fprintf(f, "\tDisable");
        fprintf(f, "\n");
    }
    fprintf(f, "# EOF\n\n");

    fclose(f);
#endif
    return true;
}
 
bool ConfigFile::save()
{
    const stdList<GroupInfo *> & groups = theEngine->groups;
    stdList<GroupInfo *>::const_iterator group;
    
    for (group=groups.begin(); group!=groups.end(); ++group)
    {
        if (! saveGroup(*group))
            return false;
    }

    return true;
}

