// -*- c++ -*-
#ifndef __BITSET_H__
#define __BITSET_H__

#include<stdString.h>

/// BitSet, inspired by STL bitset.
///
/// Reasons for reinventing this type of class:
/// - The STL version is not part of e.g. the egcs compiler (as of Sept. 1999).
/// - This BitSet grows during runtime, size isn't had-coded at compiletime.
///
class BitSet
{
public:
    BitSet ();
    ~BitSet ();

    /// Grow so that size is at least minimum
    
    /// All the following set/clear/test operations
    /// only work with bits 0...minimum, so you need
    /// to grow the BitSet to the required size before
    /// accessing any bits in there!
    void grow(size_t minimum);
    
    /// Set bit to 1
    void set(size_t bit);

    /// Clear bit, i.e. set to 0.
    void clear(size_t bit);

    /// Set bit to 0 or 1
    void set(size_t bit, bool value);

    /// Check individual bit.
    bool test(size_t bit) const;

    ///
    /// Valid bit numbers are [0...size()[.
    ///
    bool operator [] (size_t bit) const
    { return test(bit); }

    /// Whole bitset empty?
    bool empty() const;

    /// Any bit set?
    bool any() const;

    /// Number of bits set
    size_t count() const;

    /// Number of bits in Bitset
    size_t capacity() const
    { return size; }

    /// Return 001000.... type of string
    stdString to_string() const;

private:
    BitSet (const BitSet &rhs); // not implemented
    BitSet & operator = (const BitSet &rhs); // not implemented

    typedef unsigned long W32;
    W32 *bits;
    size_t num;   // # of W32 that _bits points to
    size_t size;  // bits used
};

#endif //__BITSET_H__
