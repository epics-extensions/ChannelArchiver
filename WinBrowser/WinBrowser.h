// WinBrowser.h

#if !defined(AFX_WINBROWSER_H__7DA2BA84_7079_11D3_BE4C_00105AC8D47C__INCLUDED_)
#define AFX_WINBROWSER_H__7DA2BA84_7079_11D3_BE4C_00105AC8D47C__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"		// main symbols

class CBrowserApp : public CWinApp
{
public:
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CBrowserApp)
	public:
	virtual BOOL InitInstance();
	//}}AFX_VIRTUAL

	//{{AFX_MSG(CBrowserApp)
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}

#endif // !defined(AFX_WINBROWSER_H__7DA2BA84_7079_11D3_BE4C_00105AC8D47C__INCLUDED_)
