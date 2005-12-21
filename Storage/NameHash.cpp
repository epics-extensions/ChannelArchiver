// -*- c++ -*-
// Tools
#include <BinIO.h>
#include <MsgLogger.h>
// Index
#include "NameHash.h"

// On the disk, entries are stored like this:
// long next
// long ID
// short length of name
// short length of ID_txt
// char  name[]   // Stored without the delimiting'\0' !!
// char  ID_txt[] // Stored without the '\0' !!
// ID_txt might be "", name never.

FileOffset NameHash::Entry::getSize() const
{
    return 4 + 4 + 2 + 2 + name.length() + ID_txt.length();
}

bool NameHash::Entry::write(FILE *f) const
{
    if (!(fseek(f, offset, SEEK_SET)==0 &&
          writeLong(f, next) &&
          writeLong(f, ID) &&
          writeShort(f, name.length()) &&
          writeShort(f, ID_txt.length())))
        return false;
    if (fwrite(name.c_str(), name.length(), 1, f)!=1)
        return false;
    size_t len = ID_txt.length();
    if (len > 0  &&  fwrite(ID_txt.c_str(), len, 1, f)!=1)
        return false;
    return true;
}

bool NameHash::Entry::read(FILE *f)
{
    char buffer[100];
    unsigned short name_len, ID_len;
    if (!(fseek(f, offset, SEEK_SET)==0 &&
          readLong(f, &next) &&
          readLong(f, &ID) &&
          readShort(f, &name_len) &&
          readShort(f, &ID_len)))
    {
        LOG_MSG("NameHash: Cannot read entry @ 0x%lX\n",
                (unsigned long)offset);
        return false;
    }
    if (name_len >= sizeof(buffer)-1)
    {
        LOG_MSG("NameHash: Entry's name (%d) exceeds buffer size\n",
                (int)name_len);
        return false;
    }
    if (ID_len >= sizeof(buffer)-1)
    {
        LOG_MSG("NameHash: Entry's ID_txt (%d) exceeds buffer size\n",
                (int)ID_len);
        return false;
    }
    if (fread(buffer, name_len, 1, f) != 1)
    {
        LOG_MSG("NameHash: Read error for name of entry @ 0x%lX\n",
                (unsigned long)offset);
        return false;
    }
    name.assign(buffer, name_len);
    if (name.length() != name_len)
    {
        LOG_MSG("NameHash: Error for name of entry @ 0x%lX\n",
                (unsigned long)offset);
        return false;
    }
    if (ID_len > 0)
    {
        if (fread(buffer, ID_len, 1, f) != 1)
        {
            LOG_MSG("NameHash: Read error for ID_txt of entry @ 0x%lX\n",
                    (unsigned long)offset);
            return false;
        }
        ID_txt.assign(buffer, ID_len);
        if (ID_txt.length() != ID_len)
        {
            LOG_MSG("NameHash: Error for ID_txt of entry @ 0x%lX\n",
                    (unsigned long)offset);
            return false;
        }
    }
    else
        ID_txt.assign(0, 0);
    return true;
}

NameHash::NameHash(FileAllocator &fa, FileOffset anchor)
        : fa(fa), anchor(anchor), ht_size(0), table_offset(0)
{}

bool NameHash::init(uint32_t ht_size)
{
    this->ht_size = ht_size;
    if (!(table_offset = fa.allocate(4*ht_size)))
    {
        LOG_MSG("NameHash::init: Cannot allocate hash table\n");
        return false;
    }
    if (fseek(fa.getFile(), table_offset, SEEK_SET))
    {
        LOG_MSG("NameHash::init: Cannot seek to hash table\n");
        return false;   
    }
    uint32_t i;
    for (i=0; i<ht_size; ++i)
        if (!writeLong(fa.getFile(), 0))
        {
            LOG_MSG("NameHash::init: Cannot write to hash table\n");
            return false;
        }
    if (fseek(fa.getFile(), anchor, SEEK_SET) != 0 ||
        !writeLong(fa.getFile(), table_offset) ||
        !writeLong(fa.getFile(), ht_size))
    {
        LOG_MSG("NameHash::init: Cannot write anchor info\n");
        return false;   
    }
    return true;
}

bool NameHash::reattach()
{
    if (fseek(fa.getFile(), anchor, SEEK_SET) != 0 ||
        !readLong(fa.getFile(), &table_offset) ||
        !readLong(fa.getFile(), &ht_size))
    {
        LOG_MSG("NameHash::readLong: Cannot read anchor info\n");
        return false;   
    }
    return true;
}
    
