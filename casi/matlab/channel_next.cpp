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
    if (nrhs != 1)
        mexErrMsgTxt("channel handle.");
    if (! mxIsUint32(prhs[0]) || mxGetNumberOfElements(prhs[0]) != 1)
        mexErrMsgTxt("Wrong argument type, expected channel handle.");

    UINT32_T *arg = (UINT32_T *)mxGetData(prhs[0]);
    ChannelIteratorI *channel = (ChannelIteratorI *) arg[0];

    bool valid;
    if (channel && channel->isValid())
        valid = channel->next();
    else
        valid = false;
    plhs[0] = mxCreateScalarDouble(valid ? 1.0 : 0.0);
}
} // extern "C"
 
