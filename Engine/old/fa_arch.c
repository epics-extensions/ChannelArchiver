/*
 *
 *	Current Author:		Bob Dalesio
 *	Date:			09-09-96
 *
 *      Experimental Physics and Industrial Control System (EPICS)
 *
 *      Copyright 1991, the Regents of the University of California,
 *      and the University of Chicago Board of Governors.
 *
 * Modification Log:
 * -----------------
 * .01	lrd	06-28-97	changed config parser to include global parameters
 *				for write rate, default archive rate, num channels on channel
 *				display and the time between writing new files
 * .02	lrd	06-29-97	added archive disable
 * .03	lrd	06-29-97	fixed a bug - change of frequency causes new buffer in same file -
 *				next filename was not being written to the old buffer prior to
 *				starting the new one.
 * .04	lrd	08-24-97	add use of ca_get for slow channels - check scan lists in main
 * .05	lrd	12-15-97	reduce disk usage by starting with less and adding as needed
 * .06	lrd	12-16-97	support archive on change
 * .07	lrd	01-21-98	make repeat counts use last value and 
 *				put a new estimate in each new file
 * .08	lrd	01-28-98	add "group" directive for including other config
 *				groups
 * .09	lrd	02-06-98	retry on .07 - need to save parch_chan->buf_size
 * .10	lrd	02-08-98	moved routines for list and writing disk into 2 different files
 * .11	lrd	02-11-98	for previously archived channels - creates buffers at init
 */
#include	<stdlib.h>
#include	<cadef.h>
#include	<fdmgr.h>
#include	"hash.h"
#include	"freq_arch.h"
#include 	"fa_display.h"
#include	"fa.h"

extern	char	directory_name[120];
extern	FILE	*dir_fd;
TS_STAMP	curr_file_time;
int file_cnt = 0;

extern struct faGuiInfo	arGuiInfo;
fdctx	*pfdctx;	/* file descriptor manager id */
extern unsigned short	connections_changed;

int	debug = 0;
int	time_debug = 0;


/* configuration parameters */
double		default_freq	= 1.0;	/* save no more frequently than every n seconds */
unsigned long	write_frequency = 30;
int		max_chan_displayed = 20;
unsigned long	time_per_file =	22400;	/* time saved in each file 4 hrs */
float		get_threshhold = 20.0;	/* scans >= to this use ca_get - less are monitored */
float		get_check = 1.0;	/* ca_get scan lists checked at this interval */
int		archiver_exit = 0;
unsigned short	stop_taking_events = 0;
unsigned short	disable_bit = 1;

/* other global parameters */
TS_STAMP	last_check_time;
chid 		date_chid;		/* channel access id */

/* lists fo channels being archived */
extern int		num_channels;
extern struct arch_chan	*pchan_list;

set_config(
char	*pparam,
char	*pvalue
)
{
	float			value;
	int			status;
	struct arch_event	arch_event;
	struct arch_chan	*parch_chan;

	status = sscanf(pvalue,"%f",&value);
	if (strcmp("write_freq",pparam) == 0){
		write_frequency = value;
		if (write_frequency < 1) write_frequency = 1;
		parch_chan = pchan_list;
		while (parch_chan != 0){
			/* add change frequency event for every channel */
			arch_event.event_type = ARCH_CHANGE_WRITE_FREQ;
			arch_event.stamp = parch_chan->save_time;
			arch_event.frequency = value;
			archiver_event(parch_chan,&arch_event);

			/* next channel */
			parch_chan = parch_chan->pnext;
		}

	}else if (strcmp("save_default",pparam) == 0){
		default_freq = value;
	}else if (strcmp("display_size",pparam) == 0){
		max_chan_displayed = value;
	}else if (strcmp("file_size",pparam) == 0){
		time_per_file = (value * 3600);
	}else if (strcmp("group",pparam) == 0){
		create_data_set(pvalue,directory_name);
	}else{
		printf("config %s is not a valid configuration parameter\n",pparam);
	}
}
			

