// System
#include <stdio.h>
#include <time.h>
#include <sys/time.h>
// XML-RPC
#include <xmlrpc.h>
#include <xmlrpc_cgi.h>
// Tools
#include <RegularExpression.h>
#include <BinaryTree.h>
// Storage
#include <SpreadsheetReader.h>
// XMLRPCServer
#include "DataServerFaults.h"

#define ARCH_VER 1
#define LOGFILE "/tmp/archserver.log"

// The xml-rpc API defines "char *" strings
// for what should be "const char *".
// This macro helps avoid those "deprected conversion" warnings of g++
#define STR(s) ((char *)((const char *)s))

char log_line[200];

const char *get_index(xmlrpc_env *env)
{
    const char *name = getenv("INDEX");
    if (!name)
    {
        xmlrpc_env_set_fault_formatted(env, ARCH_DAT_NO_INDEX,
                                       "INDEX is undefined");
        return 0;
    }
    return name;
}

archiver_Index *open_index(xmlrpc_env *env)
{
    archiver_Index *index = new archiver_Index;
    if (!index)
    {
        xmlrpc_env_set_fault_formatted(env, ARCH_DAT_NO_INDEX,
                                       "Cannot allocate index");
        return 0;
    }
    const char *name = get_index(env);
    if (env->fault_occurred)
        return 0;
    if (!index->open(name))
    {
        delete index;
        xmlrpc_env_set_fault_formatted(env, ARCH_DAT_NO_INDEX,
                                       "Cannot open index '%s'",
                                       name);
        return 0;
    }
    return index;
}

// archdat.info returns version information.
// ver    numeric version
// desc   a string description
//
// { int32  ver, string desc } = archdat.info()
xmlrpc_value *info(xmlrpc_env *env,
                   xmlrpc_value *args,
                   void *user)
{
    char txt[200];
    const char *index = get_index(env);
    sprintf(txt, "Channel Archiver Data Server V%d\nIndex '%s'",
            ARCH_VER, index);
    return xmlrpc_build_value(env,
                              STR("{s:i,s:s}"),
                              STR("ver"), ARCH_VER,
                              STR("desc"), STR(txt));
}

// Used by get_names
class ChannelInfo
{
public:
    stdString name;
    epicsTime start, end;

    bool operator < (const ChannelInfo &rhs)
    { return name < rhs.name; }

    bool operator == (const ChannelInfo &rhs)
    { return name == rhs.name; }

    class UserArg
    {
    public:
        xmlrpc_env *env;
        xmlrpc_value *result;
    };
    
    // "visitor" for BinaryTree of channel names
    static void add_name_to_result(const ChannelInfo &info, void *arg)
    {
        UserArg *user_arg = (UserArg *)arg;
        xmlrpc_value *channel;

        epicsTimeStamp start = info.start;
        epicsTimeStamp end = info.end;
        time_t_wrapper start_secs = info.start;
        time_t_wrapper end_secs = info.end;
        
        channel = xmlrpc_build_value(user_arg->env,
                                     STR("{s:s,s:i,s:i,s:i,s:i}"),
                                     "name", info.name.c_str(),
                                     "start_sec", start_secs,
                                     "start_nano", start.nsec,
                                     "end_sec", end_secs,
                                     "end_nano", end.nsec);
        if (channel)
        {
            xmlrpc_array_append_item(user_arg->env,
                                     user_arg->result, channel);
            xmlrpc_DECREF(channel);
        }
    }
};

    
// {string name, int32 start_sec, int32 start_nano,
//               int32 end_sec,   int32 end_nano}[]
// = archiver.get_names(string pattern)
xmlrpc_value *get_names(xmlrpc_env *env,
                        xmlrpc_value *args,
                        void *user)
{
    // Get args, maybe setup pattern
    RegularExpression *regex = 0;
    char *pattern;
    size_t pattern_len; 
    xmlrpc_parse_value(env, args, STR("(s#)"),
                       &pattern, &pattern_len);
    if (env->fault_occurred)
        return NULL;
    if (pattern_len > 0)
    {
        regex = RegularExpression::reference(pattern);
    }
    // Open Index
    archiver_Index *index = open_index(env);
    if (env->fault_occurred)
        return 0;
    // Create result
    xmlrpc_value *result = xmlrpc_build_value(env, STR("()"));
    if (!result)
    {
        delete index;
        xmlrpc_env_set_fault_formatted(env, ARCH_DAT_SERV_FAULT,
                                       "Cannot create result");
        return 0;
    }  
    channel_Name_Iterator *cni = index->getChannelNameIterator();
    if (!cni)
    {
        delete index;
        xmlrpc_env_set_fault_formatted(env, ARCH_DAT_SERV_FAULT,
                                       "Cannot get name iterator");
        return 0;
    }
    // Put all names in binary tree
 	BinaryTree<ChannelInfo> channels;
    ChannelInfo info;
    interval range;
    bool ok;
    for (ok = cni->getFirst(&info.name); ok; ok = cni->getNext(&info.name))
    {
        if (regex && !regex->doesMatch(info.name.c_str()))
            continue; // skip what doesn't match regex
        if (index->getEntireIndexedInterval(info.name.c_str(), &range))
        {
            info.start = range.getStart();
            info.end = range.getEnd();
        }
        else
        {
            info.start = info.end = nullTime;
        }
        channels.add(info);
    }
    delete cni;
    delete index;
    if (regex)
        regex->release();
    // Sorted dump of names
    ChannelInfo::UserArg user_arg;
    user_arg.env = env;
    user_arg.result = result;
    channels.traverse(ChannelInfo::add_name_to_result, (void *)&user_arg);    
    return result;
}

