/* -*-c++-*- */
#ifndef __CONFIGFILE_H__
#define __CONFIGFILE_H__

#include"Configuration.h"
#include<ASCIIParser.h>

//CLASS ConfigFile
// This implementation of a CLASS Configuration will read
// the initial configuration from a master ASCII file,
// which may include additional group files.
//
// When one of the save() methods is called,
// the set of configuration files in ConfigDir
// is updated.
// 
// See main() for the piping between
// Engine and ConfigFile:
// <PRE>
// // theEngine is created ...
// ConfigFile *config = new ConfigFile;
// config->load (config_file);
// ...
// theEngine->setConfiguration (config);               
// </PRE>
class ConfigFile : public Configuration
{
public:
    ConfigFile();

    //* Configuration interface
    bool load(const stdString &config_name);
    bool loadGroup(const stdString &group_name);
    bool save();
    bool saveGroup(const class GroupInfo *group);

private:
    void setParameter(const ASCIIParser &parser,
                      const stdString &parameter, const char *value);
    bool getChannel(ASCIIParser &parser, stdString &channel, double &period,
                     bool &monitor, bool &disable);
    stdString   _config_dir;
    stdString   _config_name;
    stdString   _file_name;
};

#endif //__CONFIGFILE_H__
