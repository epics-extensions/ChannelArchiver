// BrowserDlg.cpp

#include "stdafx.h"
#include "config.h"
#include "WinBrowser.h"
#include "BrowserDlg.h"
#include <limits>
#include "GNUPlotExporter.h"
#include "SpreadSheetExporter.h"
#include "MsgBoxF.h"
#include "DoEvents.h"
#include "DlgExport.h"
#include "VectorHelper.h"
#include "RegularExpression.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
class CAboutDlg : public CDialog
{
public:
	CAboutDlg();
	//{{AFX_DATA(CAboutDlg)
	enum { IDD = IDD_ABOUTBOX };
	//}}AFX_DATA
	//{{AFX_VIRTUAL(CAboutDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL
protected:
	//{{AFX_MSG(CAboutDlg)
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialog(CAboutDlg::IDD)
{
	//{{AFX_DATA_INIT(CAboutDlg)
	//}}AFX_DATA_INIT
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CAboutDlg)
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialog)
	//{{AFX_MSG_MAP(CAboutDlg)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CBrowserDlg dialog

CBrowserDlg::CBrowserDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CBrowserDlg::IDD, pParent)
{
	CTime time = CTime::GetCurrentTime ();

	//{{AFX_DATA_INIT(CBrowserDlg)
	m_dir_name = _T("Select directory file ->");
	m_end_date = CTime (time.GetYear(), time.GetMonth(), time.GetDay(), 23, 59, 59);
	m_end_time = CTime (time.GetYear(), time.GetMonth(), time.GetDay(), 23, 59, 59);
	//time -= CTimeSpan(1, 0, 0, 0);
	m_start_date = CTime (time.GetYear(), time.GetMonth(), time.GetDay(), 0, 0, 0);
	m_start_time = CTime (time.GetYear(), time.GetMonth(), time.GetDay(), 0, 0, 0);
	m_use_regex = FALSE;
	//}}AFX_DATA_INIT
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CBrowserDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CBrowserDlg)
	DDX_Control(pDX, IDC_CHANNEL_STATUS, m_channel_status);
	DDX_Control(pDX, IDC_PLOT, m_plot);
	DDX_Control(pDX, IDC_PATTERN, m_pattern);
	DDX_Control(pDX, IDC_CHANNEL_LIST, m_channel_list);
	DDX_Text(pDX, IDC_DIR_NAME, m_dir_name);
	DDX_DateTimeCtrl(pDX, IDC_START_DATE, m_start_date);
	DDX_DateTimeCtrl(pDX, IDC_START_TIME, m_start_time);
	DDX_DateTimeCtrl(pDX, IDC_END_DATE, m_end_date);
	DDX_DateTimeCtrl(pDX, IDC_END_TIME, m_end_time);
	DDX_Check(pDX, IDC_USE_REGEX, m_use_regex);
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CBrowserDlg, CDialog)
	//{{AFX_MSG_MAP(CBrowserDlg)
	ON_WM_SYSCOMMAND()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_SELECT_DIR, OnSelectDir)
	ON_BN_CLICKED(IDC_OK, OnOk)
	ON_BN_CLICKED(IDC_BACK, OnBack)
	ON_BN_CLICKED(IDC_FORW, OnForw)
	ON_BN_CLICKED(IDC_AUTOZOOM, OnAutozoom)
	ON_BN_CLICKED(IDC_CLEAR, OnClear)
	ON_LBN_DBLCLK(IDC_CHANNEL_LIST, OnShow)
	ON_BN_CLICKED(IDC_EXPORT, OnExport)
	ON_BN_CLICKED(IDC_PRINT, OnPrint)
	ON_BN_CLICKED(IDC_ZOOM_OUT, OnZoomOut)
	ON_BN_CLICKED(IDC_ZOOM_IN, OnZoomIn)
	ON_MESSAGE(WM_UPDATE_TIMES, OnUpdateTimes)
	ON_BN_CLICKED(IDC_ZOOM_IN_Y, OnZoomInY)
	ON_BN_CLICKED(IDC_ZOOM_OUT_Y, OnZoomOutY)
	ON_BN_CLICKED(IDC_UP, OnUp)
	ON_BN_CLICKED(IDC_DOWN, OnDown)
	ON_BN_CLICKED(IDC_SHOW, OnShow)
	ON_BN_CLICKED(IDC_ADJUST_LIMITS, OnAdjustLimits)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

