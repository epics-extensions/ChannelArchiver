// System
#include <stdio.h>
#include <time.h>
// XML-RPC
#include <xmlrpc.h>
#include <xmlrpc_cgi.h>
// Tools
#include "RegularExpression.h"
// XMLRPCServer
#include "DataServer.h"

static const char *names[] =
{
    "fred", "freddy", "jane", "janet", "Egon",
    "Fritz", "Ernie", "Bert", "Bimbo"
};
#define NUM_NAMES  (sizeof(names)/sizeof(const char *))

// archdat.info returns version information.
// ver    numeric version
// desc   a string description
//
// { int32  ver, string desc } = archdat.info()
xmlrpc_value *get_info(xmlrpc_env *env, xmlrpc_value *args, void *user)
{
    char txt[100];
    sprintf(txt, "Channel Archiver Dummy Data Server V%d",
            ARCH_VER);
    return xmlrpc_build_value(env,
                              STR("{s:i,s:s}"),
                              STR("ver"), ARCH_VER,
                              STR("desc"), STR(txt));
}

// {int32 key, string name, string path}[] = archiver.archives()
xmlrpc_value *get_archives(xmlrpc_env *env, xmlrpc_value *args, void *user)
{
    // Create result
    xmlrpc_value *archive, *result = xmlrpc_build_value(env, STR("()"));
    if (!result)
    {
        xmlrpc_env_set_fault_formatted(env, ARCH_DAT_SERV_FAULT,
                                       "Cannot create result");
        return 0;
    }
    archive = xmlrpc_build_value(env, STR("{s:i,s:s,s:s}"),
                                 STR("key"),  (xmlrpc_int32)42,
                                 STR("name"), STR("Dummy 1"),
                                 STR("path"), STR("/dev/null/index1"));
    xmlrpc_array_append_item(env, result, archive);
    xmlrpc_DECREF(archive);
    archive = xmlrpc_build_value(env, STR("{s:i,s:s,s:s}"),
                                 STR("key"),  (xmlrpc_int32)47,
                                 STR("name"), STR("Dummy 2"),
                                 STR("path"), STR("/dev/null/index2"));
    xmlrpc_array_append_item(env, result, archive);
    xmlrpc_DECREF(archive);
    
    return result;
}

// {string name, int32 start_sec, int32 start_nano,
//               int32 end_sec,   int32 end_nano}[]
// = archiver.get_names(int32 key, string pattern)
xmlrpc_value *get_names(xmlrpc_env *env,
                        xmlrpc_value *args,
                        void *user)
{
    RegularExpression *re = 0;
    xmlrpc_int32 key;
    char *pattern;
    size_t pattern_len; 
    xmlrpc_parse_value(env, args, STR("(is#)"), &key, &pattern, &pattern_len);
    if (env->fault_occurred)
        return NULL;
    if (pattern_len > 0)
    {
        re = RegularExpression::reference(pattern);
    }

    xmlrpc_value *result, *channel;
    result = xmlrpc_build_value(env, STR("()"));

    struct tm tm;
    tm.tm_sec = 1;
    tm.tm_min = 2;
    tm.tm_hour = 3;
    tm.tm_mday = 1;
    tm.tm_mon = 0;
    tm.tm_year = 75;
    tm.tm_isdst = -1;       
    time_t start = mktime(&tm);
    size_t i;
    for (i=0; i<NUM_NAMES; ++i)
    {
        if (re &&
            re->doesMatch(names[i]) == false)
            continue;
        channel = xmlrpc_build_value(env, STR("{s:s,s:i,s:i,s:i,s:i}"),
                                     "name", names[i],
                                     "start_sec", start,
                                     "start_nano", i,
                                     "end_sec", time(0),
                                     "end_nano", i);
        xmlrpc_array_append_item(env, result, channel);
        xmlrpc_DECREF(channel);
    }    
    if (re)
        re->release();
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
    xmlrpc_int32 key, start_sec, start_nano, end_sec, end_nano, count, how;
    xmlrpc_int32 secs, nano;
    xmlrpc_int32 name_count, name_index, name_len, i;
    char *name;

    // Extract arguments
    xmlrpc_parse_value(env, args, STR("(iAiiiiii)"),
                       &key, &names,
                       &start_sec, &start_nano, &end_sec, &end_nano,
                       &count, &how);
    if (env->fault_occurred)
        return NULL;
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
    return results;
}

int main(int argc, const char *argv[])
{
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
    return 0;
}





