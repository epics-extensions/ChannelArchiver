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
    if (value && value->isValid() && !value->getValue()->isInfo())
    {
        int n = value->getValue()->getCount();
        plhs[0] = mxCreateDoubleMatrix(n, 1, mxREAL);
        double *vals = mxGetPr(plhs[0]);
        for (int i=0; i<n; ++i)
            vals[i] = value->getValue()->getDouble(i);
    }
    else
        plhs[0] = mxCreateScalarDouble(mxGetNaN());
}
} // extern "C"
 