bool NameHash::insert(const stdString &name,
                      const stdString &ID_txt, FileOffset ID)
{
    LOG_ASSERT(name.length() > 0);
    uint32_t h = hash(name);
    Entry entry;
    if (!read_HT_entry(h, entry.offset))
        return false;
    FILE *f = fa.getFile();
    if (entry.offset == 0)
    {   // First entry for this hash value
        entry.name   = name;
        entry.ID_txt = ID_txt;
        entry.ID     = ID;
        entry.next   = 0;
        if (!(entry.offset = fa.allocate(entry.getSize())))
        {
            LOG_MSG("NameHash::insert: Cannot allocate entry for %s\n",
                    name.c_str());
            return false;
        }
        return entry.write(f) && write_HT_entry(h, entry.offset);
    }    
    while (true)
    {
        if (!entry.read(f))
            return false;
        if (entry.name == name)
        {   // Update existing entry
            entry.ID_txt = ID_txt;
            entry.ID     = ID;
            return entry.write(f);
        }
        if (!entry.next)
            break;
        entry.offset = entry.next;
    }
    // Add new entry to end of list for current hash value
    Entry new_entry;
    new_entry.name   = name;
    new_entry.ID_txt = ID_txt;
    new_entry.ID     = ID;
    new_entry.next   = 0;
    if (!(new_entry.offset = fa.allocate(new_entry.getSize())))
    {
        LOG_MSG("NameHash::insert: Cannot allocate new entry for %s\n",
                name.c_str());
        return false;
    }
    entry.next = new_entry.offset;
    return new_entry.write(f) && entry.write(f);
}

bool NameHash::find(const stdString &name, stdString &ID_txt, FileOffset &ID)
{
    LOG_ASSERT(name.length() > 0);
    uint32_t h = hash(name);
    Entry entry;
    FILE *f = fa.getFile();
    read_HT_entry(h, entry.offset);
    while (entry.offset && entry.read(f))
    {
        if (entry.name == name)
        {
            ID_txt = entry.ID_txt;
            ID     = entry.ID;
            return true;
        }
        entry.offset = entry.next;
    }
    return false;
}

bool NameHash::startIteration(uint32_t &hashvalue, Entry &entry)
{
    FILE *f = fa.getFile();
    if (fseek(f, table_offset, SEEK_SET))
    {
        LOG_MSG("NameHash::startIteration: Seek failed\n");
        return false;   
    }
    for (hashvalue=0; hashvalue<ht_size; ++hashvalue)
    {   // Find first uses entry in hash table
        if (!readLong(f, &entry.offset))
        {
            LOG_MSG("NameHash::startIteration: Cannot read hash table\n");
            return false;
        }
        if (entry.offset)
            break;
    }
    return entry.offset && entry.read(f); // return that entry
}  

bool NameHash::nextIteration(uint32_t &hashvalue, Entry &entry)
{
    if (entry.next) // Is another entry in list for same hashvalue?
        entry.offset = entry.next;
    else
        for (++hashvalue; hashvalue<ht_size; ++hashvalue)
        {   // Find next used entry in hash table
            if (!read_HT_entry(hashvalue, entry.offset))
            {
                LOG_MSG("NameHash::nextIteration: Cannot read hash table\n");
                return false;
            }
            if (entry.offset)
                break;
        }
    return hashvalue<ht_size && entry.offset && entry.read(fa.getFile());
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
    Entry entry;
    for (hashvalue=0; hashvalue<ht_size; ++hashvalue)
    {
        if (!read_HT_entry(hashvalue, entry.offset))
        {
            LOG_MSG("NameHash::show_stats: Cannot read hash table\n");
            return;
        }
        if (entry.offset)
        {
            ++used_entries;
            l = 0;
            do
            {
                if (!entry.read(fa.getFile()))
                {
                    LOG_MSG("NameHash::show_stats: Cannot read entry\n");
                    return;
                }
                ++l;
                ++total_list_length;
                entry.offset = entry.next;
            }
            while (entry.offset);
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

bool NameHash::read_HT_entry(uint32_t hash_value, FileOffset &offset)
{
    LOG_ASSERT(hash_value >= 0 && hash_value < ht_size);
    FileOffset o = table_offset + hash_value*4;
    if (!(fseek(fa.getFile(), o, SEEK_SET)==0 &&
          readLong(fa.getFile(), &offset)))
    {
        LOG_MSG("NameHash::read_HT_entry: Cannot read entry %ld\n",
                (long)hash_value);
        return false;
    }
    return true;
}    

bool NameHash::write_HT_entry(uint32_t hash_value,
                              FileOffset offset) const
{
    LOG_ASSERT(hash_value >= 0 && hash_value < ht_size);
    FileOffset o = table_offset + hash_value*4;
    if (!(fseek(fa.getFile(), o, SEEK_SET)==0 &&
          writeLong(fa.getFile(), offset)))
    {
        LOG_MSG("NameHash::write_HT_entry: Cannot write entry %ld\n",
                (long)hash_value);
        return false;
    }
    return true;
}    

