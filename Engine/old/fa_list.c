/*
 *
 *	Current Author:		Bob Dalesio
 *	Date:			02-09-98
 *
 *      Experimental Physics and Industrial Control System (EPICS)
 *
 *      Copyright 1991, the Regents of the University of California,
 *      and the University of Chicago Board of Governors.
 *
 * Modification Log:
 * -----------------
 * .00	lrd	02-07-98	Broken off from fa_arch.c
 * .01	lrd	02-11-98	for previously archived channels - create buffers here and now - not later
 * .02	lrd	02-23-98	keep one data fd open for the addition of an entire configuration file
 * .03	lrd	05-02-98	make config value a larger character array - filenames were overflowing it
*/
#include	<stdlib.h>
#include	<cadef.h>
#include	<fdmgr.h>
#include	"freq_arch.h"
#include 	"fa_display.h"
#include	"fa.h"

extern int	debug;
extern unsigned long	write_frequency;
extern unsigned short	disable_bit;
extern TS_STAMP	last_check_time;
extern double	default_freq;

extern FILE	*dir_fd;
FILE		*chan_data_fd = 0;
char		chan_data_file[120];

extern void dbCaLinkConnectionHandler();

int			num_channels;
struct arch_chan	*pchan_list = 0;
struct arch_group	*pgroup_list = 0;
struct arch_chan	*plast_arch_chan = 0;

set_save_time(
struct arch_chan	*parch_chan,
TS_STAMP		*pmon_time)
{
	double		round;
	unsigned long	sec_time_round;	
	unsigned long	nsec_time_round;

	parch_chan->save_time = *pmon_time;
	round = parch_chan->sample_freq;
	sec_time_round  = round;
	round -= sec_time_round;
	nsec_time_round = (round * NSEC_PER_SEC);
	nsec_time_round = nsec_time_round % NSEC_PER_SEC;

	if (nsec_time_round){
		parch_chan->save_time.nsec += nsec_time_round;
		parch_chan->save_time.nsec 
		  -= (parch_chan->save_time.nsec % nsec_time_round);
	}else{
		parch_chan->save_time.secPastEpoch += sec_time_round;
		parch_chan->save_time.secPastEpoch 
		  -= (parch_chan->save_time.secPastEpoch % sec_time_round);
		parch_chan->save_time.nsec = 0;
	}
}

