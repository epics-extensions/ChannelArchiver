// $Id$
// DlgExport.cpp

#include "stdafx.h"
#include "WinBrowser.h"
#include "DlgExport.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

CDlgExport::CDlgExport(CWnd* pParent /*=NULL*/)
	: CDialog(CDlgExport::IDD, pParent)
{
	const char *tmp = getenv ("TEMP");
	if (tmp)
		m_filename.Format ("%s\\data.xls", tmp);
	else
		m_filename = "\\data.xls";
	//{{AFX_DATA_INIT(CDlgExport)
	m_format = SpreadSheet;
	m_interpol = Raw;
	m_seconds = 10.0;
	//}}AFX_DATA_INIT
}

// How Radiobuttons work:
// Use the first one as an anchor for DDX_Radio.
// This first one must have WS_GROUP.
// The next radios have no WS_GROUP.
// The first non-radiobutton that follows must again have WS_GROUP!
void CDlgExport::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CDlgExport)
	DDX_Text(pDX, IDC_EXPORT_FILENAME, m_filename);
	DDX_Radio(pDX, IDC_GNUPLOT, m_format);
	DDX_Radio(pDX, IDC_RAW, m_interpol);
	DDX_Text(pDX, IDC_SECONDS, m_seconds);
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CDlgExport, CDialog)
	//{{AFX_MSG_MAP(CDlgExport)
	ON_BN_CLICKED(IDC_PICK_EXPORT_NAME, OnPickExportName)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

void CDlgExport::OnPickExportName() 
{
	CFileDialog	dlg(FALSE, // open
		NULL, // def. ext
		m_filename, // def. name
		OFN_EXPLORER | OFN_OVERWRITEPROMPT, // flags
		"Spreadsheet (*.xls)|*.xls|All Files (*.*)|*.*||"
		);

	int result = dlg.DoModal ();
	if (result != IDOK)
		return;

	m_filename = dlg.m_ofn.lpstrFile;
	UpdateData (FALSE);
}
