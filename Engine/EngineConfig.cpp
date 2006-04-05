// System
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
// Tools
#include <MsgLogger.h>
#include <FUX.h>
// Engine
#include "EngineConfig.h"

EngineConfigParser::EngineConfigParser()
    : listener(0)
{
}

static bool parse_double(const stdString &t, double &n)
{
    const char *s = t.c_str();
    char *e;
    n = strtod(s, &e);
    return e != s  &&  n != HUGE_VAL  && n != -HUGE_VAL;
}

void EngineConfigParser::read(const char *filename,
                              EngineConfigListener *listener)
{
    this->listener = listener;
    // We depend on the XML parser to validate.
    // The asserts are only the ultimate way out.
    FUX fux;
    FUX::Element *e, *doc = fux.parse(filename);
    if (!(doc && doc->name == "engineconfig"))
        throw GenericException(__FILE__, __LINE__,
                               "Cannot parse '%s'\n", filename);
    stdList<FUX::Element *>::const_iterator els;
    double d;
    for (els=doc->children.begin(); els!=doc->children.end(); ++els)
    {
        e = *els;
        if (e->name == "write_period")
        {
            if (!parse_double(e->value, d))
                throw GenericException(__FILE__, __LINE__,
                                       "'%s': Error in write_period\n",
                                       filename);
            setWritePeriod(d < 0 ? 1.0 : d);
        }
        else if (e->name == "get_threshold")
        {
            if (!parse_double(e->value, d))
                throw GenericException(__FILE__, __LINE__,
                                       "'%s': Error in get_threshold\n",
                                       filename);
            setGetThreshold(d);
        }
        else if (e->name == "file_size")
        {
            if (!parse_double(e->value, d))
                throw GenericException(__FILE__, __LINE__,
                                       "'%s': Error in file_size\n",
                                       filename);
            setFileSizeLimit(((size_t)d)*1024*1024);
        }
        else if (e->name == "ignored_future")
        {
            if (!parse_double(e->value, d))
                throw GenericException(__FILE__, __LINE__,
                                       "'%s': Error in ignored_future\n",
                                       filename);
            setIgnoredFutureSecs(d*60*60);
        }
        else if (e->name == "buffer_reserve")
        {
            if (!parse_double(e->value, d))
                throw GenericException(__FILE__, __LINE__,
                                       "'%s': Error in buffer_reserve\n",
                                       filename);
            setBufferReserve((size_t)d);
        }
        else if (e->name == "max_repeat_count")
        {
            if (!parse_double(e->value, d))
                throw GenericException(__FILE__, __LINE__,
                                       "'%s': Error in max_repeat_count\n",
                                       filename);
            setMaxRepeatCount((size_t) d);
        }
        else if (e->name == "disconnect")
            setDisconnectOnDisable(true);
        else if (e->name == "group")
            handle_group(e);
    }
}

void EngineConfigParser::handle_group(FUX::Element *group)
{
    stdList<FUX::Element *>::const_iterator els;
    els=group->children.begin();
    LOG_ASSERT((*els)->name == "name");
    const stdString &group_name = (*els)->value;
    ++els;
    while (els!=group->children.end())
    {
        LOG_ASSERT((*els)->name == "channel");
        handle_channel(group_name, (*els));
        ++els;
    }
}

void EngineConfigParser::handle_channel(const stdString &group_name,
                                        FUX::Element *channel)
{
    stdList<FUX::Element *>::const_iterator els;
    els = channel->children.begin();
    LOG_ASSERT((*els)->name == "name");
    const stdString &channel_name = (*els)->value;
    ++els;
    LOG_ASSERT((*els)->name == "period");
    double period;
    if (!parse_double((*els)->value, period))
        throw GenericException(__FILE__, __LINE__,
                               "Group '%s': Error in period for channel '%s'\n",
                               group_name.c_str(), channel_name.c_str());
    ++els;
    bool monitor = (*els)->name == "monitor";
    bool disabling = false;
    ++els;
    while (els != channel->children.end())
    {
        if ((*els)->name == "disable")
            disabling = true;
        ++els;
    }
    if (listener)
        listener->addChannel(group_name, channel_name,
                             period, disabling, monitor);
}

#if 0

static bool add_channel(Guard &engine_guard, FUX::Element *group,
                        const GroupInfo *gi, ArchiveChannel *c)
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

static bool add_group(Guard &engine_guard,
                      FUX::Element *doc, const GroupInfo *gi)
{
    FUX::Element *group = new FUX::Element(doc, "group");
    doc->add(group);
    FUX::Element *e = new FUX::Element(group, "name");
    e->value = gi->getName();
    group->add(e);
    const stdList<class ArchiveChannel *> &channels = gi->getChannels();
    stdList<class ArchiveChannel *>::const_iterator ci;
    for (ci = channels.begin(); ci != channels.end(); ++ci)
        if (!add_channel(engine_guard, group, gi, *ci))
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
    sprintf(buf, "%lu",
            (unsigned long)DataWriter::file_size_limit/1024/1024);
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

    e = new FUX::Element(doc, "max_repeat_count");
    sprintf(buf, "%lu", (unsigned long)SampleMechanismGet::max_repeat_count);
    e->value = buf;
    doc->add(e);

    if (engine->disconnectOnDisable(engine_guard))
        doc->add(new FUX::Element(doc, "disconnect"));

    const stdList<GroupInfo *> &groups = engine->getGroups(engine_guard);
    stdList<GroupInfo *>::const_iterator gi;
    for (gi = groups.begin(); gi != groups.end(); ++gi)
        if (! add_group(engine_guard, doc, *gi))
            return false;
    
    FILE *f = fopen("onlineconfig.xml", "wt");
    if (!f)
    {
        throw GenericException(__FILE__, __LINE__, "Cannot create 'onlineconfig.xml'\n");
        return false;
    }
    fux.dump(f);
    fclose(f);
    return true;
}

#endif
