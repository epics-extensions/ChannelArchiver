#if !defined(AFX_PLOTCANVAS_H__8BDBEA0D_713F_11D3_B94D_444553540000__INCLUDED_)
#define AFX_PLOTCANVAS_H__8BDBEA0D_713F_11D3_B94D_444553540000__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "Plot.h"

// PlotCanvas window

class PlotCanvas : public CStatic
{
// Construction
public:
	PlotCanvas();

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(PlotCanvas)
	//}}AFX_VIRTUAL

	virtual ~PlotCanvas();

	Plot *_plot;

	// Generated message map functions
protected:
	//{{AFX_MSG(PlotCanvas)
	afx_msg void OnPaint();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
	//}}AFX_MSG

	enum
	{
		idle, moving, rubberbanding
	}	_state;
	CPoint _selection;
	CPoint _rubber_start;
	CPoint _rubber_end;

	void drawRubberband (CDC *dc);

	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_PLOTCANVAS_H__8BDBEA0D_713F_11D3_B94D_444553540000__INCLUDED_)
