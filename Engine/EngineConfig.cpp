// System
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
// Tools
#include <MsgLogger.h>
#include <FUX.h>
// Storage
#include <DataWriter.h>
// Local
#include "EngineConfig.h"
#include "Engine.h"

static bool parse_double(const stdString &t, double &n)
{
    const char *s = t.c_str();
    char *e;
    n = strtod(s, &e);
    return e != s  &&  n != HUGE_VAL  && n != -HUGE_VAL;
}

bool EngineConfig::read(Guard &engine_guard, class Engine *engine,
                        const stdString &filename)
{
    // We depend on the XML parser to validate.
    // The asserts are only the ultimate way out.
    FUX fux;
    FUX::Element *e, *doc = fux.parse(filename.c_str());
    if (!(doc && doc->name == "engineconfig"))
    {
        LOG_MSG("EngineConfig: Cannot parse '%s'\n",
                filename.c_str());
        return false;
    }
    stdList<FUX::Element *>::const_iterator els;
    double d;
    for (els=doc->children.begin(); els!=doc->children.end(); ++els)
    {
        e = *els;
        if (e->name == "write_period")
        {
            if (!parse_double(e->value, d))
            {
                LOG_MSG("EngineConfig '%s': Error in write_period\n",
                        filename.c_str());
                return false;
            }
            engine->setWritePeriod(engine_guard, d < 0 ? 1.0 : d);
        }
        else if (e->name == "get_threshold")
        {
            if (!parse_double(e->value, d))
            {
                LOG_MSG("EngineConfig '%s': Error in get_threshold\n",
                        filename.c_str());
                return false;
            }
            engine->setGetThreshold(engine_guard, d);
        }
        else if (e->name == "file_size")
        {
            if (!parse_double(e->value, d))
            {
                LOG_MSG("EngineConfig '%s': Error in file_size\n",
                        filename.c_str());
                return false;
            }
            DataWriter::file_size_limit = (FileOffset)(d*1024*1024);
        }
        else if (e->name == "ignored_future")
        {
            if (!parse_double(e->value, d))
            {
                LOG_MSG("EngineConfig '%s': Error in ignored_future\n",
                        filename.c_str());
                return false;
            }
            engine->setIgnoredFutureSecs(engine_guard, d*60*60);
        }
        else if (e->name == "buffer_reserve")
        {
            if (!parse_double(e->value, d))
            {
                LOG_MSG("EngineConfig '%s': Error in buffer_reserve\n",
                        filename.c_str());
                return false;
            }
            engine->setBufferReserve(engine_guard, (int)d);
        }
        else if (e->name == "group")
        {
            if (!handle_group(engine_guard, engine, e))
                return false;
        }
    }
    return true;
}

static bool add_channel(FUX::Element *group, const GroupInfo *gi, ArchiveChannel *c)
{
    char buf[100];
    FUX::Element *channel = new FUX::Element(group, "channel");
    group->add(channel);
    FUX::Element *e = new FUX::Element(channel, "name");
    e->value = c->getName();
    channel->add(e);

    Guard guard(c->mutex);
    e = new FUX::Element(channel, "period");
    sprintf(buf, "%g", c->getPeriod(guard));
    e->value = buf;
    channel->add(e);

    if (c->getMechanism(guard)->isScanning())
        channel->add(new FUX::Element(channel, "scan"));
    else
        channel->add(new FUX::Element(channel, "monitor"));
    if (c->getGroupsToDisable(guard).test(gi->getID()))
        channel->add(new FUX::Element(channel, "disable"));

    return true;
}

static bool add_group(FUX::Element *doc, const GroupInfo *gi)
{
    FUX::Element *group = new FUX::Element(doc, "group");
    doc->add(group);
    FUX::Element *e = new FUX::Element(group, "name");
    e->value = gi->getName();
    group->add(e);
    const stdList<class ArchiveChannel *> &channels = gi->getChannels();
    stdList<class ArchiveChannel *>::const_iterator ci;
    for (ci = channels.begin(); ci != channels.end(); ++ci)
        if (!add_channel(group, gi, *ci))
            return false;
    return true;
}

