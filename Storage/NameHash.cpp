// -*- c++ -*-
// Tools
#include <BinIO.h>
#include <MsgLogger.h>
// Index
#include "NameHash.h"

bool NameHash::Entry::read(FILE *f)
{
    char buffer[100];
    short len;
    if (!(fseek(f, offset, SEEK_SET)==0 &&
          readLong(f, &next) &&
          readLong(f, &ID) &&
          readShort(f, &len)))
    {
        LOG_MSG("NameHash: Cannot read entry @ 0x%lX\n", offset);
        return false;
    }
    if (len >= (short)sizeof(buffer)-1)
    {
        LOG_MSG("NameHash: Entry's name (%d) exceeds buffer size\n",
                (int)len);
        return false;
    }
    if (fread(buffer, len, 1, f) != 1)
    {
        LOG_MSG("NameHash: Read error for entry @ 0x%lX\n", offset);
        return false;
    }
    name.assign(buffer, len);
    return (short)name.length() == len;
}

bool NameHash::Entry::write(FILE *f) const
{
    return fseek(f, offset, SEEK_SET)==0 &&
        writeLong(f, next) &&
        writeLong(f, ID) &&
        writeShort(f, name.length()) &&
        fwrite(name.c_str(), name.length(), 1, f) == 1;
}

NameHash::NameHash(FileAllocator &fa, Offset anchor)
        : fa(fa), anchor(anchor), ht_size(0), table_offset(0)
{}

bool NameHash::init(long ht_size)
{
    this->ht_size = ht_size;
    if (!(table_offset = fa.allocate(4*ht_size)))
    {
        LOG_MSG("NameHash::init: Cannot allocate hash table\n");
        return false;
    }
    int i;
    if (fseek(fa.getFile(), table_offset, SEEK_SET))
    {
        LOG_MSG("NameHash::init: Cannot seek to hash table\n");
        return false;   
    }
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
    
bool NameHash::insert(const stdString &name, long ID)
{
    long h = hash(name);
    Entry entry;
    if (!read_HT_entry(h, entry.offset))
        return false;
    if (entry.offset == 0)
    {   // First entry for this hash value
        entry.name = name;
        entry.ID = ID;
        entry.next = 0;
        if (!(entry.offset = fa.allocate(entry.getSize())))
        {
            LOG_MSG("NameHash::insert: Cannot allocate entry for %s\n",
                    name.c_str());
            return false;
        }
        return entry.write(fa.getFile()) && write_HT_entry(h, entry.offset);
    }    
    while (true)
    {
        if (!entry.read(fa.getFile()))
            return false;
        if (entry.name == name)
        {   // Update existing entry
            entry.ID = ID;
            return entry.write(fa.getFile());
        }
        if (!entry.next)
            break;
        entry.offset = entry.next;
    }
    // Add new entry to end of list for current hash value
    Entry new_entry;
    new_entry.name = name;
    new_entry.ID = ID;
    new_entry.next = 0;
    if (!(new_entry.offset = fa.allocate(new_entry.getSize())))
    {
        LOG_MSG("NameHash::insert: Cannot allocate new entry for %s\n",
                name.c_str());
        return false;
    }
    entry.next = new_entry.offset;
    return new_entry.write(fa.getFile()) && entry.write(fa.getFile());
}

bool NameHash::find(const stdString &name, long &ID)
{
   long h = hash(name);
   Entry entry;
   read_HT_entry(h, entry.offset);
   while (entry.offset && entry.read(fa.getFile()))
   {
       if (entry.name == name)
       {
           ID = entry.ID;
           return true;
       }
       entry.offset = entry.next;
   }
   return false;
}

bool NameHash::startIteration(long &hashvalue, Entry &entry)
{
    if (fseek(fa.getFile(), table_offset, SEEK_SET))
    {
        LOG_MSG("NameHash::startIteration: Seek failed\n");
        return false;   
    }
    for (hashvalue=0; hashvalue<ht_size; ++hashvalue)
    {   // Find first uses entry in hash table
        if (!readLong(fa.getFile(), &entry.offset))
        {
            LOG_MSG("NameHash::startIteration: Cannot read hash table\n");
            return false;
        }
        if (entry.offset)
            break;
    }
    return entry.offset && entry.read(fa.getFile()); // return that entry
}  

bool NameHash::nextIteration(long &hashvalue, Entry &entry)
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
long NameHash::hash(const stdString &name) const
{
    const char *c = name.c_str();
    short h = 0;
    while (*c)
        h = (128*h + *(c++)) % ht_size;
    return (long)h;
}

void NameHash::showStats(FILE *f)
{
    long l, used_entries = 0, total_list_length = 0, max_length = 0, hashvalue;
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
            used_entries, ht_size, used_entries*100/ht_size);
    fprintf(f, "Average list length  : %ld entries\n",
            total_list_length / used_entries);
    fprintf(f, "Maximum list length  : %ld entries\n", max_length);
}

bool NameHash::read_HT_entry(long hash_value, Offset &offset)
{
    LOG_ASSERT(hash_value >= 0 && hash_value < ht_size);
    Offset o = table_offset + hash_value*4;
    if (!(fseek(fa.getFile(), o, SEEK_SET)==0 &&
          readLong(fa.getFile(), &offset)))
    {
        LOG_MSG("NameHash::read_HT_entry: Cannot read entry %ld\n",
                hash_value);
        return false;
    }
    return true;
}    

bool NameHash::write_HT_entry(long hash_value, Offset offset) const
{
    LOG_ASSERT(hash_value >= 0 && hash_value < ht_size);
    Offset o = table_offset + hash_value*4;
    if (!(fseek(fa.getFile(), o, SEEK_SET)==0 &&
          writeLong(fa.getFile(), offset)))
    {
        LOG_MSG("NameHash::write_HT_entry: Cannot write entry %ld\n",
                hash_value);
        return false;
    }
    return true;
}    

