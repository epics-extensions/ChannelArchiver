// -*- c++ -*-
// Tools
#include <BinIO.h>
#include <MsgLogger.h>.
#include <AutoPtr.h>.
// Index
#include "NameHash.h"

NameHash::Entry::~Entry()
{}
        
/** The 'private' Entry used by the NameHash:
 *  A NameHashEntry that allows set, read, write
 */
class PrivateEntry : public NameHash::Entry
{
public:
    PrivateEntry();

    PrivateEntry(const stdString &name, const stdString &ID_txt, FileOffset ID);
    
    const stdString &getName() const;
    const stdString &getIdTxt() const;
    FileOffset getId() const;

    void setName(const char *buffer, size_t len)
    {   name.assign(buffer, len); }

    void setName(const stdString &new_name)
    {   name = new_name; }
    
    void setIdTxt(const char *buffer, size_t len)
    {   ID_txt.assign(buffer, len); }

    void setIdTxt(const stdString &new_id)
    {   ID_txt = new_id; }

    void setId(FileOffset new_id)
    {   ID = new_id; }    
    
    /** @return offset of next entry or 0 */
    FileOffset getNext() const
    {   return next; }

    /** @return offset of next entry or 0 */
    void setNext(FileOffset new_next)
    {   next = new_next; }
    
    /// Size of entry
    FileOffset getSize() const;

    /// Write at offset.
    /// @exception GenericException on error.
    void write(FILE *f, FileOffset offset) const;

    /// Write at offset where entry was read.
    /// @exception GenericException on error.
    void write(FILE *f) const;

    /// Read from offset.
    /// @exception GenericException on error.
    void read(FILE *f, FileOffset offset);

private:
    stdString  name;  ///< Channel Name.
    stdString  ID_txt;///< String and numeric ID
    FileOffset ID;    ///< (filename and offset to RTree for the channel).

    FileOffset next;  ///< Offset to next entry w/ same hash value
    mutable FileOffset offset;///< Offset of this Entry
};

PrivateEntry::PrivateEntry()
  : ID(0), next(0), offset(0) 
{}

PrivateEntry::PrivateEntry(const stdString &name, const stdString &ID_txt, FileOffset ID)
  : name(name), ID_txt(ID_txt), ID(ID),
    next(0),
    offset(0) 
{}

const stdString &PrivateEntry::getName() const
{   return name; }

const stdString &PrivateEntry::getIdTxt() const
{   return ID_txt; }

FileOffset PrivateEntry::getId() const
{   return ID; }

FileOffset PrivateEntry::getSize() const
{   /// next, id, name.len, ID_txt.len, name, ID_txt
    return 4 + 4 + 2 + 2 + getName().length() + getIdTxt().length();
}

void PrivateEntry::write(FILE *f, FileOffset offset) const
{
    if (fseek(f, offset, SEEK_SET) != 0)
        throw GenericException(__FILE__, __LINE__,
                               "seek(0x%08lX) error",
                               (unsigned long) offset);
    if (!(writeLong(f, next) &&
          writeLong(f, getId()) &&
          writeShort(f, getName().length()) &&
          writeShort(f, getIdTxt().length()) &&
          fwrite(getName().c_str(), getName().length(), 1, f) == 1))
        throw GenericException(__FILE__, __LINE__,
                               "write error at 0x%08lX",
                               (unsigned long) offset);
    size_t len = getIdTxt().length();
    if (len > 0  &&  fwrite(getIdTxt().c_str(), len, 1, f) != 1)
        throw GenericException(__FILE__, __LINE__,
                               "write error at 0x%08lX",
                               (unsigned long) offset);
    this->offset = offset;
}

void PrivateEntry::write(FILE *f) const
{
    LOG_ASSERT(offset != 0);
    write(f, offset);
}

