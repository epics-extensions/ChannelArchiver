/* $Id$
 *
 * CASI MEX
 *
 * kasemir@lanl.gov
 */

#include <stdio.h>
#include <string.h>
#include "mex.h"
#include "MultiArchive.h"
#include "MatlabExporter.h"

extern "C"
{
void mexFunction(int nlhs, mxArray *plhs[],
                 int nrhs, const mxArray *prhs[])
{
    if (nrhs < 2  || nrhs > 3)
        mexErrMsgTxt("Expected channel and value handle and maybe a datestr.");
    if (!mxIsUint32(prhs[0]) || mxGetNumberOfElements(prhs[0]) != 1)
        mexErrMsgTxt("Wrong argument type 1, expected channel handle.");
    if (!mxIsUint32(prhs[1]) || mxGetNumberOfElements(prhs[1]) != 1)
        mexErrMsgTxt("Wrong argument type 2, expected value handle.");
    if (nrhs == 3 && !mxIsChar(prhs[2]))
        mexErrMsgTxt("Wrong argument type 3, expected datestr.");

    UINT32_T *arg;
    arg = (UINT32_T *)mxGetData(prhs[0]);
    ChannelIteratorI *channel = (ChannelIteratorI *) arg[0];
    if (!(channel && channel->isValid()))
        mexErrMsgTxt("Invalid channel handle.");
    arg = (UINT32_T *)mxGetData(prhs[1]);
    ValueIteratorI *value = (ValueIteratorI *) arg[0];
    if (!value)
        mexErrMsgTxt("Invalid value handle.");

    char *datestr;
    if (nrhs == 3)
    {
        int len = mxGetNumberOfElements(prhs[2]) + 1;
        datestr = (char *)mxCalloc(len+1, sizeof(char));
        mxGetString(prhs[2], datestr, len+1);
    }
    else
        datestr = 0;

    bool ok;
    if (datestr)
    {
        osiTime time;
        if (! MatlabExporter::datestr2osiTime(datestr, time))
            mexErrMsgTxt("Cannot decode datestr.");
        ok = channel->getChannel()->getValueAfterTime(time, value);
    }
    else
        ok = channel->getChannel()->getFirstValue(value);

    plhs[0] = mxCreateScalarDouble((double)ok);
}
} // extern "C"
 
