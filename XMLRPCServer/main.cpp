// System
#include <stdio.h>
#include <time.h>
#include <sys/time.h>
// XML-RPC
#include <xmlrpc.h>
#include <xmlrpc_cgi.h>
// Tools
#include <AutoPtr.h>
#include <MsgLogger.h>
#include <RegularExpression.h>
#include <BinaryTree.h>
// Storage
#include <LinearReader.h>
// XMLRPCServer
#include "DataServer.h"
#include "ServerConfig.h"

#ifdef LOGFILE
static FILE *logfile;
#endif

// Return name of configuration file from environment or 0
const char *get_config_name(xmlrpc_env *env)
{
    const char *name = getenv("SERVERCONFIG");
    if (!name)
    {
        xmlrpc_env_set_fault_formatted(env, ARCH_DAT_NO_INDEX,
                                       "SERVERCONFIG is undefined");
        return 0;
    }
    return name;
}

bool get_config(xmlrpc_env *env, ServerConfig &config)
{
    const char *config_name = get_config_name(env);
    if (env->fault_occurred)
        return false;
    if (!config.read(config_name))
    {
        xmlrpc_env_set_fault_formatted(env, ARCH_DAT_NO_INDEX,
                                       "Cannot open config '%s'",
                                       config_name);
        return false;
    }
    return true;
}

// Return open index for given key or 0
IndexFile *open_index(xmlrpc_env *env, int key)
{
    ServerConfig config;
    if (!get_config(env, config))
        return 0;
    stdString index_name;
    if (!config.find(key, index_name))
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
    if (index->open(index_name))
        return index;
    delete index;
    xmlrpc_env_set_fault_formatted(env, ARCH_DAT_NO_INDEX,
                                   "Cannot open index '%s'",
                                   index_name.c_str());
    return 0;
}

// epicsTime -> time_t-type of seconds & nanoseconds
void epicsTime2pieces(const epicsTime &t,
                      xmlrpc_int32 &secs, xmlrpc_int32 &nano)
{   // TODO: This is lame, calling epicsTime's conversions twice
    epicsTimeStamp stamp = t;
    time_t time;
    epicsTimeToTime_t(&time, &stamp);
    secs = time;
    nano = stamp.nsec;
}

// Inverse ro epicsTime2pieces
void pieces2epicsTime(xmlrpc_int32 secs, xmlrpc_int32 nano, epicsTime &t)
{   // As lame as other nearby code
    epicsTimeStamp stamp;
    time_t time = secs;
    epicsTimeFromTime_t(&stamp, time);
    stamp.nsec = nano;
    t = stamp;
}

// Used by get_names to put info for channel into sorted tree & dump it
class ChannelInfo
{
public:
    stdString name;
    epicsTime start, end;

    // Required by BinaryTree for sorting. We sort by name.
    bool operator < (const ChannelInfo &rhs)   { return name < rhs.name; }
    bool operator == (const ChannelInfo &rhs)  { return name == rhs.name; }

    class UserArg
    {
    public:
        xmlrpc_env   *env;
        xmlrpc_value *result;
    };
    
    // "visitor" for BinaryTree of channel names
    static void add_name_to_result(const ChannelInfo &info, void *arg)
    {
        UserArg *user_arg = (UserArg *)arg;
        xmlrpc_int32 ss, sn, es, en;
        epicsTime2pieces(info.start, ss, sn);
        epicsTime2pieces(info.end, es, en);        

        LOG_MSG("Found name '%s'\n", info.name.c_str());

        xmlrpc_value *channel = xmlrpc_build_value(
            user_arg->env,
            STR("{s:s,s:i,s:i,s:i,s:i}"),
            "name", info.name.c_str(),
            "start_sec", ss, "start_nano", sn,
            "end_sec", es,   "end_nano", en);
        if (channel)
        {
            xmlrpc_array_append_item(user_arg->env, user_arg->result, channel);
            xmlrpc_DECREF(channel);
        }
    }
};

