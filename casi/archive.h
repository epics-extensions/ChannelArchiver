class value;
class channel;

class archive
{
public:
   archive ();
   /* Create new archive object.
      Use open to connect it to existing archive file,
      write to create new one or open for append
   */

   ~archive ();
   /* Remove archive object */

   bool open (const char *name);
   /* Open archive data source */

   bool findFirstChannel (channel &channel);
   /* Similar: Position channel object on first channel in archive */

   bool findChannelByName (const char *name, channel &channel);
   /* Try to find channel by given name,
      make channel class point to that channel.

      Result: succesful?
   */

   bool findChannelByPattern (const char *pattern, channel &channel);
   /* Find channel by pattern (regular expression) */

   const char *name ();
   /* return the name of the (already opened) archive file */
    
   bool write (const char *name, double hours_per_file);
   /* Support for write access:
    * Create new archive or open existing one for appending
    */

   bool addChannel (const char *name, channel &channel);
   /* Add a new channel to this archive.
    * It's an error to add a channel name that already exists.
    * This is also only supported after succesfully calling 'write'
    */
    
   const char *nextFileTime (const char *current_time);
   /* Helper if you want to determine the suggested time
    * for a new file, usually used in conjuntion with value.determineChunk()
    */


   void newValue (int type, int count, value &val);
   /* Create a new value with the given type and count, suitable for this
      type of archive (will fail on e.g. MultiArchive!) */

private:
   stdString _name;
   ArchiveI *_archiveI;
};


