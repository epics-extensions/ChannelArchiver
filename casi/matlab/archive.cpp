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
        mexErrMsgTxt("Expected archive name.");
    if (! mxIsChar(prhs[0]))
        mexErrMsgTxt("Wrong argument type, expected archive name.");
        
    int len = mxGetNumberOfElements(prhs[0]) + 1;
    char *name = (char *)mxCalloc(len+1, sizeof(char));
    mxGetString(prhs[0], name, len+1);
    
    ArchiveI *archive = 0;
    try
    {
        archive = new MultiArchive(name);
    }
    catch (GenericException &e)
    {
        mexErrMsgTxt(e.what());
    }
    
    plhs[0] = mxCreateNumericMatrix(1, 1, mxUINT32_CLASS, 0);
    *((UINT32_T *)mxGetData(plhs[0])) = (UINT32_T)archive;
}
} // extern "C"
 
