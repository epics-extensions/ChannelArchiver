// --------------------------------------------------------
// $Id$
//
// Please refer to NOTICE.txt,
// included as part of this distribution,
// for legal information.
//
// Kay-Uwe Kasemir, kasemir@lanl.gov
// --------------------------------------------------------

#ifndef __BUCKETINGVALUEITERATOR_H__
#define __BUCKETINGVALUEITERATOR_H__

#include "ValueIteratorI.h"

//////////////////////////////////////////////////////////////////////
//CLASS BucketingValueIteratorI
// A CLASS ValueIterator that performs bucketing data reduction.
//
// For any set of values within a certain deltaT (the bucket)
// at most 4 values are returned (first, min, max and last).
//
// When <I>next()/prev()</I> is called
// this specialization of a CLASS ValueIteratorI
// will move to the next/previous CLASS Value
// as defined by the bucketing algorithm.
//
// Data reduction is not possible between "Info" values
// line "Archive Off" or "Disabled".
// When such an info value is hit,
// the iterator will return the info value.
class BucketingValueIteratorI : public ValueIteratorI
{
public:
	//* Create a BucketingValueIterator
	// based on a basic CLASS AbstractValueIterator,
	// preferably a CLASS ExpandingValueIterator.
	//
	// <I>deltaT</I>: Bucket width.
	BucketingValueIteratorI (ValueIteratorI *base, double deltaT);

	~BucketingValueIteratorI ();

	// virtuals from ValueIteratorI
	bool isValid () const;
	const ValueI * getValue () const;
	bool next ();
	bool prev ();

	size_t determineChunk (const osiTime &until);

	//* This method will return <I>deltaT</I>,
	// not the original scan period of the underlying
	// base iterator
	double getPeriod () const;

private:
        bool iterate ( int dir );

	ValueIteratorI *_base;
	osiTime _time;
        double _first;
   	double _min;
   	double _max;
   	double _last;
        dbr_short_t _status, _sevr;
        const CtrlInfoI *_ctrlinfo;
        enum { S_INIT, S_VALUES, S_OUT } _state;
        enum { V_FIRST, V_MIN, V_MAX, V_LAST, V_NONE } _oval;
	double _deltaT;
	ValueI *_value;
        enum { D_BCK, D_FWD };
};


#endif //__BUCKETINGVALUEITERATOR_H__