/* main loop */
main(
int	argc,
char	**argv)
{
	TS_STAMP	time,lastscan;
	char		tbuf[120],buf[120];
	FILE		*active_fd;

	tsLocalTime(&time);
	tsStampToText(&time,TS_TEXT_MMDDYY,tbuf);
	sprintf(buf,"chan_arch started on %s\n",tbuf);

if ((active_fd = fopen("archive_active","r+b")) == NULL){
	/* create the new file */
	if ((active_fd = fopen("archiver_active","w+b")) == NULL){
		printf("save_file - unable to create file archive active\n");
		return;
	}
}else{
	fread(tbuf,1,40,active_fd);
	if(tbuf[0] != 0){
		printf("%s - is still active or was not properly exitted\n");
		printf("delete file - archiver_acitve - to recover\n");
		printf("first verify that the archvier is not active\n");
		return;
	}
}

fwrite(tbuf,1,strlen(tbuf),active_fd);
fclose(active_fd);

	
if(debug) printf("ca init\n");
	ca_init();
if(debug) printf("motif init\n");
	motif_init(&argc,argv);

if(debug) printf("parse args\n");
	/* parse args for default frequency */
	if (argc < 2){
		printf("Usage: freq_arch config_file<,directory_file>\n");
		return;
	}
	if (argc > 2)
		strcpy(directory_name,argv[2]);
	else
		strcpy(directory_name,"freq_directory");
	arch_log_msg(buf);
if(debug) printf("create set\n");
	create_data_set(argv[1],directory_name);
if(debug) printf("create display\n");
	my_display();
	lastscan.secPastEpoch = 0;
if(debug) printf("done init - do updates\n");

	for(;;){

		tsLocalTime(&time);

		/* wait for .2 seconds or channel access event */
		/* we wait this little so that the ca_get channels are < 50 msec */
		/* away from the time stamp required for the get */
if(debug) printf("fdmgr pend\n");
		fdmgr_pend_event(pfdctx, &one_second);
if(debug) printf("fdmgr pend done\n");

		/* fetch data with ca_gets that are too slow to monitor */
		if ((time.secPastEpoch - lastscan.secPastEpoch) >= get_check){
if(debug) printf("update - get list\n");
			check_get_list(time.secPastEpoch);
			lastscan.secPastEpoch = time.secPastEpoch;
if(debug) printf("update - get list done\n");
		}

		/* allow channel access fd events and monitors to be serviced */
		/* searches, monitor requests and puts are serviced by the capollfunc */
if (debug) printf("ca pend event\n");
		ca_pend_event(.000000000001);
if(debug) printf("update - connection status\n");
		if (connections_changed){
			connections_changed = 0;
			update_time_conn();
		}
if (debug) printf("back from update time conn\n");

		/* allow the motif events to be handled */
if (debug) printf("X pend event\n");
		while (XtAppPending(arGuiInfo.arApp)) {
                  XtAppProcessEvent(arGuiInfo.arApp, XtIMAll);
                }

 		/* look to write buffers at the highest write frequency */
		/* should this be whenever a buffer is .5 full? - in addition */
if(debug) printf("check monitor time\n");
		if ((time.secPastEpoch - last_check_time.secPastEpoch) >= write_frequency){
if(debug) printf("update - archive\n");
			check_archive(&time);

			/* last check time is modified after the archiving is done */
			/* if there is slowness in the file writing - we will check */
			/* it less frequently - thus overwriting the archive circular */
			/* buffer - thus causing events to be discarded at the monitor */
			/* receive callback */
			tsLocalTime(&last_check_time);
		}
		if (archiver_exit){
			tsLocalTime(&time);
			tsStampToText(&time,TS_TEXT_MMDDYY,tbuf);
			sprintf(buf,"chan_arch shutdown on %s\n",tbuf);
			arch_log_msg(buf);
if ((active_fd = fopen("archive_active","r+b")) != NULL){
	tbuf[0] = 0;
	fwrite(tbuf,1,1,active_fd);
	fclose(active_fd);
}

			return(0);
		}
if(debug) printf("ca flush\n");
		ca_flush_io();
if(debug) printf("ca flush done\n");
	}
}