static ActionEditCallback (PatternCallback)
{
	CBrowserDlg *me = (CBrowserDlg *) arg;
	me->ListChannelNames ((LPCSTR)txt);
}

BOOL CBrowserDlg::OnInitDialog()
{
	CDialog::OnInitDialog();
	// Add "About..." menu item to system menu.
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != NULL)
	{
		CString strAboutMenu;
		strAboutMenu.LoadString(IDS_ABOUTBOX);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon
	
	// Create the ToolTip control.
	m_tooltip.Create(this);
	m_tooltip.Activate(TRUE);
	m_tooltip.AddTool(GetDlgItem(IDC_PATTERN), "Enter Glob-Pattern or Regular Expression and Press RETURN");
	m_tooltip.AddTool(GetDlgItem(IDC_CHANNEL_LIST), "Select Channels to Plot");

	m_tooltip.AddTool(GetDlgItem(IDC_SHOW), "Add Selected Channels To Plot");
	m_tooltip.AddTool(GetDlgItem(IDC_EXPORT), "Export Plot to File");
	m_tooltip.AddTool(GetDlgItem(IDC_PLOT), "\"Rubberband\" to zoom");
	m_tooltip.AddTool(GetDlgItem(IDC_USE_REGEX), "Regular Expression or glob. pattern?");

	m_tooltip.AddTool(GetDlgItem(IDC_SELECT_DIR), "Click to Pick Directory File");

	m_tooltip.AddTool(GetDlgItem(IDC_START_DATE), "Start Date");
	m_tooltip.AddTool(GetDlgItem(IDC_START_TIME), "Start Time");
	m_tooltip.AddTool(GetDlgItem(IDC_END_DATE), "End Date");
	m_tooltip.AddTool(GetDlgItem(IDC_END_TIME), "End Time");

	m_tooltip.AddTool(GetDlgItem(IDC_BACK), "Pan Left");
	m_tooltip.AddTool(GetDlgItem(IDC_FORW), "Pan Right");
	m_tooltip.AddTool(GetDlgItem(IDC_UP), "Pan Up");
	m_tooltip.AddTool(GetDlgItem(IDC_DOWN), "Pan Down");

	m_tooltip.AddTool(GetDlgItem(IDC_ZOOM_OUT), "Zoom Out (Time axis)");
	m_tooltip.AddTool(GetDlgItem(IDC_ZOOM_IN), "Zoom In (Time axis)");
	m_tooltip.AddTool(GetDlgItem(IDC_ZOOM_OUT_Y), "Zoom Out (Y axis)");
	m_tooltip.AddTool(GetDlgItem(IDC_ZOOM_IN_Y), "Zoom In (Y axis)");
	m_tooltip.AddTool(GetDlgItem(IDC_AUTOZOOM), "Zoom All");
	m_tooltip.AddTool(GetDlgItem(IDC_ADJUST_LIMITS), "Adjust Plot's Limits");

	m_tooltip.AddTool(GetDlgItem(IDC_CLEAR), "Clear Plot");

	m_tooltip.AddTool(GetDlgItem(IDC_PRINT), "Hardcopy of Plot");

	LPTSTR	commandline = GetCommandLine();
	if (commandline)
	{
		char *archive = strstr (commandline, "-a");
		if (archive)
		{
			archive += 2;
			while (*archive == ' ')
				++archive;
			m_dir_name = archive;
			UpdateData (FALSE);

			m_pattern.SetCallback (PatternCallback, this);
			ListChannelNames ((LPCSTR)m_pattern.GetText());
		}
	}

	return TRUE;  // return TRUE  unless you set the focus to a control
}

BOOL CBrowserDlg::PreTranslateMessage(MSG* pMsg)
{	// Let the ToolTip process this message.
	m_tooltip.RelayEvent(pMsg);
	return CDialog::PreTranslateMessage(pMsg);	// CG: This was added by the ToolTips component.
}

void CBrowserDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
		CDialog::OnSysCommand(nID, lParam);
}

