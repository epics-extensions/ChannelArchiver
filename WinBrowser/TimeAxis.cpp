#pragma warning (disable: 4786)

#include "stdafx.h"
#include "osiTimeHelper.h"
#include "TimeAxis.h"

USING_NAMESPACE_TOOLS

void TimeAxis::createDefaultAxis ()
{
	double	diff, start, end, delta, tick;

	_ticks.clear ();
	start = _scaling.getS0();
	end = _scaling.getS1();

	diff = end - start;

	delta = diff / 10.0;

	tick = start;
	//	add ticks until reaching end:
	while (tick <= end)
	{
		_ticks.push_back (tick);
		tick += delta;
	}

	_tag_every = 1;
	while (_ticks.size() / _tag_every > _max_tick_count)
		++_tag_every;
}

/**	Plot this axis just <I>outside</I> the given rectangle
 *	(which contains the plotted data).
 *
 *	A x-axis will plot just beneath plot_rect,
 *	a y-axis might appear to the left of plot_rect.
 *
 *	This is to be implemented in derived classes like XAxis, YAxis etc.
 */
void TimeAxis::paint (CDC *dc, const CRect &area)
{
	CString		tag;
	double		tick;
	int			i, screen_x, screen_y;
	int year, month, day, hour, min, sec;
	unsigned long nano;

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

			osiTime2vals (osiTime (tick), year, month, day, hour, min, sec, nano);
			tag.Format ("%02d:%02d:%02d.%02d", hour, min, sec, (int)(nano/100000000L));
			metrics = dc->GetTextExtent (tag);
			dc->TextOut (screen_x - metrics.cx/2, screen_y + tag_skip, tag);

			tag.Format ("%02d/%02d/%04d", month, day, year);
			metrics = dc->GetTextExtent (tag);
			dc->TextOut (screen_x - metrics.cx/2, screen_y + tag_skip + metrics.cy, tag);
		}
		else
		{	//	half tick
			dc->MoveTo (screen_x, screen_y);
			dc->LineTo (screen_x, screen_y + tick_len/2);
		}
	}
}
