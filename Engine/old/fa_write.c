/*
 *
 *	Current Author:		Bob Dalesio
 *	Date:			02-08-97
 *
 *      Experimental Physics and Industrial Control System (EPICS)
 *
 *      Copyright 1991, the Regents of the University of California,
 *      and the University of Chicago Board of Governors.
 *
 * Modification Log:
 * -----------------
 * extracted from fa_arch.c
 */
/* note - fix crash caused by deb's config file!!!
*/
#include	<stdlib.h>
#include	<cadef.h>
#include	<fdmgr.h>
#include	"hash.h"
#include	"freq_arch.h"
#include 	"fa_display.h"
#include	"fa.h"

extern unsigned long	time_per_file;	/* time saved in each file 4 hrs */


extern int	debug;

/* if there is a repeat event at the end of a full  disk region 			*/
/* that is - there have been no repeat events and a repeat event occurs as the last 	*/
/* event of the file the allocated area will be overrun by 2 events -  			*/
/* one event for a disconnect and the other for a repeat block 				*/
/* so the disk buffer needs to be able to handle 2 extra events				*/
#define		REP_OVER	2

/* current archive file information */
char		directory_name[120] = "";
FILE		*dir_fd = 0;

/* other global parameters */
TS_STAMP	last_check_time;

init_disk_data_header(
struct disk_data_header	*pdisk_data_header,
struct arch_chan	*parch_chan,
TS_STAMP		*ptime,
FILE			*data_fd)
{
	TS_STAMP	day;
	unsigned long	end_buf_flag = 0x3456;

	pdisk_data_header->next_offset = 0;
	pdisk_data_header->prev_offset = parch_chan->data_offset;
	pdisk_data_header->dir_offset = parch_chan->dir_offset;
	pdisk_data_header->num_samples = 0;
	pdisk_data_header->buf_size = sizeof(struct disk_data_header) +
	  (parch_chan->buf_size * parch_chan->num_bufs_per_block);
	if (parch_chan->curr_freq == 0.0)
		parch_chan->curr_freq = parch_chan->sample_freq;
	pdisk_data_header->save_frequency = parch_chan->curr_freq;
	pdisk_data_header->begin_time = *ptime;
	pdisk_data_header->next_file[0] = 0;
	pdisk_data_header->config_offset = parch_chan->control_offset;
	strcpy(pdisk_data_header->prev_file,parch_chan->last_filename);
	pdisk_data_header->dbr_type = parch_chan->dbr_type;
	pdisk_data_header->nelements = parch_chan->nelements;
	pdisk_data_header->curr_offset = 0;
	pdisk_data_header->buf_free = 
	  pdisk_data_header->buf_size - sizeof(struct disk_data_header);

	/* compute the next time a file will need to be created */
	day = *ptime;
	day.secPastEpoch++;	/* in case we are right on midnight */
	tsRoundUpLocal(&day,(unsigned long)time_per_file);
	pdisk_data_header->next_file_time = day;

	/* write the header, end of data area flag and seek to data area */
	fseek(data_fd,0,SEEK_END);
	parch_chan->data_offset = ftell(data_fd);
	fwrite(pdisk_data_header,sizeof(struct disk_data_header),1,data_fd);
	fseek(data_fd,pdisk_data_header->buf_free-sizeof(long),SEEK_CUR);
	fwrite(&end_buf_flag,sizeof(long),1,data_fd);
	fseek(data_fd,
	  parch_chan->data_offset+sizeof(struct disk_data_header),SEEK_SET);
}

write_config_data(
struct arch_chan	*parch_chan,
FILE			*data_fd)
{
	/* write the control information on the end of the file */
	fseek(data_fd,0,SEEK_END);
	parch_chan->control_offset = ftell(data_fd);
	fwrite(parch_chan->pctrl_info,parch_chan->control_size,1,data_fd);
}


update_directory(
struct arch_chan	*parch_chan,
TS_STAMP		*ptime)
{
	struct directory_entry	directory_entry;
	struct directory_entry	*pdirectory_entry = &directory_entry;

	if (dir_fd == 0){
		if ((dir_fd = fopen(directory_name,"r+b")) == NULL){
			printf("could not open directory %s - help!\n",directory_name);
			return(-1);
		}
	}
	fseek(dir_fd,parch_chan->dir_offset,SEEK_SET);
	fread(pdirectory_entry,sizeof(struct directory_entry),1,dir_fd);
	strcpy(pdirectory_entry->last_file,parch_chan->last_filename);
 	pdirectory_entry->last_offset = parch_chan->data_offset;
	pdirectory_entry->last_save_time = *ptime;
	if (pdirectory_entry->first_file[0] == 0){
		strcpy(pdirectory_entry->first_file,parch_chan->last_filename);
		pdirectory_entry->first_save_time = *ptime;
		pdirectory_entry->first_offset = parch_chan->data_offset;
	}
 	fseek(dir_fd,parch_chan->dir_offset,SEEK_SET);
	fwrite(pdirectory_entry,sizeof(struct directory_entry),1,dir_fd);
}