check_archive(
TS_STAMP	*ptime){
    TS_STAMP			day;
    struct arch_chan		*parch_chan;
    unsigned long		time_expired;
    struct dbr_time_double	*pvalue;
    char	estimate[1024];	/* need storage for estimating repeat counts */
    struct dbr_time_double	temp;		/* need to read for estimated repeats */
    unsigned short		first_call;
    struct disk_data_header	disk_data_header;
    struct disk_data_header	*pdisk_data_header = &disk_data_header;
    unsigned long		offset;
    char			time_string[120],buf[120];
    struct group_list		*pgroup_list;
    unsigned short		disk_writes = 0;
TS_STAMP	time,lasttime;
    char	curr1_file[120];
    char	curr2_file[120];
    FILE	*curr1_fd = 0;
    FILE	*curr2_fd = 0;
    char	new_file[120];
    FILE	*data_fd = 0;

    curr1_file[0] = curr2_file[0] = new_file[0] = 0;
    parch_chan = pchan_list;

  while (parch_chan != 0){
	/* save when the channel has been initialized - or has been archived before */
	time_expired = ptime->secPastEpoch - parch_chan->last_saved.secPastEpoch;
	if ((time_expired < parch_chan->write_freq) || (parch_chan->pbuffer == NULL)){
		parch_chan = parch_chan->pnext;
		continue;
	}
 
	/* write the circular buffer to disk */
	first_call = 1;

	/* read in the data header */
	/* from the current file */
	if (parch_chan->last_filename[0] != 0){
		if (strcmp(parch_chan->last_filename,curr1_file) != 0){
			if (curr1_fd){
				fclose(curr1_fd);
file_cnt--;
			}
			if ((curr1_fd = fopen(parch_chan->last_filename,"r+b")) != NULL){
				strcpy(curr1_file,parch_chan->last_filename);
file_cnt++;
			}else{
				curr1_file[0] = 0;
				printf("%s: could not open data file %s - ignore\n"
				  ,parch_chan->last_filename);
				continue;
			}
		}
		data_fd = curr1_fd;
		fseek(data_fd,parch_chan->data_offset,SEEK_SET);
		fread(pdisk_data_header,1,sizeof(struct disk_data_header),data_fd);
		fseek(data_fd,pdisk_data_header->curr_offset,SEEK_CUR);
	}

	/* process events from the circular buffer */
	while (get_freq_event(parch_chan,&pvalue,first_call,&estimate)){
		/* is this the first time this channel is being archived? */
		if (parch_chan->last_filename[0] == 0){
if (debug) printf("%s first time archived\n");
			/* create a new file for this time */
			day = pvalue->stamp;
			tsRoundDownLocal(&day,(unsigned long)time_per_file);
			curr_file_time = day;
			tsStampToText(&day,TS_TEXT_MMDDYY,time_string);
			tsToFilename(&new_file,time_string);
			if (strcmp(new_file,curr1_file) != 0){
				strcpy(curr1_file,new_file);
				if (curr1_fd){
					fclose(curr1_fd);
file_cnt--;
				}

				/* open the current file */
				if ((curr1_fd = fopen(curr1_file,"r+b")) == NULL){
					/* create the new file */
					if ((curr1_fd = fopen(curr1_file,"w+b")) == NULL){
						printf("save_file - unable to create file %s",
						  curr1_file);
						return;
					}
				}
file_cnt++;
			}
			data_fd = curr1_fd;

			/* write out the control information */
			write_config_data(parch_chan,data_fd);

			/* write out the new data buffer header */
			init_disk_data_header(pdisk_data_header,parch_chan,&pvalue->stamp,data_fd);
			strcpy(parch_chan->last_filename,curr1_file);

			/* update the directory file */
			update_directory(parch_chan,&pvalue->stamp);

		/* do we need to create a new file */
		}else if (pdisk_data_header->next_file_time.secPastEpoch 
		  <= pvalue->stamp.secPastEpoch){
if (debug) printf("%s create a new file\n",parch_chan->pname);
			/* create a new file for this time */
			day = pvalue->stamp;
			tsRoundDownLocal(&day,(unsigned long)time_per_file);
			tsStampToText(&day,TS_TEXT_MMDDYY,time_string);
			curr_file_time = day;
			tsToFilename(&new_file,time_string);
				
			/* open the current file */
			if (strcmp(curr2_file,new_file) != 0){
				/* did we cross over more than 1 file while taking frequency events */
				/* if so - we need to set curr1 to curr2 - it is now the previous file */
				if (data_fd != curr1_fd){
					fclose(curr1_fd);
file_cnt--;
					curr1_fd = data_fd;		/* data_fd and curr2_fd must be the same */
					strcpy(curr1_file,curr2_file);
				/* otherwise close the next file */
				}else if (curr2_fd){
					fclose(curr2_fd);
file_cnt--;
				}

				/* the new next file */
				strcpy(curr2_file,new_file);
				if ((curr2_fd = fopen(new_file,"r+b")) == NULL){
					/* create the new file */
					if ((curr2_fd = fopen(new_file,"w+b")) == NULL){
						printf("save_file - unable to create file %s",
						  new_file);
						return;
					}
				}
file_cnt++;
			}

			/* make the new file the current file */
			data_fd = curr2_fd;

			/* write out the control information */
			/* this goes onto the new disk - before the data header */
			/* so it must preceded the next group of code */
			write_config_data(parch_chan,data_fd);

			/* finish the old file */
			pdisk_data_header->last_wrote_time = pvalue->stamp;
			fseek(data_fd,0,SEEK_END);
			pdisk_data_header->next_offset = ftell(data_fd);
			strcpy(pdisk_data_header->next_file,curr2_file);
			fseek(curr1_fd,parch_chan->data_offset,SEEK_SET);
			fwrite(pdisk_data_header,sizeof(struct disk_data_header),1,curr1_fd);

			/* write out the new data buffer header */
			if ((parch_chan->num_bufs_per_block > INIT_NUM_BUFS)
			  && (pdisk_data_header->buf_free > 
			  ((parch_chan->num_bufs_per_block/GROWTH_RATE) * parch_chan->buf_size))){
				parch_chan->num_bufs_per_block /= GROWTH_RATE;
			}
			init_disk_data_header(pdisk_data_header,parch_chan,&pvalue->stamp,data_fd);
			strcpy(parch_chan->last_filename,curr2_file);

			/* update the directory file */
			update_directory(parch_chan,&pvalue->stamp);
		/* do we need to create a new buffer area in the same file */
		}else if((pvalue->severity & ARCH_CHANGE_FREQ) == ARCH_CHANGE_FREQ){
if (debug) printf("%s change frequency\n",parch_chan->pname);
			parch_chan->curr_freq = pvalue->value;

			pdisk_data_header->last_wrote_time = pvalue->stamp;
			fseek(data_fd,0,SEEK_END);
			pdisk_data_header->next_offset = ftell(data_fd);
			strcpy(pdisk_data_header->next_file,parch_chan->last_filename);
			fseek(data_fd,parch_chan->data_offset,SEEK_SET);
			fwrite(pdisk_data_header,sizeof(struct disk_data_header),1,data_fd);

			/* write out the new data buffer header */
			init_disk_data_header(pdisk_data_header,parch_chan,&pvalue->stamp,data_fd);

			/* update the directory file */
			update_directory(parch_chan,&pvalue->stamp);

		/* do we need to create a new larger area for this channel */
		}else if (pdisk_data_header->buf_free < parch_chan->buf_size){
if (debug) printf("%s need larger disk area\n",parch_chan->pname);

			/* we used up the available space */
			/* increase the number of buffers to allocate */
			if (parch_chan->num_bufs_per_block < MAX_BUFS_PER_BLOCK)
				parch_chan->num_bufs_per_block *= GROWTH_RATE;
			pdisk_data_header->last_wrote_time = pvalue->stamp;
			fseek(data_fd,0,SEEK_END);
			pdisk_data_header->next_offset = ftell(data_fd);
			strcpy(pdisk_data_header->next_file,parch_chan->last_filename);
			fseek(data_fd,parch_chan->data_offset,SEEK_SET);
			fwrite(pdisk_data_header,sizeof(struct disk_data_header),1,data_fd);

			/* write out the new data buffer header */
			init_disk_data_header(pdisk_data_header,
			  parch_chan,&pvalue->stamp,data_fd);

			/* update the directory file */
			update_directory(parch_chan,&pvalue->stamp);
		}

		/* overwrite estimated repeat counts in the buffer - except the first one */
		if ( ((pvalue->severity & ARCH_EST_REPEAT) == ARCH_EST_REPEAT)
		  || ((pvalue->severity & ARCH_REPEAT) == ARCH_REPEAT) ){
if (debug) printf("%s write repeat event - repeat %d\n",parch_chan->pname,pvalue->status);
			if (pdisk_data_header->num_samples > 0){
				offset = ftell(data_fd);
				fseek(data_fd,-parch_chan->buf_size,SEEK_CUR);
				fread(&temp,sizeof(struct dbr_time_double),1,data_fd);
				if ((temp.severity & ARCH_EST_REPEAT) == ARCH_EST_REPEAT){
					pvalue->status += temp.status; /* accumulate repeat count */
					fseek(data_fd,-sizeof(struct dbr_time_double),SEEK_CUR);
					fwrite(pvalue,parch_chan->buf_size,1,data_fd);
				}else{
 					fseek(data_fd,offset,SEEK_SET);
					fwrite(pvalue,parch_chan->buf_size,1,data_fd);
					pdisk_data_header->num_samples++;
					pdisk_data_header->buf_free -= parch_chan->buf_size;
					pdisk_data_header->curr_offset += parch_chan->buf_size;
				}
			}else{
				fwrite(pvalue,parch_chan->buf_size,1,data_fd);
				pdisk_data_header->num_samples++;
				pdisk_data_header->buf_free -= parch_chan->buf_size;
				pdisk_data_header->curr_offset += parch_chan->buf_size;
			}
		/* write the event/monitor to disk - as long as it's not a change frequency */
		}else if (pvalue->severity != ARCH_CHANGE_FREQ){
if (debug) printf("%s write buffer to disk\n");
			fwrite(pvalue,parch_chan->buf_size,1,data_fd);
			pdisk_data_header->num_samples++;
			pdisk_data_header->buf_free -= parch_chan->buf_size;
			pdisk_data_header->curr_offset += parch_chan->buf_size;
		}
		first_call = 0;
		disk_writes++;
	}

	if (archiver_exit){
		if (curr1_fd){
			fclose (curr1_fd);
			file_cnt--;
		}
		if (curr2_fd){
			fclose(curr2_fd);
			file_cnt--;
		}
		if (dir_fd)   fclose(dir_fd);
		return;
	}

	/* write the updated data header - if there was an event */
	if (first_call == 0){
		parch_chan->last_saved = *ptime;
		pgroup_list = parch_chan->pgroup_list;
		while (pgroup_list){
			if (pgroup_list->parch_group->last_saved.secPastEpoch 
			  < pvalue->stamp.secPastEpoch)
				pgroup_list->parch_group->last_saved = pvalue->stamp;
			pgroup_list = pgroup_list->pnext_group;
		}
		pdisk_data_header->last_wrote_time = pvalue->stamp;
		fseek(data_fd,parch_chan->data_offset,SEEK_SET);
		fwrite(pdisk_data_header,sizeof(struct disk_data_header),1,data_fd);
	}
	/* check the next channel being archived */
	parch_chan->overwrites = 0;
	parch_chan = parch_chan->pnext;
  }

  /* show the new save time */
  if (disk_writes)	update_time_conn();

  if (curr1_fd){
	fclose(curr1_fd);
	file_cnt--;
  }
  if (curr2_fd){
	fclose(curr2_fd);
	file_cnt--;
  }
  if (dir_fd){
	fclose(dir_fd);
	dir_fd = 0;
  }
  if (file_cnt != 0){
	tsLocalTime(&time);
	tsStampToText(&time,TS_TEXT_MMDDYY,time_string);
	sprintf(buf,"file_cnt = %d at %s\n",file_cnt,time_string);
	arch_log_msg(buf);
	file_cnt = 0;
  }
}

