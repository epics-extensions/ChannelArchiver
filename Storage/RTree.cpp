// Tools
#include <MsgLogger.h>
#include <BinIO.h>
#include <InsertionSort.h>
// Index
#include "RTree.h"

#undef DEBUG_OVERLAP

#define RecordSize  (8+8+4)
#define NodeSize  (1+4+RecordSize*RTreeM)

long RTree::Datablock::getSize() const
{   //  data offset, name size, name (w/o '\0')
    return 4 + 2 + data_filename.length();
}

bool RTree::Datablock::write(FILE *f) const
{
    if (fseek(f, offset, SEEK_SET))
        return false;
    if (!(writeLong(f, data_offset) && writeShort(f, data_filename.length())))
        return false;
    return fwrite(data_filename.c_str(), data_filename.length(), 1, f) == 1;
}

bool RTree::Datablock::read(FILE *f)
{
    if (fseek(f, offset, SEEK_SET))
        return false;
    short len;
    char buf[300];
    if (!(readLong(f, &data_offset) && readShort(f, &len)))
        return false;
    if (len >= (short)sizeof(buf)-1)
    {
        LOG_MSG("Datablock::read: Filename exceeds buffer (%d)\n", len);
        return false;
    }
    if (fread(buf, len, 1, f) != 1)
        return false;
    buf[len] = '\0';
    data_filename.assign(buf, len);
    return (short)data_filename.length() == len;
}
    
// Check if intervals s1...e1 and s2...e2 overlap.
// Note: They might touch, e.g. s1 <= e1  <= s2 <= e2,
//       that's still not considered an overlap
inline bool do_intervals_overlap(const epicsTime &s1, const epicsTime &e1,
                                 const epicsTime &s2, const epicsTime &e2)
{
    if (e1 <= s2   ||  e2 <= s1)
        return false;
    return true;
}

// Does interval s1..e1 cover (totally overlap) s2..e2 ?
inline bool interval_covers_other(const epicsTime &s1, const epicsTime &e1,
                                  const epicsTime &s2, const epicsTime &e2)
{
    return s1 <= s2  &&  e1 >= e2;
}

// If entries s1..e1(ID1) and s2..e2(ID2) overlap,
// identify the (up to 3) non-overlapping new intervals
// and their IDs.
// In the region of overlap, the first interval "wins",
// i.e. has a higher priority.
// Entries s1.. and s2.. will be modified.
// s3.. will be set if s1 was 'inside' s2 and hence we end
// up with 3 non-overlapping intervals.
//
// An IDx = 0 indicates 'not used'.
static void resolve_overlap(epicsTime &s1, epicsTime &e1,
                            RTree::Offset &ID1,
                            epicsTime &s2, epicsTime &e2,
                            RTree::Offset &ID2,
                            epicsTime &s3, epicsTime &e3,
                            RTree::Offset &ID3)
{
    LOG_ASSERT(do_intervals_overlap(s1, e1, s2, e2));
#ifdef DEBUG_OVERLAP
    stdString txt1, txt2;
    printf("Overlap: %s..%s (%ld)", 
           epicsTimeTxt(s1, txt1), epicsTimeTxt(e1, txt2), ID1);
    printf(" and %s..%s (%ld)\n",
           epicsTimeTxt(s2, txt1), epicsTimeTxt(e2, txt2), ID2);
#endif
    // Sort the time stamps of intervals s1.. and s2...
    InsertionSort<epicsTime, 4> t;
    t.insert(s1); t.insert(e1); t.insert(s2); t.insert(e2);
    // We could now have up to 3 intervals: t0..t1, t1..t2, t2..t3.
    // How do the original intervals map to these, that is:
    // What IDs belong to which of the new intervals?
    RTree::Offset ID[3];
    if (interval_covers_other(s1, e1, t[0], t[1]))
        ID[0] = ID1; // check s1..e1/ID1 first to give it priority
    else if (interval_covers_other(s2, e2, t[0], t[1]))
        ID[0] = ID2;
    else
        ID[0] = 0;    
    if (interval_covers_other(s1, e1, t[1], t[2]))
        ID[1] = ID1;
    else if (interval_covers_other(s2, e2, t[1], t[2]))
        ID[1] = ID2;
    else
        ID[1] = 0;
    if (interval_covers_other(s1, e1, t[2], t[3]))
        ID[2] = ID1;
    else if (interval_covers_other(s2, e2, t[2], t[3]))
        ID[2] = ID2;
    else
        ID[2] = 0;
#ifdef DEBUG_OVERLAP
    printf("Separated: %s..%s (%ld);", 
           epicsTimeTxt(t[0], txt1), epicsTimeTxt(t[1], txt2), ID[0]);
    printf(" %s..%s (%ld);", 
           epicsTimeTxt(t[1], txt1), epicsTimeTxt(t[2], txt2), ID[1]);
    printf(" %s..%s (%ld)\n", 
           epicsTimeTxt(t[2], txt1), epicsTimeTxt(t[3], txt2), ID[2]);
#endif
    // Check if we can combine the new intervals since they use the same IDs.
    if (ID[2]  &&  ID[1] == ID[2])
    {
        t.set(2, t[3]);
        ID[2] = 0;
    }
    if (ID[1]  &&  ID[0] == ID[1])
    {
        t.set(1, t[2]);
        t.set(2, t[3]);
        ID[1] = ID[2];
        ID[2] = 0;
    }
    s1 = t[0]; e1 = t[1]; ID1 = ID[0];
    s2 = t[1]; e2 = t[2]; ID2 = ID[1];
    s3 = t[2]; e3 = t[3]; ID3 = ID[2];
#ifdef DEBUG_OVERLAP
    printf("Merged: %s..%s (%ld);", 
           epicsTimeTxt(t[0], txt1), epicsTimeTxt(t[1], txt2), ID[0]);
    printf(" %s..%s (%ld);", 
           epicsTimeTxt(t[1], txt1), epicsTimeTxt(t[2], txt2), ID[1]);
    printf(" %s..%s (%ld)\n", 
           epicsTimeTxt(t[2], txt1), epicsTimeTxt(t[3], txt2), ID[2]);
#endif
 }

