#pragma warning (disable: 4786)

#include "stdafx.h"
#include "osiTimeHelper.h"
#include "Plot.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

static COLORREF colors[Plot::CMAX] = 
{
	RGB(255, 0, 0),
	RGB(0, 255, 0),
	RGB(0, 0, 255),
	RGB(0, 0, 0),
	RGB(200, 200, 0),
	RGB(200, 0, 200),
	RGB(0, 200, 200),
	RGB(50, 50, 50),
	RGB(50, 0, 0),
	RGB(50, 50, 100)
};

int styles[Plot::CMAX][2] =
{
	{ PS_SOLID, 1 },
	{ PS_SOLID, 3 },
	{ PS_DOT, 0 },
	{ PS_DASH, 0 },
	{ PS_DASHDOT, 0 },
	{ PS_DASHDOTDOT, 0 },
	{ PS_DASH, 1 },
	{ PS_DASHDOT, 1 },
	{ PS_DASHDOTDOT, 1 },
	{ PS_DOT, 1 },
};

void Plot::zoom (const CRect &screen_area)
{
	if (screen_area.left < screen_area.right)
		_xaxis.setDataRange (_xaxis.getData (screen_area.left), _xaxis.getData (screen_area.right));
	else if (screen_area.left > screen_area.right)
		_xaxis.setDataRange (_xaxis.getData (screen_area.right), _xaxis.getData (screen_area.left));

	if (screen_area.top < screen_area.bottom)
		_yaxis.setDataRange (_yaxis.getData (screen_area.bottom), _yaxis.getData (screen_area.top));
	else if (screen_area.top > screen_area.bottom)
		_yaxis.setDataRange (_yaxis.getData (screen_area.top), _yaxis.getData (screen_area.bottom));

	TRACE ("Zoomed: %g ... %g, %g ... %g\n",
		_xaxis.getData0(), _xaxis.getData1(),
		_yaxis.getData0(), _yaxis.getData1());
}

void Plot::autozoom ()
{
	size_t cmax, i, vals;
	cmax = _curves.size();
	if (cmax == 0)
		return;

	if (cmax > CMAX)
		cmax = CMAX;

	Limits xlim, ylim;
	bool any = false;
	double y;
	for (size_t c=0; c<cmax; ++c)
	{
		vals = _curves[c].data.size();
		if (vals > 0)
			any = true;
		for (i=0; i<vals; ++i)
		{
			y = _curves[c].data[i]._y;
			if (y != END_CURVE_Y)
			{
				xlim.keepMinMax (_curves[c].data[i]._x);
				ylim.keepMinMax (y);
			}
		}
	}

	if (any)
	{
		xlim.expand (0.1);
		ylim.expand (0.1);
		setXRange (xlim);
		setYRange (ylim);
	}
}

void Plot::paint (CDC *dc, bool print)
{
	CFont font, *o_font;
	font.CreatePointFont(80, "Arial");
	o_font = dc->SelectObject(&font);

	CPen pen[CMAX], *o_pen;
	size_t c;
	size_t cmax = _curves.size();
	if (cmax > CMAX)
		cmax = CMAX;

	if (print)
	{
		for (c=0; c<CMAX; ++c)
			pen[c].CreatePen(styles[c][0], styles[c][1], colors[c]);
	}
	else
		for (c=0; c<CMAX; ++c)
			pen[c].CreatePen(PS_SOLID, 0, colors[c]);

	if (! print)
		dc->FillSolidRect (_plot_area.left, _plot_area.top,
			_plot_area.Width()+1, _plot_area.Height()+1, RGB(255,255,255));

	o_pen = (CPen *) dc->SelectStockObject (BLACK_PEN);
	int obk = dc->SetBkMode (TRANSPARENT);
	_xaxis.paint (dc, _plot_area);
	_yaxis.paint (dc, _plot_area);
	dc->SetBkMode (obk);

	CRgn clip;
	CRect cliprect = _plot_area;
	dc->LPtoDP (cliprect);
	clip.CreateRectRgn (cliprect.left, cliprect.top, cliprect.right+1, cliprect.bottom+1);
	dc->SelectClipRgn (&clip);

	bool start, use_dot;
	double x, y;
	int dot;
	for (c=0; c<cmax; ++c)
	{
		dot = styles[c][1]+2;
		dc->SelectObject(&pen[c]);
		start = true;
		use_dot = _curves[c].data.size() < 100;
		for (size_t i=0; i<_curves[c].data.size(); ++i)
		{
			y = _curves[c].data[i]._y;
			if (y == END_CURVE_Y)
			{
				start = true;
				continue;
			}
			x = _xaxis.getPixel (_curves[c].data[i]._x);
			y = _yaxis.getPixel (y);
			if (start)
			{
				dc->MoveTo (x, y);
				start = false;
			}
			else
				dc->LineTo (x, y);
			if (use_dot)
				dc->Ellipse (x-dot, y-dot, x+dot, y+dot);
		}
	}

	// Draw Legend
	if (cmax > 0)
	{
		// measure items
		int gap = 2;
		CSize extend, total;
		total.cx = total.cy = 0;
		for (c=0; c<cmax; ++c)
		{
			extend = dc->GetTextExtent (_curves[c].label);
			total.cy += extend.cy + gap;
			if (extend.cx > total.cx)
				total.cx = extend.cx;
		}
		_legend.right = _legend.left + total.cx + 2*gap;
		_legend.bottom = _legend.top + total.cy;
		// draw
		if (! print)
			dc->FillSolidRect (_legend.left, _legend.top, _legend.Width()+1, _legend.Height()+1, RGB(255,255,255));
		dc->SelectStockObject (BLACK_PEN);
		dc->Rectangle (_legend.left, _legend.top, _legend.right+1, _legend.bottom+1);
		obk = dc->SetBkMode (TRANSPARENT);
		extend = dc->GetTextExtent ("X", 1);
		for (c=0; c<cmax; ++c)
		{
			dc->SetTextColor (colors[c]);
			dc->TextOut (_legend.left+gap, _legend.top + gap + extend.cy*c, _curves[c].label);
		}
		dc->SetBkMode (obk);
	}

	dc->SelectObject(o_pen);
	dc->SelectObject(o_font);
}