fix_data_header(
struct disk_data_header	*pdisk_data_header,
struct directory_entry	*pdir_entry,
struct arch_chan	*parch_chan)
{
	struct disk_data_header	curr_data_header;
	unsigned long		curr_offset = 0;
	char			curr_filename[120];
	struct disk_data_header	prev_data_header;
	unsigned long		prev_offset = 0;
	char			prev_filename[120];
	FILE			*mend_fd;
	unsigned short 		done = 0;
	char			buf[120],tbuf[120];
	TS_STAMP		time;
	unsigned short		circ_ref = 0;

	if (circ_ref == 0){
		printf("bad header - needs repairing\n");
		return;
	}

	/* find the last good header */
	if ((mend_fd = fopen(pdir_entry->first_file,"r")) == NULL){
		printf("could not open data file *%s*\n",pdir_entry->first_file);
		return(0);
	}
	fseek(mend_fd,pdir_entry->first_offset,SEEK_SET);
	fread(&curr_data_header,1,sizeof(struct disk_data_header),mend_fd);
	fclose(mend_fd);
	prev_filename[0] = 0;
	curr_filename[0] = 0;
	while ( ((mend_fd = fopen(curr_data_header.next_file,"r")) != NULL) && (!done)){
		if (disk_data_header_ok(&curr_data_header)){
			bcopy(&curr_data_header,&prev_data_header,sizeof(struct disk_data_header));
			if ((curr_offset == curr_data_header.next_offset) &&
			  (strcmp(curr_filename,curr_data_header.next_file) == 0)){
				done = 1;
				circ_ref = 1;
			}else{
				strcpy(prev_filename,curr_filename);
				prev_offset = curr_offset;
				strcpy(curr_filename,curr_data_header.next_file);
				curr_offset = curr_data_header.next_offset;
				fseek(mend_fd,curr_data_header.next_offset,SEEK_SET);
				fread(&curr_data_header,1,sizeof(struct disk_data_header),mend_fd);
				done = 0;
			}
		}else{
			done = 1;
		}
		fclose(mend_fd);
	}

	if (disk_data_header_ok(&curr_data_header) && (!circ_ref)) return(0);

	sprintf(buf,"%s has bad data header in %s at %x\n",
	  parch_chan->pname,pdir_entry->last_file,pdir_entry->last_offset);
	arch_log_msg(buf);
	tsLocalTime(&time);
	tsStampToText(&time,TS_TEXT_MMDDYY,tbuf);
	sprintf(buf,"\tDate fixed: %s\n",tbuf);
	arch_log_msg(buf);
	
	/* fix the data header */
	if (circ_ref){
		strcpy(buf,"\tcircular reference found\n");
		arch_log_msg(buf);
	}
 	if (prev_filename[0]){
		sprintf(buf,"\tfirst header with problem in %s at %x\n",
		  curr_filename,curr_offset);
		arch_log_msg(buf);
		sprintf(buf,"\tnew last header in %s at %x\n",prev_filename,prev_offset);
		arch_log_msg(buf);
		mend_fd = fopen(prev_filename,"r+b");
		fseek(mend_fd,prev_offset,SEEK_SET);
		bcopy(&prev_data_header,pdisk_data_header,sizeof(struct disk_data_header));
		pdisk_data_header->next_file[0] = 0;
		pdisk_data_header->next_offset = 0;
		fwrite(pdisk_data_header,1,sizeof(struct disk_data_header),mend_fd);
		fclose(mend_fd);
	/* problem in first data header */
	}else{
		sprintf(buf,"\tthe first data header was bad - no data available\n");
		arch_log_msg(buf);
		pdir_entry->first_offset = 0;
		pdir_entry->first_file[0] = 0;
	}

	/* fix the directory entry */
	sprintf(buf,"\tupdate directory entry at %x\n",parch_chan->dir_offset);
	arch_log_msg(buf);
	fseek(dir_fd,parch_chan->dir_offset,SEEK_SET);
	fread(pdir_entry,1,sizeof(struct directory_entry),dir_fd);
	strcpy(pdir_entry->last_file,prev_filename);
	pdir_entry->last_offset = prev_offset;
	fseek(dir_fd,parch_chan->dir_offset,SEEK_SET);
	fwrite(pdir_entry,1,sizeof(struct directory_entry),dir_fd);
	parch_chan->data_offset = pdir_entry->last_offset;
	strcpy(parch_chan->last_filename,pdir_entry->last_file);

	return(0);
}
arch_log_msg(char	*buf)
{
	FILE			*log_fd;
	char			log_filename[120];

	printf("%s",buf);
	strcpy(log_filename,directory_name);
	strcat(log_filename,".log");
	if ((log_fd = fopen(log_filename,"r+b")) == NULL){
		if ((log_fd = fopen(log_filename,"w+b")) == NULL){
			printf("could not create log file %s\n",log_filename);
			return(0);
		}
	}else{
		fseek(log_fd,0,SEEK_END);
	}
	fwrite(buf,1,strlen(buf),log_fd);
	fclose(log_fd);
}

tsToFilename(
char	*filename,
char	*ts)
{
	while(*ts){
		if (*ts == '/') *filename = '-';
		else if (*ts == ' ') *filename = '-';
		else if (*ts == '.'){
			*filename = 0;
			return;
		}
		else if (*ts != ':') *filename = *ts;
		if (*ts != ':') filename++;
		ts++;
	}
}

