// BrowserDlg.h : header file
//

#if !defined(AFX_BROWSERDLG_H__7DA2BA86_7079_11D3_BE4C_00105AC8D47C__INCLUDED_)
#define AFX_BROWSERDLG_H__7DA2BA86_7079_11D3_BE4C_00105AC8D47C__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "config.h"
#include "ActionEdit.h"
#include "PlotCanvas.h"
#include "Limits.h"


/////////////////////////////////////////////////////////////////////////////
// CBrowserDlg dialog

class CBrowserDlg : public CDialog
{
public:
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	CBrowserDlg(CWnd* pParent = NULL);	// standard constructor

	bool ListChannelNames (const stdString &pattern);

	// Dialog Data
	//{{AFX_DATA(CBrowserDlg)
	enum { IDD = IDD_WINBROWSER_DIALOG };
	CStatic	m_channel_status;
	PlotCanvas	m_plot;
	CActionEdit	m_pattern;
	CListBox	m_channel_list;
	CString	m_dir_name;
	CTime	m_start_date;
	CTime	m_start_time;
	CTime	m_end_date;
	CTime	m_end_time;
	BOOL	m_use_regex;
	//}}AFX_DATA

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CBrowserDlg)
	public:
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support
	//}}AFX_VIRTUAL

protected:
	CToolTipCtrl m_tooltip;
	HICON m_hIcon;

	void setStart (const osiTime &start);
	void setEnd (const osiTime &end);

	void goUpDown (double dir);
	void OnBackForw (double dir);
	void zoomInOut (double dir);
	void zoomInOutY (double dir);

	size_t getChannelList (vector<stdString> &channels);
	void ReadCurve (Archive &archive, ChannelIterator &channel, const osiTime &start, const osiTime &end, Curve &curve,
		Limits &tlim, Limits &ylim);
	void UpdateCurves (bool redraw);

	void EnableItems (BOOL bEnable=TRUE);
	void DisableItems () { EnableItems (FALSE); }

	// Generated message map functions
	//{{AFX_MSG(CBrowserDlg)
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg HCURSOR OnQueryDragIcon();
	afx_msg void OnSelectDir();
	afx_msg void OnOk();
	afx_msg void OnBack();
	afx_msg void OnForw();
	afx_msg void OnAutozoom();
	afx_msg void OnClear();
	afx_msg void OnShow();
	afx_msg void OnExport();
	afx_msg void OnPrint();
	afx_msg void OnZoomOut();
	afx_msg void OnZoomIn();
	afx_msg LRESULT OnUpdateTimes (WPARAM wParam, LPARAM lParam);
	afx_msg void OnZoomInY();
	afx_msg void OnZoomOutY();
	afx_msg void OnUp();
	afx_msg void OnDown();
	afx_msg void OnAdjustLimits();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_BROWSERDLG_H__7DA2BA86_7079_11D3_BE4C_00105AC8D47C__INCLUDED_)
