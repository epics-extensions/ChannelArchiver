// -*- c++ -*-

#ifndef __RTREE_H__
#define __RTREE_H__

// Tools
#include <epicsTimeHelper.h>
#include <AVLTree.h>
// Index
#include "FileAllocator.h"

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

/// \ingroup Storage
/// \@{

/// Implements a file-based RTree as
/// described in Antonin Guttman:
/// "R-Trees: A Dynamic Index Structure for Spatial Searching"
/// (Proc. 1984 ACM-SIGMOD Conference on Management of Data, pp. 47-57).
///
/// The records are time intervals start...end.
/// In addition to what's described in the Guttman paper,
/// - all records are non-overlapping
///   (they might touch but they don't overlap);
/// - all records are sorted by time;
/// - node removal only for empty nodes, no reordering of records.
class RTree
{
public:
    typedef long Offset;

    class Datablock
    {
    public:
        Datablock() : next_ID(0), data_offset(0), offset(0) {}
        Offset    next_ID;       
        Offset    data_offset;   ///< This block's offset in DataFile
        stdString data_filename; ///< DataFile for this block
        
        Offset offset;  ///< Location of DataBlock in index file
        long getSize() const;
        bool write(FILE *f) const;
        bool read(FILE *f);
    };

    class Record
    {
    public:
        Record();
        void clear();
        epicsTime  start, end;  // Range
        Offset     child_or_ID; // data block ID for leaf node; 0 if unused
        bool write(FILE *f) const;
        bool read(FILE *f);
    };

    class Node
    {
    public:
        Node(int M, bool leaf);
        Node(const Node &);
        ~Node();

        Node &operator = (const Node &);
        
        bool    isLeaf;  ///< Node or Leaf?        
        Offset  parent;  ///< 0 for root
        Record  *record; ///< index records of this node
        Offset  offset;  ///< Location in file
        
        /// Write to file at offset (needs to be set beforehand)
        bool write(FILE *f) const;

        /// Read from file at offset (needs to be set beforehand)
        bool read(FILE *f);

        /// Obtain interval covered by this node
        bool getInterval(epicsTime &start, epicsTime &end) const;
    private:
        int M;
        bool operator == (const Node &); // not impl.
    };

    /// \see constructor Rtree()
    static const size_t anchor_size = 8;

    /// Attach RTree to FileAllocator.
    
    /// \param anchor: The RTree will deposit its root pointer there.
    ///                Caller needs to assert that there are RTree::anchor_size
    ///                bytes available at that location in the file.
    RTree(FileAllocator &fa, Offset anchor);
    
    /// Initialize empty tree. Compare to reattach().
    bool init(int M);

    /// Re-attach to an existing tree. Compare to init().
    bool reattach();

    /// The 'M' value, i.e. Node size, of this RTree.
    int getM() const
    { return M; }
    
    /// Return range covered by this RTree
    bool getInterval(epicsTime &start, epicsTime &end);
    
    enum YNE { YNE_Error, YNE_Yes, YNE_No };
    
    /// Create and insert a new Datablock.

    /// Note: Once a data block (offset and filename) is inserted
    ///       for a given start and end time, the RTree code assumes
    ///       that it stays that way. I.e. if we try to indert the same
    ///       start/end/offset/file again, this will result in a NOP
    ///       and return YNENo.
    ///       It is an error to insert the same offset/file again with
    ///       a different start and/or end time!
    YNE insertDatablock(const epicsTime &start, const epicsTime &end,
                        Offset data_offset,
                        const stdString &data_filename);
    
    /// Locate entry after start time.

    /// Updates Node & i and returns true if found.
    ///
    /// Specifically, the last record with data at or just before
    /// the start time is returned, so that the user can then decide
    /// if and how that value might extrapolate onto the start time.
    /// There's one exception: When requesting a start time
    /// that preceeds the first available data point, so that there is
    /// no previous data point, the very first record is returned.

    bool searchDatablock(const epicsTime &start, Node &node, int &i,
                         Datablock &block) const;

    /// Locate first datablock in tree.
    bool getFirstDatablock(Node &node, int &i, Datablock &block) const;
    
    /// \see getFirstDatablock
    bool getLastDatablock(Node &node, int &i, Datablock &block) const;    

