// System
#include <string.h>
// Tools
#include <MsgLogger.h>
#include <BinIO.h>
// Index
#include "FileAllocator.h"

#undef DEBUG_FA

static const FileOffset MaxFileSize = 0x7FFFFFFF;

// Layout of the data file:
//
// - From the beginning of the file, reserved_space
//   bytes are skipped.
// - Two list_node entries follow:
//   a) list of allocated units,
//   b) list of free units
// - Allocated or free data block follow:
//   each one starts with a list_node,
//   the user of the FileAllocator is handed
//   the offset to the next byte

FileOffset FileAllocator::minimum_size = 64;
FileOffset FileAllocator::file_size_increment = 0;


FileAllocator::Node::Node()
    : f(0), offset(0), prev(0), next(0), bytes(0)
{
}

void FileAllocator::Node::addBytes(int bytes_to_add)
{
    if (bytes_to_add < 0)
    {
        LOG_ASSERT((int) bytes >= -bytes_to_add);
    }
    bytes += bytes_to_add;
}


void FileAllocator::Node::read(FILE *f, FileOffset offset)
{
    this->f = f;
    this->offset = offset;
    if (! (fseek(f, offset, SEEK_SET) == 0  &&
           readLong(f, &bytes) &&
           readLong(f, &prev) &&
           readLong(f, &next)))
        throw GenericException(__FILE__, __LINE__,
                               "FileAllocator node read at 0x%08lX failed",
                               (unsigned long)offset);
}

void FileAllocator::Node::write(FILE *f, FileOffset offset) const
{
    this->f = f;
    this->offset = offset;
    if (! (fseek(f, offset, SEEK_SET) == 0  &&
           writeLong(f, bytes) &&
           writeLong(f, prev) &&
           writeLong(f, next)))
        throw GenericException(__FILE__, __LINE__,
                               "FileAllocator node write at 0x%08lX failed",
                               (unsigned long)offset);
}

void FileAllocator::Node::write() const
{
    LOG_ASSERT(f != 0);
    LOG_ASSERT(offset != 0);
    write(f, offset);
}

FileAllocator::FileAllocator()
{
#ifdef DEBUG_FA
    printf("New FileAllocator\n");
#endif
    f = 0;
}

FileAllocator::~FileAllocator()
{
    if (f)
    {
        fprintf(stderr, "*****************************\n");
        fprintf(stderr, "FileAllocator wasn't detached\n");
        fprintf(stderr, "*****************************\n");
    }
#ifdef DEBUG_FA
    printf("Deleted FileAllocator\n");
#endif
}    

bool FileAllocator::attach(FILE *f, FileOffset reserved_space, bool init)
{
#ifdef DEBUG_FA
    printf("FileAllocator::attach()\n");
#endif
    LOG_ASSERT(f != 0);
    this->f = f;
    this->reserved_space = reserved_space;
    if (fseek(this->f, 0, SEEK_END))
        throw GenericException(__FILE__, __LINE__, "fseek error");
    file_size = ftell(this->f);
    if (file_size < 0)
        throw GenericException(__FILE__, __LINE__, "ftell error");
    // File size should be
    // 0 for new file or at least reserved_space + list headers
    if (file_size == 0)
    {
        if (init == false)
            throw GenericException(__FILE__, __LINE__,
                                   "FileAllocator in read-only mode found empty file");
        // create empty list headers
        allocated_head.write(this->f, this->reserved_space);
        free_head.write(this->f, this->reserved_space+Node::size());
        file_size = ftell(this->f);
        FileOffset expected = reserved_space + 2*Node::size();
        if (file_size != expected)
             throw GenericException(__FILE__, __LINE__,
                                    "Initialization error: "
                                    "Expected file size %ld, found %ld",
                                    (long) expected, (long) file_size);
        return true; 
    }
    FileOffset expected = this->reserved_space + 2*Node::size();
    if (file_size < expected)
        throw GenericException(__FILE__, __LINE__,
                               "FileAllocator: Broken file header,"
                               "expected at least %ld bytes",
                               (long) expected);
    // read existing list headers
    allocated_head.read(this->f, this->reserved_space);
    free_head.read(this->f, this->reserved_space+Node::size());
    return false; // Didn't initialize the file
}

