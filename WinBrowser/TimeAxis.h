#include "Axis.h"
#include "epicsTime.h"

//
//
// TimeAxis
//
// The limits of this axis, a set of double numbers,
// are interpretes as an epicsTimeStamp:
// secPastEpoch, + 1.0e-9*nsec
//
//

double epicsTime2double(const epicsTime &t);
epicsTime double2epicsTime(double d);

class TimeAxis : public Axis
{
public:
	TimeAxis ()
	{
		_max_tick_count = 3;
		createDefaultAxis ();
	}

	/*
	void setDataRange (const epicsTime &start, const epicsTime &end)
	{
		epicsTimeStamp s = start, e = end;

		Axis::setDataRange((double)s.secPastEpoch + s.nsec/1.0e9,
						(double)e.secPastEpoch + e.nsec/1.0e9);
	}
	*/

	void createDefaultAxis ();

	/**	Plot this axis just <I>outside</I> the given rectangle
	 *	(which contains the plotted data).
	 *
	 *	A x-axis will plot just beneath plot_rect,
	 *	a y-axis might appear to the left of plot_rect.
	 *
	 *	This is to be implemented in derived classes like XAxis, YAxis etc.
	 */
	void paint (CDC *dc, const CRect &area);

};
