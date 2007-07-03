// -*- c++ -*-

#ifndef __RTREE_H__
#define __RTREE_H__

// Tools
#include <AVLTree.h>
// Storage
#include <FileAllocator.h>
#include <Interval.h>

// When using the ArchiveDataTool to convert about 500 channels,
// 230MB of data, 102K directory file into an index, these were the
// results:
// Via NFS:
// M=3
// real    0m30.871s
// user    0m0.790s
// sys     0m2.030s
//  
// M=10
// real    0m23.944s
// user    0m0.830s
// sys     0m1.690s
//
// Local disk:
// No profiling, M=10
// real    0m17.148s
// user    0m0.700s
// sys     0m0.990s
//  
// No profiling, M=50
// real    0m3.402s  (!)
// user    0m1.290s
// sys     0m0.770s
//
// --> NFS is bad, small RTreeM values are bad.

/** \ingroup Storage
 *  Implements a file-based RTree.
 *
 * See the Antonin Guttman paper
 * "R-Trees: A Dynamic Index Structure for Spatial Searching"
 * (Proc. 1984 ACM-SIGMOD Conference on Management of Data, pp. 47-57).
 *
 * The records are time intervals start...end.
 * In addition to what's described in the Guttman paper,
 * - all records are non-overlapping
 *   (they might touch but they don't overlap);
 * - all records are sorted by time;
 * - node removal only for empty nodes, no reordering of records.
 */
class RTree
{
public:
	/** Each RTree leaf points to Datablocks;
	 *  they describe where the actual data that
	 *  the index references resides via
	 *  a filename and an offset into that file.
	 */
    class Datablock
    {
    public:
        Datablock();
        virtual ~Datablock();

        /** @return true if this data block info is valid. */
        virtual bool isValid() const = 0;
           
        /** The file name referenced by this entry. */
        const stdString &getDataFilename() const
        {
            LOG_ASSERT(isValid());
            return data_filename;
        }
        
        /** The file offset of this entry. */
        FileOffset getDataOffset() const
        {
            LOG_ASSERT(isValid());
            return data_offset;
        }
        
        virtual const Interval &getInterval() const = 0;
        
        /** Get a sub-block that's under the current block.
	     * 
	     *  A record might not only point to the 'main' data block,
	     *  the one originally inserted and commonly used
	     *  for data retrieval. It can also contain a chain of
	     *  data blocks that were inserted later (at a lower priority).
	     *  In case you care about those, invoke getNextChainedBlock()
	     *  until it returns false.
         *  <p>
         *  A possible usage scenario would be to
         *  <ol>
         *  <li>Start with a valid data block.
         *  <li>iterate over chained blocks until getNextChainedBlock()
         *      returns false
         *  <li>then continue with getNextDatablock()
         *  </ol>
         *  To allow for this, getNextChainedBlock() will leave the
         *  data block 'valid' when it reaches the last chained block.
	     *
	     *  @exception GenericException on read error, or when called
         *            while isValid() == false.
	     */
	    virtual bool getNextChainedBlock() = 0;
	    
        /** Get the data block for the next time interval from
         *  the RTree.
         *  @return true if one found. Otherwise, false is returned
         *          and isValid() will also indicate false.
         *  @see getPrevDatablock
         *  @exception GenericException on read error, or when called
         *             with isValid() == false.
         */
        virtual bool getNextDatablock() = 0;

        /** Absolutely no clue what this one could do.
	     *  @see getNextDatablock
	     */
	    virtual bool getPrevDatablock() = 0;
        
    protected:
        PROHIBIT_DEFAULT_COPY(Datablock);

        FileOffset data_offset;
        stdString  data_filename;
    };

    /** \see constructor Rtree() */
    static const size_t anchor_size = 8;

    /** Attach RTree to FileAllocator.
     *
     * \param anchor: The RTree will deposit its root pointer there.
     *                Caller needs to assert that there are
     *                RTree::anchor_size
     *                bytes available at that location in the file.
     */
    RTree(FileAllocator &fa, FileOffset anchor);
    
    /** Initialize empty tree. Compare to reattach().
      * @exception GenericException on write error.
      */
    void init(int M);

    /** Re-attach to an existing tree. Compare to init().
      * @exception GenericException on write error.
      */
    void reattach();

    /** The 'M' value, i.e. Node size, of this RTree. */
    int getM() const
    {   return M; }
    
    /** Return range covered by this RTree
      * @exception GenericException on read error
      */
    Interval getInterval();

    /** Locate data after start time.
     *
     *  Specifically, the last record with data at or just before
     *  the start time is returned, so that the user can then decide
     *  if and how that value might extrapolate onto the start time.
     *  There's one exception: When requesting a start time
     *  that preceeds the first available data point, so that there is
     *  no previous data point, the very first record is returned.
     *  
     * @return Data block info or 0 if nothing found.
     *         Caller must delete.
     *
     * @exception GenericException on read error.
     */
    Datablock *search(const epicsTime &start) const;