static bool writeEpicsTime(FILE *f, const epicsTime &t)
{
    epicsTimeStamp stamp = t;
    return writeLong(f, stamp.secPastEpoch) && writeLong(f, stamp.nsec);
}

static bool readEpicsTime(FILE *f, epicsTime &t)
{
    epicsTimeStamp stamp;
    if (! (readLong(f, (long *)&stamp.secPastEpoch) &&
           readLong(f, (long *)&stamp.nsec)))
        return false;
    t = stamp;
    return true;
}

RTree::Record::Record()
{
    child_or_ID = 0;
}

bool RTree::Record::write(FILE *f) const
{
    return writeEpicsTime(f, start) &&
        writeEpicsTime(f, end) &&
        writeLong(f, child_or_ID);
}

bool RTree::Record::read(FILE *f)
{
    return readEpicsTime(f, start) &&
        readEpicsTime(f, end) &&
        readLong(f, &child_or_ID);
}

RTree::Node::Node(bool leaf)
{
    isLeaf = leaf;
    parent = 0;
    offset = 0;
}

bool RTree::Node::write(FILE *f) const
{
    if (fseek(f, offset, SEEK_SET))
        return false;
    if (! (writeByte(f, isLeaf) &&
           writeLong(f, parent)))
        return false;
    int i;
    for (i=0; i<RTreeM; ++i)
        if (!record[i].write(f))
            return false;
    return true;
}

bool RTree::Node::read(FILE *f)
{
    if (fseek(f, offset, SEEK_SET))
        return false;
    char c;
    if (! (readByte(f, &c) &&
           readLong(f, &parent)))
        return false;
    isLeaf = c > 0;
    int i;
    for (i=0; i<RTreeM; ++i)
        if (!record[i].read(f))
            return false;
    return true;
}

// Calc total covered interval for all records in node.
bool RTree::Node::getInterval(epicsTime &start, epicsTime &end) const
{
    bool valid = false;
    int i;
    for (i=0; i<RTreeM; ++i)
    {
        if (!record[i].child_or_ID)
            continue;
        if (i==0  ||  start > record[i].start)
        {
            start = record[i].start;
            valid = true;
        }
        if (i==0  ||  end < record[i].end)
        {
            end = record[i].end;
            valid = true;
        }
    }
    return valid;
}

RTree::RTree(FileAllocator &fa, Offset anchor)
        :  fa(fa), anchor(anchor), root_offset(0)
{
    cache_misses = cache_hits = 0;
}

