// System
#include <stdio.h>
// Tools
#include <MsgLogger.h>
#include <FUX.h>
// Local
#include "ServerConfig.h"

bool ServerConfig::read(const char *filename)
{
    // We depend on the XML parser to validate.
    // The asserts are only the ultimate way out.
    FUX fux;
    FUX::Element *arch, *doc = fux.parse(filename);
    if (!doc)
    {
        LOG_MSG("ServerConfig: Cannot parse '%s'\n", filename);
        return false;
    }
    LOG_ASSERT(doc->name == "serverconfig");
    Entry entry;
    stdList<FUX::Element *>::const_iterator archs, e;
    for (archs=doc->children.begin(); archs!=doc->children.end(); ++archs)
    {
        arch = *archs;
        LOG_ASSERT(arch->name == "archive");
        e = arch->children.begin();
        LOG_ASSERT((*e)->name == "key");
        entry.key = atoi((*e)->value.c_str());
        ++e;
        LOG_ASSERT((*e)->name == "name");
        entry.name = (*e)->value;
        ++e;
        LOG_ASSERT((*e)->name == "path");
        entry.path = (*e)->value;
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


