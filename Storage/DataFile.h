// -*- c++ -*-
#if !defined(_DATAFILE_H_)
#define _DATAFILE_H_

#include <stdio.h>
#include "string2cp.h"
#include "Filename.h"
#include "RawValue.h"

/// Each data block in the binary data files starts with
/// a DataHeader.
///
class DataHeader
{
public:
    enum // Scott Meyers' "enum hack":
    {   FilenameLength = 40     };

    // NOTE: For now, the layout of the following must not
    // change because it defines the on-disk layout (except for byte order)!
    FileOffset      dir_offset;     // offset of the directory entry
    FileOffset      next_offset;    // abs. offs. of data header in next buffer
    FileOffset      prev_offset;    // abs. offs. of data header in prev buffer
    FileOffset      curr_offset;    // rel. offs. from data header to curr data
    unsigned long   num_samples;    // number of samples written in this buffer
    FileOffset      ctrl_info_offset;  // abs. offset to CtrlInfo
    unsigned long   buf_size;       // disk space alloc. for this channel including sizeof(DataHeader)
    unsigned long   buf_free;       // remaining space f. channel in this file
    DbrType         dbr_type;       // ca type of data
    DbrCount        nelements;      // array dimension of this data type
    char            pad[4];         // to align double period...
    double          period;         // period at which the channel is archived (secs)
    epicsTimeStamp  begin_time;     // first time stamp of data in this file
    epicsTimeStamp  next_file_time; // first time stamp of data in the next file
    epicsTimeStamp  end_time;       // last time this file was updated
    char            prev_file[FilenameLength];
    char            next_file[FilenameLength];

    /// Fill the data with zeroes.
    void clear();

    /// Read (and convert) from file/offset
    bool read(FILE *file, FileOffset offset);

    /// Convert and write to file/offset
    bool write(FILE *file, FileOffset offset) const;
};

/// The DataFile class handles access to the binary data files.
/// One important feature is the reference counting.
/// When the ArchiveEngine adds samples, it is very likely
/// to add samples for several channels to the same collection
/// of data files.
/// The DataFile class suppors this:
/// - When referencing a data file for the first time, the file gets opened.
/// - When releasing it, the data file stays open.
/// - Then, when the data file is referenced again, it's already open.
/// - Finally, close_all() should be called to close all the data files.
class DataFile
{
public:
    // Max. number of samples per header.
    // Though a header could hold more than this,
    // it's considered an error by e.g. the (old) archive engine:
    enum { MAX_SAMPLES_PER_HEADER = 4000 };

    /// Reference a data file
    
    /// More than one client might use the same DataFile,
    /// so there's no public constructor but a counted
    /// reference mechanism instead.
    /// Get DataFile for given file:
    static DataFile *reference(const stdString &dirname,
                               const stdString &filename, bool for_write);

    /// Add reference to current DataFile
    DataFile *reference();

    /// De-reference a data file (Call instead of delete)

    /// \sa close_all
    ///
    ///
    void release();

    /// For synchr. with a file that's actively written
    /// by another prog. is might help to reopen:
    ///
    bool reopen();

    /// Close all data files that are currently open.

    /// Returns true if all files could be closed.
    /// Returns false if at least one data file is still
    /// referenced and thus we couldn't close it.
    static bool close_all();

    class DataHeaderIterator *getHeader(FileOffset offset);

    /// The current data file:
    FILE * file;
    
    const stdString &getFilename() {   return _filename; }
    const stdString &getDirname () {   return _dirname;  }
    const stdString &getBasename() {   return _basename; }
  
private:
    friend class DataHeaderIterator;

    size_t  _ref_count;

    bool   _file_for_write;
    stdString _filename;
    stdString _dirname;
    stdString _basename;

    // Attach DataFile to disk file of given name.
    // Existing file is opened, otherwise new one is created.
    DataFile(const stdString &dirname,
             const stdString &basename,
             const stdString &filename, bool for_write);

    // Close file.
    ~DataFile();

    // prohibit assignment or implicit copy:
    // (these are not implemented, use reference() !)
    DataFile(const DataFile &other);
    DataFile &operator = (const DataFile &other);
};

/// The DataHeaderIterator iterates over the Headers in DataFiles,
/// switching from one file to the next etc.
///
class DataHeaderIterator
{
public:
    DataHeaderIterator();
    DataHeaderIterator(const DataHeaderIterator &rhs);
    DataHeaderIterator & operator = (const DataHeaderIterator &rhs);
    ~DataHeaderIterator();

    void attach(DataFile *file, FileOffset offset=INVALID_OFFSET,
                DataHeader *header=0);
    void clear();

    // Cast to bool tests if DataHeader is valid.
    // All the other operations are forbidden if DataHeader is invalid!
    bool isValid() const
    {   return header_offset != INVALID_OFFSET;    }

    DataHeader  header;
    
    // Get previous/next header (might be in different data file).
    // Returns invalid iterator if there is no prev./next header (see bool-cast).
    // Careful people might try haveNext/PrevHeader(),
    // since this DataHeaderIterator is toast
    // after stepping beyond the ends of the double-linked header list.
    bool getNext();
    bool getPrev();

    bool haveNextHeader()
    {   return Filename::isValidFilename(header.next_file);    }

    bool havePrevHeader()
    {   return Filename::isValidFilename(header.prev_file);    }

    // Re-read the current DataHeader
    void sync();

    // Save the current DataHeader
    void save()
    {   header.write(datafile->file, header_offset); }

    FileOffset getOffset() const
    {   return header_offset; }

    FileOffset getDataOffset() const;

private:
    DataFile    *datafile;
    FileOffset  header_offset; // valid or INVALID_OFFSET

    void init();

    bool getHeader(FileOffset position);
};

#endif // !defined(_DATAFILE_H_)