bool RTree::init()
{
    // Create initial Root Node = Empty Leaf
    if (!(root_offset = fa.allocate(NodeSize)))
    {
        LOG_MSG("RTree::init cannot allocate node offset\n");
        return false;
    }
    Node node(true);
    node.offset = root_offset;
    if (!write_node(node))
    {
        LOG_MSG("RTree::init cannot write root node\n");
        return false;
    }    
    // Update Root pointer
    return fseek(fa.getFile(), anchor, SEEK_SET)==0 &&
        writeLong(fa.getFile(), root_offset)==true &&
        writeLong(fa.getFile(), RTreeM) == true;
}

bool RTree::reattach()
{
    long M;
    if (!(fseek(fa.getFile(), anchor, SEEK_SET)==0 &&
          readLong(fa.getFile(), &root_offset)==true &&
          readLong(fa.getFile(), &M) == true))
        return false;
    if (M != RTreeM)
    {
        LOG_MSG("RTree::reattach: Expected M to be %d, got %d\n",
                RTreeM, M);
        return false;
    }
    return true;
}

bool RTree::getInterval(epicsTime &start, epicsTime &end)
{
    Node node;
    node.offset = root_offset;
    return read_node(node) && node.getInterval(start, end);
}

// Insertion follows Guttman except as indicated
RTree::InsertResult RTree::insert(const epicsTime &start,
                                  const epicsTime &end, Offset ID)
{
    stdString txt1, txt2;
    if (start > end)
    {
        LOG_MSG("RTree::insert: illegal interval %s...%s\n",
                epicsTimeTxt(start, txt1), epicsTimeTxt(end, txt2));
        return InsError;
    }
    if (! ID)
    {
        LOG_MSG("RTree::insert: Cannot insert zero ID\n");
        return InsError;
    }
    int i, j;
    Node node;
    node.offset = root_offset;
    if (!choose_leaf(start, end, node))
    {
        LOG_MSG("RTree::insert cannot find leaf\n");
        return InsError;
    }
    for (i=0; i<RTreeM; ++i) // find record[i] <= [start...end]
    {   
        if (node.record[i].child_or_ID == ID &&
            node.record[i].start == start && 
            node.record[i].end   == end) // Already inserted?
            return InsExisted;
        if (node.record[i].child_or_ID == 0)
            break;
        if (do_intervals_overlap(node.record[i].start, node.record[i].end,
                                 start, end))
        {   // Special: Resolve overlap by inserting the non-overlapping pieces
            epicsTime s1=node.record[i].start;
            epicsTime e1=node.record[i].end;
            epicsTime s2=start, e2=end, s3, e3;
            Offset ID1=node.record[i].child_or_ID, ID2=ID, ID3;
            // Delete original entry
            if (!remove_record(node, i))
            {
                LOG_MSG("RTree::insert: Cannot remove record "
                        "from node @ 0x%lX\n", node.offset);
                return InsError;
            }
            resolve_overlap(s1, e1, ID1, s2, e2, ID2, s3, e3, ID3);
            if (ID1 && !insert(s1, e1, ID1))
            {
                LOG_MSG("RTree::insert: Cannot insert %s..%s (%d)\n",
                        epicsTimeTxt(s1, txt1), epicsTimeTxt(e1, txt2), ID1);
                return InsError;
            }
            if (ID2 && !insert(s2, e2, ID2))
            {
                LOG_MSG("RTree::insert: Cannot insert %s..%s (%d)\n",
                        epicsTimeTxt(s2, txt1), epicsTimeTxt(e2, txt2), ID2);
                return InsError;
            }
            if (ID3 && !insert(s3, e3, ID3))
            {
                LOG_MSG("RTree::insert: Cannot insert %s..%s (%d)\n",
                        epicsTimeTxt(s3, txt1), epicsTimeTxt(e3, txt2), ID3);
                return InsError;
            }
            return InsOK;
        }
        // Special: records sorted in time 
        if (end <= node.record[i].start) // new entry belongs into rec[i]
            break;
    }
    // Need to insert new data into record[i]
    Record overflow;
    if (i<RTreeM)
    {
        overflow = node.record[RTreeM-1]; // From i on, shift all recs right;
        for (j=RTreeM-1; j>i; --j)        // Last record goes into overflow.
            node.record[j] = node.record[j-1];
        node.record[i].start = start;
        node.record[i].end = end;
        node.record[i].child_or_ID = ID;
        if (!write_node(node))
        {
            LOG_MSG("RTree::insert cannot write node\n");
            return InsError;
        }
    }
    else
    {
        overflow.start = start;
        overflow.end = end;
        overflow.child_or_ID = ID;
    }
    if (overflow.child_or_ID)
    {   // Did either new entry or shifting result in overflow?
        Node new_node(true);
        if (!(new_node.offset = fa.allocate(NodeSize)))
        {
            LOG_MSG("RTree::insert cannot allocate new node\n");
            return InsError;
        }
        new_node.record[0] = overflow;
        return adjust_tree(node, &new_node) ? InsOK : InsError;
    }
    return adjust_tree(node, 0) ? InsOK : InsError;
}

