/// -*- c++ -*-

#ifndef __INDEX_H__
#define __INDEX_H__

// Tools
#include <ToolsConfig.h>
// Storage
#include <NameHash.h>
#include <RTree.h>

/// \addtogroup Storage
/// \@{

/** Base interface for the archiver's indices. */
class Index
{
public:
    Index();
    virtual ~Index();

    /** Modes used for open */
    enum ReadWrite { ReadOnly, ReadAndWrite  };
    
    /** Open an index.
     * 
     *  @exception GenericException on error
     *             (file not found, wrong file format, ...).
     */
    virtual void open(const stdString &filename,
                      ReadWrite readwrite=ReadOnly) = 0;
                
    /** Get the basename
     *  @return Filename of this index.
     *  @see #getDirname()
     */
    const stdString &getFilename()
    {   return filename; }

    /** Get the directory.
     *  @return Dirname of this index.
     *  @see #getFilename()
     */
    const stdString &getDirectory()
    {   return dirname; }
    
    /** @return Full filename, directory and base.
     *  @see #getDirname()
     *  @see #getFilename()
     */
    const stdString &getFullName()
    {   return fullname; }

    /** Close the index. */
    virtual void close() = 0;
    
    /** Return value of addChannel() and findChannel() */
    class Result
    {
    public:
        /** Construct result. Only used inside Index implementation. */
        Result(RTree *tree, const stdString &directory);
        /** Destructor, deletes the RTree pointer. */
        ~Result();

        /** @return RTree part of the result. */
        RTree *getRTree() const
        {   return tree; }
        
        /** @return Base directory for all file names in the RTree. */
        const stdString &getDirectory() const
        {   return directory; }
        
    private:
        // This class maintains the RTree pointer. Prohibit copy.
        PROHIBIT_DEFAULT_COPY(Result);
        RTree *tree;
        stdString directory;
    };
    
    /** Add a channel to the index.
     * 
     *  A channel has to be added before data blocks get defined
     *  for the channel. When channel is already in index, existing
     *  tree gets returned.
     * 
     *  @return Result. Caller must delete.
     *  @exception GenericException on internal error.
     */
    virtual Result *addChannel(const stdString &channel) = 0;

    /** Obtain the RTree for a channel.
     * 
     *  Directory is set to the path/directory of the index,
     *  which together with the data block in the RTree will then
     *  lead to the actual data files.
     * 
     *  @return Result or 0. Caller must delete.
     *  @exception GenericException on internal error.
     */
    virtual Result *findChannel(const stdString &channel) = 0;
    
    /** @see iterator() */
    class NameIterator
    {
    public:
        virtual ~NameIterator();
        
        /** @return true if valid */
        virtual bool isValid() const = 0;
        
        /** @return Current channel name. */
        virtual const stdString &getName() const = 0;
        
        /** Get next entry.
         *  @see isValid
         */
        virtual void  next() = 0;
    };

    /** Get NameIterator, located on first channel.
     * 
     *  @return Returns iterator. Might not be 'valid'. Caller must delete.
     *  @exception GenericException on error.
     */
    virtual NameIterator *iterator() = 0;
protected:
    PROHIBIT_DEFAULT_COPY(Index);
    
    /** Set the filename and dirname from the full path name */
    void setFilename(const stdString &full_name);
    
    /** Clear the filename and dirname. */
    void clearFilename();
    
private:
    /** Basename of filename. */
    stdString filename;
    
    /** Directory part of filename. */
    stdString dirname;

    /** Full filename, directory and base. */
    stdString fullname;
};

/// \@}

#endif
