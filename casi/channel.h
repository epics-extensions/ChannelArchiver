
class value;
class channel
{
public:
   channel ();
   ~channel ();

   bool valid () const; 
   /* Is this channel valid?
      The following operations are only allowed
      when the channel is valid!
   */

   const char *name () const; 
   /* Get name of current channel */

   bool next (); 
   /* Position on next channel. Result: succesful? */

   const char *getFirstTime () const; 
   /* Get first available time stamp of current channel */

   const char *getLastTime () const; 
   /* Get last available time stamp of current channel */

   bool getFirstValue (value &value) const; 
   /* Get first available value of current channel */

   bool getLastValue (value &value) const; 
   /* Get last available value of current channel */

   bool getValueAfterTime (const char *time, value &value) const; 
   /* Get first value in archive that follows given time stamp */

   bool getValueBeforeTime (const char *time, value &value) const; 
   /* Get first value in archive that preceeds given time stamp */

   bool getValueNearTime (const char *time, value &value) const; 
   /* Get value in archive that is closest to given time stamp */

   int lockBuffer (const value &value);
   /* Write support:
    * Lock current buffer, return number of free entries
    * for values of given type
    */

   void addBuffer (const value &value, int value_count);
   /* Call this if current buffer is full */

   bool addValue (const value &value);
   /* Add new value, calls lock/addBuffer if needed */

   void releaseBuffer ();
   /* Call after valued were added to update pointers */
    
private:
#ifndef SWIG
   friend class archive;
#endif

   void setIter (ArchiveI *archiveI);
   bool testValue (value &value) const; 
   ArchiveI *_archiveI;
   ChannelIteratorI *_iter;

   size_t buf_size;
   static const size_t init_buf_size;
   static const size_t max_buf_size;
   static const size_t buf_size_fact;
};