bool EngineConfig::write(Guard &engine_guard, class Engine *engine)
{
    char buf[100];
    FUX fux;
    FUX::Element *e, *doc = new FUX::Element(0, "engineconfig");

    fux.setDoc(doc);

    e = new FUX::Element(doc, "write_period");
    sprintf(buf, "%g", engine->getWritePeriod());
    e->value = buf;
    doc->add(e);
    
    e = new FUX::Element(doc, "get_threshold");
    sprintf(buf, "%g", engine->getGetThreshold());
    e->value = buf;
    doc->add(e);

    e = new FUX::Element(doc, "file_size");
    sprintf(buf, "%lu", DataWriter::file_size_limit/1024/1024);
    e->value = buf;
    doc->add(e);

    e = new FUX::Element(doc, "ignored_future");
    sprintf(buf, "%g", engine->getIgnoredFutureSecs()/60.0/60.0);
    e->value = buf;
    doc->add(e);

    e = new FUX::Element(doc, "buffer_reserve");
    sprintf(buf, "%d", engine->getBufferReserve());
    e->value = buf;
    doc->add(e);

    const stdList<GroupInfo *> &groups = engine->getGroups(engine_guard);
    stdList<GroupInfo *>::const_iterator gi;
    for (gi = groups.begin(); gi != groups.end(); ++gi)
        if (! add_group(doc, *gi))
            return false;
    
    FILE *f = fopen("onlineconfig.xml", "wt");
    if (!f)
    {
        LOG_MSG("Cannot create 'onlineconfig.xml'\n");
        return false;
    }
    fux.dump(f);
    fclose(f);
    return true;
}


bool EngineConfig::handle_group(Guard &engine_guard, Engine *engine,
                                FUX::Element *group)
{
    stdList<FUX::Element *>::const_iterator els;
    els=group->children.begin();
    LOG_ASSERT((*els)->name == "name");
    const stdString &group_name = (*els)->value;
    ++els;
    GroupInfo *group_info = engine->findGroup(engine_guard, group_name);
    if (! group_info)
    {
        group_info = engine->addGroup(engine_guard, group_name);
        if (! group_info)
        {
            LOG_MSG("Group '%s': cannot add to Engine.\n",
                    group_name.c_str());
            return false;
        }
    }
    while (els!=group->children.end())
    {
        LOG_ASSERT((*els)->name == "channel");
        if (!handle_channel(engine_guard, engine, group_info, (*els)))
            return false;
        ++els;
    }
    return true;
}

bool EngineConfig::handle_channel(Guard &engine_guard, Engine *engine,
                                  GroupInfo *group,
                                  FUX::Element *channel)
{
    stdList<FUX::Element *>::const_iterator els;
    els = channel->children.begin();
    LOG_ASSERT((*els)->name == "name");
    const stdString &name = (*els)->value;
    ++els;
    LOG_ASSERT((*els)->name == "period");
    double period;
    if (!parse_double((*els)->value, period))
    {
        LOG_MSG("EngineConfig '%s': Error in period for channel '%s'\n",
                name.c_str());
        return false;
    }
    ++els;
    bool monitor = (*els)->name == "monitor";
    bool disable = false;
    ++els;
    while (els != channel->children.end())
    {
        if ((*els)->name == "disable")
            disable = true;
        ++els;
    }

    printf("'%s' - '%s': period %g, %s%s\n",
           group->getName().c_str(), name.c_str(), period,
           (monitor ? "monitor" : "scan"),
           (disable ? ", disable" : ""));
    engine->addChannel(engine_guard, group, name, period, disable, monitor);
    return true;
}

/// EOF EngineConfig