add_channel(
char			*pchan_name,
float			sample_frequency,
struct arch_group	*parch_group,
unsigned short		disable_flag,
int			monitor)
{
	unsigned short		hash_offset;
	unsigned short		found = 0;
	unsigned long		chan_offset,last_chan_offset;
	struct directory_entry	dir_entry;	/* directory entry     */
	struct directory_entry	*pdir_entry = &dir_entry;
	struct directory_entry	new_dir_entry;	/* new directory entry */
	struct directory_entry	*pnew_dir_entry = &new_dir_entry;
	struct arch_chan	*parch_chan;
	struct disk_data_header	disk_data_header;
	struct disk_data_header	*pdisk_data_header = &disk_data_header;
	struct arch_event	arch_event;
	struct group_list	*pgroup_list;

	/* if this is already active - return */
	if ( (found = find_channel(dir_fd,pchan_name,pdir_entry,
	  &hash_offset,&chan_offset,&last_chan_offset)) 
	  != 0){
		parch_chan = pchan_list;
		while (parch_chan != 0){
			if (parch_chan->dir_offset == chan_offset){
				/* add the config group to this channel */
				if ((pgroup_list = (struct group_list *)calloc(1,sizeof(struct group_list))) == 0){
					printf("malloc failed\n"); 
					return(-1);
				}
				pgroup_list->parch_group = parch_group;
				pgroup_list->pnext_group =
				  parch_chan->pgroup_list;
				parch_chan->pgroup_list = pgroup_list;
				parch_group->num_channels++;
				if (parch_chan->connected)
					parch_group->num_connected++;
				return(0);
			}
			parch_chan = parch_chan->pnext;
		}
	/* add this name to the directory */
	}else{
		/* initialize the new entry */
		bzero(pnew_dir_entry,sizeof(struct directory_entry));
		pnew_dir_entry->next_entry_offset = 0xffffffff;
		strcpy(pnew_dir_entry->name,pchan_name);
		pnew_dir_entry->last_offset = 0xffffffff;
		tsLocalTime(&pnew_dir_entry->create_time);

		/* add the channel to the directory */
		fseek(dir_fd,0,SEEK_END);
		chan_offset = ftell(dir_fd);
		fwrite(pnew_dir_entry,1,sizeof(struct directory_entry),dir_fd);

		/* store it's offset in the hash table - or link to it */
		if (last_chan_offset == 0xffffffff){
			fseek(dir_fd,hash_offset*sizeof(unsigned long),SEEK_SET);
			fwrite(&chan_offset,1,sizeof(unsigned long),dir_fd);
		}else{
			/* point the last channel to this one */
			pdir_entry->next_entry_offset = chan_offset;
			fseek(dir_fd,last_chan_offset,SEEK_SET);
			fwrite(pdir_entry,1,sizeof(struct directory_entry),dir_fd);
		}
	}

	/* initialize channel info */
	if ((parch_chan = (struct arch_chan *)calloc(1,sizeof (struct arch_chan)))
	  == (struct arch_chan *)0){
		printf("create arch_chan failed\n");
		return(-1);
 	}
	if ((parch_chan->pname = (char *)calloc(1,strlen(pchan_name)+1)) == 0){
		printf("malloc chan_name failed\n");
		return(-1);
 	}
	strcpy(parch_chan->pname,pchan_name);
	parch_chan->dir_offset = chan_offset;
	parch_chan->num_bufs_per_block = INIT_NUM_BUFS;
	parch_chan->monitor = monitor;
	parch_chan->sample_freq = sample_frequency;
	if (parch_chan->sample_freq > write_frequency)
		parch_chan->write_freq = parch_chan->sample_freq;
	else
		parch_chan->write_freq = write_frequency;

	/* for preexisting channels - this data is on disk */
	if ((found) && (pdir_entry->last_file[0] != 0)){
		/* need to put in a time asleep event */
		/* get the last time the value was written */
		parch_chan->data_offset = pdir_entry->last_offset;
		strcpy(parch_chan->last_filename,pdir_entry->last_file);
		if (strcmp(chan_data_file,parch_chan->last_filename) != 0){
			if (chan_data_fd != 0) fclose(chan_data_fd);
			if ((chan_data_fd = fopen(parch_chan->last_filename,"r")) == NULL){
				printf("%x: cannot open this data file %s\n",
				  parch_chan,parch_chan->last_filename);
				return(-1);
			}
			strcpy(chan_data_file,parch_chan->last_filename);
		}
		fseek(chan_data_fd,parch_chan->data_offset,SEEK_SET);
		fread(pdisk_data_header,sizeof(struct disk_data_header),1,chan_data_fd);
		if (!disk_data_header_ok(pdisk_data_header))
			fix_data_header(pdisk_data_header,pdir_entry,parch_chan);

		/* init parch_chan */
		parch_chan->last_saved = pdisk_data_header->last_wrote_time;
		parch_chan->last_queue_time
		  = pdisk_data_header->last_wrote_time;
		parch_chan->nelements = pdisk_data_header->nelements;
		parch_chan->dbr_type = pdisk_data_header->dbr_type;
		if (!make_arch_bufs(parch_chan)) return(0);
		if (!make_circ_buf(parch_chan)) return(0);

		/* set the next save time to the next frequency */
		/* we need to round up from the time stamp of the last monitor first */
		/* for archive sets - we always use the last frequency */
		set_save_time(parch_chan,&pdisk_data_header->last_wrote_time);

		/* then advance it to the next save frequency */
		tsAddDouble(&parch_chan->save_time,&parch_chan->save_time
		  ,parch_chan->sample_freq);

		/* add an archive start status to the file */
		/* set the time stamp to one frequency past the last saved value */
		arch_event.event_type = ARCH_STOPPED;
		arch_event.stamp = parch_chan->last_saved;
		tsAddDouble(&arch_event.stamp,&arch_event.stamp,parch_chan->sample_freq);
		archiver_event(parch_chan,&arch_event);
	}

	/* is this channel used to disable archiving */
	if (disable_flag){
		if (parch_chan->disable_bit == 0){
			parch_chan->disable_bit = disable_bit;
			disable_bit *= 2;  /* left shift the disable bit */
		}
	}

	/* link it into the active list */
	if (plast_arch_chan == NULL){
		pchan_list = parch_chan;
	}else{
		plast_arch_chan->pnext = parch_chan;
	}
	plast_arch_chan = parch_chan;
if (debug) printf("put channel in config group\n");
	/* add the config group to this channel */
	if ((pgroup_list = (struct group_list *)calloc(1,sizeof(struct group_list))) == 0){
		printf("malloc failed\n"); 
		return(-1);
	}
	pgroup_list->parch_group = parch_group;
	pgroup_list->pnext_group = parch_chan->pgroup_list;
	parch_chan->pgroup_list = pgroup_list;
	parch_group->num_channels++;
	num_channels++;

	tsLocalTime(&last_check_time);

	/* connect over channel access */
	SEVCHK(ca_search_and_connect(pchan_name,&(parch_chan->chid),
	  dbCaLinkConnectionHandler,(void *)parch_chan),NULL);
if (debug) printf("done add channel\n");
}

/*
 * create a data set
 */