void PrivateEntry::read(FILE *f, FileOffset offset)
{
    FileOffset ID;
    char buffer[100];
    unsigned short name_len, ID_len;
    if (!(fseek(f, offset, SEEK_SET)==0 &&
          readLong(f, &next) &&
          readLong(f, &ID) &&
          readShort(f, &name_len) &&
          readShort(f, &ID_len)))
        throw GenericException(__FILE__, __LINE__,
                               "read error at 0x%08lX",
                               (unsigned long) offset);
    setId(ID);
    if (name_len >= sizeof(buffer)-1)
        throw GenericException(__FILE__, __LINE__,
                               "Entry's name (%d) exceeds buffer size\n",
                               (int)name_len);
    if (ID_len >= sizeof(buffer)-1)
        throw GenericException(__FILE__, __LINE__,
                               "Entry's ID_txt (%d) exceeds buffer size\n",
                               (int)ID_len);
    if (fread(buffer, name_len, 1, f) != 1)
        throw GenericException(__FILE__, __LINE__,
                               "Read error for name of entry @ 0x%lX\n",
                               (unsigned long)offset);
    setName(buffer, name_len);
    if (getName().length() != name_len)
        throw GenericException(__FILE__, __LINE__,
                               "NameHash: Error for name '%s' @ 0x%lX\n",
                               getName().c_str(),
                               (unsigned long)offset);
    if (ID_len > 0)
    {
        if (fread(buffer, ID_len, 1, f) != 1)
            throw GenericException(__FILE__, __LINE__,
                                   "Read error for ID_txt of entry @ 0x%lX\n",
                                   (unsigned long)offset);
        setIdTxt(buffer, ID_len);
        if (getIdTxt().length() != ID_len)
            throw GenericException(__FILE__, __LINE__,
                                   "Error for ID_txt '%s' @ 0x%lX\n",
                                   getIdTxt().c_str(),
                                   (unsigned long)offset);
    }
    else
        setIdTxt(0, 0);
    this->offset = offset;
}

NameHash::NameHash(FileAllocator &fa, FileOffset anchor)
        : fa(fa), anchor(anchor), ht_size(0), table_offset(0)
{}

void NameHash::init(uint32_t ht_size)
{
    this->ht_size = ht_size;
    if (!(table_offset = fa.allocate(4*ht_size)))
        throw GenericException(__FILE__, __LINE__,
                               "NameHash::init: Cannot allocate hash table\n");
    if (fseek(fa.getFile(), table_offset, SEEK_SET))
        throw GenericException(__FILE__, __LINE__,
                               "NameHash::init: Cannot seek to hash table\n");
    uint32_t i;
    for (i=0; i<ht_size; ++i)
        if (!writeLong(fa.getFile(), 0))
            throw GenericException(__FILE__, __LINE__,
                               "NameHash::init: Cannot write to hash table\n");
    if (fseek(fa.getFile(), anchor, SEEK_SET) != 0 ||
        !writeLong(fa.getFile(), table_offset) ||
        !writeLong(fa.getFile(), ht_size))
        throw GenericException(__FILE__, __LINE__,
                               "NameHash::init: Cannot write anchor info\n");
}

void NameHash::reattach()
{
    if (fseek(fa.getFile(), anchor, SEEK_SET) != 0 ||
        !readLong(fa.getFile(), &table_offset) ||
        !readLong(fa.getFile(), &ht_size))
        throw GenericException(__FILE__, __LINE__,
                               "NameHash::readLong: Cannot read anchor info\n");
}
    
bool NameHash::insert(const stdString &name,
                      const stdString &ID_txt, FileOffset ID)
{
    LOG_ASSERT(name.length() > 0);
    uint32_t h = hash(name);
    FileOffset offset = read_HT_entry(h);
    FILE *f = fa.getFile();
    if (offset == 0)
    {   // First entry for this hash value
        PrivateEntry entry(name, ID_txt, ID);
        offset = fa.allocate(entry.getSize());
        entry.write(f, offset);
        write_HT_entry(h, offset);
        return true; // new entry
    }    
    PrivateEntry entry;
    while (offset)
    {
        entry.read(f, offset);
        if (entry.getName() == name)
        {   // Update existing entry
            entry.setIdTxt(ID_txt);
            entry.setId(ID);
            entry.write(f);
            return false; // old entry
        }
        // read next entry in list
        offset = entry.getNext();
    }
    // Add new entry to end of list for current hash value
    PrivateEntry new_entry(name, ID_txt, ID);
    FileOffset new_offset = fa.allocate(new_entry.getSize());
    new_entry.write(f, new_offset);
    entry.setNext(new_offset);
    entry.write(f);
    return true; // new entry
}

NameHash::Entry *NameHash::find(const stdString &name)
{
    LOG_ASSERT(name.length() > 0);
    uint32_t h = hash(name);
    AutoPtr<PrivateEntry> entry(new PrivateEntry());
    FILE *f = fa.getFile();
    FileOffset offset = read_HT_entry(h);
    while (offset)
    {
        entry->read(f, offset);
        if (entry->getName() == name) // Found!
            return entry.release();
        offset = entry->getNext();
    }
    // No read error, but nothing found, either.
    return 0;
}

NameHash::Iterator::~Iterator()
{}

class PrivateIterator : public NameHash::Iterator
{
public:
    PrivateIterator(const NameHash *name_hash);
    
