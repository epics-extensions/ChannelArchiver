// -*- c++ -*-
#ifndef __CONFIG_FILE_H
#define __CONFIG_FILE_H

#include "ToolsConfig.h"

/// Configuration File

/// The ConfigFile will read the initial configuration from an ASCII file,
/// which may include additional group files.
///
/// When one of the save() methods is called, the set of configuration files
/// in ConfigDir is updated by querying the Engine class for the current setup.
class ConfigFile
{
public:
    ConfigFile();

    /// Load the main config file.
    bool load(const stdString &config_name);

    /// Load a single group
    bool loadGroup(const stdString &group_name);

    /// Called when Engine's whole configuration should be saved.
    bool save();    
    
	/// Save a groups's configuration has been changed.
    bool saveGroup(const class GroupInfo *group);
    
private:
    void setParameter(const class ASCIIParser &parser,
                      const stdString &parameter, const char *value);
    bool getChannel(class ASCIIParser &parser,
                    stdString &channel, double &period,
                    bool &monitor, bool &disable);
    stdString   config_dir;
    stdString   config_name;
    stdString   file_name;
};

#endif 
