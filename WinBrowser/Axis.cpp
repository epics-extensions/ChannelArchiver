#include "stdafx.h"
#include "Axis.h"

// This routine doesn't seem to work perfectly,
// sometimes it creates MANY ticks.
// try to limit the wreckage by MAX_NUM_TICKS
#define MAX_NUM_TICKS	100

/**	Create a default Axis from start to end of data
 */
void Axis::createDefaultAxis ()
{
	double	start, end, tick;

	_ticks.clear ();
	start = _scaling.getS0();
	end = _scaling.getS1();

	//	Determine "interesting" digits.
	double diff = end - start;
	long log_diff = (long) (log10(diff)-1);
	double delta = pow(10, log_diff);
	if (log_diff <= 0)
		_precision = -log_diff;
	else
		_precision = 1;

	//	start position, rounded down to precision
	if (start <= 0  &&  end >= 0)
	{	//	one tick should be exactly at 0
		tick = 0;
		while (tick > start)
			tick -= delta;
	}
	else
		tick = ((int)(start * delta)) / delta;

	//	find first visible tick
	while (tick < start)
		tick += delta;
	//	add ticks until reaching end:
	while (tick <= end)
	{
		_ticks.push_back (tick);
		if (_ticks.size() > MAX_NUM_TICKS)
			break;
		tick += delta;
	}

	_tag_every = 1;
	while (_ticks.size() / _tag_every > _max_tick_count)
		++_tag_every;
}

