// ArchiveData.cpp
//
// Interfaces the Channel Archiver's Network Data Server
// to Matlab and GNU Octave.
//
// See ArchiveData.m for usage, Makefile for compilation.
//
// kasemir@lanl.gov

#define DEBUG

// Generic Stuff -------------------------------------------------------
#include <ArchiveDataClient.h>

// Octave Stuff --------------------------------------------------------
#ifdef OCTAVE
#include <octave/oct.h>
#include <octave/pager.h>
DEFUN_DLD(ArchiveData, args, nargout, "ArchiveData: See ArchiveData.m")
{
    octave_value_list retval;
    int nargin = args.length();
    if (nargin < 2)
    {
        error("At least two arguments (URL, CMD) required\n");
        return retval;
    }
    if (!(args(0).is_string() && args(1).is_string()))
    {
        error("URL & CMD must be strings");
        return retval;
    }
    const std::string &url = args(0).string_value();
    const std::string &cmd = args(1).string_value();
    int key = 0;
    if (nargin >= 3)
    {
        if (!args(2).is_real_scalar())
        {
            error("KEY must be a number");
            return retval;
        }
        key = args(2).int_value();
    }
#ifdef DEBUG
    octave_stdout << "URL='" << url << "'\n";
    octave_stdout << "CMD='" << cmd << "'\n";
    octave_stdout << "KEY='" << key << "'\n";
#endif
    ArchiveDataClient client(url.c_str());
    if (cmd == "info")
    {
        int                  version;
        stdString            description;
        stdVector<stdString> how_strings, stat_strings;
        stdVector<ArchiveDataClient::SeverityInfo> sevr_infos;
        if (client.getInfo(version, description, how_strings,
                           stat_strings, sevr_infos))
        {
            retval.append(octave_value((double)version));
            retval.append(octave_value(description.c_str()));
        }
        else
            error("'%s' failed\n", cmd.c_str());
    }
    else if (cmd == "archives")
    {
        stdVector<ArchiveDataClient::ArchiveInfo> archives;
        if (client.getArchives(archives))
        {
            size_t        i, num = archives.size();
            ColumnVector  keys(num);
            string_vector names(num);
            for (i=0; i<num; ++i)
            {
                keys(i) = (double)archives[i].key;
                names[i] = archives[i].name.c_str();
            }
            retval.append(octave_value(keys));
            retval.append(octave_value(names));
        }
        else
            error("'%s' failed\n", cmd.c_str());
    }
    else if (cmd == "names")
    {
        if (nargin < 3 || nargin > 4)
        {
            error("Need URL, CMD, KEY [, PATTERN]\n");
            return retval;
        }
        stdString pattern;
        if (nargin == 4)
        {
            if (!args(3).is_string())
            {
                error("PATTERN must be a string");
                return retval;
            }
            pattern = args(3).string_value().c_str();
        }
        stdVector<ArchiveDataClient::NameInfo> name_infos;
        if (client.getNames(key, pattern, name_infos))
        {
            size_t i, num = name_infos.size();
            string_vector names(num);
            for (i=0; i<num; ++i)
            {
                names[i] = name_infos[i].name.c_str();
            }
            retval.append(octave_value(names));
        }
        else
            error("'%s' failed\n", cmd.c_str());
    }
    else
        error("Unknown command '%s'\n", cmd.c_str());
    return retval;
}

#else
// Matlab Stuff --------------------------------------------------------
#include "mex.h"

static bool get_string(const mxArray *arg, char **buf, const char *name)
{
    int buflen = mxGetNumberOfElements(arg) + 1;
    *buf = (char *)mxCalloc(buflen, sizeof(char));
    if (mxGetString(arg, *buf, buflen) != 0)
    {
        mexErrMsgIdAndTxt("ArchiveData", "Could not fetch %s", name);
        return false;
    }
    return true;
}

void mexFunction(int nresult, mxArray *result[],
                 int nargin, const mxArray *args[])
{
    if (nargin < 2)
    {
        mexErrMsgTxt("At least two arguments (URL, cmd) required.");
        return;
    }
    if (!(mxIsChar(args[0]) && mxIsChar(args[1])))
    {
        mexErrMsgTxt("URL & CMD must be strings.");
        return;
    }
    char *url, *cmd;
    int key = 0;
    if (! (get_string(args[0], &url, "URL") &&
           get_string(args[1], &cmd, "CMD")))
        return;
    if (nargin >= 3)
        key = (int)mxGetScalar(args[2]);
#ifdef DEBUG
    mexPrintf("URL='%s'\n", url);
    mexPrintf("CMD='%s'\n", cmd);
    mexPrintf("KEY=%d\n", key);
#endif
}

#endif

