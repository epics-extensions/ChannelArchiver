#ifndef __RTREE_PRIVATE_H__
#define __RTREE_PRIVATE_H__

// Index
#include "RTree.h"

/** One record in the RTree:
 *  A start...end interval and a pointer
 *  to either another RTree sub-node,
 *  or a leaf node that points to a Datablock.
 */
class Record
{
public:
    Record();
    Record(const Interval &interval, FileOffset child_or_ID);
    Record(const Record &other);
    Record &operator = (const Record &other);
    
    /** Size of a record in the file. */
    static size_t size()
    {   // Interval start, end; offset 
        return 8+8+4;
    }
    
    /** Get the time range covered by this record. */
    const Interval &getInterval() const
    {   return interval; }
    
    /** Update the interval. */
    void setInterval(const Interval &range)
    {   interval = range; }
    
    void setEnd(const epicsTime &end)
    {   interval.setEnd(end); }

    /** For records in leaf nodes, this is the ID (e.g. data block info offset).
     *  Otherwise, it's the offset to a sub-tree Node.
     */
    FileOffset getChildOrID() const
    {   return child_or_ID; }

    /** Set the sub-tree offset (non-leaf) or data block offset (leaf node). */
    void setChildOrID(FileOffset ID)
    {   child_or_ID = ID; }

    /** Clear info in record. */
    void clear();
    
    /** Write record to file at current location.
     *  @exception GenericException on write error.
     */
    void write(FILE *f) const;

    /** Read record from file at current location.
     *  @exception GenericException on read error.
     */
    void read(FILE *f);
    
private:
    Interval interval;
    FileOffset child_or_ID; // data block ID for leaf node; 0 if unused
    
	void writeEpicsTime(FILE *f, const epicsTime &t) const;
	epicsTime readEpicsTime(FILE *f) const;

    bool operator == (const Record &other); // not impl.
};

/** One node of the RTree, containing several Records. */ 
class Node
{
public:
    Node(FileAllocator &fa, int M, bool leaf=true);
    Node(const Node &);
    ~Node();

    /** Size of this node in the file. */
    size_t size() const
    {   // is_leaf, parent, records
        return 1+4+Record::size()*M;
    }

    Node &operator = (const Node &);
    
    /** Obtain interval covered by this node
      * @return True if there is a valid interval, false if empty.
      */
    Interval getInterval() const;
    
    /** @return RTree 'M', # of records. */
    int getM() const
    {   return M; }
        
    /** @parm i Record index, 0...M
     *  @return Read-only reference to record.
     */
    const Record &getRecord(int i) const
    {
        LOG_ASSERT(i >= 0);
        LOG_ASSERT(i < M);
        return record[i];
    }

    /** @parm i Record index, 0...M
     *  @return Reference to record.
     */
    Record &getRecord(int i)
    {
        if (i < 0  ||  i >= M)
        {
            printf("i == %d ?!\n", i);
        }
        LOG_ASSERT(i >= 0);
        LOG_ASSERT(i < M);
        return record[i];
    }

    /** Update record i with given data. */
    void setRecord(int i, const Record &new_values)
    {
        LOG_ASSERT(i >= 0);
        LOG_ASSERT(i < M);
        record[i] = new_values;
    }

    /** @return true for leaf nodes. */
    bool isLeaf() const
    {   return is_leaf; }
    
    /** @return Offset of parent Node inside rtree file. */
    FileOffset getParent() const
    {   return parent; }

    /** Set pointer to parent Node. */
    void setParent(const Node &new_parent)
    {   parent = new_parent.offset; }

    /** Clear pointer to parent Node. */
    void clearParent()
    {   parent = 0; }

    /** @return index of the Record that points to given ID (or child).
     *  @exception When not found.
     */
    int findRecordByID(FileOffset ID) const;

    /** @return index of the first valid Record,
     *          or -1 if nothing found.
     */
    int findLowestRecord() const;

    /** @return index of the highest valid Record,
     *          or -1 if nothing found.
     */
    int findHighestRecord() const;

    /** Write to file at offset.
     *  @exception GenericException on write error
     */
    void write(FileOffset offset);

    /** Write to file at offset where node was read.
     *  @exception GenericException on write error
     */
    void write()
    {   write(offset); }

    /** Read from file at offset.
     *  @exception GenericException on read error
     */
    void read(FileOffset offset);

    /** @return File that contains this Node. */
    FILE *getFile() const
    {   return fa.getFile(); }

    /** @return Offset of this Node inside rtree file. */
    FileOffset getOffset() const
    {   return offset; }
    
    /** Drill down from this node to a leaf that's suitable for range.
     *  Usually invoked with node at root_offset.
     *  @exception GenericException on read error.
     */
    void chooseLeaf(const Interval &range);
    