RTree::InsertResult RTree::insertDatablock(const epicsTime &start,
                                           const epicsTime &end,
                                           Offset data_offset,
                                           stdString data_filename)
{
    // Beware: We must not simply insert a new datablock,
    // because then that datablock will have a new offset/ID
    // be be different from an equivalent datablock that might
    // already be in the tree. So check first:
    Datablock block;
    Node node;
    int i;
    if (searchDatablock(start, node, i, block) &&
        block.data_offset == data_offset && 
        node.record[i].start == start &&
        node.record[i].end == end &&
        block.data_filename == data_filename)
        return InsExisted;
    // Insert new block:
    block.data_offset = data_offset;
    block.data_filename = data_filename;
    block.offset = fa.allocate(block.getSize());
    if (!(block.offset && block.write(fa.getFile())))
    {
        fprintf(stderr, "RTree::insertDatablock(%s @ 0x%lX) cannot alloc\n",
                data_filename.c_str(), data_offset);
        return InsError;
    }
    return insert(start, end, block.offset);
}

bool RTree::search(const epicsTime &start, Node &node, int &i) const
{
    node.offset = root_offset;
    bool go;
    do
    {
        if (!read_node(node))
        {
            LOG_MSG("RTree::search cannot read node @ 0x%lX\n", node.offset);
            return false;
        }
        if (start < node.record[0].start) // request before start of tree?
            return getFirst(node, i);
        for (go=false, i=RTreeM-1;  i>=0;  --i)
        {   // Find right-most record with data at-or-before 'start'
            if (node.record[i].child_or_ID == 0)
                continue; // nothing
            if (node.record[i].start <= start)
            {
                if (node.isLeaf)   // Found!
                    return true;
                else
                {   // Search subtree
                    node.offset = node.record[i].child_or_ID;
                    go = true;
                    break;
                }
            }
        }
    }
    while (go);
    return false;
}

bool RTree::searchDatablock(const epicsTime &start, Node &node, int &i,
                            Datablock &block) const
{
    if (!search(start, node, i))
        return false;
    block.offset = node.record[i].child_or_ID;
    return block.read(fa.getFile());
}

bool RTree::getFirst(Node &node, int &i) const
{
    // Descent using leftmost children
    node.offset = root_offset;
    while (node.offset)
    {
        if (!read_node(node))
        {
            LOG_MSG("RTree::getFirst: read error\n");
            return false;
        }
        for (i=0; i<RTreeM; ++i) // Locate leftmost record
            if (node.record[i].child_or_ID)
                break;
        if (i>=RTreeM)
            return false;
        if (node.isLeaf)           // Done or continue to go down?
            return true;
        node.offset = node.record[i].child_or_ID;
    }    
    return false;
}

bool RTree::getLast(Node &node, int &i) const
{
    // Descent using rightmost children
    node.offset = root_offset;
    while (node.offset)
    {
        if (!read_node(node))
        {
            LOG_MSG("RTree::getLast: read error\n");
            return false;
        }
        for (i=RTreeM-1; i>=0; --i) // Locate rightmost record
            if (node.record[i].child_or_ID)
                break;
        if (i<0)
            return false;
        if (node.isLeaf)           // Done or continue to go down?
            return true;
        node.offset = node.record[i].child_or_ID;
    }    
    return false;
}

bool RTree::getFirstDatablock(Node &node, int &i, Datablock &block) const
{
    if (!getFirst(node, i))
        return false;
    block.offset = node.record[i].child_or_ID;
    return block.read(fa.getFile());
}

bool RTree::getLastDatablock(Node &node, int &i, Datablock &block) const
{  
    if (!getLast(node, i))
        return false;
    block.offset = node.record[i].child_or_ID;
    return block.read(fa.getFile());
}

bool RTree::prevDatablock(Node &node, int &i, Datablock &block) const
{
    if (!prev(node, i))
        return false;
    block.offset = node.record[i].child_or_ID;
    return block.read(fa.getFile());
}

