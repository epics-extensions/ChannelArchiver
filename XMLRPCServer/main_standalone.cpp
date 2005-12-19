// System
#include <stdio.h>
#include <math.h>
#include <time.h>
#include <sys/time.h>
// Tools
#include <MsgLogger.h>
// XML-RPC
#include <xmlrpc.h>
#include <xmlrpc_abyss.h>
// XMLRPCServer
#include "ArchiveDataServer.h"

static ServerConfig the_config;


const char *get_config_name(xmlrpc_env *env)
{
    return "<Command Line>";
}

bool get_config(xmlrpc_env *env, ServerConfig &config)
{
    config = the_config;
    return true;
}

// Return open index for given key or 0
IndexFile *open_index(xmlrpc_env *env, int key)
{
    stdString index_name;
    if (!the_config.find(key, index_name))
    {
        xmlrpc_env_set_fault_formatted(env, ARCH_DAT_NO_INDEX,
                                       "Invalid key %d", key);
        return 0;
    } 
    LOG_MSG("Open index, key %d = '%s'\n", key, index_name.c_str());
    IndexFile *index = new IndexFile(50);
    if (!index)
    {
        xmlrpc_env_set_fault_formatted(env, ARCH_DAT_NO_INDEX,
                                       "Cannot allocate index");
        return 0;
    }
    ErrorInfo error_info;
    if (index->open(index_name, true, error_info))
        return index;
    delete index;
    xmlrpc_env_set_fault_formatted(env, ARCH_DAT_NO_INDEX,
                                   "%s", error_info.info.c_str());
    return 0;
}

int main(int argc, char *argv[])
{
    if (argc != 3)
    {
        fprintf(stderr, "USAGE: ArchiveDataServerStandalone <abyss.conf> <index>\n");
        return -1;
    }

    ServerConfig::Entry entry;
    entry.key  = 1;
    entry.name = "Archive";
    entry.path = argv[2];
    the_config.config.push_back(entry);
    entry.clear();

    xmlrpc_server_abyss_init(XMLRPC_SERVER_ABYSS_NO_FLAGS, argv[1]);

    fprintf(stderr, "ArchiveDataServerStandalone Running\n");
    fprintf(stderr, "Unless your '%s' selects a different port number,\n", argv[1]);
    fprintf(stderr, "the data should now be available via the XML-RPC URL\n");
    fprintf(stderr, "    http://localhost:8080/RPC2\n");

    try
    {
        MsgLogger logger("archserver.log");
        LOG_MSG("---- ArchiveServer Started ----\n");
    
        xmlrpc_server_abyss_add_method_w_doc("archiver.info",     &get_info,     0, "S:", "Get info");
        xmlrpc_server_abyss_add_method_w_doc("archiver.archives", &get_archives, 0, "A:", "Get archives");
        xmlrpc_server_abyss_add_method_w_doc("archiver.names",    &get_names,    0, "A:is", "Get channel names");
        xmlrpc_server_abyss_add_method_w_doc("archiver.values",   &get_values,   0, "A:iAiiiiii", "Get values");
        xmlrpc_server_abyss_run();    
    }
    catch (GenericException &e)
    {
	fprintf(stderr, "Error: %s", e.what());
    }
    return 0;
}