HCURSOR CBrowserDlg::OnQueryDragIcon()
{	return (HCURSOR) m_hIcon;	}

void CBrowserDlg::EnableItems (BOOL bEnable)
{
	CWnd *wnd;
	wnd = GetDlgItem(IDC_START_DATE);	if (wnd) wnd->EnableWindow (bEnable);
	wnd = GetDlgItem(IDC_SELECT_DIR);	if (wnd) wnd->EnableWindow (bEnable);
	wnd = GetDlgItem(IDC_CHANNEL_LIST);	if (wnd) wnd->EnableWindow (bEnable);
	wnd = GetDlgItem(IDC_PATTERN);		if (wnd) wnd->EnableWindow (bEnable);
	wnd = GetDlgItem(IDC_START_TIME);	if (wnd) wnd->EnableWindow (bEnable);
	wnd = GetDlgItem(IDC_END_DATE);		if (wnd) wnd->EnableWindow (bEnable);
	wnd = GetDlgItem(IDC_END_TIME);		if (wnd) wnd->EnableWindow (bEnable);
	wnd = GetDlgItem(IDC_PLOT);			if (wnd) wnd->EnableWindow (bEnable);
	wnd = GetDlgItem(IDC_BACK);			if (wnd) wnd->EnableWindow (bEnable);
	wnd = GetDlgItem(IDC_FORW);			if (wnd) wnd->EnableWindow (bEnable);
	wnd = GetDlgItem(IDC_CLEAR);		if (wnd) wnd->EnableWindow (bEnable);
	wnd = GetDlgItem(IDC_AUTOZOOM);		if (wnd) wnd->EnableWindow (bEnable);
	wnd = GetDlgItem(IDC_SHOW);			if (wnd) wnd->EnableWindow (bEnable);
	wnd = GetDlgItem(IDC_EXPORT);		if (wnd) wnd->EnableWindow (bEnable);
	wnd = GetDlgItem(IDC_PRINT);		if (wnd) wnd->EnableWindow (bEnable);
	wnd = GetDlgItem(IDC_ZOOM_OUT);		if (wnd) wnd->EnableWindow (bEnable);
	wnd = GetDlgItem(IDC_ZOOM_IN);		if (wnd) wnd->EnableWindow (bEnable);
	wnd = GetDlgItem(IDC_ZOOM_OUT_Y);	if (wnd) wnd->EnableWindow (bEnable);
	wnd = GetDlgItem(IDC_ZOOM_IN_Y);	if (wnd) wnd->EnableWindow (bEnable);
	wnd = GetDlgItem(IDC_UP);			if (wnd) wnd->EnableWindow (bEnable);
	wnd = GetDlgItem(IDC_DOWN);			if (wnd) wnd->EnableWindow (bEnable);
}

inline void osiTime2CTime (const osiTime &osi, CTime &date, CTime &time)
{
	int nYear, nMonth, nDay, nHour, nMin, nSec;
	unsigned long nano;
	osiTime2vals (osi, nYear, nMonth, nDay, nHour, nMin, nSec, nano);
	if (nYear == 0  ||  nMonth == 0  ||  nDay == 0)
		return;
	date = CTime (nYear, nMonth, nDay, nHour, nMin, nSec);
	time = CTime (nYear, nMonth, nDay, nHour, nMin, nSec);
}

void CBrowserDlg::setStart (const osiTime &start)
{	osiTime2CTime (start, m_start_date, m_start_time);	}

void CBrowserDlg::setEnd (const osiTime &end)
{	osiTime2CTime (end, m_end_date, m_end_time);	}

