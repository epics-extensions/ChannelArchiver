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
void mexFunction(int nlhs, mxArray *plhs[],
                 int nrhs, const mxArray *prhs[])
{
    if (nrhs < 1  || nrhs > 2)
        mexErrMsgTxt("Expected archive handle and maybe a channel name.");
    if (!mxIsUint32(prhs[0]) || mxGetNumberOfElements(prhs[0]) != 1)
        mexErrMsgTxt("Wrong argument type 1, expected archive handle.");
    
    if (nrhs == 2 && !mxIsChar(prhs[1]))
        mexErrMsgTxt("Wrong argument type 2, expected channel name.");

    UINT32_T *arg = (UINT32_T *)mxGetData(prhs[0]);
    ArchiveI *archive = (ArchiveI *) arg[0];
    if (!archive)
        mexErrMsgTxt("Invalid archive handle.");

    char *name;
    if (nrhs == 2)
    {
        int len = mxGetNumberOfElements(prhs[1]) + 1;
        name = (char *)mxCalloc(len+1, sizeof(char));
        mxGetString(prhs[1], name, len+1);
    }
    else
        name = 0;

    ChannelIteratorI *channel = archive->newChannelIterator();
    if (name)
        archive->findChannelByName(name, channel);
    else
        archive->findFirstChannel(channel);

    plhs[0] = mxCreateNumericMatrix(1, 1, mxUINT32_CLASS, 0);
    *((UINT32_T *)mxGetData(plhs[0])) = (UINT32_T)channel;
}
} // extern "C"
 