int create_data_set(
char    	*filename, 		/* configuration file */
char		*directoryname)		/* name of the directory to use */
{
	FILE   		*inp_fd;
	int		status;
	char		temp[120],temp1[120],temp2[120];
	float		frequency;
	int		monitor = 0;
	int		i;
	unsigned long	offset = 0xffffffff;
	struct arch_group	*parch_group;
	TS_STAMP	time,lasttime;

	/* does this config file already exist */
	parch_group = pgroup_list;
	while (parch_group){
		if (strcmp(parch_group->name,filename) == 0){
			printf("group already active\n");
			return;
		}
		parch_group = parch_group->pnext;
	}

	/* open the configuration file */
	if ((inp_fd = fopen(filename, "r")) == NULL){
		printf("create_data_set: unable to open file %s\n",filename);
		return (-1);
	}

	/* create a new archive group */
	if ((parch_group = (struct arch_group *)calloc(1,sizeof(struct arch_group))) == (struct arch_group *)0){
		printf("Unable to malloc memory for configuration group %s\n",filename);
		return(-1);
	}
	strcpy(parch_group->name,filename);
	/* add it to the group list */
	parch_group->pnext = pgroup_list;
	pgroup_list = parch_group;
if (debug) printf("in add group\n");

	/* open the directory */
	if (dir_fd == 0){
		if ((dir_fd = fopen(directoryname,"r+b")) == NULL){
			if ((dir_fd = fopen(directoryname,"w+b")) == NULL){
				printf("Unable to create directory file %s\n",directoryname);
				fclose(inp_fd);
				return (-1);
			}else{
				for (i = 0; i < dirHashTableSize; i++)
					fwrite(&offset,1,sizeof(unsigned long),dir_fd);
			}
		}
	}

	/* get the next non-comment, non-config parameter line */
	while (get_line(&temp[0],&temp1[0],&temp2[0],inp_fd) != EOF){
		/* get any frequency that's specified */
		if (isdigit(temp1[0]) || temp1[0] == '.'){
			status = sscanf(temp1,"%f",&frequency);
		}else{
			frequency = default_freq;
		}

		/* archive on change? */
		monitor = 0;
		if ((temp1[0] == 'M') || (temp2[0] == 'M')
		  || (temp1[0] == 'm') || (temp2[0] == 'm'))
			monitor = 1;

		/* Disable channels stop archiving when non-zero */
		if ((temp1[0] == 'D') || (temp2[0] == 'D')
		  || (temp1[0] == 'd') || (temp2[0] == 'd'))
			add_channel(temp,frequency,parch_group,1,monitor);
		else
			add_channel(temp,frequency,parch_group,0,monitor);
	}

	fclose(inp_fd);
	fclose(dir_fd);
	dir_fd = 0;
	if (chan_data_fd){
		fclose(chan_data_fd);
		chan_data_fd = 0;
		chan_data_file[0] = 0;
	}
	return(0);
}
get_line(
char	*pstring1,
char	*pstring2,
char	*pstring3,
FILE   	*inp_fd
)
{
	char		inp_char = ' ';
	char		config_param[80];
	char		config_value[80];
	unsigned short	inx;

	*pstring1 = 0;
	*pstring2 = 0;
	*pstring3 = 0;
	while(1){
		/* skip white space */
		while((inp_char != EOF) && isspace(inp_char)) inp_char = getc(inp_fd);
		if (inp_char == EOF) return(EOF);

		/* we have the first character */
		/* if it is a comment - skip the line */
		if (inp_char == '#'){
			while(inp_char != '\n'){
				inp_char = getc(inp_fd);
			}

		/* do we have a pv */
		}else if (isalpha(inp_char) || isdigit(inp_char)){
			while(!isspace(inp_char) && (inp_char != EOF)){
				*pstring1 = inp_char;
				pstring1++;
				inp_char = getc(inp_fd);
			}
			*pstring1 = 0;
			*pstring2 = 0;
			*pstring3 = 0;

			while(isspace(inp_char)){
				if (inp_char == '\n') return;
				inp_char = getc(inp_fd);
			}
			while(!isspace(inp_char) && (inp_char != EOF)){
				*pstring2 = inp_char;
				pstring2++;
				inp_char = getc(inp_fd);
			}
			*pstring2 = 0;

			while(isspace(inp_char)){
				if (inp_char == '\n') return;
				inp_char = getc(inp_fd);
			}
			while(!isspace(inp_char) && (inp_char != EOF)){
				*pstring3 = inp_char;
				pstring3++;
				inp_char = getc(inp_fd);
			}
			*pstring3 = 0;

			while((inp_char != '\n') && (inp_char != EOF))
				inp_char = getc(inp_fd);
			return(1);

		/* otherwise it is a special character and we assume it's a config param */
		}else{
			inp_char = getc(inp_fd);
			while(isspace(inp_char) && (inp_char != EOF))
				inp_char = getc(inp_fd);
			inx = 0;
			while((inp_char != EOF) && !isspace(inp_char)){ 
				config_param[inx] = inp_char;
				inx++;
				inp_char = getc(inp_fd);
			}
			config_param[inx] = 0;

			inx = 0;
			while(isspace(inp_char) && (inp_char != EOF))
				inp_char = getc(inp_fd);
			if ((inp_char != '\n') && (inp_char != EOF)){
				while((inp_char != EOF) && !isspace(inp_char)){ 
					config_value[inx] = inp_char;
					inx++;
					inp_char = getc(inp_fd);
				}
				config_value[inx] = 0;
if (debug) printf("config %s %s\n",config_param,config_value);
				set_config(config_param,config_value);
			}
		}
	}
}
