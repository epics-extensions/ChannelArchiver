// System
#include <stdio.h>
// XML-RPC
#include <xmlrpc.h>
#include <xmlrpc_cgi.h>

#define ARCH_VER 2

// archdat.info returns version information.
// ver    numeric version
// desc   a string description
//
// { int32  ver, string desc } = archdat.info()
xmlrpc_value *info(xmlrpc_env *env,
                   xmlrpc_value *args,
                   void *user)
{
    char txt[100];
    sprintf(txt, "Channel Archiver Data Server V%d",
            ARCH_VER);
    return xmlrpc_build_value(env,
                              "{s:i,s:s}",
                              "ver", ARCH_VER,
                              "desc", txt);
}

// string name[] = archdat.get_names(string pattern)
xmlrpc_value *get_names(xmlrpc_env *env,
                        xmlrpc_value *args,
                        void *user)
{
    char *pattern;
    size_t pattern_len;
    xmlrpc_parse_value(env, args, "(s#)",
                       &pattern, &pattern_len);

    xmlrpc_value *result, *name;
    result = xmlrpc_build_value(env, "()");

    if (pattern_len > 0)
    {
        name = xmlrpc_build_value(env, "s", pattern);
        xmlrpc_array_append_item(env, result, name);
        xmlrpc_DECREF(name);
    }
    else
    {
        name = xmlrpc_build_value(env, "s", "janet");
        xmlrpc_array_append_item(env, result, name);
        xmlrpc_DECREF(name);
        
        name = xmlrpc_build_value(env, "s", "jane");
        xmlrpc_array_append_item(env, result, name);
        xmlrpc_DECREF(name);        
    }
    return result;
}


int main(int argc, const char *argv[])
{
    xmlrpc_cgi_init(XMLRPC_CGI_NO_FLAGS);

    xmlrpc_cgi_add_method_w_doc("archdat.info",
                                &info, 0,
                                "S:",
                                "Get info");
    xmlrpc_cgi_add_method_w_doc("archdat.get_names",
                                &get_names, 0,
                                "A:s",
                                "Get channel names");
    xmlrpc_cgi_process_call();
    xmlrpc_cgi_cleanup();
    return 0;
}





