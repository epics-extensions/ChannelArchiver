// System
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
// Tools
#include <MsgLogger.h>
#include <FUX.h>
// Engine
#include "EngineConfig.h"


void EngineConfig::addToFUX(FUX::Element *doc)
{
    doc->add(new FUX::Element(doc, "write_period", "%g", getWritePeriod()));    
    doc->add(new FUX::Element(doc, "get_threshold", "%g", getGetThreshold()));
    doc->add(new FUX::Element(doc, "file_size", "%zu", getFileSizeLimit()));
    doc->add(new FUX::Element(doc, "ignored_future", "%g",
            getIgnoredFutureSecs()/60.0/60.0));
    doc->add(new FUX::Element(doc, "buffer_reserve", "%d", getBufferReserve()));
    doc->add(new FUX::Element(doc, "max_repeat_count", "%zu",
             getMaxRepeatCount()));
    if (getDisconnectOnDisable())
        doc->add(new FUX::Element(doc, "disconnect"));
}

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
    if (!(doc && doc->getName() == "engineconfig"))
        throw GenericException(__FILE__, __LINE__,
                               "Cannot parse '%s'\n", filename);
    stdList<FUX::Element *>::const_iterator els;
    double d;
    for (els=doc->getChildren().begin(); els!=doc->getChildren().end(); ++els)
    {
        e = *els;
        if (e->getName() == "write_period")
        {
            if (!parse_double(e->getValue(), d))
                throw GenericException(__FILE__, __LINE__,
                                       "'%s': Error in write_period\n",
                                       filename);
            setWritePeriod(d < 0 ? 1.0 : d);
        }
        else if (e->getName() == "get_threshold")
        {
            if (!parse_double(e->getValue(), d))
                throw GenericException(__FILE__, __LINE__,
                                       "'%s': Error in get_threshold\n",
                                       filename);
            setGetThreshold(d);
        }
        else if (e->getName() == "file_size")
        {
            if (!parse_double(e->getValue(), d))
                throw GenericException(__FILE__, __LINE__,
                                       "'%s': Error in file_size\n",
                                       filename);
            setFileSizeLimit((size_t)(d*1024*1024));
        }
        else if (e->getName() == "ignored_future")
        {
            if (!parse_double(e->getValue(), d))
                throw GenericException(__FILE__, __LINE__,
                                       "'%s': Error in ignored_future\n",
                                       filename);
            setIgnoredFutureSecs(d*60*60);
        }
        else if (e->getName() == "buffer_reserve")
        {
            if (!parse_double(e->getValue(), d))
                throw GenericException(__FILE__, __LINE__,
                                       "'%s': Error in buffer_reserve\n",
                                       filename);
            setBufferReserve((size_t)d);
        }
        else if (e->getName() == "max_repeat_count")
        {
            if (!parse_double(e->getValue(), d))
                throw GenericException(__FILE__, __LINE__,
                                       "'%s': Error in max_repeat_count\n",
                                       filename);
            setMaxRepeatCount((size_t) d);
        }
        else if (e->getName() == "disconnect")
            setDisconnectOnDisable(true);
        else if (e->getName() == "group")
            handle_group(e);
    }
}

void EngineConfigParser::handle_group(FUX::Element *group)
{
    stdList<FUX::Element *>::const_iterator els;
    els=group->getChildren().begin();
    LOG_ASSERT((*els)->getName() == "name");
    const stdString &group_name = (*els)->getValue();
    ++els;
    while (els!=group->getChildren().end())
    {
        LOG_ASSERT((*els)->getName() == "channel");
        handle_channel(group_name, (*els));
        ++els;
    }
}

void EngineConfigParser::handle_channel(const stdString &group_name,
                                        FUX::Element *channel)
{
    stdList<FUX::Element *>::const_iterator els;
    els = channel->getChildren().begin();
    LOG_ASSERT((*els)->getName() == "name");
    const stdString &channel_name = (*els)->getValue();
    ++els;
    LOG_ASSERT((*els)->getName() == "period");
    double period;
    if (!parse_double((*els)->getValue(), period))
        throw GenericException(__FILE__, __LINE__,
                               "Group '%s': Error in period for channel '%s'\n",
                               group_name.c_str(), channel_name.c_str());
    ++els;
    bool monitor = (*els)->getName() == "monitor";
    bool disabling = false;
    ++els;
    while (els != channel->getChildren().end())
    {
        if ((*els)->getName() == "disable")
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
    FUX::Element *e = new FUX::Element(channel, "name", c->getName());
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
    FUX::Element *doc = new FUX::Element(0, "engineconfig");
    fux.setDoc(doc);



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
