// ASCII read/write routines
//
// When there is more time,
// these should become part of an ASCIIArchive/ChannelIterator/..
// class collection like the BinArchive/... family.

void output_ascii (const stdString &archive_name, const stdString &channel_name,
	const osiTime &start, const osiTime &end);

void input_ascii (const stdString &archive_name, const stdString &file_name);

