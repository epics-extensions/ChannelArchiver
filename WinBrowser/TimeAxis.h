#include "Axis.h"
#include "osiTime.h"

//
//
// TimeAxis
//
//
class TimeAxis : public Axis
{
public:
	TimeAxis ()
	{
		_max_tick_count = 3;
		createDefaultAxis ();
	}

	void setDataRange (const osiTime &start, const osiTime &end)
	{
		Axis::setDataRange (double (start), double (end));
	}

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