FILE *FileAllocator::getFile() const
{
    if (!f)
        fprintf(stderr, "FileAllocator::getFile called while closed\n");
    return f;
}

void FileAllocator::detach()
{
    f = 0;
#ifdef DEBUG_FA
    printf("FileAllocator::detach()\n");
#endif
}

FileOffset FileAllocator::allocate(FileOffset num_bytes)
{
    if (num_bytes < minimum_size)
        num_bytes = minimum_size;
    Node node;
    FileOffset node_offset;
    // Check free list for a valid entry
    node_offset = free_head.getNext();
    while (node_offset)
    {
        node.read(f, node_offset);
        if (node.getBytes() >= num_bytes)
        {   // Found an appropriate block on the free list.
            if (node.getBytes() < num_bytes + Node::size() + minimum_size)
            {   // not worth splitting, return as is:
                remove_node(free_head, node);
                insert_node(allocated_head, node_offset, node);
                return node_offset + Node::size();
            }
            else
            {
                // Split requested size off, correct free list
                node.addBytes(- Node::size() - num_bytes);
                free_head.addBytes(- Node::size() - num_bytes);
                node.write();
                free_head.write();
                // Create, insert & return new node
                FileOffset new_offset =
                    node_offset + Node::size() + node.getBytes();
                Node new_node;
                new_node.setBytes(num_bytes);
                insert_node(allocated_head, new_offset, new_node);
                return new_offset + Node::size();
            }
        }
        node_offset = node.getNext();
    }
    // Need to append new block, i.e. grow file
    // Is that possible?
    FileOffset grow = Node::size()+num_bytes;
    if (file_size + grow >= MaxFileSize)
        throw GenericException(__FILE__, __LINE__,
                               "File size exceeds %.2f GB",
                               MaxFileSize/1024.0/1024.0/1024.0);
    
    if (fseek(f, file_size + grow - 1, SEEK_SET) != 0  ||
        fwrite("", 1, 1, f) != 1)
        throw GenericException(__FILE__, __LINE__, "Write Error");
    // write new node
    node.setBytes(num_bytes);
    node.setPrev(allocated_head.getPrev());
    node.setNext(0);
    node.write(f, file_size);
    // maybe update what used to be the last block
    if (allocated_head.getPrev())
    {
        node.read(f, allocated_head.getPrev());
        node.setNext(file_size);
        node.write();
    }
    // update head of list
    allocated_head.addBytes(num_bytes);
    if (allocated_head.getNext() == 0)
        allocated_head.setNext(file_size);
    allocated_head.setPrev(file_size);
    allocated_head.write();
    // Update overall file size, return offset of new data block
    FileOffset data_offset = file_size + Node::size();
    file_size = data_offset + num_bytes;
    if (file_size_increment > 0)
    {
        // Grow to desired multiple of file_size_increment
        // after the fact, but this way it can use the
        // allocate() & free() calls as is:
        if (file_size % file_size_increment)
        {
            size_t bytes_to_next_increment =
                ((file_size / file_size_increment)+1)*file_size_increment
                - file_size;
            // We'll allocate the block and free it
            // so that the file grows as intended and
            // the unused mem is on the free list
            this->free(this->allocate(bytes_to_next_increment - Node::size()));
        }
    }
    return data_offset;
}