// Return CtrlInfo encoded a per "meta" returned by archiver.get_values
static xmlrpc_value *encode_ctrl_info(xmlrpc_env *env, const CtrlInfo *info)
{
    if (info && info->getType() == CtrlInfo::Enumerated)
    {
        xmlrpc_value *state, *states = xmlrpc_build_value(env, STR("()"));
        stdString state_txt;
        size_t i, num = info->getNumStates();
        for (i=0; i<num; ++i)
        {
            info->getState(i, state_txt);
            state = xmlrpc_build_value(env, STR("s#"),
                                       state_txt.c_str(), state_txt.length());
            xmlrpc_array_append_item(env, states, state);
            xmlrpc_DECREF(state);
        }
        xmlrpc_value *meta = xmlrpc_build_value(
            env, STR("{s:i,s:V}"),
            "type", (xmlrpc_int32)META_TYPE_ENUM, "states", states);
        xmlrpc_DECREF(states);
        return meta;
    }
    if (info && info->getType() == CtrlInfo::Numeric)
    {
        return xmlrpc_build_value(
            env, STR("{s:i,s:d,s:d,s:d,s:d,s:d,s:d,s:i,s:s}"),
            "type", (xmlrpc_int32)META_TYPE_NUMERIC,
            "disp_high",  info->getDisplayHigh(),
            "disp_low",   info->getDisplayLow(),
            "alarm_high", info->getHighAlarm(),
            "warn_high",  info->getHighWarning(),
            "warn_low",   info->getLowWarning(),
            "alarm_low",  info->getLowAlarm(),
            "prec", (xmlrpc_int32)info->getPrecision(),
            "units", info->getUnits());
    }
    return  xmlrpc_build_value(env, STR("{s:i,s:(s)}"),
                               "type", (xmlrpc_int32)META_TYPE_ENUM,
                               "states", "<undefined>");
}

// Given a raw sample dbr_type/dbr_count,data,
// map it onto xml_type/xml_count and add to values
void encode_value(xmlrpc_env *env,
                  DbrType dbr_type, DbrCount dbr_count,
                  const RawValue::Data *data,
                  xmlrpc_int32 xml_type, xmlrpc_int32 xml_count,
                  xmlrpc_value *values)
{
    if (xml_count > dbr_count)
        xml_count = dbr_count;
    xmlrpc_value *element, *val_array =  xmlrpc_build_value(env, STR("()"));
    if (env->fault_occurred)
        return;
    int i;
    switch (xml_type)
    {
        case XML_STRING:
        {
            stdString txt;
            RawValue::getValueString(txt, dbr_type, dbr_count, data, 0);
            element = xmlrpc_build_value(env, STR("s#"),
                                         txt.c_str(), txt.length());
            xmlrpc_array_append_item(env, val_array, element);
            xmlrpc_DECREF(element);
        }            
        case XML_INT:
        case XML_ENUM:
        {
            long l;
            for (i=0;
                 i < xml_count &&
                     RawValue::getLong(dbr_type, dbr_count, data, l, i);
                 ++i)
            {
                element = xmlrpc_build_value(env, STR("i"), (xmlrpc_int32)l);
                xmlrpc_array_append_item(env, val_array, element);
                xmlrpc_DECREF(element);
            }
        }
        case XML_DOUBLE:
        {
            double d;
            for (i=0;
                 i < xml_count &&
                     RawValue::getDouble(dbr_type, dbr_count, data, d, i);
                 ++i)
            {
                element = xmlrpc_build_value(env, STR("d"), d);
                xmlrpc_array_append_item(env, val_array, element);
                xmlrpc_DECREF(element);
            }
        }
    }
    xmlrpc_int32 secs, nano;
    epicsTime2pieces(RawValue::getTime(data), secs, nano);
    xmlrpc_value *value = xmlrpc_build_value(
        env, STR("{s:i,s:i,s:i,s:i,s:V}"),
        "stat", (xmlrpc_int32)RawValue::getStat(data),
        "sevr", (xmlrpc_int32)RawValue::getSevr(data),
        "secs", secs,
        "nano", nano,
        "value", val_array);
    xmlrpc_DECREF(val_array);
    xmlrpc_array_append_item(env, values, value);
    xmlrpc_DECREF(value);
}