bool RTree::nextDatablock(Node &node, int &i, Datablock &block) const
{
    if (!next(node, i))
        return false;
    block.offset = node.record[i].child_or_ID;
    return block.read(fa.getFile());
}

// Follows Guttman except that we don't care about
// half-filled nodes. Only empty nodes get removed.
bool RTree::remove(const epicsTime &start, const epicsTime &end, Offset ID)
{
    int i;
    Node node;
    node.offset = root_offset;
    bool go;
    do
    {
        if (!read_node(node))
        {
            LOG_MSG("RTree::remove cannot read node @ 0x%lX\n", node.offset);
            return false;
        }
        for (go=false, i=0;  i<RTreeM;  ++i)
        {   // Find left-most record that includes our target interval
            if (node.record[i].child_or_ID == 0)
                return false;
            if (!node.isLeaf &&
                node.record[i].start <= start && node.record[i].end >= end)
            {   // Search subtree
                node.offset = node.record[i].child_or_ID;
                go = true;
                break;
            }
            if (node.isLeaf &&
                node.record[i].start == start && node.record[i].end == end &&
                node.record[i].child_or_ID == ID)
                return remove_record(node, i);
        }
    }
    while (go);
    return false;
}

bool RTree::updateLast(const epicsTime &start, const epicsTime &end, Offset ID)
{
    int i;
    Node node;
    if (!getLast(node, i))
        return false;
    if (node.record[i].child_or_ID != ID  ||
        node.record[i].start != start)
        return false; // Cannot update, different data block
    // Update end time, done.
    node.record[i].end = end;
    return write_node(node) && adjust_tree(node, 0);
}

bool RTree::updateLastDatablock(const epicsTime &start, const epicsTime &end,
                                Offset data_offset, stdString data_filename)
{
    Node node;
    int i;
    if (getLast(node, i) &&
        node.record[i].start == start)
    {
        Datablock block;
        block.offset = node.record[i].child_or_ID;
        if (!block.read(fa.getFile()))
            return false;
        if (block.data_offset == data_offset &&
            block.data_filename == data_filename)
        {
            node.record[i].end = end;
            return write_node(node) && adjust_tree(node, 0);
        }
    }
    return insertDatablock(start, end, data_offset, data_filename);
}    

void RTree::makeDot(const char *filename, bool show_data)
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
    make_node_dot(dot, fa.getFile(), root_offset, show_data);
    fprintf(dot, "}\n");
    fclose(dot);
}

bool RTree::selfTest()
{
    return self_test_node(root_offset, 0, epicsTime(), epicsTime());
}

// Comparison routine for AVLTree (node_cache)
static int sort_compare(const RTree::Node &a, const RTree::Node &b)
{    return b.offset - a.offset; }

bool RTree::read_node(Node &node) const
{
    if (node_cache.find(node))
    {
        ++cache_hits;
        return true;
    }
    ++cache_misses;
    if (!node.read(fa.getFile()))
        return false;
    node_cache.add(node);
    return true;
}

bool RTree::write_node(const Node &node)
{
    node_cache.add(node);
    return node.write(fa.getFile());
}    

bool RTree::self_test_node(Offset n, Offset p, epicsTime start, epicsTime end)
{
    stdString txt1, txt2;
    epicsTime s, e;
    int i;
    Node node;
    node.offset = n;
    if (!read_node(node))
    {
        LOG_MSG("RTree::self_test cannot read node @ 0x%lX\n", node.offset);
        return false;
    }
    node.getInterval(s, e);
    if (node.parent != p)
    {
        LOG_MSG("Node @ 0x%lX, %s ... %s: parent = 0x%lX != 0x%lX\n",
                node.offset, epicsTimeTxt(s, txt1), epicsTimeTxt(e, txt2),
                node.parent, p);
        return false;
    }
    if (p && (s != start || e != end))
    {
        LOG_MSG("Node @ 0x%lX: Node Interval %s ... %s\n",
                node.offset, epicsTimeTxt(s, txt1), epicsTimeTxt(e, txt2));
        LOG_MSG("              Parent        %s ... %s\n",
                epicsTimeTxt(start, txt1), epicsTimeTxt(end, txt2));
        return false;
    }
    for (i=1; i<RTreeM; ++i)
    {
        if (node.record[i].child_or_ID &&
            node.record[i-1].end > node.record[i].start)
        {
            LOG_MSG("Node @ 0x%lX: Records not in order\n", node.offset);
            return false;
        }
        if (node.record[i].child_or_ID && !node.record[i-1].child_or_ID) 
        {
            LOG_MSG("Node @ 0x%lX: Empty record before filled one\n",
                    node.offset);
            return false;
        }
    }
    if (node.isLeaf)
        return true;
    for (i=0; i<RTreeM; ++i)
    {
        if (node.record[i].child_or_ID)
        {
            if (!self_test_node(node.record[i].child_or_ID,
                                node.offset,
                                node.record[i].start,
                                node.record[i].end))
                return false;
        }
    }       
    return true;
}

