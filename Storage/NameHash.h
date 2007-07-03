// -*- c++ -*-

#ifndef __NAME_HASH_H__
#define __NAME_HASH_H__

// Tools
#include <stdString.h>
#include <NoCopy.h>
// Storage
#include <FileAllocator.h>

/// \addtogroup Storage
/// \@{

/// A file-based Hash table for strings.
///
/// Beginning at the 'anchor' position in the file,
/// NameHash deposits the start offset of the hash table
/// and the number of table entries.
///
/// Each hash table entry is a file offset that points
/// to the beginnig of the NameHash::Entry list for that
/// hash value.
///
/// One NameHash entry on the disk is stored like this:
/// @code
///     long next
///     long ID
///     short length of name
///     short length of ID_txt
///     char  name[]   // Stored without the delimiting'\0' !!
///     char  ID_txt[] // Stored without the '\0' !!
/// @endcode
/// ID_txt might be "", but the name can never be empty.
class NameHash
{
public:
    
    static const uint32_t anchor_size = 8;
    
    /// Constructor.
    ///
    /// @param anchor: The NameHash will deposit its root pointer there.
    ///                Caller needs to assert that there are anchor_size
    ///                bytes available at that location in the file.
    NameHash(FileAllocator &fa, FileOffset anchor);

    /// Create a new hash table of given size.
    ///
    /// @param ht_size determines the hash table size and should be prime.
    /// @exception GenericException on error.
    void init(uint32_t ht_size=1009);

    /// Attach to existing hash table
    /// @exception GenericException on error.
    void reattach();

    /// Insert name w/ ID
    /// @return True if a new entry was generated, false if an existing
    ///         entry for that name was updated with possibly new information.
    /// @exception GenericException on error.
    bool insert(const stdString &name, const stdString &ID_txt, FileOffset ID);

    /** One entry in the NameHash. */
    class Entry
    {
    public:
        virtual ~Entry();
        
        /** Channel Name. */
        virtual const stdString &getName() const = 0;
        
        /** ID string 
         *  (for index file, that might be the data file name)
         */
        virtual const stdString &getIdTxt() const = 0;
    
        /** ID code 
         *  (for index file, that might be the file offset)
         */
        virtual FileOffset getId() const = 0;
    };

    /// Locate name and obtain its ID.
    /// @return Returns NameInfo or 0 if nothing found. Receiver must delete.
    /// @exception GenericException on internal error.
    Entry *find(const stdString &name);

    /** Iterator over names in the hash. */    
    class Iterator : public Entry
    {
    public:
        virtual ~Iterator();
        
        /** @return true if valid */
        virtual bool isValid() const = 0;
        
        /** Get next entry.
         *  @see isValid
         */
        virtual void next() = 0;
    };
    
    /// Start iterating over all entries (in table's order).
    ///
    /// @return Returns iterator. Might not be 'valid'. Caller must delete.
    /// @exception GenericException on error.
    Iterator *iterator();

    /// Generate info on table fill ratio and list length
    void showStats(FILE *f);
private:
    friend class PrivateIterator;

    PROHIBIT_DEFAULT_COPY(NameHash);
    
    FileAllocator &fa;
    FileOffset anchor;       // Where offset gets deposited in file
    uint32_t ht_size;   // Hash Table size (entries, not bytes)
    FileOffset table_offset; // Start of HT in file
    
    /// Get hash value
    uint32_t hash(const stdString &name) const;  

    /** Seek to hash_value, read offset.
     *  @exception GenericException on read error.
     */
    FileOffset read_HT_entry(uint32_t hash_value) const;
    
    /** Seek to hash_value, write offset.
     *  @exception GenericException on write error.
     */
    void write_HT_entry(uint32_t hash_value, FileOffset offset) const;
};

/// \@}

#endif
