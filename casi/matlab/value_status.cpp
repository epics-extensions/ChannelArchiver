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

    if (value && value->isValid())
    {
        stdString stat;
        value->getValue()->getStatus(stat);
        plhs[0] = mxCreateStringFromNChars(stat.c_str(), stat.length());
    }
    else
        plhs[0] = mxCreateString("<invalid>");
}
} // extern "C"
 
