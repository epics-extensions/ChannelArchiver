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
//#include <octave/config.h>
//#include <iostream.h>
//#include <octave/defun-dld.h>
//#include <octave/error.h>
//#include <octave/oct-obj.h>
#include <octave/pager.h>
//#include <octave/symtab.h>
//#include <octave/variables.h>
DEFUN_DLD(ArchiveData, args, nargout, "ArchiveData: See ArchiveData.m")
{
    octave_value_list retval;
    int nargin = args.length();
    if (nargin < 2)
    {
        error("Need at least two arguments\n");
        return retval;
    }
    if (!(args(0).is_string() && args(1).is_string()))
    {
        error("URL & CMD must be strings");
        return retval;
    }
    const std::string &url = args(0).string_value();
    const std::string &cmd = args(1).string_value();
#ifdef DEBUG
    octave_stdout << "URL='" << url << "'\n";
    octave_stdout << "CMD='" << cmd << "'\n";
#endif
    if (cmd == "info")
    {
        ArchiveDataClient    client(url.c_str());
        int                  version;
        stdString            description;
        stdVector<stdString> how_strings, stat_strings;
        stdVector<ArchiveDataClient::SeverityInfo> sevr_infos;
        if (client.getInfo(version, description, how_strings, stat_strings,
                           sevr_infos))
        {
            retval.append(octave_value((double)version));
            retval.append(octave_value(description.c_str()));
        }
        else
            error("'info' failed\n");
    }
    else if (cmd == "archives")
    {
        ArchiveDataClient                         client(url.c_str());
        stdVector<ArchiveDataClient::ArchiveInfo> archives;
        if (client.getArchives(archives))
        {
            size_t i, num = archives.size();
            octave_value_list keys;
            string_vector names(num);
            for (i=0; i<num; ++i)
            {
                keys.append(octave_value((double)archives[i].key));
                names[i] = archives[i].name.c_str();
            }
            retval.append(octave_value(keys));
            retval.append(octave_value(names));
        }
        else
            error("'archives' failed\n");
    }
    else
    {
        error("Unknown command %s\n", cmd.c_str());
    }
#if 0
    int key = args(1).int_value();
    if (!args(1).is_real_scalar())
    {
        error("key must be a number");
        return retval;
    }
#endif
    
    return retval;
}

#else
// Matlab Stuff --------------------------------------------------------
#include "mex.h"
void mexFunction(int nlhs, mxArray *plhs[],
                 int nrhs, const mxArray *prhs[])
{
    if (nrhs < 2)
    {
        mexErrMsgTxt("At least two arguments (URL, cmd) required\n");
        return;
    }
}

#endif