void FileAllocator::free(FileOffset block_offset)
{
    // Node should precede the memory block,
    // so it cannot start before reserved_space + heads + 1st buffer node,
    // and it cannot start beyond the known file size.
    if (block_offset < reserved_space + 3*Node::size()  ||
        block_offset >= file_size)
        throw GenericException(__FILE__, __LINE__,
                               "FileAllocator::free, impossible offset %ld\n",
                               (unsigned long)block_offset);
    Node node;
    FileOffset node_offset = block_offset - Node::size();
    node.read(f, node_offset);
    if (node_offset + node.getBytes() > file_size)
        throw GenericException(__FILE__, __LINE__,
                               "FileAllocator::free called with broken node %ld\n",
                               (unsigned long)block_offset);
    // remove node at 'offset' from list of allocated blocks,
    // add node to list of free blocks
    remove_node(allocated_head, node);
    insert_node(free_head, node_offset, node);
    // Check if we can merge with the preceeding block
    Node pred;
    FileOffset pred_offset = node.getPrev();
    if (pred_offset)
    {
        pred.read(f, pred_offset);
        if (pred_offset + Node::size() + pred.getBytes() == node_offset)
        {   // Combine this free'ed node with prev. block
            pred.addBytes(Node::size() + node.getBytes());
            // skip the current node
            pred.setNext(node.getNext());
            pred.write();
            if (pred.getNext())
            {   // correct back pointer
                node.read(f, pred.getNext());
                node.setPrev(pred_offset);
                node.write();
            }
            else
            {   // we changed the tail of the free list
                free_head.setPrev(pred_offset);
            }
            free_head.addBytes(Node::size());
            free_head.write();
            
            // 'pred' is now the 'current' node
            node_offset = pred_offset;
            node = pred;
        }
    }

    // Check if we can merge with the succeeding block
    Node succ;
    FileOffset succ_offset = node.getNext();
    if (succ_offset)
    {
        if (node_offset + Node::size() + node.getBytes() == succ_offset)
        {   // Combine this free'ed node with following block
            succ.read(f, succ_offset);
            node.addBytes(Node::size() + succ.getBytes());
            // skip the next node
            node.setNext(succ.getNext());
            node.write(f, node_offset);
            if (node.getNext())
            {   // correct back pointer
                Node after;
                after.read(f, node.getNext());
                after.setPrev(node_offset);
                after.write();
            }
            else
            {   // we changed the tail of the free list
                free_head.setPrev(node_offset);
            }
            free_head.addBytes(Node::size());
            free_head.write();
        }
    }
}

bool FileAllocator::dump(int level, FILE *out)
{
    bool ok = true;
    try
    {
        Node allocated_node, free_node;
        FileOffset allocated_offset, free_offset;
        FileOffset allocated_prev = 0, free_prev = 0;
        FileOffset allocated_mem = 0, allocated_blocks = 0;
        FileOffset free_mem = 0, free_blocks = 0;
        FileOffset next_offset = 0;
        if (level > 0)
            fprintf(out, "bytes in file: %ld. Reserved/Allocated/Free: %ld/%ld/%ld\n",
                   (long)file_size,
                   (long)reserved_space,
                   (long)allocated_head.getBytes(),
                   (long)free_head.getBytes());
        allocated_offset = allocated_head.getNext();
        free_offset = free_head.getNext();
        while(allocated_offset || free_offset)
        {
            // Show the node that's next in the file
            if (allocated_offset &&
                (free_offset == 0 || free_offset > allocated_offset))
            {
                allocated_node.read(f, allocated_offset);
                ++allocated_blocks;
                allocated_mem += allocated_node.getBytes();
                if (level > 0)
                    fprintf(out, "Allocated Block @ %10ld: %10ld bytes\n",
                           (long)allocated_offset, (long)allocated_node.getBytes());
                if (next_offset && next_offset != allocated_offset)
                {
                    fprintf(out, "! There is a gap, %ld unmaintained bytes "
                           "before this block!\n",
                           (long)allocated_offset - (long)next_offset);
                    ok = false;
                }
                if (allocated_prev != allocated_node.getPrev())
                {
                    fprintf(out, "! Block's ''prev'' pointer is broken!\n");
                    ok = false;
                }
                next_offset = allocated_offset + Node::size() + allocated_node.getBytes();
                allocated_prev = allocated_offset;
                allocated_offset = allocated_node.getNext();
            }
            else if (free_offset)
            {
                free_node.read(f, free_offset);
                ++free_blocks;
                free_mem += free_node.getBytes();
                if (level > 0)
                    fprintf(out, "Free      Block @ %10ld: %10ld bytes\n",
                           (long)free_offset, (long)free_node.getBytes());
                if (next_offset && next_offset != free_offset)
                {
                    fprintf(out, "! There is a gap, %ld unmaintained bytes "
                           "before this block!\n", (long)free_offset - (long)next_offset);
                    ok = false;
                }
                if (free_prev != free_node.getPrev())
                {
                    fprintf(out, "! Block's ''prev'' pointer is broken!\n");
                    ok = false;
                }
                next_offset = free_offset + Node::size() + free_node.getBytes();
                free_prev = free_offset;
                free_offset = free_node.getNext();
            }
        }
        if (file_size !=
            (reserved_space +
             2*Node::size() + // allocated/free list headers
             allocated_blocks*Node::size() +
             allocated_head.getBytes() +
             free_blocks*Node::size() +
             free_head.getBytes()))
        {
            fprintf(out, "! The total file size does not compute!\n");
            ok = false;
        }
        if (allocated_mem != allocated_head.getBytes())
        {
            fprintf(out, "! The amount of allocated space does not compute!\n");
            ok = false;
        }
        if (free_mem != free_head.getBytes())
        {
            fprintf(out, "! The amount of allocated space does not compute!\n");
            ok = false;
        }
    }
    catch (GenericException &e)
    {
        fprintf(out, "Error: %s", e.what());
        return false;
    }
    return ok;
}