void RTree::make_node_dot(FILE *dot, FILE *f, Offset node_offset,
                          bool show_data)
{
    Datablock data_block;
    stdString txt1, txt2;
    int i;
    Node node;
    node.offset = node_offset;
    if (! node.read(f))
    {
        LOG_MSG("RTree::make_node_dot cannot read node @ 0x%lX\n",
                node.offset);
        return;
    }
    fprintf(dot, "\tnode%ld [ label=\"", node.offset);
    for (i=0; i<RTreeM; ++i)
    {
        if (i>0)
            fprintf(dot, "|");
        epicsTime2string(node.record[i].start, txt1);
        epicsTime2string(node.record[i].end, txt2);
        if (txt1.length() > 10)
            fprintf(dot, "<f%d> %s \\r-%s \\r", i, txt1.c_str(), txt2.c_str());
        else
            fprintf(dot, "<f%d>%s-%s", i, txt1.c_str(), txt2.c_str());
    }    
    fprintf(dot, "\"];\n");
    if (node.isLeaf)
    {
        for (i=0; i<RTreeM; ++i)
        {
            if (node.record[i].child_or_ID)
            {
                if (show_data)
                {
                    data_block.offset = node.record[i].child_or_ID;
                    if (!data_block.read(f))
                    {
                        LOG_MSG("RTree::make_node_dot cannot read data "
                                "@ 0x%lX\n", data_block.offset);
                        return;
                    }
                    fprintf(dot, "\tid%ld "
                            "[ label=\"'%s' \\r@ 0x%lX \\r\",style=filled ];\n",
                            node.record[i].child_or_ID,
                            data_block.data_filename.c_str(),
                            data_block.data_offset);                    
                }
                else
                {
                    fprintf(dot, "\tid%ld "
                            "[ label=\"ID %ld\",style=filled ];\n",
                            node.record[i].child_or_ID,
                            node.record[i].child_or_ID);
                }
                fprintf(dot, "\tnode%ld:f%d->id%ld;\n",
                        node.offset, i, node.record[i].child_or_ID);
            }
        }
    }
    else
    {
        for (i=0; i<RTreeM; ++i)
        {            
            if (node.record[i].child_or_ID)
                fprintf(dot, "\tnode%ld:f%d->node%ld:f0;\n",
                        node.offset, i, node.record[i].child_or_ID);
        }
        for (i=0; i<RTreeM; ++i)
        {
            if (node.record[i].child_or_ID)
                make_node_dot(dot, f, node.record[i].child_or_ID, show_data);
        }
    }
}

bool RTree::choose_leaf(const epicsTime &start, const epicsTime &end,
                        Node &node)
{
    if (!read_node(node))
        return false;
    if (node.isLeaf)
        return true;
    // If a subtree already contains data for the time range
    // or there's an overlap, use that one. Otherwise follow
    // the RTree paper:
    // Find entry which needs the least enlargement.
    epicsTime t0, t1;
    double enlarge, min_enlarge=0;
    int i, min_i=-1;
    for (i=0; i<RTreeM; ++i)
    {
        if (!node.record[i].child_or_ID)
            continue;
        if (do_intervals_overlap(node.record[i].start, node.record[i].end,
                                 start, end))
        {
            node.offset = node.record[i].child_or_ID;
            return choose_leaf(start, end, node);
        }
        // t0 .. t1 = union(start...end, record[i].start...end)
        if (start < node.record[i].start)
            t0 = start;
        else
            t0 = node.record[i].start;
        if (end > node.record[i].end)
            t1 = end;
        else
            t1 = node.record[i].end;
        enlarge = (t1 - t0) - (node.record[i].end - node.record[i].start);
        // Pick rightmost record of those with min. enlargement
        if (i==0  ||  enlarge <= min_enlarge)
        {
            min_enlarge = enlarge;
            min_i = i;
        }
    }
    node.offset = node.record[min_i].child_or_ID;
    LOG_ASSERT(node.offset != 0);
    return choose_leaf(start, end, node);
}

