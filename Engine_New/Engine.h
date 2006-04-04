#ifndef ENGINE_H_
#define ENGINE_H_

/** \defgroup Engine Engine
 *  Classes related to the ArchiveEngine.
 *  <p>
 *  The Engine is what keeps track of the various groups of channels,
 *  periodically processes the one and only ScanList,
 *  starts the HTTPServer etc.
 *  <p>
 *  Fundamentally, the data flows from the ProcessVariable,
 *  i.e. the network client, through one or more ProcessVariableFilter
 *  types into the CircularBuffer of the SampleMechanism.
 *  The following image shows the main actors:
 *  <p>
 *  @image html engine_api.png
 *  <p>
 *  This is the lock order. When locking more than
 *  one object from the following list, they have to be taken
 *  in this order, for example: First lock the PV, then the PVCtx.
 *  <ol>
 *  <li>Engine
 *  <li>GroupInfo
 *  <li>ArchiveChannel
 *  <li>SampleMechanism (same lock as ProcessVariable)
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
