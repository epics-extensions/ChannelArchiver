
class channel;

class archive
{
public:
	/* Create new archive object.
	   Use open to connect it to existing archive file
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

private:
	stdString _name;
	ArchiveI *_archiveI;
};

