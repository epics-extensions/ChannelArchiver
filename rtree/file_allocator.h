// -*- c++ -*-

#ifndef _FILE_ALLOCATOR_H_
#define _FILE_ALLOCATOR_H_

#include <stdio.h>

/**
 * \class file_allocator
 * \brief Maintains memory blocks within a file.
 *
 * The file_allocator maintains the space in a given file
 * by keeping track of allocated blocks.
 * Sections can also be released, in which case they
 * are placed in a free-list for future allocations.
 *
 * \author kasemir@lanl.gov
 */
class file_allocator
{
public:
    /// Constructor/destructor check if attach/detach have been called.
    /** We could have made these perform the attach/detach,
     * but then the error reporting would have had to use
     * exceptions which we'd like to avoid.
     */
    file_allocator();
    /// Destructor checks if detach has been called.
    ~file_allocator();
    
    /// <B>Must be</B> invoked to attach (& initialize) a file.
    bool attach(FILE *f, long reserved_space);

    /// After attaching to a file, this returns the file
    FILE *getFile() const { return f; }
    
    /// <B>Must be</B> called before destroying the file_allocator and closing the file.
    void detach();

    /// Allocate a block with given size, returning a file offset (for fseek).
    /**
     * \see free()
     */
    long allocate(long num_bytes);

    /// Release a file block (will be placed in free list).
    /**
     * \param offset A file offset previously obtained from allocate()
     * \warning It is an error to free space that wasn't allocated.
     * There is no 100% dependable way to check this,
     * but free() will perform some basic test and return false
     * for inknown memory regions.
     */
    bool free(long offset);

    /// To avoid allocating tiny areas,
    /// also to avoid splitting free blocks into pieces
    /// that are then too small to be ever useful,
    /// all memory regions are at least this big.
    /// Meaning: Whenever you allocate less than minimum_size,
    /// you will get a block that's actually minimum_size.
    static long minimum_size;

    /// Setting file_size_increment will cause the file size
    /// to jump in the given increments.
    static long file_size_increment;
    
    /// Show ASCII-type info about the file structure.
    /**
     * This routines lists all the allocated and free blocks
     * together with some other size information.
     * It also performs some sanity checks.
     * When called with <i>level=0</i>, it will only perform
     * the tests and only report possible problems.
     */
    void dump(int level=1);
    
private:
    typedef struct
    {
        long bytes;
        long prev;
        long next;
    } list_node;
    
    FILE *f;
    long reserved_space; // Bytes we ignore in header
    long file_size; // Total # of bytes in file
    // For the head nodes,
    // 'prev' = last entry, tail of list,
    // 'next' = first entry, head of list!
    // ! These nodes are always a current copy
    //   of what's on the disk!
    list_node allocated_head, free_head;
    
    bool read_node(long offset, list_node *node);
    bool write_node(long offset, const list_node *node);
    // Unlink node from list, node itself remains unchanged
    bool remove_node(long head_offset, list_node *head,
                     long node_offset, const list_node *node);
    // Insert node (sorted), node's prev/next get changed
    bool insert_node(long head_offset, list_node *head,
                     long node_offset, list_node *node);
};

#endif // _R_TREE_H_



















