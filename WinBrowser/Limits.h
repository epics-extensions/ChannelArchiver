#ifndef __LIMITS_H__
#define __LIMITS_H__

#include <limits>
#ifdef min
#undef min
#endif
#ifdef max
#undef max
#endif

using std::numeric_limits;

class Limits
{
public:
	Limits ()
	{	reset ();	}

	Limits (double min, double max)
	{
		maxval=min;
		minval=max;
	}

	void reset ()
	{
		maxval= -numeric_limits<double>::max();
		minval= +numeric_limits<double>::max();
	}

	double getMin () const
	{	return minval;	}

	double getMax () const
	{	return maxval;	}

	void keepMin (double val)
	{
		if (val < minval)
			minval = val;
	}

	void keepMax (double val)
	{
		if (val > maxval)
			maxval = val;
	}

	void keepMinMax (double val)
	{
		keepMin (val);
		keepMax (val);
	}

	void expand (double f)
	{
		double d = f*(maxval - minval);
		minval -= f*d;
		maxval += f*d;
	}

	void shrink (double f)
	{	expand (-f);	}

private:
	double minval, maxval;
};

#endif //__LIMITS_H__