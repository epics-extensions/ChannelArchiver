// ArchiveData.cpp
//
// Interfaces the Channel Archiver's Network Data Server
// to Matlab and GNU Octave.
//
// See ArchiveData.m for usage, Makefile for compilation.
//
// kasemir@lanl.gov

#undef DEBUG

// Generic Stuff -------------------------------------------------------
#include <ArchiveDataClient.h>

#define OneSecond (1.0/24/60/60)
static double epicsTime2DateNum(const epicsTime &t)
{
    xmlrpc_int32 secs, nano;
    epicsTime2pieces(t, secs, nano);
    if (!timezone) // either not set or we're in England
        tzset();
    time_t local_secs = secs - timezone;
    // From Paul Kienzle's collection of Matlab-compatibility
    // routines for Octave:
    // seconds since 1970-1-1 divided by 86400 sec/day
    // plus datenum(1970,1,1).
    // For an unknown reason, octave has a ~1sec rounding
    // problem.
    return (double)local_secs/86400.0 + 719529.0
#ifdef OCTAVE
        + OneSecond/2
#endif
        ;
}    

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
            string_vector paths(num);
            for (i=0; i<num; ++i)
            {
                keys(i) = (double)archives[i].key;
                names[i] = archives[i].name.c_str();
                paths[i] = archives[i].path.c_str();
            }
            retval.append(octave_value(keys));
            retval.append(octave_value(names));
            retval.append(octave_value(paths));
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
            ColumnVector  start(num);
            ColumnVector  end(num);
            for (i=0; i<num; ++i)
            {
                names[i] = name_infos[i].name.c_str();
                start(i) = epicsTime2DateNum(name_infos[i].start);
                end(i) = epicsTime2DateNum(name_infos[i].end);
            }
            retval.append(octave_value(names));
            retval.append(octave_value(start));
            retval.append(octave_value(end));
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
    ArchiveDataClient client(url);
    if (!strcmp(cmd, "info"))
    {
        int                  version;
        stdString            description;
        stdVector<stdString> how_strings, stat_strings;
        stdVector<ArchiveDataClient::SeverityInfo> sevr_infos;
        if (client.getInfo(version, description, how_strings,
                           stat_strings, sevr_infos))
        {
            result[0] = mxCreateDoubleScalar((double)version);
            if (nresult > 1)
                result[1] = mxCreateStringFromNChars(description.c_str(),
                                                     description.length());
        }
        else
            mexErrMsgIdAndTxt("ArchiveData", "'%s' failed\n", cmd);
    }
    else if (!strcmp(cmd, "archives"))
    {
        stdVector<ArchiveDataClient::ArchiveInfo> archives;
        if (client.getArchives(archives))
        {
            size_t        i, num = archives.size();
            result[0] = mxCreateDoubleMatrix(num, 1, mxREAL);
            double *keys = mxGetPr(result[0]);
            if (nresult > 1)
                result[1] = mxCreateCellMatrix(num, 1);
            if (nresult > 2)
                result[2] = mxCreateCellMatrix(num, 1);
            for (i=0; i<num; ++i)
            {
                keys[i] = (double)archives[i].key;
                if (nresult > 1)
                    mxSetCell(result[1], i,
                              mxCreateStringFromNChars(
                                  archives[i].name.c_str(),
                                  archives[i].name.length()));
                if (nresult > 2)
                    mxSetCell(result[2], i,
                              mxCreateStringFromNChars(
                                  archives[i].path.c_str(),
                                  archives[i].path.length()));
            }
        }
        else
            mexErrMsgIdAndTxt("ArchiveData", "'%s' failed\n", cmd);
    }
    else if (!strcmp(cmd, "names"))
    {
        if (nargin < 3 || nargin > 4)
        {
            mexErrMsgTxt("Need URL, CMD, KEY [, PATTERN]\n");
            return;
        }
        stdString pattern;
        if (nargin == 4)
        {
            char *patt;
            if (!(mxIsChar(args[3]) && get_string(args[3], &patt, "PATTERN")))
            {
                mexErrMsgTxt("PATTERN must be a string");
                return;
            }
            pattern = patt;
        }
        stdVector<ArchiveDataClient::NameInfo> name_infos;
        if (client.getNames(key, pattern, name_infos))
        {
            size_t i, num = name_infos.size();
            double *start = 0, *end = 0;
            result[0] = mxCreateCellMatrix(num, 1);
            if (nresult > 1)
            {
                result[1] = mxCreateDoubleMatrix(num, 1, mxREAL);
                start = mxGetPr(result[1]);
            }
            if (nresult > 2)
            {
                result[2] = mxCreateDoubleMatrix(num, 1, mxREAL);
                end = mxGetPr(result[2]);
            }
            for (i=0; i<num; ++i)
            {
                mxSetCell(result[0], i,
                          mxCreateStringFromNChars(
                              name_infos[i].name.c_str(),
                              name_infos[i].name.length()));
                if (start)
                    start[i] = epicsTime2DateNum(name_infos[i].start);
                if (end)
                    end[i] = epicsTime2DateNum(name_infos[i].end);
            }
        }
        else
            mexErrMsgIdAndTxt("ArchiveData", "'%s' failed\n", cmd);
    }
    else
        mexErrMsgIdAndTxt("ArchiveData", "Unknown command '%s'", cmd);

}

#endif

