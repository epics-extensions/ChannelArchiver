// PlotCanvas.cpp : implementation file

#include "stdafx.h"
#include "epicsTimeHelper.h"
#include "PlotCanvas.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

PlotCanvas::PlotCanvas()
{
	_plot = 0;
	_state = idle;
}

PlotCanvas::~PlotCanvas()
{
	delete _plot;
}

BEGIN_MESSAGE_MAP(PlotCanvas, CStatic)
	//{{AFX_MSG_MAP(PlotCanvas)
	ON_WM_PAINT()
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONUP()
	ON_WM_MOUSEMOVE()
	ON_WM_RBUTTONDOWN()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

void PlotCanvas::OnPaint() 
{
	CWaitCursor wait;
	CPaintDC dc(this);

	CRect border;
	GetClientRect (&border);

	if (! _plot)
	{
		CRect plot_area = border;
		plot_area.DeflateRect (80, 10, 10, 50);
		_plot = new Plot (border, plot_area);
	}

	dc.DrawEdge (&border, EDGE_SUNKEN, BF_RECT);
	border.DeflateRect (2, 2, 2, 2);
	dc.FillSolidRect (border, RGB (200, 200, 200));

	_plot->paint (&dc);
}

void PlotCanvas::drawRubberband (CDC *dc)
{
	int rop = dc->SetROP2 (R2_NOT);

	dc->MoveTo (_rubber_start);
	dc->LineTo (_rubber_start.x, _rubber_end.y);
	dc->LineTo (_rubber_end);
	dc->LineTo (_rubber_end.x, _rubber_start.y);
	dc->LineTo (_rubber_start);

	dc->SetROP2 (rop);
}

void PlotCanvas::OnLButtonDown(UINT nFlags, CPoint point) 
{
	if (! _plot)
		return;

	if (_plot->isLegendHit (point))
	{
		_state = moving;
		_selection = point;
		_rubber_start = _plot->getLegend().TopLeft ();
		_rubber_end = _plot->getLegend().BottomRight ();
	}
	else
	{
		_state = rubberbanding;
		_rubber_start = point;
		_rubber_end = CPoint (point.x + 2, point.y + 2);
	}

	CClientDC dc (this);
	drawRubberband (&dc);
}

void PlotCanvas::OnLButtonUp(UINT nFlags, CPoint point) 
{
	CClientDC dc (this);
	drawRubberband (&dc);

	switch (_state)
	{
	case moving:
		if (_plot->isGraphHit (_rubber_start)  && 
			_plot->isGraphHit (_rubber_end))
			_plot->moveLegend (_rubber_start);
		break;
	case rubberbanding:
		_plot->zoom (CRect (_rubber_start, _rubber_end));
		{
			CWnd* p = GetParent();
			if (p)
				p->PostMessage(WM_UPDATE_TIMES);
		}
		break;
	}
	_state = idle;
	InvalidateRect (NULL);
}

void PlotCanvas::OnMouseMove(UINT nFlags, CPoint point) 
{
	double x, y;
	CPoint moved;
	CClientDC dc (this);
	if (_state != idle)
		drawRubberband (&dc);
	if (! _plot->isHit(point)) // (*)
	{
		_state = idle;
		return;
	}

	switch (_state)
	{
	case idle:
		// _plot->isHit(point), see (*)
		_plot->getDataPoint (point, x, y);
		TRACE ("Over y=%lg\n", y);
		break;
	case moving:
		moved = point - _selection;
		_selection = point;
		_rubber_start += moved;
		_rubber_end += moved;
		break;
	case rubberbanding:
		_rubber_end = point;
		break;
	}
	if (_state != idle)
		drawRubberband (&dc);
}

void PlotCanvas::OnRButtonDown(UINT nFlags, CPoint point) 
{
	if (! _plot)
		return;

	_plot->autozoom ();
	InvalidateRect (NULL);
}
