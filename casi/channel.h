
class value;
class channel
{
public:
	channel ();
	~channel ();

	/* Is this channel valid?
	   The following operations are only allowed
	   when the channel is valid!
	 */
	bool valid (); 

	/* Get name of current channel */
	const char *name (); 

	/* Position on next channel. Result: succesful? */
	bool next (); 

	/* Get first available time stamp of current channel */
	const char *getFirstTime (); 

	/* Get last available time stamp of current channel */
	const char *getLastTime (); 

	/* Get first available value of current channel */
	bool getFirstValue (value &value); 

	/* Get last available value of current channel */
	bool getLastValue (value &value); 

	/* Get first value in archive that follows given time stamp */
	bool getValueAfterTime (const char *time, value &value); 

	/* Get first value in archive that preceeds given time stamp */
	bool getValueBeforeTime (const char *time, value &value); 

	/* Get value in archive that is closest to given time stamp */
	bool getValueNearTime (const char *time, value &value); 

    /* Write support:
     * Lock current buffer, return number of free entries
     * for values of given type
     */
     int lockBuffer (const value &value);

     /* Call this if current buffer is full */
     void addBuffer (const value &value, int value_count);

     /* Add new value, call lock/addBuffer before */
     bool addValue (const value &value);

     /* Call after valued were added to update pointers */
     void releaseBuffer ();
    
private:
	friend class archive;

	void setIter (ArchiveI *archiveI);
	bool testValue (value &value); 
	ArchiveI *_archiveI;
	ChannelIteratorI *_iter;
};
