//
// A T A C . c p p 
//
// "A Tcl Archive Client"
//
// KUK
//

#ifndef INCLUDE_ATAC_H
#define INCLUDE_ATAC_H

#include <tcl.h>

#ifdef BUILD_atac
#	undef TCL_STORAGE_CLASS
#	define TCL_STORAGE_CLASS DLLEXPORT
#endif

EXTERN int Atac_Init (Tcl_Interp *interp);
EXTERN int atac_archiveCmd (ClientData clientData,
		Tcl_Interp *interp, int objc, struct Tcl_Obj * CONST objv[]);

#	undef TCL_STORAGE_CLASS
#	define TCL_STORAGE_CLASS DLLIMPORT

#endif