// Return the data for all the names[], start .. end etc.
// as get_values() is supposed to return them.
//
// Returns raw values if interpol <= 0.0.
// Returns 0 on error.
xmlrpc_value *get_data(xmlrpc_env *env,
                       int key,
                       const stdVector<stdString> names,
                       const epicsTime &start, const epicsTime &end,
                       long count, double interpol)
{
    xmlrpc_value   *results = 0;
#ifdef LOGFILE
    stdString txt;
    LOG_MSG("Start: %s\n", epicsTimeTxt(start, txt));
    LOG_MSG("End  : %s\n", epicsTimeTxt(end, txt));
    LOG_MSG("Interpolating onto %g seconds\n", interpol);
#endif
    AutoPtr<IndexFile> index(open_index(env, key));
    AutoPtr<DataReader> reader;
    if (env->fault_occurred)
        goto exit_from_get_data;
    if (interpol <= 0.0)
        reader = new RawDataReader(*index);
    else
        reader = new LinearReader(*index, interpol);
    if (!reader)
    {
        xmlrpc_env_set_fault_formatted(env, ARCH_DAT_SERV_FAULT,
                                       "Cannot create reader");
        goto exit_from_get_data;
    }
    results = xmlrpc_build_value(env, STR("()"));
    if (env->fault_occurred)
        goto exit_from_get_data;
    const RawValue::Data *data;
    xmlrpc_value *result, *meta, *values;
    xmlrpc_int32 xml_type, xml_count;
    long i, num_vals, name_count;
    name_count = names.size();
    for (i=0; i<name_count; ++i)
    {
#ifdef LOGFILE
        LOG_MSG("Handling '%s'\n", names[i].c_str());
#endif
        values = xmlrpc_build_value(env, STR("()"));
        if (env->fault_occurred)
            goto exit_from_get_data;
        data = reader->find(names[i], &start, &end);
        if (data == 0)
        {
            meta = encode_ctrl_info(env, 0);
            xml_type = XML_ENUM;
            xml_count = 1;
        }
        else
        {
            // Fix meta/type/count based on first value
            meta = encode_ctrl_info(env, &reader->getInfo());
            switch (reader->getType())
            {
                case DBR_TIME_STRING: xml_type = XML_STRING; break;
                case DBR_TIME_ENUM:   xml_type = XML_ENUM;   break;
                case DBR_TIME_SHORT:
                case DBR_TIME_CHAR:
                case DBR_TIME_LONG:   xml_type = XML_INT;    break;
                case DBR_TIME_FLOAT:    
                case DBR_TIME_DOUBLE:
                default:              xml_type = XML_DOUBLE; break;
            }
            xml_count = reader->getCount();
            for (num_vals = 0;
                 data && num_vals < count
                     && RawValue::getTime(data) < end;
                 ++num_vals, data = reader->next())
            {
                encode_value(env,
                             reader->getType(), reader->getCount(), data,
                             xml_type, xml_count, values);
            }
        }
        // Assemble result = { name, meta, type, count, values }
        result = xmlrpc_build_value(env, STR("{s:s,s:V,s:i,s:i,s:V}"),
                                    "name",   names[i].c_str(),
                                    "meta",   meta,
                                    "type",   xml_type,
                                    "count",  xml_count,
                                    "values", values);
        xmlrpc_DECREF(meta);
        xmlrpc_DECREF(values);
        // Add to result array
        xmlrpc_array_append_item(env, results, result);
        xmlrpc_DECREF(result);
    }
  exit_from_get_data:
    index->close();
    return results;
}

