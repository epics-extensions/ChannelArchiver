// -*- c++ -*-

// Tools
#include "IndexConfig.h"
#include "FUX.h"

bool IndexConfig::parse(const stdString &config_name)
{
    FUX fux;
    FUX::Element *e, *doc = fux.parse(config_name.c_str());
    if (!(doc && doc->name == "indexconfig"))
    {
        fprintf(stderr, "Cannot parse '%s'\n", config_name.c_str());
        return false;
    }
    stdList<FUX::Element *>::const_iterator els;
    for (els=doc->children.begin(); els!=doc->children.end(); ++els)
        if ((e = (*els)->find("index")))
            subarchives.push_back(e->value);
    return true;
}