bool CBrowserDlg::ListChannelNames(const stdString &pattern)
{
	try
	{
		UpdateData ();
		DisableItems ();
		CWaitCursor wait;
		CString	msg;
		size_t count=0;

		m_channel_status.SetWindowText ("Reading channels, please wait");

		osiTime start, end, nullTime;
		m_channel_list.ResetContent ();

		Archive archive (new ARCHIVE_TYPE((LPCTSTR) m_dir_name));
		ChannelIterator channel (archive);
		
		if (pattern.empty())
			archive.findFirstChannel (channel);
		else
		{
			if (m_use_regex)
				archive.findChannelByPattern (pattern, channel);
			else
			{
				stdString regex = RegularExpression::fromGlobPattern (pattern);
				archive.findChannelByPattern (regex, channel);
			}
		}

		while (channel && DoEvents())
		{
			m_channel_list.AddString (channel->getName ());
			wait.Restore ();
			osiTime time = channel->getFirstTime ();
			if (time != nullTime && (time < start  ||  start == nullTime))
				start = time;
			time = channel->getLastTime ();
			if (time != nullTime && (time > end  || end == nullTime))
				end = time;
			++channel;
			++count;
			if ((count % 100) == 0)
			{
				msg.Format ("Read %d channels, be patient", count);
				m_channel_status.SetWindowText (msg);
			}
		}

		if (end != nullTime)
		{
			start = end;
			start -= 24*60*60;
			setStart (start);
			setEnd (end);
		}
		UpdateData (FALSE);
	}
	catch (ArchiveException &e)
	{
		MsgBoxF ("Archive Error:\n%s", e.what());
		OnClear();
		m_channel_list.ResetContent ();
		UpdateData (FALSE);
	}
	m_channel_status.SetWindowText ("");
	EnableItems ();

	return true;
}

void CBrowserDlg::OnSelectDir() 
{
	CFileDialog	dlg(TRUE, // open
		NULL, // def. ext
		NULL, // def. name
		OFN_EXPLORER | OFN_FILEMUSTEXIST
		);

	if (dlg.DoModal () == IDOK)
	{
		UpdateData (TRUE);
		m_dir_name = dlg.m_ofn.lpstrFile;
		UpdateData (FALSE);

		m_pattern.SetCallback (PatternCallback, this);

		ListChannelNames ((LPCSTR)m_pattern.GetText());
	}
}

// Used to catch <Return> which would by default exit the main dialog
void CBrowserDlg::OnOk() 
{}

LRESULT CBrowserDlg::OnUpdateTimes (WPARAM wParam, LPARAM lParam)
{
	setStart (osiTime (m_plot._plot->getX0()));
	setEnd (osiTime (m_plot._plot->getX1()));
	UpdateData (FALSE);
	return 0;
}

// For channel - which has to be valid! - put values from [start..end[
// into curve while updating limits
void CBrowserDlg::ReadCurve (Archive &archive, ChannelIterator &channel, const osiTime &start, const osiTime &end,
	Curve &curve, Limits &tlim, Limits &ylim)
{
	double ot=numeric_limits<double>::min(), t, y;

	curve.data.clear ();

	CString msg;
	msg.Format ("%s: reading values", channel->getName());
	m_channel_status.SetWindowText (msg);

	ValueIterator value (archive);
	channel->getValueAfterTime (start, value);
	while (value && DoEvents() && (end==nullTime || value->getTime() < end))
	{
		t = (double) value->getTime ();
		// don't go back in time (fix for error in old archives)
		if (t < ot)
		{
			++value;
			continue;
		}
		ot = t;
		tlim.keepMinMax (t);

		if (value->isInfo ())
			y = END_CURVE_Y;
		else
		{
			y = value->getDouble ();
			ylim.keepMinMax (y);
		}

		curve.data.push_back (DataPoint (t, y));
		++value;
		if ((curve.data.size() % 100) == 0)
		{
			msg.Format ("%s: %d values", channel->getName(), curve.data.size());
			m_channel_status.SetWindowText (msg);
		}
	}
	m_channel_status.SetWindowText ("");
}