    /** Insert new record (start/end/ID) into node's record[idx],
     *  moving remaining records to the 'right'.
     *  If that causes an overflow, use the overflow node.
     *  Overflow needs to be initialized:
     *  All records 0, isLeaf as it needs to be,
     *  but mustn't be allocated, yet: This routine will allocate
     *  if overflow gets used.
     *  idx may actually be == M, in which case it uses overflow.record(0)
     *  caused_overflow indicates if the overflow Node is used.
     *  rec_in_overflow indicates if the new record ended up in overflow.
     */
    void insertRecord(int idx, const Record &new_record,
                      Node &overflow,
                      bool &caused_overflow, bool &rec_in_overflow);
    
    /** Add another record to the list of chained data blocks under this node.
     *  @return true if new block offset/filename was added under node/i,
     *          false if block with offset/filename was already there.
     *  @exception GenericException if something's messed up.
     *             This includes the case that the Node has no blocks, yet.
     */
    bool addToRecord(int i,
                     FileOffset data_offset, const stdString &data_filename);
        
    /** Remove data from the chained data blocks under this node.
     *  @return true if given block offset/filename was removed under node/i,
     *          false if block with offset/filename was not found there.
     *  @exception GenericException if something's messed up.
     */
    bool removeFromRecord(int i,
                       FileOffset data_offset, const stdString &data_filename);

    /** Perform self-test of node and sub-nodes.
     *  @parm nodex, records are updated with count of nodes and records
     *  @parm start, end are the times that the parent node holds
     *        for this node.
     *  @exception GenericException on error.
     */
    void selfTest(unsigned long &nodes, unsigned long &records,
                  FileOffset parent_offset, epicsTime start, epicsTime end);
    
    /** Add 'dot' file commands for node and subnodes to given file. */
    void makeDot(FILE *dot);
    
    void dump() const;
    
private:
    /** Allocator for the file that contains this node. */
    FileAllocator &fa;
    
    /** Location in file. */
    FileOffset offset;

    /** RTree 'M', # of records in node. */
    const int M;    

    // Actual Node data that's also kept in disk file
    
    /** true for leaf nodes. */
    bool is_leaf;

    /** Pointer to parent node; 0 for root node. */
    FileOffset parent;

    /** Record array of this node */
    Record *record;

    // not impl.
    bool operator == (const Node &);
};

/** Datablock that can read/write itself.
 *  Also allowing for a chain of data blocks
 *  below one tree leaf.
 */
class PrivateDatablock : public RTree::Datablock
{
public:
    /** Constructor. Doesn't read or write file. */
    PrivateDatablock(const Node &node, int record_index);
   
    /** Constructor. Doesn't read or write file. */
    PrivateDatablock(const Node &node, int record_index,
                     const stdString &data_filename,
                     FileOffset data_offset);

    size_t size() const;
    
    /** Read the first datablock that the node points to.
     *  @returns true if there was one, false if there is no datablock.
     */
    bool readFirst();
    
    /** Read datablock from anywhere in file
     *  @exception GenericException on read error.
     */
    void read(FileOffset offset);

    /** Write datablock to given location.
     *  @exception GenericException on write error
     */
    void write(FileOffset offset);

    /** @return Location in file where this block was last read or written. */
    FileOffset getOffset() const
    {   return offset; }

    /** Write datablock to where it was last read.
     *  @exception GenericException on write error
     */
    void write()
    {   write(offset); }
    
    // @see Datablock
    virtual bool isValid() const;
    virtual const Interval &getInterval() const;
    virtual bool getNextChainedBlock();
    virtual bool getNextDatablock();
    virtual bool getPrevDatablock();
    
    /** Add 'dot' commands for this data block and chained blocks to file. */
    void makeDot(FILE *dot);
    
    /** @return Pointer to next (chained) data block or 0. */
    FileOffset getNext() const
    {   return next_ID; }       
    
    /** Set pointer to next (chained) data block. */
    void setNext(FileOffset ID)
    {   next_ID = ID;  }
    
    bool isLast() const
    {   return next_ID == 0; }

private:
    PROHIBIT_DEFAULT_COPY(PrivateDatablock);

    /** Location of DataBlock in file.
     *  A value > 0 is also used to indicate 'valid'.
     */
    FileOffset offset;

    /** Node that points to this data block.
     *  Starts out as a copy of what's passed to constructor,
     *  but getPrevDatablock/getNextDatablock can change that.
     */
    Node node;

    /** Index of record in node that points to this block. */
    int record_index;
    
    // The data file name and offset are inherited from RTree::Datablock
    // This is the added info for each datablock on the disk
    
    /** Pointer to next (chained) data block or 0. */
    FileOffset next_ID;       

    enum PrevNextDir { PREV = -1, NEXT = +1 };
    /** Update node and record_index, currently poining to a leaf record,
     *  to the next/previous leaf record.
     *  @return true if anything found.
     */
    bool prev_next(PrevNextDir dir);
};

#endif
