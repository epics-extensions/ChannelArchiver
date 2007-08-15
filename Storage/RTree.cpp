// Tools
#include <MsgLogger.h>
#include <BinIO.h>
#include <AutoPtr.h>
// Index
#include "RTreePrivate.h"

// -- Datablock
RTree::Datablock::Datablock()
   : data_offset(0)
{}

RTree::Datablock::~Datablock()
{}

// -- Record
Record::Record()
    : child_or_ID(0)
{}

Record::Record(const Interval &interval, FileOffset child_or_ID)
    : interval(interval), child_or_ID(child_or_ID)
{}

Record::Record(const Record &other)
    : interval(other.interval), child_or_ID(other.child_or_ID)
{}

Record &Record::operator = (const Record &other)
{
	interval = other.interval;
	child_or_ID = other.child_or_ID;
	return *this;
}

void Record::clear()
{
	interval.clear();
    child_or_ID = 0;
}

void Record::write(FILE *f) const
{
    writeEpicsTime(f, interval.getStart());
    writeEpicsTime(f, interval.getEnd());
    if (!writeLong(f, child_or_ID))
        throw GenericException(__FILE__, __LINE__, "write error");
}

void Record::read(FILE *f)
{
    interval.setStart(readEpicsTime(f));
    interval.setEnd(readEpicsTime(f));
    if (!readLong(f, &child_or_ID))
        throw GenericException(__FILE__, __LINE__, "read error");
}

void Record::writeEpicsTime(FILE *f, const epicsTime &t) const
{
    epicsTimeStamp stamp = t;
    if (!(writeLong(f, stamp.secPastEpoch) && writeLong(f, stamp.nsec)))
        throw GenericException(__FILE__, __LINE__, "write error");
}

