#ifndef __PLOT_H__
#define __PLOT_H__

#include "TimeAxis.h"
#include "YAxis.h"
#include "Limits.h"

class DataPoint
{
public:
	DataPoint (double x, double y)
	{
		_x = x;
		_y = y;
	}

	double _x, _y;
};

class Curve
{
public:
	CString				label;
	vector<DataPoint>	data;
};

// Magic y value to indicate end of connected curve
#define END_CURVE_Y	numeric_limits<double>::min()

//CLASS Plot
class Plot
{
public:
	enum
	{
		CMAX = 10
	};


	//* New plot that will use plot_area inside a given total.
	Plot (const CRect &total, const CRect &plot_area);
	CSize getTotalSize ();

	bool isHit (const CPoint &point) const;
	bool isGraphHit (const CPoint &point) const;

	void clear ();
	bool addCurve (const Curve &curve);
	vector<Curve> &getCurves();

	//* Get/set display limits
	double getX0 () const;
	double getX1 () const;
	double getY0 () const;
	double getY1 () const;
	void setXRange (const Limits &x);
	void setXRange (double min, double max);
	void setYRange (const Limits &y);
	void setYRange (double min, double max);
	Limits getXRange ();
	Limits getYRange ();

	void getDataPoint (const CPoint &point, double &x, double &y) const;

	void zoom (const CRect &screen_area);
	void autozoom ();

	bool isLegendHit (const CPoint &point) const;
	const CRect &getLegend () const;
	void moveLegend (const CPoint &new_pos);

	void paint (CDC *dc, bool print = false);

private:
	CRect _total;
	CRect _plot_area;
	CRect _legend;
	TimeAxis _xaxis;
	YAxis _yaxis;
	vector<Curve>	_curves;
};

inline Plot::Plot (const CRect &total, const CRect &plot_area)
{
	static const double M_PI = 3.1415926535897932384626433832795;
	_total = total;
	_plot_area = plot_area;

	_legend = _plot_area;
	_legend.left = (_plot_area.left + _plot_area.right)/2;
	_legend.bottom = (_plot_area.top + _plot_area.bottom)/2;

#ifdef PLOT_DEMO
	Curve	curve;
	for (int i=0; i<100; ++i)
		curve.push_back (DataPoint (2*M_PI*i/99, sin (2*M_PI*i/99)));
	addCurve  (curve);
	curve.clear ();
	for (i=0; i<100; ++i)
		curve.push_back (DataPoint (2*M_PI*i/99, cos (2*M_PI*i/99)));
	addCurve  (curve);
	curve.clear ();
	for (i=0; i<100; ++i)
		curve.push_back (DataPoint (2*M_PI*i/99, pow (2*M_PI*i/99, 1.5)));
	addCurve  (curve);

	_xaxis.setDataRange (0, 2*M_PI);
	_yaxis.setDataRange (-1, 1);
#endif
}

inline bool Plot::addCurve (const Curve &curve)	
{
	if (_curves.size() >= CMAX)
		return false;
	_curves.push_back (curve);
	return true;
}

inline void Plot::setXRange (const Limits &x)
{	_xaxis.setDataRange (x.getMin(), x.getMax());	}

inline void Plot::setXRange (double min, double max)
{	_xaxis.setDataRange (min, max);	}

inline Limits Plot::getXRange ()
{	return Limits (_xaxis.getData0 (), _xaxis.getData1 ());	}

inline Limits Plot::getYRange ()
{	return Limits (_yaxis.getData0 (), _yaxis.getData1 ());	}

inline double Plot::getX0 () const
{	return _xaxis.getData0 ();	}

inline double Plot::getX1 () const
{	return _xaxis.getData1 ();	}

inline void Plot::setYRange (const Limits &y)
{	_yaxis.setDataRange (y.getMin(), y.getMax());	}

inline void Plot::setYRange (double min, double max)
{	_yaxis.setDataRange (min, max);	}

inline double Plot::getY0 () const
{	return _yaxis.getData0 ();	}

inline double Plot::getY1 () const
{	return _yaxis.getData1 ();	}

inline void Plot::getDataPoint (const CPoint &point, double &x, double &y) const
{
	x = _xaxis.getData (point.x);
	y = _yaxis.getData (point.y);
}

inline bool Plot::isLegendHit (const CPoint &point) const
{	return _legend.PtInRect (point) == TRUE;	}

inline const CRect &Plot::getLegend () const
{	return _legend;	}

inline bool Plot::isHit (const CPoint &point) const
{	return _total.PtInRect (point) == TRUE;	}

inline bool Plot::isGraphHit (const CPoint &point) const
{	return _plot_area.PtInRect (point) == TRUE;	}

inline void Plot::moveLegend (const CPoint &new_pos)
{
	_legend.left = new_pos.x;
	_legend.top = new_pos.y;
}

inline void Plot::clear ()
{	_curves.clear ();	}

inline CSize Plot::getTotalSize ()
{	return _total.Size();	}

inline vector<Curve> &Plot::getCurves()
{	return _curves; }

#endif //__PLOT_H__