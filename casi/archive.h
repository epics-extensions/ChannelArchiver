
class channel;

class archive
{
public:
	/* Create new archive object.
	   Use open to connect it to existing archive file,
       write to create new one or open for append
	 */
	archive ();

	/* Remove archive object */
	~archive ();

	/* Open archive data source */
	bool open (const char *name);

	/* Similar: Position channel object on first channel in archive */
	bool findFirstChannel (channel &channel);

	/* Try to find channel by given name,
	   make channel class point to that channel.

	   Result: succesful?
	 */
	bool findChannelByName (const char *name, channel &channel);

	/* Find channel by pattern (regular expression) */
	bool findChannelByPattern (const char *pattern, channel &channel);

    const char *name ();
    
    /* Support for write access:
	 * Create new archive or open existing one for appending
     */
	bool write (const char *name, double hours_per_file);

    /* Add a new channel to this archive.
     * It's an error to add a channel name that already exists.
     * This is also only supported after succesfully calling 'write'
     */
    bool addChannel (const char *name, channel &channel);
    
    /* Helper if you want to determine the suggested time
     * for a new file, usually used in conjuntion with value.determineChunk()
     */
    const char *nextFileTime (const char *current_time);
    
private:
	stdString _name;
	ArchiveI *_archiveI;
};