// Update curves:
// Does plot cover a greater time range than what we hold in curves?
// -> read more samples into curves
void CBrowserDlg::UpdateCurves(bool redraw) 
{
	Plot *plot = m_plot._plot;
	if (! plot)
		return;

	// Try to read more data:
	vector<Curve> &curves = plot->getCurves ();
	Curve new_curve;
	Limits tlim, ylim;
	try
	{
		Archive archive (new ARCHIVE_TYPE ((LPCTSTR) m_dir_name));
		ChannelIterator channel (archive);
		osiTime start, end;
		for (size_t i=0; i<curves.size(); ++i)
		{
			if (! archive.findChannelByName ((LPCTSTR) curves[i].label, channel))
				continue;

			// Try to prepend new data
			start = plot->getX0();
			if (curves[i].data.size() > 0)
				end = curves[i].data.front()._x;
			else
				end = plot->getX1();
			if (start < end)
			{
				{
					stdString start_t, end_t;
					osiTime2string(start, start_t);
					osiTime2string(end, end_t);
					TRACE ("%s: Prepend %s ..%s ... ", curves[i].label,
						start_t.c_str(), end_t.c_str());
				}
				ReadCurve (archive, channel, start, end, new_curve, tlim, ylim);
				TRACE ("-> %d values\n", new_curve.data.size());
				if (new_curve.data.size() > 0)
				{
					redraw = true;
					vec_add_head (curves[i].data, new_curve.data);
				}
			}

			// Try to append new data
			if (curves[i].data.size() > 0)
				start = curves[i].data.back()._x + 0.000001; // don't read last value twice
			else
				start = plot->getX1();
			end = plot->getX1();
			if (start >= end)
				continue;

			{
				stdString start_t, end_t;
				osiTime2string(start, start_t);
				osiTime2string(end, end_t);
				TRACE ("%s: Append %s .. %s ... ", curves[i].label,
					start_t.c_str(), end_t.c_str());
			}

			ReadCurve (archive, channel, start, end, new_curve, tlim, ylim);
			TRACE ("-> %d values\n", new_curve.data.size());
			if (new_curve.data.size() > 0)
			{
				redraw = true;
				vec_add_tail (curves[i].data, new_curve.data);
			}
		}
	}
	catch (ArchiveException &e)
	{
		MsgBoxF ("Archive Error:\n%s", e.what());
	}
	if (redraw)
	{
		m_plot.InvalidateRect (NULL);
		PostMessage(WM_UPDATE_TIMES);
	}
}

void CBrowserDlg::OnBackForw (double dir) 
{
	CWaitCursor	wait;
	Plot *plot = m_plot._plot;
	if (! plot)
		return;
	double x0 = plot->getX0 ();
	double x1 = plot->getX1 ();
	double d = dir*(x1 - x0) / 2;
	x0 += d;
	x1 += d;
	plot->setXRange (x0, x1);
	UpdateCurves (true);
}

void CBrowserDlg::OnBack() 
{	OnBackForw (-1.0);	}

void CBrowserDlg::OnForw() 
{	OnBackForw (+1.0);	}

void CBrowserDlg::goUpDown (double dir) 
{
	CWaitCursor	wait;
	Plot *plot = m_plot._plot;
	if (! plot)
		return;
	double y0 = plot->getY0 ();
	double y1 = plot->getY1 ();
	double d = dir * (y1 - y0) / 2;
	y0 += d;
	y1 += d;
	plot->setYRange (y0, y1);
	m_plot.InvalidateRect (NULL);
}

void CBrowserDlg::OnUp() 
{	goUpDown (1);}

void CBrowserDlg::OnDown() 
{	goUpDown (-1);}

void CBrowserDlg::OnAutozoom() 
{
	CWaitCursor	wait;
	if (!m_plot._plot)
		return;
	m_plot._plot->autozoom ();

	m_plot.InvalidateRect (NULL);
	PostMessage(WM_UPDATE_TIMES);
}

void CBrowserDlg::zoomInOut (double dir) 
{
	CWaitCursor	wait;
	Plot *plot = m_plot._plot;
	if (! plot)
		return;
	double x0 = plot->getX0 ();
	double x1 = plot->getX1 ();
	double d = dir*(x1 - x0) / 4;
	x0 += d;
	x1 -= d;
	plot->setXRange (x0, x1);

	if (dir < 0)
		UpdateCurves (true);
	else
	{
		m_plot.InvalidateRect (NULL);
		PostMessage(WM_UPDATE_TIMES);
	}
}

void CBrowserDlg::OnZoomIn() 
{	zoomInOut (1.0);	}

void CBrowserDlg::OnZoomOut() 
{	zoomInOut (-1.0);	}

void CBrowserDlg::zoomInOutY (double dir) 
{
	Plot *plot = m_plot._plot;
	double y0 = plot->getY0 ();
	double y1 = plot->getY1 ();
	double d = dir*(y1 - y0) / 4;
	y0 += d;
	y1 -= d;
	plot->setYRange (y0, y1);
	m_plot.InvalidateRect (NULL);
	PostMessage(WM_UPDATE_TIMES);
}

