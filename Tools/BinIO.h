#ifndef BIN_IO_H
#define BIN_IO_H

#include <stdio.h>

/// \addtogroup Tools
/// @{

/// OS-independent binary I/O.

/// All values are written with the most significant
/// byte first, e.g. 0x12345678 is written as
/// 0x12, 0x34, ... 0x78.

/// Write 'value' to file with fixed byte order
bool writeLong(FILE *f, unsigned long value);

/// Read 'value' from file with fixed byte order
bool readLong(FILE *f, unsigned long *value);

/// Write 'value' to file with fixed byte order
bool writeShort(FILE *f, unsigned short value);

/// Read 'value' from file with fixed byte order
bool readShort(FILE *f, unsigned short *value);

inline bool writeByte(FILE *f, char byte)
{   return fwrite(&byte, 1, 1, f) == 1; }

inline bool readByte(FILE *f, char *byte)
{   return fread(byte, 1, 1, f) == 1; }

/// @}

#endif
// BIN_IO_H