epicsTime Record::readEpicsTime(FILE *f) const
{
	epicsTime result;
    epicsTimeStamp stamp;
    if (! (readLong(f, (uint32_t *)&stamp.secPastEpoch) &&
           readLong(f, (uint32_t *)&stamp.nsec)))
        throw GenericException(__FILE__, __LINE__, "read error");
    if (stamp.nsec < 1000000000L)
        result = stamp;
    else
    {	// Nanosecond field runs into full seconds.
    	// This should never happen but obviously does,
	    // and R3.14.8 will abort, so we have to catch it:
	    size_t nsec = (size_t) stamp.nsec;
	    stamp.nsec = 0;
	    result = stamp;
	    stdString txt;
	    epicsTime2string(result, txt);
#if 0
	    // In principle, one should ignore those values,
	    // throw an exception, so we get a good message
	    // which includes the context:
	    throw GenericException(__FILE__, __LINE__,
#else
	    // However, it's often better to patch the nsecs
	    // and show the data. Unfortunalely, we don't know
	    // the channel name etc. at this point, so we can't
	    // provide a good error message.
	    LOG_MSG(
#endif
  	            "RTree: Invalid time stamp with %zu nsecs: %s\n",
	            nsec, txt.c_str());
    }
    return result;
}

// -- Node
Node::Node(FileAllocator &fa, int M,  bool leaf)
    : fa(fa), offset(0), M(M), is_leaf(leaf), parent(0)
{
    record = new Record[M];
    LOG_ASSERT(record != 0);
}

Node::Node(const Node &rhs)
    : fa(rhs.fa), offset(rhs.offset), M(rhs.M), is_leaf(rhs.is_leaf),
      parent(rhs.parent)
{
    record = new Record[M];
    LOG_ASSERT(record != 0);
    for (int i=0; i<M; ++i)
        record[i] = rhs.record[i];
}

Node::~Node()
{
    delete [] record;
    record = 0;
}

Node &Node::operator = (const Node &rhs)
{
    LOG_ASSERT(& fa == & rhs.fa);
    offset = rhs.offset;
    LOG_ASSERT(M == rhs.M);
    is_leaf = rhs.is_leaf;
    parent = rhs.parent;
    for (int i=0; i<M; ++i)
        record[i] = rhs.record[i];
    return *this;
}

int Node::findRecordByID(FileOffset ID) const
{
    for (int i=0; i<M; ++i)
        if (record[i].getChildOrID() == ID)
            return i;
    throw GenericException(__FILE__, __LINE__,
                           "Cannot find node 0x%08lX in parent node 0x%08lX\n",
                           (unsigned long) ID, (unsigned long) offset);
}

int Node::findLowestRecord() const
{
    for (int i=0; i<M; ++i)
        if (record[i].getChildOrID())
            return i;
    return -1;
}

int Node::findHighestRecord() const
{
    for (int i=M-1; i>=0; --i)
        if (record[i].getChildOrID())
            return i;
    return -1;
}

void Node::write(FileOffset offset)
{
    FILE *f = fa.getFile();
    if (fseek(f, offset, SEEK_SET))
        throw GenericException(__FILE__, __LINE__, "fseek(0x%08lX) failed",
                               (unsigned long) offset);
    if (! (writeByte(f, is_leaf) &&
           writeLong(f, parent)))
        throw GenericException(__FILE__, __LINE__, "write failed");
    for (int i=0; i<M; ++i)
        record[i].write(f);
    this->offset = offset;
}

void Node::read(FileOffset offset)
{
    LOG_ASSERT(offset != 0);
    FILE *f = fa.getFile();
    if (fseek(f, offset, SEEK_SET))
        throw GenericException(__FILE__, __LINE__, "fseek(0x%08lX) failed",
                               (unsigned long) offset);
    uint8_t c;
    if (! (readByte(f, &c) && readLong(f, &parent)))
        throw GenericException(__FILE__, __LINE__, "read failed");
    is_leaf = c > 0;
    for (int i=0; i<M; ++i)
        record[i].read(f);
    this->offset = offset;
    // node_cache.add(node);
}

Interval Node::getInterval() const
{
    Interval range;
    for (int i=0; i<M; ++i)
    {
        if (record[i].getChildOrID() == 0)
            continue;
        // In case we have an empty node,
        // we never get here, and the
        // resulting range will be invalid.
        // But if there are valid entries,
        // we check their time range.
        if (! record[i].getInterval().isValid())
        {
            stdString txt1, txt2;
            epicsTime2string(record[i].getInterval().getStart(), txt1);
            epicsTime2string(record[i].getInterval().getEnd(),   txt2);
            char buf[500];
            snprintf(buf, sizeof(buf), "Node Interval %d is invalid: %s - %s",
                     i, txt1.c_str(), txt2.c_str());
            throw GenericException(__FILE__, __LINE__, buf);
        }
        // We also assume that the first record is
        // used in a non-empty node,
        // so its interval initializes the range.
        if (i == 0)
            range = record[0].getInterval();
        else
            range.add(record[i].getInterval());
    }
    return range;
}

void Node::chooseLeaf(const Interval &range)
{
    if (is_leaf)
        return; // done.
    // If a subtree already contains data for the time range
    // or there's an overlap, use that one. Otherwise follow
    // the RTree paper:
    // Find entry which needs the least enlargement.
    double min_enlarge=0;
    int min_i=-1;
    for (int i=0; i<M; ++i)
    {
        if (!record[i].getChildOrID())
            continue;
        if (record[i].getInterval().overlaps(range))
        {
            read(record[i].getChildOrID());
            chooseLeaf(range);
            return;
        }
        // t0 .. t1 = union(start...end, getRecord(i).start...end)
        Interval new_range(range);
        new_range.add(record[i].getInterval());
        double enlarge = new_range.width()
                          - record[i].getInterval().width();
        // Pick rightmost record of those with min. enlargement
        if (i==0  ||  enlarge <= min_enlarge)
        {
            min_enlarge = enlarge;
            min_i = i;
        }
    }
    read(record[min_i].getChildOrID());
    chooseLeaf(range);
}

void Node::insertRecord(int idx, const Record &new_record,
                        Node &overflow,
                        bool &caused_overflow,
                        bool &rec_in_overflow)
{
    rec_in_overflow = idx >= M;
    if (rec_in_overflow)
        overflow.record[0] = new_record;
    else
    {   // Move last rec. into overflow,
        overflow.record[0] = record[M-1];
        for (int j=M-1; j>idx; --j) // shift all recs right from idx on.   
            record[j] = record[j-1];
        record[idx] = new_record;
    }
    
    caused_overflow = overflow.record[0].getChildOrID() != 0;
    if (caused_overflow)
    {
        // Node is full, one record in overflow.
        // Split node?
#if 1
        // This results in a 50/50 split between node and overflow,
        // as suggested for general RTree use.
        // It results in the best looking tree,
        // i.e. smallest number of Nodes and empty records,
        // no matter if the data blocks are added in time order
        // or random order.
        int cut = M/2+1;
#else
        // On the other hand, the engine is mostly adding to the end,
        // so having more space available in the overflow record,
        // were 'new' data is likely to get added, seems better.
        // For data that's added in time order, this might be a little bit
        // faster, since fewer rebalance ops are needed.
        // But on the other hand, the tree can degrade (many underused nodes),
        // so we're not using this mode.
        int cut = M;
#endif
        // There are M records in node and 1 in overflow.
        // Shift from node.record[cut] on into overflow.
        overflow.record[M-cut] = overflow.record[0];
        for (int j=cut; j<M; ++j)
        {   // copy from node and clear the copied entries in node
            LOG_ASSERT(j-cut >= 0);
            LOG_ASSERT(j-cut < M);
            overflow.record[j-cut] = record[j];
            record[j].clear();
        }
        rec_in_overflow = (idx >= cut);
        overflow.write(fa.allocate(overflow.size()));
    }
    write();
}

bool Node::addToRecord(int i,
                       FileOffset data_offset, const stdString &data_filename)
{
    PrivateDatablock block(*this, i);
    bool have_first_block = block.readFirst();
    LOG_ASSERT(have_first_block);
    // Check if this data block is already listed.
    do
    {
        if (block.getDataOffset() == data_offset &&
            block.getDataFilename() == data_filename)
            return false; // found an existing datablock
    }
    while (block.getNextChainedBlock());
    // Block's now the last data block in the chain.

    // Create new block, add to the end of the chain:
    // An old block might contain the last sample & "off" event,
    // while the new block can have that same last sample plus new
    // samples.
    // Adding to the end of the chain means: We will still see
    // the "off" event upon retrieval!
    PrivateDatablock new_block(*this, i, data_filename, data_offset);
    FileOffset new_block_offset = fa.allocate(new_block.size());
    // Add new block to the end of the chain
    new_block.write(new_block_offset);
    block.setNext(new_block_offset);
    block.write();
    return true; // added a new datablock
}

bool Node::removeFromRecord(int i,
                        FileOffset data_offset, const stdString &data_filename)
{
    PrivateDatablock block(*this, i);
    FileOffset prev_block_offset = 0;
    bool go = block.readFirst();
    while (go)
    {
        if (block.getDataOffset() == data_offset &&
            block.getDataFilename() == data_filename)
        {
            const FileOffset deleted_block_offset = block.getOffset();
            const FileOffset following_block_offset = block.getNext();
            if (prev_block_offset)
            {   // unlink from the previous data block
                block.read(prev_block_offset);
                block.setNext(following_block_offset);
                block.write();
            }
            else
            {   // unlink from root pointer in node
                record[i].setChildOrID(following_block_offset);
                write();
            }
            // TODO Don't free data block, because
            //      that just takes too long in the end
            //      when the file gets fragmented?
            fa.free(deleted_block_offset);
            return true; // found and removed
        }
        prev_block_offset = block.getOffset();
        go = block.getNextChainedBlock();
    }
    return false; // not found, not removed
}

void Node::selfTest(unsigned long &nodes, unsigned long &records,
                    FileOffset parent_offset, epicsTime start, epicsTime end)
{
    stdString txt1, txt2, txt3, txt4;
    int i;
    ++nodes;
    Interval range = getInterval();
    if (parent_offset != parent)
        throw GenericException(__FILE__, __LINE__,
                               "Node @ 0x%lX, %s ... %s: "
                               "parent = 0x%lX != 0x%lX\n",
                               (unsigned long)offset,
                               epicsTimeTxt(range.getStart(), txt1),
                               epicsTimeTxt(range.getEnd(), txt2),
                               (unsigned long)parent,
                               (unsigned long)parent_offset);
    if (parent_offset && (range.getStart() != start || range.getEnd() != end))
                throw GenericException(__FILE__, __LINE__,
                                       "Node @ 0x%lX: Node Interval %s ... %s\n"
                                       "              Parent        %s ... %s\n",
                                       (unsigned long)offset,
                                       epicsTimeTxt(range.getStart(), txt1),
                                       epicsTimeTxt(range.getEnd(), txt2),
                                       epicsTimeTxt(start, txt3),
                                       epicsTimeTxt(end, txt4));
    if (record[0].getChildOrID())
        ++records;
    for (i=1; i<M; ++i)
    {
        if (record[i].getChildOrID())
        {
            ++records;
            if (record[i-1].getInterval().getEnd() >
                record[i].getInterval().getStart())
                throw GenericException(__FILE__, __LINE__,
                                       "Node @ 0x%lX: Records not in order\n",
                                       (unsigned long)offset);
            if (!record[i-1].getChildOrID()) 
                throw GenericException(__FILE__, __LINE__,
                                       "Node @ 0x%lX: "
                                       "Empty record before filled one\n",
                                       (unsigned long)offset);
        }
    }
    if (is_leaf)
        return; // OK until end of this branch
    for (i=0; i<M; ++i)
    {
        if (record[i].getChildOrID())
        {
            Node child(fa, M);
            child.read(record[i].getChildOrID());
            child.selfTest(nodes, records, offset,
                           record[i].getInterval().getStart(),
                           record[i].getInterval().getEnd());
        }
    }       
}

void Node::makeDot(FILE *dot)
{
    stdString txt1, txt2;
    int i;
    
    fprintf(dot, "\tnode%ld [ label=\"", (unsigned long)offset);
    for (i=0; i<M; ++i)
    {
        if (i>0)
            fprintf(dot, "|");
        epicsTime2string(record[i].getInterval().getStart(), txt1);
        epicsTime2string(record[i].getInterval().getEnd(), txt2);
        if (txt1.length() > 10)
            fprintf(dot, "<f%d> %s \\r-%s \\r", i, txt1.c_str(), txt2.c_str());
        else
            fprintf(dot, "<f%d>%s-%s", i, txt1.c_str(), txt2.c_str());
    }    
    fprintf(dot, "\"];\n");
    if (is_leaf)
    {   // List data blocks
        for (i=0; i<M; ++i)
        {
            FileOffset data_offset = record[i].getChildOrID();
            if (data_offset)
            {
                fprintf(dot, "\tnode%ld:f%d->id%ld;\n",
                        (unsigned long)offset, i,
                        (unsigned long)data_offset);
                PrivateDatablock datablock(*this, i);
                datablock.readFirst();
                datablock.makeDot(dot);
            }
        }
    }
    else
    {   // dump subtrees
        for (i=0; i<M; ++i)
        {            
            if (record[i].getChildOrID())
                fprintf(dot, "\tnode%lu:f%d->node%ld:f0;\n",
                        (unsigned long)offset, i,
                        (long)record[i].getChildOrID());
        }
        for (i=0; i<M; ++i)
        {
            if (record[i].getChildOrID())
            {
                Node child(fa, M);
                child.read(record[i].getChildOrID());
                child.makeDot(dot);
            }
        }
    }
}

void Node::dump() const
{
    printf("Node @ 0x%lX (parent 0x%lX, leaf: %s)\n",
           (unsigned long)offset,
           (unsigned long)parent,
           (is_leaf ? "yes" : "no"));
    printf("|");
    for (int i=0; i<M; ++i)
    {
        printf(" %s (0x%lX) |",
               record[i].getInterval().toString().c_str(),
               (unsigned long) record[i].getChildOrID());
    }    
    printf("\n");
}

// Comparison routine for AVLTree (node_cache)
int sort_compare(const Node &a, const Node &b)
{    return b.getOffset() - a.getOffset(); }

// -- PrivateDatablock
PrivateDatablock::PrivateDatablock(const Node &node, int record_index)
    : offset(0), node(node), record_index(record_index),
      next_ID(0) 
{
    LOG_ASSERT(node.isLeaf());
}

PrivateDatablock::PrivateDatablock(const Node &node, int record_index,
                                   const stdString &data_filename,
                                   FileOffset data_offset)
     : offset(0), node(node), record_index(record_index),
       next_ID(0)
{
    LOG_ASSERT(node.isLeaf());
    this->data_filename = data_filename;
    this->data_offset = data_offset;
}                                   

size_t PrivateDatablock::size() const
{   //  next_ID, data offset, name size, name (w/o '\0')
    return 4 + 4 + 2 + data_filename.length();
}

bool PrivateDatablock::readFirst()
{
    FileOffset first = node.getRecord(record_index).getChildOrID();
    if (! first)
        return false;
    read(first);
    return true;
}

void PrivateDatablock::read(FileOffset offset)
{
    LOG_ASSERT(offset > 0);
    FILE *f = node.getFile();
    if (fseek(f, offset, SEEK_SET))
        throw GenericException(__FILE__, __LINE__, "fseek(0x%08lX) failed",
                               (unsigned long) offset);
    unsigned short len;
    char buf[300];
    if (!(readLong(f, &next_ID) &&
          readLong(f, &data_offset) &&
          readShort(f, &len)))
        throw GenericException(__FILE__, __LINE__, "read failed @ 0x%lX",
                               (unsigned long)offset);
    if (len >= sizeof(buf)-1)
        throw GenericException(__FILE__, __LINE__,
                               "Datablock filename exceeds buffer (%d)",len);
    if (fread(buf, len, 1, f) != 1)
        throw GenericException(__FILE__, __LINE__,
                               "Datablock filename read error @ 0x%lX",
                               (unsigned long) offset);
    buf[len] = '\0';
    data_filename.assign(buf, len);
    if (data_filename.length() != len)
        throw GenericException(__FILE__, __LINE__,
                               "Datablock filename length error @ 0x%lX\n",
                               (unsigned long)offset);
    this->offset = offset;
}
    
void PrivateDatablock::write(FileOffset offset)
{
    this->offset = offset;
    FILE *f = node.getFile();
    if (fseek(f, offset, SEEK_SET))
        throw GenericException(__FILE__, __LINE__, "fseek(0x%08lX) failed",
                               (unsigned long) offset);
    if (!(writeLong(f, next_ID) && writeLong(f, data_offset) &&
          writeShort(f, data_filename.length()) &&
          fwrite(data_filename.c_str(), data_filename.length(), 1, f) == 1))
        throw GenericException(__FILE__, __LINE__, "write failed @ 0x%08lX",
                               (unsigned long) offset);
}

bool PrivateDatablock::isValid() const
{
    return offset > 0;     
}

const Interval &PrivateDatablock::getInterval() const
{
    LOG_ASSERT(isValid());
    return node.getRecord(record_index).getInterval();
}

bool PrivateDatablock::getNextChainedBlock()
{
    LOG_ASSERT(isValid());
    if (next_ID == 0)
        return false; // stay where we are, leave valid, but return false
    // Read next chained block
    read(next_ID);
    return true;
}

bool PrivateDatablock::prev_next(PrevNextDir dir)
{
    LOG_ASSERT(isValid());
    LOG_ASSERT(dir == -1  ||  dir == 1);
    LOG_ASSERT(node.isLeaf());
    LOG_ASSERT(record_index >= 0  &&  record_index < node.getM());
    record_index += dir;
    // Another rec. in curr.node?
    if (record_index >= 0 &&
        record_index < node.getM() &&
        node.getRecord(record_index).getChildOrID())
        return true;
    Node parent(node);
    // Go up to parent nodes...
    while (true)
    {
        FileOffset offset = node.getParent();
        if (! offset) // at root; can't go up further
            return false;
        parent.read(offset);
        record_index = parent.findRecordByID(node.getOffset());
        record_index += dir;
        if (record_index >= 0 &&
            record_index < node.getM() &&
            parent.getRecord(record_index).getChildOrID())
            break;
        // else: go up another level
        node = parent;
    }
    FileOffset offset = parent.getRecord(record_index).getChildOrID();
    // Decend using rightmost (prev) or leftmost (next) children
    while (offset)
    {
        node.read(offset);
        if (dir < 0)
        {
            record_index = node.findHighestRecord();
            if (record_index < 0)
                throw GenericException(__FILE__, __LINE__,
                                       "RTree::prev: empty?\n");
        }
        else
            record_index = 0;
        if (node.isLeaf())
            return node.getRecord(record_index).getChildOrID() != 0;
        offset = node.getRecord(record_index).getChildOrID();
    }
    return false;
}

bool PrivateDatablock::getPrevDatablock()
{
    if (!prev_next(PREV))
    {
        offset = 0; // invalidate
        return false;
    }
    LOG_ASSERT(isValid());
    read(node.getRecord(record_index).getChildOrID());
    return true;
}

bool PrivateDatablock::getNextDatablock()
{
    if (!prev_next(NEXT))
    {
        offset = 0; // invalidate
        return false;
    }
    LOG_ASSERT(isValid());
    read(node.getRecord(record_index).getChildOrID());
    return true;
}

void PrivateDatablock::makeDot(FILE *dot)
{
    LOG_ASSERT(isValid());
    do
    {
        fprintf(dot, "\tid%lu "
                "[ label=\"'%s' \\r@ 0x%lX \\r\",style=filled ];\n",
                (unsigned long)offset,
                data_filename.c_str(),
                (unsigned long)data_offset);
        if (next_ID)
        {
            fprintf(dot, "\tid%lu -> id%ld;\n",
                    (unsigned long)offset,
                    (long)next_ID);
        }
    }
    while (getNextChainedBlock());
}

// -- RTree
RTree::RTree(FileAllocator &fa, FileOffset anchor)
        :  fa(fa), anchor(anchor), root_offset(0), M(-1),
           cache_misses(0), cache_hits(0)
{
}

void RTree::init(int M)
{
    if (M <= 2)
        throw GenericException(__FILE__, __LINE__,
                               "RTree::init(%d): M should be > 2", M);
    this->M = M;
    Node node(fa, M);
    // Create initial Root Node = Empty Leaf
    root_offset = fa.allocate(node.size());
    node.write(root_offset);
    // Update Root pointer
    if (! (fseek(fa.getFile(), anchor, SEEK_SET)==0 &&
           writeLong(fa.getFile(), root_offset)==true &&
           writeLong(fa.getFile(), M) == true))
        throw GenericException(__FILE__, __LINE__,
                               "write error @ 0x%08lX",
                               (unsigned long) anchor);
}

void RTree::reattach()
{
    uint32_t RTreeM;
    if (!(fseek(fa.getFile(), anchor, SEEK_SET)==0 &&
          readLong(fa.getFile(), &root_offset)==true &&
          readLong(fa.getFile(), &RTreeM) == true))
        throw GenericException(__FILE__, __LINE__,
                               "read error @ 0x%08lX",
                               (unsigned long) anchor);
    if (RTreeM < 1  ||  RTreeM > 100)
        throw GenericException(__FILE__, __LINE__,
                               "RTree::reattach: Suspicious RTree M %ld\n",
                               (long)RTreeM);
    M = RTreeM;
}

Interval RTree::getInterval()
{
    Node node(fa, M);
    node.read(root_offset);
    return node.getInterval();
}

bool RTree::getFirstRecord(class Node &node, int &record_index) const
{   // Descent using leftmost children
    FileOffset offset = root_offset;
    while (offset)
    {
        node.read(offset);
        record_index = node.findLowestRecord();
        if (record_index < 0)
            return false;  // nothing
        if (node.isLeaf()) // Done, or continue to go down?
            return true;   // Found it.
        // Point offset to child, one level down in tree.
        offset = node.getRecord(record_index).getChildOrID();
    }    
    return false; // nothing
}

bool RTree::getLastRecord(class Node &node, int &record_index) const
{   // Descent using rightmost children
    FileOffset offset = root_offset;
    while (offset)
    {
        node.read(offset);
        record_index = node.findHighestRecord();
        if (record_index < 0)
            return false;  // nothing
        if (node.isLeaf()) // Done or continue to go down?
            return true;   // Found it.
        // Point offset to child, one level down in tree.
        offset = node.getRecord(record_index).getChildOrID();
    }    
    return 0; // nothing
}

RTree::Datablock *RTree::getFirstDatablock() const
{
    Node node(fa, M);
    int record_index;
    if (getFirstRecord(node, record_index))
    {
        PrivateDatablock *block = new PrivateDatablock(node, record_index);
        block->readFirst();
        return block;
    }
    return 0; // nothing
}


RTree::Datablock *RTree::getLastDatablock() const
{  
    Node node(fa, M);
    int record_index;
    if (getLastRecord(node, record_index))
    {
        PrivateDatablock *block = new PrivateDatablock(node, record_index);
        block->readFirst();
        return block;
    }
    return 0; // nothing
}

RTree::Datablock * RTree::search(const epicsTime &start) const
{
    Node node(fa, M);
    node.read(root_offset);
    // Is request before start of the whole (sub-)tree?
    int record_index;
    if (start < node.getRecord(0).getInterval().getStart())
    {
        if (!getFirstRecord(node, record_index))
            return 0;
        PrivateDatablock *block = new PrivateDatablock(node, record_index);
        block->readFirst();
        return block;
    }

    bool go;
    do
    {
        for (go=false, record_index=M-1;  record_index>=0;  --record_index)
        {   // Find right-most record with data at-or-before 'start'
            if (node.getRecord(record_index).getChildOrID() == 0)
                continue; // nothing
            if (node.getRecord(record_index).getInterval().getStart() <= start)
            {
                if (node.isLeaf())   // Found!
                {
                    PrivateDatablock *block = new PrivateDatablock(node, record_index);
                    block->readFirst();
                    return block;
                }
                else
                {   // Search subtree of this record
                    node.read(node.getRecord(record_index).getChildOrID());
                    go = true;
                    break;
                }
            }
        }
    }
    while (go);
    return 0;
}

bool RTree::updateLastDatablock(const Interval &range,
                                FileOffset data_offset,
                                stdString data_filename)
{
    Node node(fa, M);
    int i;
    
    if (getLastRecord(node, i))
    {   // Scenario that we hope for:
        // Engine added values to an existing block, so only
        // the end time changed.
        // --> update end time, done.
        const Interval rec_range = node.getRecord(i).getInterval();
        // Last block's range: |----------|
        // New/update range:   |------------------| ??
        if (rec_range.getStart() == range.getStart() &&
            rec_range.getEnd()   <= range.getEnd())
        {  
            PrivateDatablock block(node, i);
            block.readFirst();
            // Is this the one and only block under the last node,
            // does it point to offset/filename,
            // and the start time matches,
            // just the end time differs? 
            if (block.isLast() &&
                block.getDataOffset() == data_offset &&
                block.getDataFilename() == data_filename)
            {   // Update end time
                node.getRecord(i).setEnd(range.getEnd());
                node.write();
                adjust_tree(node, 0);
                return false; // just update; no new record inserted
            }
        }
    }
    
    // Fallback: Last-block-update wahoo didn't work, probably because
    // there has already been another data block been added after that.
    // In case this is an existing data block,
    // just adding it again can lead to adding pieces of it
    // to many records. First removing any references, then adding from scratch
    // prevents that.
    // But in the real world, that also takes a lot of time,
    // while the few added data blocks don't hurt:
    // removeDatablock(range, data_offset, data_filename);
    return insertDatablock(range, data_offset, data_filename);
}

void RTree::makeDot(const char *filename)
{
    FILE *dot = fopen(filename, "wt");
    if (!dot)
        return;

    fprintf(dot, "# Example for dotting & viewing:\n");
    fprintf(dot, "# dot -Tpng -o x.png %s && eog x.png &\n", filename);
    fprintf(dot, "\n");
    fprintf(dot, "digraph RTree\n");
    fprintf(dot, "{\n");
    fprintf(dot, "\tnode [shape = record, height=.1];\n");
    Node node(fa, M);
    node.read(root_offset);
    node.makeDot(dot);
    fprintf(dot, "}\n");
    fclose(dot);
}

bool RTree::selfTest(unsigned long &nodes, unsigned long &records)
{
    nodes = records = 0;
    try
    {
        Node node(fa, M);
        node.read(root_offset);
        node.selfTest(nodes, records, 0, epicsTime(), epicsTime());
    }
    catch (GenericException &e)
    {
        LOG_MSG("RTree Error: %s", e.what());
        return false;
    }
    return true;
}

Node RTree::chooseLeaf(const Interval &range)
{
    Node node(fa, M);
    node.read(root_offset);
    node.chooseLeaf(range);
    return node;
}

// Insertion follows Guttman except as indicated
bool RTree::insertDatablock(const Interval &range,
                            FileOffset data_offset,
                            const stdString &data_filename)
{
    // Shortcuts for the new start/end times        
    const epicsTime &n0 = range.getStart(), &n1 = range.getEnd();
    size_t    additions = 0;
    Node node = chooseLeaf(range);
    // find record _before_ which to add new range.
    int idx;
    for (idx=0; idx<M; ++idx) 
    {   
        Record &record = node.getRecord(idx);
        FileOffset orig_data_blocks = record.getChildOrID();
        // Stop on first empty record
        if (orig_data_blocks == 0)
            break;
        // Get copy of record range, since the following might adjust that.
        const Interval rec_range = record.getInterval();
        // Shortcuts for the old start/end times        
        const epicsTime &o0 = rec_range.getStart(), &o1 = rec_range.getEnd();
        
        // Check for possible overlap situations.
        // Note: Overlap! Just "touching" is not an "overlap".

        // Does new entry belong after this one?
        // Existing record:  |----|...
        // New block      :          |####|
        if (o1 <= n0)
            continue; // check next record

        // Does new entry belong before this one?
        // Existing record:       ...|----|
        // New block      :  |####|
        if (n1 <= o0)
            break; // Found insertion point

        // Exact match: ==> Add block to record.
        if (rec_range == range)
            return node.addToRecord(idx, data_offset, data_filename);
            
        // Existing record:        |-------|
        // New block      :        |#############|
        // ==> Add to existing, then add   |#####|
        if (o0 == n0  &&  o1 < n1)
        { 
            if (node.addToRecord(idx, data_offset, data_filename))
                ++additions;
            if (insertDatablock(Interval(o1, n1), data_offset, data_filename))
                ++additions;
            return additions>0;
        }
        // Existing record:        |-------|
        // New block      :            |########|
        // ==> Split existing into |---|---|,
        //     then add                |###|####|
        if (o0 < n0  &&  n0 < o1  &&  o1 < n1)
        {
            split_record(node, idx, n0);
            if (insertDatablock(Interval(n0, o1), data_offset, data_filename))
                ++additions;
            if (insertDatablock(Interval(o1, n1), data_offset, data_filename))
                ++additions;
            return additions>0;
        }
        // Existing record:                              |-------|
        // New block      :                          |###########|
        // ==> Add to existing plus the non-overlap  |###|           
        if (n0 < o0   &&   n1 == o1)
        { 
            if (node.addToRecord(idx, data_offset, data_filename))
                ++additions;
            if (insertDatablock(Interval(n0, o0), data_offset, data_filename))
                ++additions;
            return additions>0;
        }
        // Existing record:                              |-------|
        // New block      :                          |#######|
        // ==> Split existing into                       |---|---|,
        //     then add                              |###|###|
        if (n0 < o0  &&  n1 < o1)
        { 
            split_record(node, idx, n1);
            if (insertDatablock(Interval(n0, o0), data_offset, data_filename))
                ++additions;
            if (insertDatablock(Interval(o0, n1), data_offset, data_filename))
                ++additions;
            return additions>0;
        }
        // Existing record:        |---|
        // New block      :     |----------|
        // ==> Add              |##|###|###|
        if (n0 < o0  &&  o1 < n1)
        {
            if (node.addToRecord(idx, data_offset, data_filename))
                ++additions;
            if (insertDatablock(Interval(n0, o0), data_offset, data_filename))
                ++additions;
            if (insertDatablock(Interval(o1, n1), data_offset, data_filename))
                ++additions;
            return additions>0;
        }
        // Existing record:        |-------|
        // New block      :        |###|
        // ==> Split existing into |---|---|,
        //     then add            |###|
        if (o0 == n0  &&  n1 < o1)
        {
            split_record(node, idx, n1);
            return insertDatablock(range, data_offset, data_filename);
        }
        // Existing record:        |-------.....|
        // New block      :            |###|
        // ==> Split existing into |---|---.....|,
        //     then add                |###|, i.e. reduce to previous case
        if (o0 < n0  &&  n1 <= o1)
        {
            split_record(node, idx, n0);
            return insertDatablock(range, data_offset, data_filename);
        }
    }
    
    // Need to insert new (first) block and record before record[idx]
    PrivateDatablock new_block(node, idx, data_filename, data_offset);
    FileOffset new_block_offset = fa.allocate(new_block.size());
    new_block.write(new_block_offset);
    
    // i might be == M, so the new record goes into the overflow node
    Node overflow(fa, M, true);
    bool caused_overflow, rec_in_overflow;
    node.insertRecord(idx, Record(range, new_block_offset),
                      overflow, caused_overflow, rec_in_overflow);
    if (caused_overflow)
        adjust_tree(node, &overflow);
    else
        adjust_tree(node, 0);
    return true;
}

/** Split an existing record at the given 'cut' time, so that
 *  there are two records start..cut and cut...end with the same
 *  data blocks as the original record.
 */
void RTree::split_record(Node &node, int idx, const epicsTime &cut)
{
    // Only allow this for leaf nodes
    LOG_ASSERT(node.isLeaf());
    
    Record &record = node.getRecord(idx);
    // Get copy of record range, since the following adjusts that.
    Interval range = record.getInterval();

    // See if the 'cut' time lies within the record.
    // Prohibit start...cut == empty  and  cut...end == empty.
    LOG_ASSERT(range.getStart() < cut);
    LOG_ASSERT(cut < range.getEnd());

    // Shrink range of record to start...cut
    record.setEnd(cut);
    node.write();
    adjust_tree(node, 0);

    // Add new record cut...end by inserting all the data blocks
    // again with time range cut...end
    range.setStart(cut);
    PrivateDatablock old_block(node, idx);
    bool go = old_block.readFirst();
    while (go)
    {
        if (! insertDatablock(range,
                              old_block.getDataOffset(),
                              old_block.getDataFilename()))
        {
            LOG_MSG("split_record failed to copy data block\n");
        }
        go = old_block.getNextChainedBlock();
    }
}

// This is the killer routine which keeps the tree balanced.
void RTree::adjust_tree(Node &node, Node *new_node)
{
    int i;
    if (!node.getParent()) // reached root?
    {   
        if (!new_node)
            return; // done
        // Have new_node parallel to root
        //    [ node (== root) ]  [ new_node ]
        //  -> grow taller, add new root
        //         [ new_root ]
        //    [ node ]  [ new_node ]
        Node new_root(fa, M, false);
        // new_root.child[0] = node
        new_root.getRecord(0).setInterval(node.getInterval());
        new_root.getRecord(0).setChildOrID(node.getOffset());
        // new_root.child[1] = new_node
        new_root.getRecord(1).setInterval(new_node->getInterval());
        new_root.getRecord(1).setChildOrID(new_node->getOffset());
        new_root.write(fa.allocate(new_root.size()));

        // Now that new_root is written and thus has a valid offset,
        // we can write the updated child nodes
        node.setParent(new_root);
        node.write();
        new_node->setParent(new_root);
        new_node->write();

        // Update Root pointer
        root_offset = new_root.getOffset();
        if (! (fseek(fa.getFile(), anchor, SEEK_SET)==0 &&
               writeLong(fa.getFile(), root_offset)==true))
            throw GenericException(__FILE__, __LINE__, "write error @ 0x%08lX",
                                   (unsigned long) anchor);
        return; // done.
    }
    // node is not the root
    Node parent(fa, M, true);
    parent.read(node.getParent());
    // update parent's interval for node
    i = parent.findRecordByID(node.getOffset());
    const Interval node_range = node.getInterval();
    if (parent.getRecord(i).getInterval() != node_range)
    {
        parent.getRecord(i).setInterval(node_range);
        parent.write();
    }
    if (!new_node) // Done at this level, go on up.
    {
        adjust_tree(parent, 0);
        return;
    }
    // Have to add new_node to parent
    Interval new_range = new_node->getInterval();
    if (! new_range.isValid())
        throw GenericException(__FILE__, __LINE__, "Empty new node?");
    for (i=0; i<M; ++i)
        if (parent.getRecord(i).getChildOrID() == 0 ||
            new_range.getEnd() <= parent.getRecord(i).getInterval().getStart())
            break;  // new entry belongs into parent record #i
    Node overflow(fa, M, false);    
    bool caused_overflow, rec_in_overflow;
    parent.insertRecord(i, Record(new_range, new_node->getOffset()),
                        overflow, caused_overflow, rec_in_overflow);
    if (rec_in_overflow)
        new_node->setParent(overflow);
    else
        new_node->setParent(parent);
    new_node->write();
    if (!caused_overflow)
    {
        adjust_tree(parent, 0); // no overflow; go on up.
        return;
    }
    // Either new_node or overflow from parent ended up in overflow
    overflow.write();
    // Adjust 'parent' pointers of all children that were moved to overflow
    Node overflow_child(fa, M, true);
    for (i=0; i<M; ++i)
    {
        FileOffset child_offset = overflow.getRecord(i).getChildOrID();
        // No child, or the new_node that we already handles?
        if (child_offset == 0 || child_offset == new_node->getOffset())
            continue;
        overflow_child.read(child_offset); 
        overflow_child.setParent(overflow);
        overflow_child.write();
    }
    adjust_tree(parent, &overflow);
}

size_t RTree::removeDatablock(const Interval &range,
                              FileOffset data_offset,
                              const stdString &data_filename)
{   // Search for data block to remove from the the root on down.
    Node node(fa, M);
    node.read(root_offset);
    return removeDatablock(node, range, data_offset, data_filename);
}

size_t RTree::removeDatablock(Node &node, const Interval &range,
                              FileOffset data_offset,
                              const stdString &data_filename)
{   // Look for reference to data block in this node or subnodes
    for (int record_index=0;  record_index<M;  ++record_index)
    {  
        if (node.getRecord(record_index).getChildOrID() == 0)
            return 0; // no more records in node; nothing found
            
        // If the data block range overlaps the record interval,
        // then at least one reference to the
        // data block should be found below that record.
        if (range.overlaps(node.getRecord(record_index).getInterval()))
        {   
            if (removeDatablock(node, record_index,
                                range, data_offset, data_filename))
            {   // In case one was removed, that can change the tree
                // structure, so it's easiest to start over from the root,
                // since we want to remove ALL references.
                return 1 + removeDatablock(range, data_offset, data_filename);
            }
            // In case nothing was removed, one reason could be that
            // we're recursing and already handled this one.
            // So check other records of this node.
        }
    }
    return 0;
}

size_t RTree::removeDatablock(Node &node, int record_index,
                             const Interval &range,
                             FileOffset data_offset,
                             const stdString &data_filename)
{   // Look for references to data block under specified record.
    if (node.isLeaf())
    {   // Try to remove data block reference.
        if (node.removeFromRecord(record_index, data_offset, data_filename))
        {   // Removed a reference to datablock from this record.
            // In case node's record is empty, remove whole rec.
            if (node.getRecord(record_index).getChildOrID() == 0)
                remove_record(node, record_index);
            return 1;
        }
    }
    else
    {   // Search subtree of this record
        Node subnode(fa, M);
        subnode.read(node.getRecord(record_index).getChildOrID());
        return removeDatablock(subnode, range,
                               data_offset, data_filename);
    }
    return 0;
}

void RTree::remove_record(Node &node, int i)
{   // Delete original entry
    int j;
    for (j=i; j<M-1; ++j)
        node.setRecord(j, node.getRecord(j+1));
    node.getRecord(j).clear();
    node.write();
    condense_tree(node);
}

void RTree::condense_tree(Node &node)
{
    int i;
    if (node.getParent() == 0)
    {   // reached root
        if (!node.isLeaf())
        {   // Count children of root
            int last_child = -1;
            for (i=0; i<M; ++i)
            {
                if (node.getRecord(i).getChildOrID())
                {
                    if (last_child >= 0)
                        return; // More then one child; all done.
                    last_child = i;
                }
            }
            if (last_child >= 0)
            {   // only last_child left => make that one root
                FileOffset old_root = node.getOffset();
                root_offset = node.getRecord(last_child).getChildOrID();
                node.read(root_offset);
                node.clearParent();
                node.write();
                if (fseek(fa.getFile(), anchor, SEEK_SET) != 0)
                    throw GenericException(__FILE__, __LINE__, "seek failed");
                writeLong(fa.getFile(), root_offset);
                fa.free(old_root);
            }
        }
        return;
    }
    bool empty = true;
    for (i=0; i<M; ++i)
        if (node.getRecord(i).getChildOrID())
        {
            empty = false;
            break;
        }
    Node parent(fa, M);
    parent.read(node.getParent());
    int child_index = parent.findRecordByID(node.getOffset());
    if (child_index < 0)
        throw GenericException(__FILE__, __LINE__,
                               "Cannot find getChildOrID() in parent\n");
    if (empty)
    {   // Delete the empty node, remove from parent
        fa.free(node.getOffset());
        for (i=child_index; i<M-1; ++i)
            parent.setRecord(i, parent.getRecord(i+1));
        parent.getRecord(M-1).clear();
    }
    else // Update parent with current interval of node
    {
        Interval range = node.getInterval();
        if (!range.isValid())
            throw GenericException(__FILE__, __LINE__, "empty node?");
        parent.getRecord(child_index).setInterval(range);
    }
    // Write updated parent and move up the tree
    parent.write();
    condense_tree(parent);
}

bool RTree::updateLast(const epicsTime &start, const epicsTime &end,
                       FileOffset ID)
{
    Node node(fa, M);
    int i;
    if (!getLastRecord(node, i))
        return false;
    if (node.getRecord(i).getChildOrID() != ID  ||
        node.getRecord(i).getInterval().getStart() != start)
        return false; // Cannot update, different data block
    // Update end time, done.
    node.getRecord(i).setEnd(end);
    node.write();
    adjust_tree(node, 0);
    return true;
}