// { int32  ver, string desc } = archiver.info()
xmlrpc_value *get_info(xmlrpc_env *env, xmlrpc_value *args, void *user)
{
#ifdef LOGFILE
    LOG_MSG("archiver.info\n");
#endif
    char txt[500];
    const char *config = get_config_name(env);
    if (!config)
    {
#ifdef LOGFILE
        LOG_MSG("config: '%s'\n", config);
#endif
        return 0;
    }
    sprintf(txt,
            "Channel Archiver Data Server V%d\n"
            "Config '%s'\n"
            "Supports how=0 (raw), 1 (interpol/average)\n",
            ARCH_VER, config);
    return xmlrpc_build_value(env, STR("{s:i,s:s}"),
                              STR("ver"), ARCH_VER,
                              STR("desc"), STR(txt));
}

// {int32 key, string name, string path}[] = archiver.archives()
xmlrpc_value *get_archives(xmlrpc_env *env, xmlrpc_value *args, void *user)
{
#ifdef LOGFILE
    LOG_MSG("archiver.archives\n");
#endif
    // Get Configuration
    ServerConfig config;
    if (!get_config(env, config))
        return 0;
    // Create result
    xmlrpc_value *result = xmlrpc_build_value(env, STR("()"));
    if (!result)
    {
        xmlrpc_env_set_fault_formatted(env, ARCH_DAT_SERV_FAULT,
                                       "Cannot create result");
        return 0;
    }
    stdList<ServerConfig::Entry>::const_iterator i;
    for (i=config.config.begin(); i!=config.config.end(); ++i)
    {
        xmlrpc_value *archive =
            xmlrpc_build_value(env, STR("{s:i,s:s,s:s}"),
                               STR("key"),  i->key,
                               STR("name"), i->name.c_str(),
                               STR("path"), i->path.c_str());
        xmlrpc_array_append_item(env, result, archive);
        xmlrpc_DECREF(archive);
    }
    return result;
}

// {string name, int32 start_sec, int32 start_nano,
//               int32 end_sec,   int32 end_nano}[]
// = archiver.names(int32 key, string pattern)
xmlrpc_value *get_names(xmlrpc_env *env, xmlrpc_value *args, void *user)
{
#ifdef LOGFILE
    LOG_MSG("archiver.names\n");
#endif
    // Get args, maybe setup pattern
    RegularExpression *regex = 0;
    xmlrpc_int32 key;
    char *pattern;
    size_t pattern_len; 
    xmlrpc_parse_value(env, args, STR("(is#)"), &key, &pattern, &pattern_len);
    if (env->fault_occurred)
        return NULL;
    if (pattern_len > 0)
        regex = RegularExpression::reference(pattern);
    // Create result
    xmlrpc_value *result = xmlrpc_build_value(env, STR("()"));
    if (!result)
    {
        if (regex)
            regex->release();
        xmlrpc_env_set_fault_formatted(env, ARCH_DAT_SERV_FAULT,
                                       "Cannot create result");
        return 0;
    }
    // Open Index
    AutoPtr<IndexFile> index(open_index(env, key));
    if (env->fault_occurred)
    {
        if (regex)
            regex->release();
        return 0;
    }
    // Put all names in binary tree
    IndexFile::NameIterator ni;
    BinaryTree<ChannelInfo> channels;
    ChannelInfo info;
    bool ok;
    for (ok = index->getFirstChannel(ni);
         ok; ok = index->getNextChannel(ni))
    {
        if (regex && !regex->doesMatch(ni.getName()))
            continue; // skip what doesn't match regex
        info.name = ni.getName();
        AutoPtr<RTree> tree(index->getTree(info.name));
        if (tree)
            tree->getInterval(info.start, info.end);
        else
            info.start = info.end = nullTime;
        channels.add(info);
    }
    index->close();
    if (regex)
        regex->release();
    // Sorted dump of names
    ChannelInfo::UserArg user_arg;
    user_arg.env = env;
    user_arg.result = result;
    channels.traverse(ChannelInfo::add_name_to_result, (void *)&user_arg);
#ifdef LOGFILE
    LOG_MSG("get_names(%d, '%s') -> %d names\n",
            key,
            (pattern ? pattern : "<no pattern>"),
            xmlrpc_array_size(env, result));
#endif
    return result;
}