void CBrowserDlg::OnZoomInY() 
{	zoomInOutY (1.0);	}

void CBrowserDlg::OnZoomOutY() 
{	zoomInOutY (-1.0);	}

void CBrowserDlg::OnAdjustLimits() 
{
	Plot *plot = m_plot._plot;
	if (! plot)
		return;

	osiTime start, end;
	UpdateData ();
	vals2osiTime (m_start_date.GetYear(), m_start_date.GetMonth(), m_start_date.GetDay(),
		   m_start_time.GetHour(), m_start_time.GetMinute(), m_start_time.GetSecond(), 0, start);
	vals2osiTime (m_end_date.GetYear(), m_end_date.GetMonth(), m_end_date.GetDay(),
		   m_end_time.GetHour(), m_end_time.GetMinute(), m_end_time.GetSecond(), 0, end);
	plot->setXRange (double(start), double(end));
	UpdateCurves (true);
}

void CBrowserDlg::OnClear() 
{
	m_plot._plot->clear ();
	m_plot.InvalidateRect (NULL);
}

void CBrowserDlg::OnShow() 
{
	int num = m_channel_list.GetSelCount ();
	if (num == LB_ERR)
		return;
	if (num > Plot::CMAX)
	{
		num = Plot::CMAX;
		MsgBoxF ("Cannot handle more than %d curves", Plot::CMAX);
	}

	UpdateData ();
	DisableItems ();
	CString msg, channelname;
	CWaitCursor wait;
	Plot *plot = m_plot._plot;
	Curve	curve;
	bool have_curve = false;
	int *indices = new int[num];
	int got = m_channel_list.GetSelItems (num, indices); VERIFY(num == got);
	Limits tlim, ylim;
	osiTime first_time, last_time;

	try
	{
		Archive archive (new ARCHIVE_TYPE ((LPCTSTR) m_dir_name));
		ChannelIterator channel (archive);
		for (int i=0; i<got; ++i)
		{
			m_channel_list.GetText (indices[i], channelname);	
			if (! archive.findChannelByName ((LPCTSTR) channelname, channel))
				continue;
			osiTime start, end;
			vals2osiTime (m_start_date.GetYear(), m_start_date.GetMonth(), m_start_date.GetDay(),
				   m_start_time.GetHour(), m_start_time.GetMinute(), m_start_time.GetSecond(), 0, start);
			vals2osiTime (m_end_date.GetYear(), m_end_date.GetMonth(), m_end_date.GetDay(),
				   m_end_time.GetHour(), m_end_time.GetMinute(), m_end_time.GetSecond(), 0, end);
			ReadCurve (archive, channel, start, end, curve, tlim, ylim);

			if (first_time==nullTime  ||  first_time > channel->getFirstTime())
				first_time = channel->getFirstTime();
			if (last_time < channel->getLastTime())
				last_time = channel->getLastTime();

			if (curve.data.empty ())
				continue;
			curve.label = channel->getName();
			if (plot->addCurve (curve))
				have_curve = true;
		}
		if (! have_curve)
		{
			stdString ft, lt;
			osiTime2string (first_time, ft);
			osiTime2string (last_time, lt);
			MsgBoxF ("No values found in given time range.\nFor these channels, the archive seems to have values\nfrom\n%s to\n%s.",
				ft.c_str(), lt.c_str());
		}
	}
	catch (ArchiveException &e)
	{
		MsgBoxF ("Archive Error:\n%s", e.what());
	}
	if (have_curve)
	{
		tlim.expand (0.1);
		ylim.expand (0.1);
		plot->setXRange (tlim);
		plot->setYRange (ylim);
		m_plot.InvalidateRect (NULL, TRUE);
	}
	delete indices;
	EnableItems ();
	m_channel_status.SetWindowText ("");
}