// This is the killer routine which keeps the tree balanced
bool RTree::adjust_tree(Node &node, Node *new_node)
{
    int i, j;
    epicsTime start, end;
    Node parent;
    parent.offset = node.parent;
    if (!parent.offset) // reached root?
    {   
        if (!new_node)
            return true; // done
        // Have new_node parallel to root -> grow taller, add new root
        Node new_root(false);
        if (!(new_root.offset = fa.allocate(NodeSize)))
        {
            LOG_MSG("RTree::adjust_tree cannot allocate new root\n");
            return false;
        }
        node.getInterval(new_root.record[0].start, new_root.record[0].end);
        new_root.record[0].child_or_ID = node.offset;
        node.parent = new_root.offset;
        new_node->getInterval(new_root.record[1].start,
                               new_root.record[1].end);
        new_node->parent = new_root.offset;
        new_root.record[1].child_or_ID = new_node->offset;
        if (!(write_node(node) && write_node(*new_node) &&
              write_node(new_root)))
        {
            LOG_MSG("RTree::adjust_tree cannot write new root\n");
            return false;
        }        
        // Update Root pointer
        root_offset = new_root.offset;
        return fseek(fa.getFile(), anchor, SEEK_SET)==0 &&
            writeLong(fa.getFile(), root_offset)==true;
    }
    if (!read_node(parent))
    {
        LOG_MSG("RTree::adjust_tree cannot read node @ 0x%lX\n",parent.offset);
        return false;
    }
    for (i=0; i<RTreeM; ++i)   // update parent's interval for node
        if (parent.record[i].child_or_ID == node.offset)
        {
            node.getInterval(start, end);
            if (start!=parent.record[i].start || end!=parent.record[i].end)
            {
                parent.record[i].start = start;
                parent.record[i].end   = end;
                if (!write_node(parent))
                {
                    LOG_MSG("RTree::adjust_tree cannot write node @ 0x%lX\n",
                            parent.offset);
                    return false;
                }               
            }
            break;
        }
    if (!new_node) // Done at this level, go on up.
        return adjust_tree(parent, 0);
    // Have to add new_node to parent
    new_node->getInterval(start, end);
    for (i=0; i<RTreeM; ++i)
        if (parent.record[i].child_or_ID == 0 || end <= parent.record[i].start)
            break;  // new entry belongs into rec[i]
    Node new_parent(false);
    if (i<RTreeM) // add new node to this parent, maybe causing overflow
    {
        new_parent.record[0] = parent.record[RTreeM-1]; // Right-shift fr.i on;
        for (j=RTreeM-1; j>i; --j)        // Last rec. goes into new_parent.
            parent.record[j] = parent.record[j-1];
        parent.record[i].start = start;
        parent.record[i].end = end;
        parent.record[i].child_or_ID = new_node->offset;
        if (!write_node(parent))
        {
            LOG_MSG("RTree::adjust_tree cannot write parent\n");
            return false;
        }
        new_node->parent = parent.offset;
        if (!write_node(*new_node))
        {
            LOG_MSG("RTree::adjust_tree cannot write new node\n");
            return false;
        }
    }
    else
    {   // add new node to overflow
        new_parent.record[0].start = start;
        new_parent.record[0].end = end;
        new_parent.record[0].child_or_ID = new_node->offset;
    }
    if (new_parent.record[0].child_or_ID == 0)
        return adjust_tree(parent, 0); // no overflow; go on up.
    // Either new_node or overflow from parent ended up in new_parent
    if (!(new_parent.offset = fa.allocate(NodeSize)))
    {
        LOG_MSG("RTree::adjust_tree cannot allocate new parent\n");
        return false;
    }
    if (!write_node(new_parent))
    {
        LOG_MSG("RTree::adjust_tree cannot write new parent\n");
        return false;
    }
    Node new_parent_child_or_ID;
    new_parent_child_or_ID.offset = new_parent.record[0].child_or_ID;
    if (new_parent_child_or_ID.offset == new_node->offset)
        new_parent_child_or_ID = *new_node;
    else if (!read_node(new_parent_child_or_ID))
    {
        LOG_MSG("RTree::adjust_tree cannot read new parent's child_or_ID\n");
        return false;
    }
    new_parent_child_or_ID.parent = new_parent.offset;
    if (!write_node(new_parent_child_or_ID))
    {
        LOG_MSG("RTree::adjust_tree cannot write new parent's child_or_ID\n");
        return false;
    }
    return adjust_tree(parent, &new_parent);
}

