#ifndef BIN_IO_H
#define BIN_IO_H

#include <stdio.h>

/// \ingroup Tools

/**
 * \file bin_io.h
 * These "Binary I/O" routines provide OS-independent binary I/O.
 * All values are written with the most significant
 * byte first, e.g. 0x12345678 is written as
 * 0x12, 0x34, ... 0x78.
 */

/// Write 'value' to file with fixed byte order
bool writeLong(FILE *f, long value);

/// Read 'value' from file with fixed byte order
bool readLong(FILE *f, long *value);

/// Write 'value' to file with fixed byte order
bool writeShort(FILE *f, short value);

/// Read 'value' from file with fixed byte order
bool readShort(FILE *f, short *value);

inline bool writeByte(FILE *f, char byte)
{   return fwrite(&byte, 1, 1, f) == 1; }

inline bool readByte(FILE *f, char *byte)
{   return fread(byte, 1, 1, f) == 1; }

#endif
// BIN_IO_H


