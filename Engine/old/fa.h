/*
 *	Current Author:		Bob Dalesio
 *	Date:			09-09-96
 *
 *      Experimental Physics and Industrial Control System (EPICS)
 *
 *      Copyright 1991, the Regents of the University of California,
 *      and the University of Chicago Board of Governors.
 *
 * Modification Log
 * ----------------
 * .01	06-29-97	lrd	added archive disable index to channel structure
 * .02	07-11-98	lrd	added disable flag to group structure to support
 *						disable by group
 */

/* as a general comment - all strings could e kept in a  string table and
make all of these files a lot smaller!!!!!!!!
*/
struct	arch_event{
	TS_STAMP	stamp;
	float		frequency;
	unsigned short	nelements;
	unsigned short	dbr_type;
	unsigned short	event_type;
};

/* some time values for timeouts */
static struct timeval   one_second      = { 1,0};
static struct timeval   tenth_second    = { 0,100000};
static struct timeval   twentyth_second    = { 0,50000};
#define NSEC_PER_SEC	1000000000	/* nano seconds per second */
#define MSEC_PER_SEC	1000		/* milli seconds per second */


struct arch_group{
	char		name[80];
	unsigned short	num_channels;
	unsigned short	num_connected;
	TS_STAMP	last_saved;
	Widget		connected_widget;
	Widget		time_widget;
	Widget		enable_widget;
	struct arch_group	*pnext;
	unsigned long	disabled;
};
struct group_list{
	struct arch_group	*parch_group;
	struct group_list	*pnext_group;
};

struct arch_chan{		/* database channel list element */
	unsigned char	connected;
	unsigned char	connected_changed;
	unsigned char	monitor;		/* 1 = archive on change */
	unsigned char	ca_monitored;		/* 1 = monitor issued */
	unsigned short	buf_size;		/* size of a single sample */
	unsigned short	num_bufs_per_block;	/* samples per disk block*/
	unsigned short	nelements;
	unsigned short	num_bufs;		/* size of circ buffer */
	unsigned short	dbr_type;
	unsigned short	head_inx;		/* head of circular buffer */
	unsigned short	tail_inx;		/* tail of curcular buffer */
	unsigned short	overwrites;
	unsigned short	curr_written;
	unsigned short	pend;
	unsigned long	disable_bit;
	unsigned long	disabled;
	unsigned long	dir_offset;		/* directory header offset */
	unsigned long	data_offset;		/* need to point backward - offset */
	char		last_filename[40];	/* 			- filename */
	float		sample_freq;		/* time between saves for periodic set */
	float		write_freq;
	float		curr_freq;		/* frequency to write to disk */
	TS_STAMP	last_saved;
	TS_STAMP	save_time;
	TS_STAMP	last_queue_time;
	chid 		chid;			/* channel access id */
	char		*pbuffer;		/* circular buffer of monitors */
	char		*pname;			/* channel name */
	struct dbr_time_double	*pcurr_value;
	struct dbr_time_double	*ppend_value;
	struct dbr_time_double	*pget_buffer;
	struct arch_chan	*pnext;		/* next arch_list */
	struct arch_chan	*pnext_get;	/* list of chans to 'get' at this freq */
	Widget		chan_widget;
	Widget		status_widget;
	Widget		time_widget;
	Widget		frequency_widget;
	Widget		wfrequency_widget;
/*	Widget		filename_widget; */
	Widget		disable_widget;
	struct group_list	*pgroup_list;
	void		*pctrl_info;	/* pointer to control info */
	short		control_size;	/* size of control info */
	unsigned long	control_offset;	/* disk offset of control info */
};

/* constants for maintaining the circular buffer */
#define	INIT_NUM_BUFS		16
#define	GROWTH_RATE		4
#define	MAX_BUFS_PER_BLOCK	1000

/* constants for curr_value status */
#define	WRITTEN	0
 #define UPDATED	2