bool RTree::remove_record(Node &node, int i)
{
    int j;
    // Delete original entry
    for (j=i; j<RTreeM-1; ++j)
        node.record[j] = node.record[j+1];
    node.record[j].start = nullTime;
    node.record[j].end = nullTime;
    node.record[j].child_or_ID = 0;
    if (!write_node(node))
    {
        LOG_MSG("RTree::remove_record: "
                "Cannot remove write node @ 0x%lX\n", node.offset);
        return false;
    }
    return condense_tree(node);
}

bool RTree::condense_tree(Node &node)
{
    int i, j=-1;
    if (node.parent==0)
    {   // reached root
        if (!node.isLeaf)
        {
            int children = 0;
            for (i=0; i<RTreeM; ++i)
                if (node.record[i].child_or_ID)
                {
                    ++children;
                    j=i;
                }
            if (children==1)
            {   // only child_or_ID j left => make that one root
                Offset old_root = node.offset;
                root_offset = node.offset = node.record[j].child_or_ID;
                if (!read_node(node))
                {
                    LOG_MSG("RTree::condense_tree cannot read "
                            "node @ 0x%lX\n", node.offset);
                    return false;
                }
                node.parent = 0;
                if (!(write_node(node) &&
                      fseek(fa.getFile(), anchor, SEEK_SET)==0 &&
                      writeLong(fa.getFile(), root_offset)))
                {
                    LOG_MSG("RTree::condense_tree cannot update root\n");
                    return false;      
                }
                fa.free(old_root);
            }
        }
        return true;
    }
    bool empty = true;
    for (i=0; i<RTreeM; ++i)
        if (node.record[i].child_or_ID)
        {
            empty = false;
            break;
        }
    Node parent;
    parent.offset = node.parent;
    if (!read_node(parent))
    {
        LOG_MSG("RTree::condense_tree cannot read node @ 0x%lX\n",
                node.offset);
        return false;
    }
    for (i=0; i<RTreeM; ++i)
        if (parent.record[i].child_or_ID == node.offset)
        {
            if (empty)
            {   // Delete the empty node, remove from parent
                fa.free(node.offset);
                for (j=i; j<RTreeM-1; ++j)
                    parent.record[j] = parent.record[j+1];
                parent.record[RTreeM-1].start = nullTime;
                parent.record[RTreeM-1].end   = nullTime;
                parent.record[RTreeM-1].child_or_ID = 0;
            }
            else
                node.getInterval(parent.record[i].start,parent.record[i].end);
            if (!write_node(parent))
            {
                LOG_MSG("RTree::condense_tree cannot write node @ 0x%lX\n",
                        node.offset);
                return false;           
            }
            return condense_tree(parent);
        }
    LOG_MSG("RTree::condense_tree: Cannot find child_or_ID in parent\n");
    return false;
}

bool RTree::prev_next(Node &node, int &i, int dir) const
{
    LOG_ASSERT(node.isLeaf);
    LOG_ASSERT(i>=0  &&  i<RTreeM);
    LOG_ASSERT(dir == -1  ||  dir == 1);
    i += dir;
    // Another rec. in curr.node?
    if (i>=0 && i<RTreeM && node.record[i].child_or_ID)
        return true;
    Node parent;
    // Go up to parent nodes...
    while (true)
    {
        if (!(parent.offset = node.parent))
            return false;
        if (!read_node(parent))
        {
            LOG_MSG("RTree::next: read error\n");
            return false;
        }
        for (i=0; i<RTreeM; ++i)
            if (parent.record[i].child_or_ID == node.offset)
                break;
        if (i>=RTreeM)
        {
            LOG_MSG("RTree::next: child_or_ID not listed in parent?\n");
            return false;
        }
        i += dir;
        if (i>=0 && i<RTreeM && parent.record[i].child_or_ID)
            break;
        // else: go up another level
        node = parent;
    }
    node.offset = parent.record[i].child_or_ID;
    // Decend using rightmost (prev) or leftmost (next)  children
    i = 0;
    while (node.offset)
    {
        if (!read_node(node))
        {
            LOG_MSG("RTree::next: read error\n");
            return false;
        }
        if (dir < 0)
            for (i=RTreeM-1; i>0; --i)
                if (node.record[i].child_or_ID)
                    break;
        if (node.isLeaf)
            return node.record[i].child_or_ID != 0;
        node.offset = node.record[i].child_or_ID;
    }
    return false;
}

