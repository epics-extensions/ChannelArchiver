#include "Axis.h"

//
//
// YAxis
//
//
class YAxis : public Axis
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

		setPixelRange (area.bottom, area.top);

		CSize metrics = dc->GetTextExtent ("X", 1);
		int			tag_skip = 0;
		int			tick_len = metrics.cx;

		//	draw line left from plotting area (just 1 pixel outside !)
		dc->MoveTo (area.left-1, area.top);
		dc->LineTo (area.left-1, area.bottom +1);

		//	Plot y-axis
		screen_x = area.left-1;
		tag_skip = 2*tick_len;
		for (i=0; i<_ticks.size(); ++i)
		{
			tick = _ticks[i];
			screen_y = _scaling.transform (tick);
			if (i % _tag_every == 0)
			{	//	full tick and text
				dc->MoveTo (screen_x-tick_len, screen_y);
				dc->LineTo (screen_x, screen_y);
				tag.Format ("%.*f", _precision, tick);
				metrics = dc->GetTextExtent (tag);
				dc->TextOut (screen_x - metrics.cx - tag_skip,
					screen_y - metrics.cy / 2, tag);
			}
			else
				if (_ticks.size() < 100)
				{	//	half tick
					dc->MoveTo (screen_x-tick_len/2, screen_y);
					dc->LineTo (screen_x, screen_y);
				}
		}
	}
};
