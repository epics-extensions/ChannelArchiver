// System
#include <stdio.h>
// Tools
#include <MsgLogger.h>
#include <FUX.h>
// Local
#include "ServerConfig.h"

bool ServerConfig::read(const char *filename)
{
    FUX fux;
    FUX::Element *arch, *doc = fux.parse(filename);
    if (!doc)
    {
        LOG_MSG("ServerConfig: Cannot parse '%s'\n", filename);
        return false;
    }

    if (doc->name != "serverconfig")
    {
        LOG_MSG("ServerConfig: Got '%s' instead of 'serverconfig'\n",
                doc->name.c_str());
        return false;
    }
    Entry entry;
    stdList<FUX::Element *>::const_iterator c;
    for (c=doc->children.begin(); c!=doc->children.end(); ++c)
    {
        arch = *c;
        if (arch->name != "archive")
        {
            LOG_MSG("ServerConfig: Got '%s' instead of 'archive'\n",
                    arch->name.c_str());
            return false;
        }
        stdList<FUX::Element *>::const_iterator e;
        for (e=arch->children.begin(); e!=arch->children.end(); ++e)
        {
            if ((*e)->name == "key")
                entry.key = atoi((*e)->value.c_str());
            else if ((*e)->name == "name")
                entry.name = (*e)->value;
            else if ((*e)->name == "path")
                entry.path = (*e)->value;

        }
        if (entry.key == 0)
        {
            LOG_MSG("ServerConfig: Missing archive key\n");
            return false;
        }
        if (entry.name.length() == 0)
        {
            LOG_MSG("ServerConfig: Missing archive name\n");
            return false;
        }
        if (entry.path.length() == 0)
        {
            LOG_MSG("ServerConfig: Missing archive path\n");
            return false;
        }
        config.push_back(entry);
        entry.clear();
    }
    return true;
}

void ServerConfig::dump()
{
    stdList<Entry>::const_iterator i;
    for (i=config.begin(); i!=config.end(); ++i)
        printf("key %d: '%s' in '%s'\n",
               i->key, i->name.c_str(), i->path.c_str());
}

bool ServerConfig::find(int key, stdString &path)
{
    stdList<Entry>::const_iterator i;
    for (i=config.begin(); i!=config.end(); ++i)
        if (i->key == key)
        {
            path = i->path;
            return true;
        }
    return false;
}


