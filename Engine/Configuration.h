#ifndef __CONFIGURATION_H__
#define __CONFIGURATION_H__

#include "GroupInfo.h"

/// Interface for defining (persistent) configuration information:
///
/// When asked to, the Configuration class uses the public interface
/// of the Engine class to load and save the state of the Engine.
///
/// The default Configuration is the class ConfigFile.
/// Whoever intends to implement another Configuration system
/// should
/// - derive from class Configuration
/// - instantiate that new class in main()
/// - connect theEngine to it by calling
///	  setConfiguration(Configuration *c);
///   just before entering the main loop.
class Configuration
{
public:
	virtual ~Configuration();

	/// These have to be implemented
	/// so that the CLASS Engine (or HTTPD) can load
	/// the whole configuration or (re-)load a specifig group.
	virtual bool load(const stdString &config_name) = 0;
	virtual bool loadGroup(const stdString &group_name) = 0;
	
	/// Called when Engine's whole configuration should be saved
	virtual bool save();

	/// Called when Engine's configuration has been changed,
	/// for example the default period etc., not Group or Channel Information.
	///
	/// Default implementation: call save()
	virtual bool saveEngine();

	/// Called when Channel's configuration has been changed
	///
	/// Default implementation: call save()
	virtual bool saveChannel(const class ArchiveChannel *channel);

	/// Called when Groups's configuration has been changed
	///
	/// Default implementation: call save()
	virtual bool saveGroup(const class GroupInfo *group);
};

#endif //__CONFIGURATION_H__
