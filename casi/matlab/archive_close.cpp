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

extern "C"
{
/* mexFunction is the gateway routine for the MEX-file. */ 
void mexFunction(int nlhs, mxArray *plhs[],
                 int nrhs, const mxArray *prhs[])
{
    if (nrhs != 3)
        mexErrMsgTxt("Expected archive, channel and value handles.");
    if (! mxIsUint32(prhs[0]) ||
        mxGetNumberOfElements(prhs[0]) != 1)
        mexErrMsgTxt("Wrong argument type 1, expected archive handle.");
    if (! mxIsUint32(prhs[1]) ||
        mxGetNumberOfElements(prhs[1]) != 1)
        mexErrMsgTxt("Wrong argument type 2, expected channel handle.");
    if (! mxIsUint32(prhs[2]) ||
        mxGetNumberOfElements(prhs[2]) != 1)
        mexErrMsgTxt("Wrong argument type 3, expected value handle.");

    UINT32_T *arg;
    arg = (UINT32_T *)mxGetData(prhs[2]);
    ValueIteratorI *value = (ValueIteratorI *) arg[0];
    if (value)
        delete value;

    arg = (UINT32_T *)mxGetData(prhs[1]);
    ChannelIteratorI *channel = (ChannelIteratorI *) arg[0];
    if (channel)
        delete channel;

    arg = (UINT32_T *)mxGetData(prhs[0]);
    ArchiveI *archive = (ArchiveI *) arg[0];
    if (archive)
        delete archive;
}
} // extern "C"
 