// Get list of channels
// 1) from Plot
// 2) from listbox
size_t CBrowserDlg::getChannelList (vector<stdString> &channels)
{
	int i;
	channels.clear ();

	const vector<Curve> &curves = m_plot._plot->getCurves();
	if (curves.size() > 0)
	{
		for (i=0; i<curves.size(); ++i)
			channels.push_back ((LPCSTR)curves[i].label);
		return channels.size();
	}

	CString channelname;
	int num = m_channel_list.GetSelCount ();
	if (num == LB_ERR)
		return 0;

	int *indices = new int[num];
	int got = m_channel_list.GetSelItems (num, indices);
	for (i=0; i<got; ++i)
	{
		m_channel_list.GetText (indices[i], channelname);	
		channels.push_back ((LPCSTR)channelname);
	}
	delete indices;

	return channels.size();
}

void CBrowserDlg::OnExport() 
{
	try
	{
		vector<stdString>	channels;
		if (getChannelList (channels) <= 0)
		{
			MsgBoxF ("There are no channels selected\nnor plotted to be exported!");
			return;
		}
		CDlgExport	dlg;
		if (dlg.DoModal () != IDOK)
			return;

		stdString filename = dlg.m_filename;
		Archive archive (new ARCHIVE_TYPE ((LPCTSTR) m_dir_name));
		Exporter *export;
		if (dlg.m_format == CDlgExport::SpreadSheet)
			export = new SpreadSheetExporter (archive, filename);
		else
			export = new GNUPlotExporter (archive, filename);
		export->setMaxChannelCount (1000);

		switch (dlg.m_interpol)
		{
		case CDlgExport::Raw:
		case CDlgExport::Linear:
			if (dlg.m_seconds > 0.0)
				export->setLinearInterpolation (dlg.m_seconds);
			break;
		case CDlgExport::Fill:
			export->useFilledValues ();
			break;
		}

		osiTime	time;
		vals2osiTime (m_start_date.GetYear(), m_start_date.GetMonth(), m_start_date.GetDay(),
				   m_start_time.GetHour(), m_start_time.GetMinute(), m_start_time.GetSecond(), 0, time);
		export->setStart (time);
		vals2osiTime (m_end_date.GetYear(), m_end_date.GetMonth(), m_end_date.GetDay(),
				   m_end_time.GetHour(), m_end_time.GetMinute(), m_end_time.GetSecond(), 0, time);
		export->setEnd (time);

		export->exportChannelList (channels);
		delete export;
		MsgBoxF ("Created %s", filename.c_str());
	}
	catch (ArchiveException &e)
	{
		MsgBoxF ("Archive Error:\n%s", e.what());
		return;
	}
}

void CBrowserDlg::OnPrint() 
{
	CPrintDialog dlg(FALSE);
	if (dlg.DoModal () != IDOK)
		return;

	// is a default printer set up?
	HDC hdcPrinter = dlg.GetPrinterDC();
	if (hdcPrinter == NULL)
	{
		AfxMessageBox("Buy a printer!");
		return;
	}

	LPDEVMODE devmode = dlg.GetDevMode ();

	if (devmode->dmPrintQuality  <= 0)
		devmode->dmPrintQuality = 300; // asume 300 DPI

	CSize page = CSize (6*devmode->dmPrintQuality, 6*devmode->dmPrintQuality);

	// create a CDC and attach it to the default printer
	CDC dcPrinter;
	dcPrinter.Attach(hdcPrinter);

	// call StartDoc() to begin printing
	DOCINFO docinfo;
	memset(&docinfo, 0, sizeof(docinfo));
	docinfo.cbSize = sizeof(docinfo);
	docinfo.lpszDocName = "EPICS Channel Archive";

	// if it fails, complain and exit gracefully
	if (dcPrinter.StartDoc(&docinfo) < 0)
	{
		AfxMessageBox ("Printer wouldn't initalize");
		return;
	}

	// start a page
	if (dcPrinter.StartPage() < 0)
	{
		MessageBox("Could not start page");
		dcPrinter.AbortDoc();
	}
	else
	{
		// actually do some printing
		dcPrinter.SetMapMode (MM_ISOTROPIC);
		dcPrinter.SetWindowExt (m_plot._plot->getTotalSize ());
		dcPrinter.SetViewportExt (page);

		m_plot._plot->paint (&dcPrinter, true);

		dcPrinter.EndPage();
		dcPrinter.EndDoc();
	}
}

