#include "Axis.h"

//
//
// XAxis
//
//
class XAxis : public Axis
{
public:
	/**	Plot this axis just <I>outside</I> the given rectangle
	 *	(which contains the plotted data).
	 *
	 *	A x-axis will plot just beneath plot_rect,
	 *	a y-axis might appear to the left of plot_rect.
	 *
	 *	This is to be implemented in derived classes like XAxis, YAxis etc.
	 */
	void paint (CDC *dc, const CRect &area)
	{
		CString		tag;
		double		tick;
		int			i, screen_x, screen_y;

		setPixelRange (area.left, area.right);

		CSize metrics = dc->GetTextExtent ("X", 1);
		int			tag_skip = metrics.cy/2;
		int			tick_len = metrics.cy/2;

		//	draw line under plotting area (just 1 pixel outside !)
		dc->MoveTo (area.left, area.bottom+1);
		dc->LineTo (area.right+1, area.bottom+1);

		//	Plot x-axis
		screen_y = area.bottom+1;
		for (i=0; i<_ticks.size(); ++i)
		{
			tick = _ticks[i];
			screen_x = _scaling.transform (tick);
			if (i % _tag_every == 0)
			{	//	full tick and text
				dc->MoveTo (screen_x, screen_y);
				dc->LineTo (screen_x, screen_y + tick_len);

				tag.Format ("%.*f", _precision, tick);
				metrics = dc->GetTextExtent (tag);
				dc->TextOut (screen_x - metrics.cx/2, screen_y + tag_skip, tag);
			}
			else
			{	//	half tick
				dc->MoveTo (screen_x, screen_y);
				dc->LineTo (screen_x, screen_y + tick_len/2);
			}
		}
	}

};
