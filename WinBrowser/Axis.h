#ifndef __AXIS_H__
#define __AXIS_H__
#include <vector>
#include "math.h"
#include "LinearInterpolation.h"

using namespace std;

//
//	The <b>Axis</b> class
//	is a helper for e.g. the Plot class.
//
class Axis 
{
public:
	Axis ()
	{
		_max_tick_count = 5;
		createDefaultAxis ();
		_zoom_limit = 1;
	}

	/**	Set length of area on screen (in pixels)
	 *	to display axis in.
	 *
	 *	The low end of the data range will be
	 *	mapped on the low end of the pixel range etc.,
	 *	<I>low</I> might be greater than <I>high<I>!
	 *
	 * @see #setDataRange
	 */
	void setPixelRange (int low, int high);

	/**	Set range of data that this axis should display.
	 *
	 *	The low end of the data range will be
	 *	mapped on the low end of the pixel range etc.,
	 *	<I>low</I> might be greater than <I>high<I>!
	 */
	bool setDataRange (double low, double high);

	double getData0 () const;
	double getData1 () const;

	//* The data range must be at least this wide:
	void setZoomLimit (double limit)
	{	_zoom_limit = limit;	}

	/**	Transform given pixel coord. on axis in data value.
	 *
	 * @exception LinearTransformationException for empty data range
	 */
	double getData (int pixel) const;

	/**	Transform given data  on axis in pixel coord. */
	int getPixel (double data);

	/**	Create a default Axis from start to end of data
	 */
	virtual void createDefaultAxis ();

	int getPrecision ();

	/**	Plot this axis just <I>outside</I> the given rectangle
	 *	(which contains the plotted data).
	 *
	 *	A x-axis will plot just beneath area,
	 *	a y-axis might appear to the left of area.
	 *
	 *	This is to be implemented in derived classes like XAxis, YAxis etc.
	 */
	virtual void paint (CDC *dc, const CRect &area) = 0;

protected:
	vector<double>	_ticks; //	Vector of tick positions
	double	_zoom_limit;	// _scaling.S0..S1 must be apart by at least _zoom_limit
	LinearTransformation	_scaling; //...  data (source) into screen coords (dest.)
	int		_tag_every;/**	Set to 1 to put a tag on every tick etc. */
	int		_precision;/**	Precision to print tags with: trailing digits */
	int		_max_tick_count;
};

inline void Axis::setPixelRange (int low, int high)
{
	_scaling.setDestination (low, high);
}

inline bool Axis::setDataRange (double low, double high)
{
	if ((high - low) < _zoom_limit)
		return false;
	_scaling.setSource (low, high);
	createDefaultAxis ();
	return true;
}

inline double Axis::getData0 () const
{	return _scaling.getS0();	}

inline double Axis::getData1 () const
{	return _scaling.getS1();	}

inline double Axis::getData (int pixel) const
{
	return _scaling.inverse (pixel);
}

inline int Axis::getPixel (double data)
{
	return (int) (_scaling.transform (data) + 0.5);
}

inline int Axis::getPrecision ()
{
	return _precision;
}


#endif //__AXIS_H__