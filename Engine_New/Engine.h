#ifndef ENGINE_H_
#define ENGINE_H_

/** \defgroup Engine Engine
 *  Classes related to the ArchiveEngine
 * 
 *  TODO: add \image
 * 
 *  This is the lock order. When locking more than
 *  one object from the following list, they have to be taken
 *  in this order, for example: First lock the PV, then the PVCtx.
 *  <ol>
 *  <li>Engine
 *  <li>GroupInfo
 *  <li>ArchiveChannel
 *  <li>SampleMechanism
 *  <li>CircularBuffer
 *  <li>ProcessVariable
 *  <li>ProcessVariableContext
 *  </ol>
 */
 
/** \ingroup Engine
 *  The archive engine.
 */
class Engine
{
private:
	stdList<ArchiveChannel *> channels;// all the channels
    stdList<GroupInfo *>      groups;    // scan-groups of channels
};

#endif /*ENGINE_H_*/
