#include "stdafx.h"
#include "MsgBoxF.h"
#include <stdarg.h>

void MsgBoxF (const char *format, ...)
{
	CString	txt;
	va_list	ap;

	va_start (ap, format);

	vsprintf (txt.GetBuffer (200), format, ap);
	txt.ReleaseBuffer ();

	va_end (ap);

	AfxMessageBox (txt);
}
