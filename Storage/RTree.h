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

#define RTreeM 50

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
        epicsTime  start, end;  // Range
        Offset     child_or_ID; // data block ID for leaf node; 0 if unused
        bool write(FILE *f) const;
        bool read(FILE *f);
    };

    class Node
    {
    public:
        Node(bool leaf = true);
        
        bool    isLeaf;  ///< Node or Leaf?        
        Offset  parent;  ///< 0 for root
        Record  record[RTreeM]; ///< index records of this node
        Offset  offset;  ///< Location in file
        
        /// Write to file at offset (needs to be set beforehand)
        bool write(FILE *f) const;

        /// Read from file at offset (needs to be set beforehand)
        bool read(FILE *f);

        /// Obtain interval covered by this node
        bool getInterval(epicsTime &start, epicsTime &end) const;
    };

    /// \see constructor Rtree()
    static const size_t anchor_size = 8;

    /// Attach RTree to FileAllocator.
    
    /// \param anchor: The RTree will deposit its root pointer there.
    ///                Caller needs to assert that there are RTree::anchor_size
    ///                bytes available at that location in the file.
    RTree(FileAllocator &fa, Offset anchor);

    ~RTree();
    
    /// Initialize empty tree. Compare to reattach().
    bool init();

    /// Re-attach to an existing tree. Compare to init().
    bool reattach();
    
    /// Return range covered by this RTree
    bool getInterval(epicsTime &start, epicsTime &end);

    /// Insert interval start...end with ID into tree.
    bool insert(const epicsTime &start, const epicsTime &end, Offset ID);

    /// Create and insert a new Datablock.
    bool insertDatablock(const epicsTime &start, const epicsTime &end,
                         Offset data_offset, stdString data_filename);
    
    /// Locate entry after start time.

    /// Updates Node & i and returns true if found.
    ///
    /// Specifically, the last record with data at or just before
    /// the start time is returned, so that the user can then decide
    /// if and how that value might extrapolate onto the start time.
    /// There's one exception: When requesting a start time
    /// that preceeds the first available data point, so that there is
    /// no previous data point, the very first record is returned.
    bool search(const epicsTime &start, Node &node, int &i);

    /// Like search(), but also gets datablock ref'ed by node&i.
    bool searchDatablock(const epicsTime &start, Node &node, int &i,
                         Datablock &block);
    
    /// Locate first entry.
    bool getFirst(Node &node, int &i);

    /// Like getFirst(), but also gets datablock
    bool getFirstDatablock(Node &node, int &i, Datablock &block);
    
    /// Locate last entry.
    bool getLatest(Node &node, int &i);
    
    /// If node & i were set to a valid entry by search(), update to prev
    bool prev(Node &node, int &i)
    {    return prev_next(node, i, -1); }
    
    /// If node & i were set to a valid entry by search(), update to next
    bool next(Node &node, int &i)
    {    return prev_next(node, i, +1); }

    bool nextDatablock(Node &node, int &i, Datablock &block);
    
    /// Remove entry from tree.
    bool remove(const epicsTime &start, const epicsTime &end, Offset ID);

    /// Set node & record index to last entry in tree

    /// (Used by Engine to append to last data block)
    ///
    ///
    bool getLatestDatablock(Datablock &block);
    
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
    bool updateLatest(const epicsTime &start,
                      const epicsTime &end, Offset ID);

    /// Tries to update existing datablock.

    /// Tries to use updateLatest, will then fall back to insertDatablock.
    ///
    ///
    bool updateLatestDatablock(const epicsTime &start, const epicsTime &end,
                               Offset data_offset, stdString data_filename);
    
    /// Create a graphviz 'dot' file.
    void makeDot(const char *filename, bool show_data=false);

    /// Returns true if tree passes self test, otherwise prints errors.
    bool selfTest();

    size_t cache_misses, cache_hits; 

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

    AVLTree<Node> node_cache;

    bool read_node(Node &node);

    bool write_node(const Node &node);
    
    bool self_test_node(Offset n, Offset p, epicsTime start, epicsTime end);
    
    void make_node_dot(FILE *dot, FILE *f, Offset node_offset, bool show_data);

    // Sets node to selected leaf for new entry start/end/ID or returns false.
    // Invoke by setting node.offset == root_offset.
    bool choose_leaf(const epicsTime &start, const epicsTime &end, Node &node);

    // Adjusts tree from node on upwards (update parents).
    // If new_node!=0, it's added to node's parent,
    // handling all resulting splits.
    // adjust_tree will write new_node, but not necessarily node!
    bool adjust_tree(Node &node, Node *new_node);

    bool remove_record(Node &node, int i);

    bool condense_tree(Node &node);

    bool prev_next(Node &node, int &i, int dir);
};


/// \@}
#endif

