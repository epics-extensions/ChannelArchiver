// -*- c++ -*-
#ifndef __STORAGE_TYPES_H
#define __STORAGE_TYPES_H

/// \defgroup Storage 
/// @{
/// Defines routines to access the binary data files.
///
///

/// \typedef FileOffset is used as a system independent
/// type for, well, offsets into files.
///
typedef unsigned long FileOffset;

// used internally for offsets inside files:
const FileOffset INVALID_OFFSET = 0xffffffff;

/// @}

#endif