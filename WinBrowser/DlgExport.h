// DlgExport.h

#if !defined(AFX_DLGEXPORT_H__EA91FD41_7826_11D3_BE5C_00105AC8D47C__INCLUDED_)
#define AFX_DLGEXPORT_H__EA91FD41_7826_11D3_BE5C_00105AC8D47C__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif

class CDlgExport : public CDialog
{
public:
	CDlgExport(CWnd* pParent = NULL);   // standard constructor

	// Trick for Radio Buttons:
	// * have to be in tab sequence
	// * first one sets "group"
	enum
	{
		GNUPlot,
		SpreadSheet,
        Matlab
	};
	enum
	{
		Raw,
		Linear,
		Fill
	};
	//{{AFX_DATA(CDlgExport)
	enum { IDD = IDD_EXPORT };
	CString	m_filename;
	int m_format;
	int m_interpol;
	double	m_seconds;
	//}}AFX_DATA

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CDlgExport)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

protected:
	//{{AFX_MSG(CDlgExport)
	afx_msg void OnPickExportName();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
	static CString	m_last_filename;
};

//{{AFX_INSERT_LOCATION}}

#endif // !defined(AFX_DLGEXPORT_H__EA91FD41_7826_11D3_BE5C_00105AC8D47C__INCLUDED_)