// very_complex_array = archiver.values(key, names[], start, end, ...)
xmlrpc_value *get_values(xmlrpc_env *env, xmlrpc_value *args, void *user)
{
    LOG_MSG("archiver.get_values\n");
    xmlrpc_value *names;
    xmlrpc_int32 key, start_sec, start_nano, end_sec, end_nano, count, how;
    xmlrpc_int32 actual_count;
    // Extract arguments
    xmlrpc_parse_value(env, args, STR("(iAiiiiii)"),
                       &key, &names,
                       &start_sec, &start_nano, &end_sec, &end_nano,
                       &count, &how);    
    if (env->fault_occurred)
        return 0;
    // Put an upper limit on count to avoid outrageous requests:
    if (count > 10000)
        actual_count = 10000;
    else
        actual_count = count;
    // Build start/end
    epicsTime start, end;
    pieces2epicsTime(start_sec, start_nano, start);
    pieces2epicsTime(end_sec, end_nano, end);    
    // Pull names into stdVector<stdString>
    // for SpreadsheetReader and just because.
    xmlrpc_value *name_val;
    char *name;
    int i;
    xmlrpc_int32 name_count = xmlrpc_array_size(env, names);
    stdVector<stdString> name_vector;
    name_vector.reserve(name_count);
    for (i = 0; i < name_count; ++i)
    {   // no new ref!
        name_val = xmlrpc_array_get_item(env, names, i);
        if (env->fault_occurred)
            return 0;
        xmlrpc_parse_value(env, name_val, STR("s"), &name);
        if (env->fault_occurred)
            return 0;
        name_vector.push_back(stdString(name));
    }
    // Build results
    switch (how)
    {
        case 0:
            return get_data(env, key, name_vector, start, end, actual_count, -1.0);
        case 1:
            if (count <= 1)
                count = 1;
            return get_data(env, key, name_vector, start, end, actual_count,
                            (end-start)/count);
    }
    xmlrpc_env_set_fault_formatted(env, ARCH_DAT_ARG_ERROR,
                                   "Invalid how=%d", how);
    return 0;
}

#ifdef LOGFILE
static void LogRoutine(void *arg, const char *text)
{
    if (logfile)
    {
        fputs(text, logfile);
        fflush(logfile);
    }
    else
        printf("log: %s", text);
}
#endif

int main(int argc, const char *argv[])
{
#ifdef LOGFILE
    struct timeval t0, t1;
    gettimeofday(&t0, 0);
    logfile = fopen(LOGFILE, "a");
    TheMsgLogger.SetPrintRoutine(LogRoutine, 0);
    LOG_MSG("---- ArchiveServer Started ----\n");
    gettimeofday(&t0, 0);
#endif
    xmlrpc_cgi_init(XMLRPC_CGI_NO_FLAGS);
    xmlrpc_cgi_add_method_w_doc(STR("archiver.info"),
                                &get_info, 0,
                                STR("S:"),
                                STR("Get info"));
    xmlrpc_cgi_add_method_w_doc(STR("archiver.archives"),
                                &get_archives, 0,
                                STR("A:"),
                                STR("Get archives"));
    xmlrpc_cgi_add_method_w_doc(STR("archiver.names"),
                                &get_names, 0,
                                STR("A:is"),
                                STR("Get channel names"));
    xmlrpc_cgi_add_method_w_doc(STR("archiver.values"),
                                &get_values, 0,
                                STR("A:iAiiiiii"),
                                STR("Get values"));
    xmlrpc_cgi_process_call();
    xmlrpc_cgi_cleanup();
#ifdef LOGFILE
    gettimeofday(&t1, 0);
    double run_secs = (t1.tv_sec + t1.tv_usec/1.0e6)
        - (t0.tv_sec + t0.tv_usec/1.0e6);
    LOG_MSG("ArchiveServer ran %g seconds\n", run_secs);
    if (logfile)
        fclose(logfile);
#endif
    
    return 0;
}
