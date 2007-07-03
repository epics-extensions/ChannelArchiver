// local
#include "Conversions.h"
#include "MsgLogger.h"

/** Convert the given raw time stamp from disk format
 *  to local format, patching nanoseconds which overflow
 *  into seconds.
 *  <p>
 *  In principle, that's an error, but since some systems
 *  provide such garbage nsecs, the data would be lost.
 *  This way, we still get some data back out.
 *  <p>
 *  @parm context some context string to display with the error message
 *  @parm stamp TimeStamp to convert
 */
bool safeEpicsTimeStampFromDisk(epicsTimeStamp &stamp)
{
    epicsTimeStampFromDisk(stamp);
    if (stamp.nsec >= 1000000000L)
    {
        stamp.nsec = 0;
        return true;
    }
    return false;
}

