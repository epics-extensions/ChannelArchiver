// -*- c++ -*-

// Tools
#include <ToolsConfig.h>
#include <Guard.h>
#include <FUX.h>

/// \ingroup Engine The ArchiveEngine Configuration

/// Reads the config from an XML file
/// that should conform to engineconfig.dtd.
///
class EngineConfig
{
public:
    /// Read XML file that matches engineconfig.dtd.
    bool read(Guard &engine_guard, class Engine *engine,
              const stdString &filename);

    /// Write XML file
    bool write(Guard &engine_guard, class Engine *engine);
private:
    bool handle_group(Guard &engine_guard, class Engine *engine,
                      class FUX::Element *group);
    bool handle_channel(Guard &engine_guard, class Engine *engine,
                        class GroupInfo *group,
                        class FUX::Element *channel);
};


