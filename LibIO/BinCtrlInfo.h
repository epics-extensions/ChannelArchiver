#ifndef __BINCTRLINFO_H__
#define __BINCTRLINFO_H__

#include "BinTypes.h"
#include "CtrlInfoI.h"

class BinCtrlInfo : public CtrlInfoI
{
public:
	void read(FILE *file, FileOffset offset);
	void write(FILE *file, FileOffset offset) const;
};

#endif //__BINCTRLINFO_H__