static xmlrpc_value *make_ctrl_info(xmlrpc_env *env,
                                    const char *units)
{
	xmlrpc_value *meta =
        xmlrpc_build_value(env,
                           STR("{s:i,s:d,s:d,s:d,s:d,s:d,s:d,s:i,s:s}"),
                           "type", (xmlrpc_int32)1,
                           "disp_high",  10.0, "disp_low",    0.0,
                           "alarm_high", 10.0, "warn_high",   9.0,
                           "warn_low",    1.0, "alarm_low",   0.0,
                           "prec", (xmlrpc_int32)4,
                           "units",     units);
	return meta;
}

xmlrpc_value *get_values(xmlrpc_env *env,
                         xmlrpc_value *args,
                         void *user)
{
    xmlrpc_value *names;
    xmlrpc_value *name_val, *results, *result, *meta;
    xmlrpc_value *values, *val_array, *value;
    xmlrpc_int32 start_sec, start_nano, end_sec, end_nano, count, how;
    xmlrpc_int32 secs, nano;
    xmlrpc_int32 name_count, name_index, name_len, i;
    char *name;

    // Extract arguments
    xmlrpc_parse_value(env, args, STR("(Aiiiiii)"),
                       &names,
                       &start_sec, &start_nano, &end_sec, &end_nano,
                       &count, &how);
    if (env->fault_occurred)
        return 0;

    if (count == 42)
    {
        xmlrpc_env_set_fault_formatted(
            env, ARCH_DAT_SERV_FAULT,
            "I don't like count==%d", count);
        return 0;
    }

    name_count = xmlrpc_array_size(env, names);
    // Build result for each requested channel name
    results = xmlrpc_build_value(env, STR("()"));
    for (name_index = 0; name_index < name_count; ++name_index)
    {
        // extract name from array (no new ref!)
        name_val = xmlrpc_array_get_item(env, names, name_index);
        if (env->fault_occurred)
        {
            xmlrpc_DECREF(results);
            return NULL;
        }
        xmlrpc_parse_value(env, name_val, STR("s#"),
                           &name, &name_len);                       
        // Meta information
        meta = make_ctrl_info(env, name);
        // Values
        values = xmlrpc_build_value(env, STR("()"));
        for (i=0; i<count; ++i)
        {
            secs = start_sec + i*(end_sec - start_sec)/count;
            nano = start_nano + i*(end_nano - start_nano)/count;
            val_array = xmlrpc_build_value(env, STR("(d)"),
                                           ((double)3.14+i));
            value = xmlrpc_build_value(env,
                                       STR("{s:i,s:i,s:i,s:i,s:V}"),
                                       "stat", (xmlrpc_int32)0,
                                       "sevr", (xmlrpc_int32)0,
                                       "secs", (xmlrpc_int32)secs,
                                       "nano", (xmlrpc_int32)nano,
                                       "value", val_array);
            xmlrpc_DECREF(val_array);
            xmlrpc_array_append_item(env, values, value);
            xmlrpc_DECREF(value);
        }
        // Assemble channel = { meta, data }
        result = xmlrpc_build_value(env, STR("{s:s,s:V,s:i,s:i,s:V}"),
                                    "name", name,
                                    "meta", meta,
                                    "type", (xmlrpc_int32) 3,
                                    "count",(xmlrpc_int32) 1,
                                    "values", values);
        xmlrpc_DECREF(meta);
        xmlrpc_DECREF(values);
        // Add to result array
        xmlrpc_array_append_item(env, results, result);
        xmlrpc_DECREF(result);
    }
    sprintf(log_line, "get_values(%d channels, each %d values = %d values)",
            name_count, count, name_count*count);
    return results;
}

int main(int argc, const char *argv[])
{
    log_line[0] = '\0';
    struct timeval t0, t1;
    gettimeofday(&t0, 0);
    
    xmlrpc_cgi_init(XMLRPC_CGI_NO_FLAGS);

    xmlrpc_cgi_add_method_w_doc(STR("archiver.info"),
                                &info, 0,
                                STR("S:"),
                                STR("Get info"));
    xmlrpc_cgi_add_method_w_doc(STR("archiver.get_names"),
                                &get_names, 0,
                                STR("A:s"),
                                STR("Get channel names"));
    xmlrpc_cgi_add_method_w_doc(STR("archiver.get_values"),
                                &get_values, 0,
                                STR("A:siiiiii"),
                                STR("Get values"));
    xmlrpc_cgi_process_call();
    xmlrpc_cgi_cleanup();


    gettimeofday(&t1, 0);
    double run_secs = (t1.tv_sec + t1.tv_usec/1.0e6)
        - (t0.tv_sec + t0.tv_usec/1.0e6);
    FILE *f = fopen(LOGFILE, "a");
    if (f)
    {
        fprintf(f, "%s ran %g seconds\n",
                log_line, run_secs);
        fclose(f);
    }
    
    return 0;
}





