#include <stdio.h>

/* non - channel access events to the archiver */
/* some are archived - some are directives     */
#define	ARCH_NO_VALUE		0x0f00
#define	ARCH_EST_REPEAT		0x0f80
#define	ARCH_DISCONNECT		0x0f40
#define	ARCH_STOPPED		0x0f20
#define	ARCH_REPEAT		0x0f10
#define	ARCH_DISABLED		0x0f08
#define	ARCH_CHANGE_WRITE_FREQ	0x0f04
#define	ARCH_CHANGE_FREQ	0x0f02
#define	ARCH_CHANGE_SIZE	0x0f01

struct	disk_data_header{
	unsigned long	dir_offset;		/* offset of the directory entry */
	unsigned long	next_offset;		/* absolute offset of data header in next buffer */
	unsigned long	prev_offset;		/* absolute offset of data header in prev buffer */
	unsigned long	curr_offset;		/* relative offset from data header to curr data */
	unsigned long	num_samples;		/* number of samples written in this buffer */
	unsigned long	config_offset;		/* dbr_ctrl information */
	unsigned long	buf_size;		/* disk space allocated for this channel */
	unsigned long	buf_free;		/* disk space remaining for this channel in this file */
	unsigned short	dbr_type;		/* ca type of data */
	unsigned short	nelements;		/* array dimension of this data type */
	double		save_frequency;		/* frequency at which the channel is archived */
	TS_STAMP	begin_time;		/* first time stamp of data in this file */
	TS_STAMP	next_file_time;		/* first time stamp of data in the next file */
	TS_STAMP	last_wrote_time;	/* last time this file was updated */
	char		prev_file[40];
	char		next_file[40];
};
struct fetch_context{
	struct disk_data_header	curr_header;
	char			filename[80];
	unsigned long		curr_inx;
	FILE			*curr_fd;
	unsigned long		value_size;
	unsigned long		curr_offset;
	char			config_info[512];		
};



/* directory entry for a channel */
#define	dirHashTableSize  256
struct directory_entry{
	char		name[80];		/* channel name */
	unsigned long	next_entry_offset;	/* offset of the next channel in the directory */
	unsigned long	last_offset;		/* offset of the last buffer saved for this channel */
	unsigned long	first_offset;		/* offset of the first buffer saved for this channel */
	TS_STAMP	create_time;
	TS_STAMP	first_save_time;
	TS_STAMP	last_save_time;
	char		last_file[40];		/* filename where the last buffer was saved */
	char		first_file[40];		/* filename where the last buffer was saved */
};

#define	NUMBER_VALUE	1
#define	ENUM_VALUE	2
struct arch_number{
	float	disp_high;	/* high display range */
	float	disp_low;	/* low display range */
	float	low_warn;
	float	low_alarm;
	float	high_warn;
	float	high_alarm;
	int	prec;		/* display precision */
	char	*units;
};
struct arch_enum{
	short	num_states;
	char	*state_strings;
};
union arch_value{
	struct	arch_number	analog;
	struct	arch_enum	index;
};
struct ctrl_info{
	short			size;
	short			type;
	union	arch_value	value;
};
