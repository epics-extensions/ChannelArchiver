// -*- c++ -*-
#if !defined(_DATAFILE_H_)
#define _DATAFILE_H_

// System
#include <stdio.h>
// Tools
#include "Filename.h"
// Storage
#include "RawValue.h"

/// \addtogroup Storage
/// @{

/// The DataFile class handles access to the binary data files.
/// One important feature is reference counting.
/// When the ArchiveEngine adds samples, it is very likely
/// to add samples for several channels to the same collection
/// of data files.
/// The DataFile class suppors this:
/// - When referencing a data file for the first time, the file gets opened.
/// - When releasing it, the data file stays open.
/// - During a write cycle, it is likely that at least some of the channels
///   reference the same files, and voila: The file is already open.
/// - Finally, close_all() should be called to close all the data files.
class DataFile
{
public:
    /// Reference a data file
    
    /// \param dirname: Path/directory up to the filename
    /// \param basename: filename inside dirname
    /// \param for_write: open for writing or read-only?
    /// \sa release
    static DataFile *reference(const stdString &dirname,
                               const stdString &basename, bool for_write);

    /// Add reference to current DataFile
    DataFile *reference();

    /// De-reference a data file (Call instead of delete)

    /// \sa close_all
    ///
    ///
    void release();

    /// Returns true if DataFile is writable.
    bool is_writable() const
    { return for_write; }

    /// For synchr. with a file that's actively written
    /// by another prog. is might help to reopen:
    ///
    bool reopen();

    /// Close all data files that are currently open.

    /// Returns true if all files could be closed.
    /// Returns false if at least one data file is still
    /// referenced and thus we couldn't close it.
    static bool close_all(bool verbose=false);

    /// Check if any data files are still open (e.g. at end of program)
    static bool any_still_open();
    
    const stdString &getFilename() {   return filename; }
    const stdString &getDirname () {   return dirname;  }
    const stdString &getBasename() {   return basename; }

    /// Read header at given offset

    /// \return Alloc'ed DataHeader or 0 in case of error.
    /// 
    class DataHeader *getHeader(FileOffset offset);

    /// Add a new DataHeader to the file.
    
    /// \return Alloc'ed header or 0.
    /// The header's data type and buffer size info
    /// will be initialized.
    /// Links (dir, prev, next) need to be configured and saved.
    class DataHeader *addHeader(DbrType dbr_type, DbrCount dbr_count,
                                double period, size_t num_samples);

    /// Add CtrlInfo to the data file

    /// \return true for OK
    /// \param offset is set to offset of the info
    ///
    bool addCtrlInfo(const CtrlInfo &info, FileOffset &offset);
private:
    friend class DataHeader;
    friend class CtrlInfo;
    friend class RawValue;
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

    // The current data file
    FILE * file;
    size_t ref_count;
    bool   for_write;
    stdString filename;
    stdString dirname;
    stdString basename;
};

/// Each data block in the binary data files starts with
/// a DataHeader.
///
class DataHeader
{
public:
    /// Create header for given data file.

    /// DataHeader references and releases the DataFile.
    /// Note that operations like readNext might switch
    /// to another DataFile!
    DataHeader(DataFile *datafile);

    /// Destructor releases the current DataFile.
    ~DataHeader();
    
    enum // Scott Meyers' "enum hack":
    {   FilenameLength = 40     };

    // NOTE: For now, the layout of the following must not
    // change because it defines the on-disk layout (except for byte order)!

    /// The header data
    struct DataHeaderData
    {
        FileOffset      dir_offset;     ///< offset of the old directory entry
        FileOffset      next_offset;    ///< abs. offs. of data header in next buffer
        FileOffset      prev_offset;    ///< abs. offs. of data header in prev buffer
        FileOffset      curr_offset;    ///< rel. offs. from data header to free entry
        unsigned long   num_samples;    ///< number of samples written in this buffer
        FileOffset      ctrl_info_offset;  ///< abs. offset to CtrlInfo
        unsigned long   buf_size;       ///< disk space alloc. for this channel including sizeof(DataHeader)
        unsigned long   buf_free;       ///< remaining space f. channel in this file
        DbrType         dbr_type;       ///< ca type of data
        DbrCount        dbr_count;      ///< array dimension of this data type
        char            pad[4];         ///< to align double period...
        double          period;         ///< period at which the channel is archived (secs)
        epicsTimeStamp  begin_time;     ///< first time stamp of data in this file
        epicsTimeStamp  next_file_time; ///< first time stamp of data in the next file
        epicsTimeStamp  end_time;       ///< last time this file was updated
        char            prev_file[FilenameLength]; ///< basename for prev. buffer
        char            next_file[FilenameLength]; ///< basename for next buffer
    } data;

    /// The currently used DataFile (DataHeader handles ref and release).
    DataFile *datafile;

    /// Offset in current file for this header.
    FileOffset offset;
    
    /// Fill the data with zeroes, invalidate the data.
    void clear();

    /// Is offset valid?
    bool isValid();
    
    /// Returns number of unused samples in buffer
    size_t available();

    /// Returns max. number of values in buffer.
    size_t capacity();
    
    /// Read (and convert) from offset in current DataFile, updating offset.
    bool read(FileOffset offset);

    /// Convert and write to current offset in current DataFile.
    bool write() const;

    /// Read the next data header.
    bool read_next();

    /// Read the previous data header.
    bool read_prev();

    /// Helper to set data.prev_file/offset
    void set_prev(const stdString &basename, FileOffset offset);
     
    /// Helper to set data.next_file/offset
    void set_next(const stdString &basename, FileOffset offset);

    void show(FILE *f);

private:
    bool get_prev_next(const char *name, FileOffset new_offset);
};

/// @}

#endif // !defined(_DATAFILE_H_)
