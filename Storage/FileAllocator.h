// -*- c++ -*-

#ifndef __FILE_ALLOCATOR_H__
#define __FILE_ALLOCATOR_H__

// System
#include <stdio.h>
// Storage
#include <StorageTypes.h>

/// \addtogroup Storage
/// @{
///

/// Maintains memory blocks within a file.
///
/// The FileAllocator maintains the space in a given file
/// by keeping track of allocated blocks.
/// Sections can also be released, in which case they
/// are placed in a free-list for future allocations.
class FileAllocator
{
public:
    /// Constructor/destructor check if attach/detach have been called.
    ///
    /// We could have made these perform the attach/detach,
    /// but then the error reporting would have had to use
    /// exceptions which we'd like to avoid.
    FileAllocator();

    /// Destructor checks if detach has been called.
    ~FileAllocator();
    
    /// <B>Must be</B> invoked to attach (& initialize) a file.
    ///
    /// Can be invoked on an empty file with init=true,
    /// in thich case the reserved space
    /// and some FileAllocator header will be added
    /// to the file.
    /// Can also be invoked on an existing file (init=false or true)
    /// which was initialized by the FileAllocator,
    /// in which case the reserved_space has to match
    /// the value that was used when initializing the file!
    ///
    /// init=true means this file is used for writing,
    /// and we are allowed to create a new file allocator
    /// header for an empty file.
    ///
    /// @return true when an empty file was initialized.
    ///         Otherwise, some weak tests are performed
    ///         so see if this file does indeed contain
    ///         a FileAllocator header, and false is returned.
    /// @exception GenericException on error.
    bool attach(FILE *f, FileOffset reserved_space, bool init);

    /** @return After attaching to a file, this returns the file. */
    FILE *getFile() const;
    
    /** <B>Must be</B> called before destroying the FileAllocator
     *  and closing the file.
     */
    void detach();

    /// Allocate a block with given size, returning a file offset (for fseek).
    ///
    /// @see free()
    /// @exception GenericException on error.
    FileOffset allocate(FileOffset num_bytes);

    /** @return Total size of file in bytes. */
    FileOffset size() const
    {   return file_size; }
    
    /// Release a file block (will be placed in free list).
    ///
    /// @param offset A file offset previously obtained from allocate()
    /// @warning It is an error to free space that wasn't allocated.
    /// There is no 100% dependable way to check this,
    /// but free() will perform some basic test and return false
    /// for inknown memory regions.
    ///
    /// @exception GenericException on error.
    void free(FileOffset offset);

    /// To avoid allocating tiny areas,
    /// also to avoid splitting free blocks into pieces
    /// that are then too small to be ever useful,
    /// all memory regions are at least this big.
    /// Meaning: Whenever you allocate less than minimum_size,
    /// you will get a block that's actually minimum_size.
    static FileOffset minimum_size;

    /// Setting file_size_increment will cause the file size
    /// to jump in the given increments.
    static FileOffset file_size_increment;
    
    /// Show ASCII-type info about the file structure.
    ///
    /// This routines lists all the allocated and free blocks
    /// together with some other size information.
    /// It also performs some sanity checks.
    /// When called with <i>level=0</i>, it will only perform
    /// the tests and only report possible problems.
    ///
    /// Should not throw exceptions but simply return false
    /// on errors.
    ///
    /// @param level Level of verbosity.
    /// @param filename When non-zero, it is used instead of stdout.
    ///
    /// Returns true for 'OK'.
    bool dump(int level=1, FILE *out=stdout);
    
private:
    PROHIBIT_DEFAULT_COPY(FileAllocator);

    /** One node of the 'free' or 'allocated' list of disk blocks. */
    class Node
    {
    public:
        Node();
        
        /** Size of the node itself on the disk, excluding the actual data. */
        static FileOffset size()
        {   return 12; }

        /** @return File offset of next node. */
        FileOffset getNext() const
        {   return next; }

        /** @return File offset of next node. */
        void setNext(FileOffset new_next)
        {   next = new_next; }

        /** @return File offset of previous node. */
        FileOffset getPrev() const
        {   return prev; }

        /** @return File offset of previous node. */
        void setPrev(FileOffset new_prev)
        {   prev = new_prev; }

        /** @return Number of bytes in this node. */
        FileOffset getBytes() const
        {   return bytes; }

        /** Set number of bytes in this node. */
        void setBytes(FileOffset new_bytes)
        {   bytes = new_bytes; }

        /** Add to number of bytes in this node. */
        void addBytes(int bytes_to_add);

        /** Read node from file
         *  @exception GenericException on error.
         */
        void read(FILE *f, FileOffset offset);
        
        /** Write node to file
         *  @exception GenericException on error.
         */
        void write(FILE *f, FileOffset offset) const;
        
        /** Write node location where last read or written.
         *  @exception GenericException on error.
         */
        void write() const;
        
        /** @return File offset where node was read or written. */
        FileOffset getOffset() const
        {   return offset; }

    private:
        mutable FILE *f;
        mutable FileOffset offset;
        FileOffset prev;
        FileOffset next;
        FileOffset bytes;
    };
    
    FILE *f;
    FileOffset reserved_space; // Bytes we ignore in header
    FileOffset file_size; // Total # of bytes in file
    // For the head nodes,
    // 'prev' = last entry, tail of list,
    // 'next' = first entry, head of list!
    // ! These nodes are always a current copy
    //   of what's on the disk!
    Node allocated_head, free_head;
    
    // Unlink node from list, node itself remains unchanged
    void remove_node(Node &head, const Node &node);
    // Insert node (sorted), node's prev/next get changed
    void insert_node(Node &head,
                     FileOffset node_offset, Node &node);
};

///
/// @}
///

#endif

