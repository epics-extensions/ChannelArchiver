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
    if (nrhs != 1)
        mexErrMsgTxt("Expected value handle.");
    if (!mxIsUint32(prhs[0]) || mxGetNumberOfElements(prhs[0]) != 1)
        mexErrMsgTxt("Wrong argument type, expected value handle.");

    UINT32_T *arg = (UINT32_T *)mxGetData(prhs[0]);
    ValueIteratorI *value = (ValueIteratorI *) arg[0];

    char datestr[MatlabExporter::DATESTR_LEN];
    if (value && value->isValid())
        MatlabExporter::osiTime2datestr(value->getValue()->getTime(), datestr);
    else
        strcpy(datestr, "<invalid>");
    plhs[0] = mxCreateString(datestr);
}
} // extern "C"
 