void FileAllocator::remove_node(Node &head, const Node &node)
{
    Node tmp;
    FileOffset tmp_offset;

    if (head.getNext() == node.getOffset())
    {   // first node; make head skip it
        head.addBytes(- node.getBytes());
        head.setNext(node.getNext());
        if (head.getNext() == 0) // list now empty?
            head.setPrev(0);
        else
        {   // correct 'prev' ptr of what's now the first node
            tmp.read(f, head.getNext());
            tmp.setPrev(0);
            tmp.write();
        }
        head.write();
    }
    else
    {   // locate a node that's not the first
        tmp_offset = head.getNext();
        tmp.read(f, tmp_offset);
        while (tmp.getNext() != node.getOffset())
        {
            tmp_offset = tmp.getNext();
            if (!tmp_offset)
                throw GenericException(__FILE__, __LINE__,
                                       "node not found");
            tmp.read(f, tmp_offset);
        }
        // tmp/tmp_offset == node before the one to be removed
        tmp.setNext(node.getNext());
        tmp.write();
        if (node.getNext())
        {   // adjust prev pointer of node.next
            tmp.read(f, node.getNext());
            tmp.setPrev(tmp_offset);
            tmp.write();
        }
        head.addBytes(- node.getBytes());
        if (head.getPrev() == node.getOffset())
            head.setPrev(node.getPrev());
        head.write();
    }
}

void FileAllocator::insert_node(Node &head, FileOffset node_offset, Node &node)
{
    // add node to list of free blocks
    if (head.getNext() == 0)
    {   // first in free list
        head.setNext(node_offset);
        head.setPrev(node_offset);
        node.setNext(0);
        node.setPrev(0);
        head.addBytes(node.getBytes());
        node.write(f, node_offset);
        head.write();
        return;
    }
    // insert into free list, sorted by position (for nicer dump() printout)
    if (node_offset < head.getNext())
    {   // new first item
        node.setPrev(0);
        node.setNext(head.getNext());
        node.write(f, node_offset);
        Node tmp;
        tmp.read(f, node.getNext());
        tmp.setPrev(node_offset);
        tmp.write();
        head.setNext(node_offset);
        head.addBytes(node.getBytes());
        head.write();
        return;
    }
    // find proper location in free list
    Node pred;
    FileOffset pred_offset = head.getNext();
    pred.read(f, pred_offset);
    while (pred.getNext() && pred.getNext() < node_offset)
    {
        pred_offset = pred.getNext();
        pred.read(f, pred_offset);
    }
    // pred.next == 0  or >= node_offset: insert here!
    node.setNext(pred.getNext());
    node.setPrev(pred_offset);
    node.write(f, node_offset);
    if (pred.getNext())
    {
        Node after;
        after.read(f, pred.getNext());
        after.setPrev(node_offset);
        after.write();
    }
    pred.setNext(node_offset);
    pred.write(f, pred_offset);
    if (head.getPrev() == pred_offset)
        head.setPrev(node_offset);
    head.addBytes(node.getBytes());
    head.write();
}

