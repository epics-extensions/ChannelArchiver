// System
#include <stdlib.h>
// Index/Tools
#include "BinIO.h"

bool writeLong(FILE *f, long value)
{
    unsigned char c[4];
    c[0] = (unsigned char) (((unsigned long)value) >> 24) & 0xFF;
    c[1] = (unsigned char) (((unsigned long)value) >> 16) & 0xFF;
    c[2] = (unsigned char) (((unsigned long)value) >>  8) & 0xFF;
    c[3] = (unsigned char) ((unsigned long)value) & 0xFF;
    return fwrite(c, 4, 1, f) == 1;
}

bool readLong(FILE *f, long *value)
{
    unsigned char c[4];
    if (fread(c, 4, 1, f) != 1)
        return false;
    *((unsigned long *)value) =
        ((unsigned long)c[0]) << 24 |
        ((unsigned long)c[1]) << 16 |
        ((unsigned long)c[2]) <<  8 |
        (unsigned long)c[3];
    return true;
}

bool writeShort(FILE *f, short value)
{
    unsigned char c[2];
    c[0] = (unsigned char) (((unsigned short)value) >>  8) & 0xFF;
    c[1] = (unsigned char) ((unsigned short)value) & 0xFF;
    return fwrite(c, 2, 1, f) == 1;
}

bool readShort(FILE *f, short *value)
{
    unsigned char c[2];
    if (fread(c, 2, 1, f) != 1)
        return false;
    *((unsigned short *)value) =
        ((unsigned short)c[0]) <<  8 |
        (unsigned short)c[1];
    return true;
};