    /// Get a sub-block that's under the current block.

    /// A record might not only point to the 'main' data block,
    /// the one originally inserted and commonly used
    /// for data retrieval. It can also contain a chain of
    /// data blocks that were inserted later (at a lower priority).
    /// In case you care about those, invoke getCoveredBlock()
    /// until it returns false.
    bool getNextChainedBlock(Datablock &block) const;
    
    /// Absolutely no clue what this one could do.
    bool getPrevDatablock(Node &node, int &i, Datablock &block) const;

    /// \see getPrevDatablock
    bool getNextDatablock(Node &node, int &i, Datablock &block) const;
    
    /// Tries to update existing datablock.

    /// Tries to use updateLatest, will then fall back to insertDatablock.
    ///
    ///
    bool updateLastDatablock(const epicsTime &start, const epicsTime &end,
                             Offset data_offset, stdString data_filename);
    
    /// Create a graphviz 'dot' file.
    void makeDot(const char *filename);

    /// Returns true if tree passes self test, otherwise prints errors.

    /// On success, nodes will contain the number of nodes in the tree
    /// and record contains the number of used records.
    /// One can compare that to the total number of available records,
    /// nodes*getM().
    bool selfTest(unsigned long &nodes, unsigned long &records);

    mutable size_t cache_misses, cache_hits; 

private:
    FileAllocator &fa;
    // This is the (fixed) offset into the file
    // where the RTree information starts.
    // It points to
    // long current root offset
    // long RTreeM
    Offset anchor;
    
    // Offset to the root = content of what's at anchor
    Offset root_offset;

    int M;
    
    mutable AVLTree<Node> node_cache;

    bool read_node(Node &node) const;

    bool write_node(const Node &node);
    
    bool self_test_node(unsigned long &nodes, unsigned long &records,
                        Offset n, Offset p, epicsTime start, epicsTime end);
    
    void make_node_dot(FILE *dot, FILE *f, Offset node_offset);

    bool search(const epicsTime &start, Node &node, int &i) const;

    /// Locate first entry.
    bool getFirst(Node &node, int &i) const;

    /// Set node & record index to last entry in tree.
    bool getLast(Node &node, int &i) const;

    /// If node & i were set to a valid entry by search(), update to prev.
    bool prev(Node &node, int &i) const
    {    return prev_next(node, i, -1); }
    
    /// If node & i were set to a valid entry by search(), update to next.
    bool next(Node &node, int &i) const
    {    return prev_next(node, i, +1); }

    bool prev_next(Node &node, int &i, int dir) const;
    
    YNE add_block_to_record(const Node &node, int i,
                            Offset data_offset,
                            const stdString &data_filename);
    
    bool write_new_datablock(Offset data_offset,
                             const stdString &data_filename,
                             Datablock &block);
    
    // Sets node to selected leaf for new entry start/end/ID or returns false.
    // Invoke by setting node.offset == root_offset.
    bool choose_leaf(const epicsTime &start, const epicsTime &end, Node &node);

    bool insert_record_into_node(Node &node,
                                 int idx,
                                 const epicsTime &start,
                                 const epicsTime &end,
                                 Offset ID,
                                 Node &overflow,
                                 bool &caused_overflow, bool &rec_in_overflow);
    
    // Adjusts tree from node on upwards (update parents).
    // If new_node!=0, it's added to node's parent,
    // handling all resulting splits.
    // adjust_tree will write new_node, but not necessarily node!
    bool adjust_tree(Node &node, Node *new_node);

    /// Remove entry from tree.
    bool remove(const epicsTime &start, const epicsTime &end, Offset ID);

    bool remove_record(Node &node, int i);

    bool condense_tree(Node &node);

    /// Special 'update' call for usage by the ArchiveEngine.

    /// The engine usually appends to the last buffer.
    /// So most of the time, the ID and start time in this
    /// call have not changed, only the end time has been extended,
    /// and the intention is to update the tree.
    /// Sometimes, however, the engine created a new block,
    /// in which case it will call append_latest a final time
    /// to update the end time and then it'll follow with an insert().
    /// \return True if start & ID refer to the existing last block
    ///         and the end time was succesfully updated.
    bool updateLast(const epicsTime &start,
                    const epicsTime &end, Offset ID);
};


/// \@}
#endif

