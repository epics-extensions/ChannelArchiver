// -*- c++ -*-

// Tools
#include "IndexConfig.h"
#include "FUX.h"

bool IndexConfig::parse(const stdString &config_name)
{
    FILE *f = fopen(config_name.c_str(), "rt");
    if (!f)
        return false;
    char line[5];
    bool ok = fread(line, 1, 5, f) == 5;
    fclose(f);
    if (!ok)
        return false;
    if (strncmp(line, "<?xml", 5))
        return false;
    FUX fux;
    FUX::Element *e, *doc = fux.parse(config_name.c_str());
    if (!(doc && doc->name == "indexconfig"))
    {
        fprintf(stderr, "'%s' is no XML indexconfig\n", config_name.c_str());
        return false;
    }
    stdList<FUX::Element *>::const_iterator els;
    for (els=doc->children.begin(); els!=doc->children.end(); ++els)
        if ((e = (*els)->find("index")))
            subarchives.push_back(e->value);
    return true;
}