    /** @return Locate first datablock in tree, or 0 if there is nothing.
     *          Client must delete.
     *  @exception GenericException on read error.
     */
    Datablock *getFirstDatablock() const;
    
    /** \see getFirstDatablock */
    Datablock *getLastDatablock() const; 
    
    /** @return Leaf node suitable for range. */
    class Node chooseLeaf(const Interval &range);
    
    /** Create and insert a new Datablock.
     *
     * Note: Once a data block (offset and filename) is inserted
     *       for a given start and end time, the RTree code assumes
     *       that it stays that way. I.e. if we try to insert the same
     *       start/end/offset/file again, this will result in a NOP
     *       and return false.
     *       It is an error to insert the same offset/file again with
     *       a different start and/or end time!
     *
     * @see updateLastDatablock for the special case up updating the _end_ time.
     * @return true if a new entry was created, false if offset and filename
     *         were already found.
     * @exception GenericException on write error.
     */
    bool insertDatablock(const Interval &range,
                         FileOffset data_offset,
                         const stdString &data_filename);
    
    /** Remove reference to given data block.
     *  @return Number of data block references found and removed, or 0.
     *  @exception GenericException on internal error.
     */
    size_t removeDatablock(const Interval &range,
                         FileOffset data_offset,
                         const stdString &data_filename);

    /** Tries to update existing datablock.
     *
     * Tries to update the end time of the last datablock,
     * in case start, data_offset and data_filename all match.
     * Will otherwise fall back to insertDatablock.
     *
     * @return true if a new entry was created, false if offset and filename
     *         were already found.
     * @exception GenericException on write error.
     */
    bool updateLastDatablock(const Interval &range,
                             FileOffset data_offset, stdString data_filename);
    
    /** Create a graphviz 'dot' file. */
    void makeDot(const char *filename);

    /** Returns true if tree passes self test, otherwise prints errors.
     * 
     * On success, nodes will contain the number of nodes in the tree
     * and record contains the number of used records.
     * One can compare that to the total number of available records,
     * nodes*getM().
     */
    bool selfTest(unsigned long &nodes, unsigned long &records);

private:
    PROHIBIT_DEFAULT_COPY(RTree);
    
    /** FileAllocator of the file that contains this RTree. */
    FileAllocator &fa;
    
    /** This is the (fixed) offset into the file
     *  where the RTree information starts.
     *  It points to
     *  FileOffset current root offset
     *  FileOffset RTreeM
     */
    const FileOffset anchor;
    
    // FileOffset to the root = content of what's at anchor
    FileOffset root_offset;

    int M;
    
    // mutable AVLTree<Node> node_cache;
    mutable size_t cache_misses, cache_hits; 
    
    void make_node_dot(FILE *dot, FILE *f, FileOffset node_offset);

    /** Locate node and index for the first valid record in the tree.
     *  @return true on success, false if tree is empty.
     * @exception GenericException on read error
     */
    bool getFirstRecord(class Node &node, int &record_index) const;

    /** @see getFirstRecord() */
    bool getLastRecord(class Node &node, int &record_index) const;

    /** Split an existing record at the given 'cut' time, so that
     *  there are two records start..cut and cut...end with the same
     *  data blocks as the original record.
     */
    void split_record(Node &node, int idx, const epicsTime &cut);

    /** Adjusts tree from node on upwards (update parents).
     *  If new_node!=0, it's added to node's parent,
     *  handling all resulting splits.
     *  adjust_tree will write new_node, but not necessarily node!
     *  @exception GenericException on read/write error
     */
    void adjust_tree(Node &node, Node *new_node);
    
    /** Remove reference to given data block from given node on down.
     *  @return Number of references found and removed.
     *  @exception GenericException on internal error.
     */
    size_t removeDatablock(Node &node, const Interval &range,
                       FileOffset data_offset, const stdString &data_filename);
      
    /** Remove data block under given node's record. */
    size_t removeDatablock(Node &node, int record_index, const Interval &range,
                       FileOffset data_offset, const stdString &data_filename);
                       
    /** Remove record i from node, condense tree.
     * @exception GenericException on read/write error.
     */
    void remove_record(Node &node, int i);

    /** Starting at a node that was just made "smaller",
     *  go back up to root and update all parents.
     *  @exception GenericException on read/write error
     */
    void condense_tree(Node &node);

    /** Special 'update' call for usage by the ArchiveEngine.
     *
     * The engine usually appends to the last buffer.
     * So most of the time, the ID and start time in this
     * call have not changed, only the end time has been extended,
     * and the intention is to update the tree.
     * Sometimes, however, the engine created a new block,
     * in which case it will call updateLast a final time
     * to update the end time and then it'll follow with an insert().
     * \return True if start & ID refer to the existing last block
     *         and the end time was succesfully updated.
     */
    bool updateLast(const epicsTime &start,
                    const epicsTime &end, FileOffset ID);
};

#endif