    // NameHash::Iterator API
    bool isValid() const;    
    void next();
    // NameHashEntry API
    const stdString &getName() const;
    const stdString &getIdTxt() const;
    FileOffset getId() const;
    
private:
    // Hash over which we iterate. Also used for NULL => !isValid()
    const NameHash *name_hash;
    uint32_t hashvalue;
    PrivateEntry entry;
};

PrivateIterator::PrivateIterator(const NameHash *name_hash)
    : name_hash(0)
{
    FILE *f = name_hash->fa.getFile();
    if (fseek(f, name_hash->table_offset, SEEK_SET))
        throw GenericException(__FILE__, __LINE__,
                               "Seek 0x%08lX failed\n",
                               (unsigned long)name_hash->table_offset);
    FileOffset offset;
    for (hashvalue=0;  hashvalue < name_hash->ht_size;  ++hashvalue)
    {   // Find first uses entry in hash table
        if (!readLong(f, &offset))
            throw GenericException(__FILE__, __LINE__,
                                   "Cannot read hash table @ 0x%08lX\n",
                                   (unsigned long)offset);
        if (offset)
        {   
            entry.read(f, offset);
            this->name_hash = name_hash; // indicate 'isValid'
            return;
        }
    }
    // else: !isValid() 
}

bool PrivateIterator::isValid() const
{   return name_hash != 0;  }


/** Get next entry.
 *  @see isValid
 */
void PrivateIterator::next()
{
    if (!isValid())
        return;
    // Is another entry in list for same hashvalue?
    FileOffset offset = entry.getNext();
    if (! offset)
    {   // No, find next used entry in hash table
        for (++hashvalue; hashvalue < name_hash->ht_size; ++hashvalue)
        {
            offset = name_hash->read_HT_entry(hashvalue);
            if (offset)
                break;
        }
    }
    if (offset)
    {
        entry.read(name_hash->fa.getFile(), offset);
        return;
    }
    name_hash = 0; // invalid
}

const stdString &PrivateIterator::getName() const
{   return entry.getName(); }

const stdString &PrivateIterator::getIdTxt() const
{   return entry.getIdTxt(); }

FileOffset PrivateIterator::getId() const
{   return entry.getId(); }

NameHash::Iterator *NameHash::iterator()
{
    return new PrivateIterator(this);
}

// From Sergei Chevtsov's rtree code:
uint32_t NameHash::hash(const stdString &name) const
{
    const int8_t *c = (const int8_t *)name.c_str();
    uint32_t h = 0;
    while (*c)
        h = (128*h + *(c++)) % ht_size;
    return (uint32_t)h;
}

void NameHash::showStats(FILE *f)
{
    unsigned long l, used_entries = 0, total_list_length = 0, max_length = 0;
    unsigned long hashvalue;
    PrivateEntry entry;
    FileOffset offset;
    for (hashvalue=0; hashvalue<ht_size; ++hashvalue)
    {
        offset = read_HT_entry(hashvalue);
        if (offset)
        {
            ++used_entries;
            l = 0;
            do
            {
                entry.read(fa.getFile(), offset);
                ++l;
                ++total_list_length;
                offset = entry.getNext();
            }
            while (offset);
            if (l > max_length)
                max_length = l;
        }
    }
    fprintf(f, "Hash table fill ratio: %ld out of %ld entries (%ld %%)\n",
            used_entries, (unsigned long)ht_size, used_entries*100/ht_size);
    if (used_entries > 0)
        fprintf(f, "Average list length  : %ld entries\n",
                total_list_length / used_entries);
    fprintf(f, "Maximum list length  : %ld entries\n", max_length);
}

FileOffset NameHash::read_HT_entry(uint32_t hash_value) const
{
    LOG_ASSERT(hash_value >= 0 && hash_value < ht_size);
    FileOffset o = table_offset + hash_value*4;
    FileOffset offset;
    if (!(fseek(fa.getFile(), o, SEEK_SET)==0 &&
          readLong(fa.getFile(), &offset)))
        throw GenericException(__FILE__, __LINE__,
                               "Cannot read HT entry @ 0x%lX\n",
                               (long)hash_value);
    return offset;
} 

void NameHash::write_HT_entry(uint32_t hash_value,
                              FileOffset offset) const
{
    LOG_ASSERT(hash_value >= 0 && hash_value < ht_size);
    FileOffset o = table_offset + hash_value*4;
    if (!(fseek(fa.getFile(), o, SEEK_SET)==0 &&
          writeLong(fa.getFile(), offset)))
        throw GenericException(__FILE__, __LINE__,
                               "Cannot write HT entry @ 0x%lX\n",
                               (long)hash_value);
}    

