#ifndef __BINCTRLINFO_H__
#define __BINCTRLINFO_H__

#include "BinTypes.h"
#include "CtrlInfoI.h"

BEGIN_NAMESPACE_CHANARCH

class BinCtrlInfo : public CtrlInfoI
{
public:
	void read (FILE *fd, FileOffset offset);
	void write (FILE *fd, FileOffset offset) const;
};

END_NAMESPACE_CHANARCH

#endif //__BINCTRLINFO_H__

