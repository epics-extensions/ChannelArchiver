/*
 *
 *	Current Author:		Bob Dalesio
 *	Date:			07-27-96
 *
 *      Experimental Physics and Industrial Control System (EPICS)
 *
 *      Copyright 1991, the Regents of the University of California,
 *      and the University of Chicago Board of Governors.
 *
 * Modification Log:
 * -----------------
 * .01	lrd	01-10-98	now prints enumerated string instead of index
 * .02	lrd	01-23-98	make retrieval library
 * .03	lrd	02-06-98	get_values uses control info for min and max now
 * .04	lrd	02-06-98	only open files on new file not change of data block
 * .05	lrd	02-06-98	go_bac_to_start and count_events replaced by
 *				find_time_range
 */
#include	<cadef.h>
#include	<fdmgr.h>
#include	<malloc.h>
#include	<stdlib.h>
#include	<string.h>
#include	"hash.h"
#include	"freq_arch.h"

#define NSEC_PER_SEC	1000000000	/* nano seconds per second */
#define MSEC_PER_SEC	1000		/* milli seconds per second */

data_ok(
struct disk_data_header	*pheader,
struct dbr_time_string	*pdbr_time_str)
{
	if (pdbr_time_str->stamp.secPastEpoch == 0) /*pheader->begin_time.secPastEpoch) 
 ||
	  (pdbr_time_str->stamp.secPastEpoch > pheader->last_wrote_time.secPastEpoch)

)*/
		return(0);
	else
		return(1);
}
config_data_ok(
struct ctrl_info	*pctrl_info)
{
 	if(pctrl_info->type == NUMBER_VALUE){
 		if (pctrl_info->value.analog.prec > 10) return(0);
 		return(1);
	}else if (pctrl_info->type == ENUM_VALUE){
		if (pctrl_info->value.index.num_states > 30) return(0);
 		return(1);
	}else{
 		return(0);
	}
}
disk_data_header_ok(
struct disk_data_header	*pheader)
{
	if( (pheader->begin_time.secPastEpoch != 0) &&
	  (pheader->last_wrote_time.secPastEpoch != 0) &&
	  (pheader->num_samples != 0) &&
	  (pheader->num_samples <= 4000) &&
	  (pheader->buf_size != 0) && 
	  (pheader->buf_free <= pheader->buf_size) &&
	  (((pheader->next_file[0] == 0) && (pheader->next_offset == 0))|| (pheader->next_file[0] != 0) ) &&
	  (((pheader->prev_file[0] == 0) && (pheader->prev_offset == 0)) || (pheader->prev_file[0] != 0)) &&
	  (pheader->dbr_type < DBR_GR_STRING) &&
	  (pheader->dir_offset != 0) &&
	  (pheader->save_frequency > 0.0))
		return(1);
	else
		return(0);
}
report_data_header_error(
struct disk_data_header	*pheader)
{
	if(pheader->begin_time.secPastEpoch == 0)
		printf("\tbegin time 0\n");
	if(pheader->last_wrote_time.secPastEpoch == 0)
		printf("\twrote time 0\n");
	if (pheader->num_samples == 0)
		printf("\tnum samples 0\n");
	if (pheader->num_samples <= 4000)
		printf("\tnum samples > 4000\n");
	if (pheader->buf_size == 0)
		printf("\tbuf size 0\n"); 
	if (pheader->buf_free > pheader->buf_size)
		printf("\tbuf free exceeds buf size\n");
	if ((pheader->next_file[0] == 0) && (pheader->next_offset != 0))
		printf("\tnext file 0 next offset !0\n");
	if ((pheader->next_file[0] != 0) && (pheader->next_offset == 0))
		printf("\tnext file !0 next offset 0\n");
 	if ((pheader->prev_file[0] != 0) && (pheader->prev_offset == 0))
		printf("\tprev file !0 prev offset 0\n");
 	if ((pheader->prev_file[0] == 0) && (pheader->prev_offset != 0))
		printf("\tprev file 0 prev offset !0\n");
 	if (pheader->dbr_type >= DBR_GR_STRING)
		printf("\tdbr type > %d\n",DBR_GR_STRING);
	if (pheader->dir_offset == 0)
		printf("\tdir offset 0\n");
	if (pheader->save_frequency <= 0.0)
		printf("\tsave freq <= 0\n");
  }

rounded_dbr_size_n(
unsigned short	dbr_type,
unsigned short	nelements)
{
	int	buf_size;

	buf_size = dbr_size_n(dbr_type,nelements);
	/* need to make the buffer size be a properly structure aligned number */
	/* for pointer references later !!!!!!!!!!!!!!!!!                      */
	if (buf_size % 8)
		buf_size += 8 - (buf_size % 8);
	return(buf_size);
}


unsigned short hash( char *pname, int length)
{
    unsigned char h0=0;
    unsigned char h1=0;
    unsigned short ind0,ind1;
    int		even = 1;
    unsigned char  c;
    int		i;

    for(i=0; i<length; i++, pname++) {
	c = *pname;
	if(even) {h0 = T[h0^c]; even = 0;}
	else {h1 = T[h1^c]; even = 1;}
    }
    ind0 = (unsigned short)h0;
    ind1 = (unsigned short)h1;
    return((ind1<<hashTableShift) ^ ind0);
}

/* open the directory file containing the archive channel list */
/* and pointers to the channel data in the archive files       */
open_directory(
char			*pdir_name,	/* name of file with channel directory */
FILE			**dir_fd)	/* channel directory feil descriptor */
{
	/* open the directory */
	if ((*dir_fd = fopen(pdir_name,"r")) == NULL){
		printf("couldn't open directory\n");
		return(0);
	}
	return(1);
}
close_directory(
FILE	*dir_fd)	/* channel directory feil descriptor */
{
	/* open the directory */
	fclose(dir_fd);
}
/* given the open channel directory and a channel name 	*/
/* return the dirctory information 			*/
find_channel(
FILE			*dir_fd,	/* fiel descriptor of the channel directory */
char			*pchan_name,	/* name of channel to find */
struct directory_entry	*pdir_entry,	/* channel information from directory */
unsigned short		*hash_offset,
unsigned long		*pchan_offset,
unsigned long		*last_offset)	/* previous offset for adding channels */
{
	unsigned short		found = 0;

	/* find the record */
	*last_offset = 0xffffffff;
	*hash_offset = hash(pchan_name,strlen(pchan_name));
	fseek(dir_fd,*hash_offset*sizeof(unsigned long),SEEK_SET);
	fread(pchan_offset,1,sizeof(unsigned long),dir_fd);

	/* follow the chain of defined records that hashed to this value */
	while((*pchan_offset != 0xffffffff) && (!found)){
		fseek(dir_fd,*pchan_offset,SEEK_SET);
		fread(pdir_entry,1,sizeof(struct directory_entry),dir_fd);
		*last_offset = *pchan_offset;
		if (strcmp(pchan_name,pdir_entry->name) == 0)  found = 1;
		else		*pchan_offset = pdir_entry->next_entry_offset;
	}
	if (found) return(1);
	return(0);
}
/* put the channel names in the buffer provided */
chan_list(
FILE			*dir_fd,
char			*pbuffer,
int			inx,
int			num_channels,
char			terminator)
{
	unsigned short		hash_offset;
	int			i;
	unsigned long		chan_offset;
	struct directory_entry	dir_entry;
	int			count;

	/* find the record */
	count = 0;
	for (i=inx; i < dirHashTableSize; i++){
		fseek(dir_fd,i*sizeof(unsigned long),SEEK_SET);
		fread(&chan_offset,1,sizeof(unsigned long),dir_fd);

		/* follow the chain of defined records that hashed to here */
		while(chan_offset != 0xffffffff){
			fseek(dir_fd,chan_offset,SEEK_SET);
			fread(&dir_entry,1,sizeof(struct directory_entry),dir_fd);
			strcpy(pbuffer,dir_entry.name);
			pbuffer += strlen(dir_entry.name);
			*pbuffer = terminator;
			pbuffer++;
			if (++count >= num_channels) return(1);
			chan_offset = dir_entry.next_entry_offset;
		}
	}
}
get_pvalue(
int	type,
void	*pdata,
void	**pvalue)
{
	switch (type){
	case DBR_TIME_STRING:
		*pvalue = (void *)&(((struct dbr_time_string *)pdata)->value);
		break;
	case DBR_TIME_SHORT: 
		*pvalue = (void *)&(((struct dbr_time_short *)pdata)->value);
		break;
	case DBR_TIME_FLOAT:
		*pvalue = (void *)&(((struct dbr_time_float *)pdata)->value);
		break;
	case DBR_TIME_ENUM:
		*pvalue = (void *)&(((struct dbr_time_short *)pdata)->value);
		break;
	case DBR_TIME_CHAR:
		*pvalue = (void *)&(((struct dbr_time_string *)pdata)->value);
		break;
	case DBR_TIME_LONG:
		*pvalue = (void *)&(((struct dbr_time_long *)pdata)->value);
		break;
	case DBR_TIME_DOUBLE:
		*pvalue = (void *)&(((struct dbr_time_double *)pdata)->value);
		break;
	default:
		*pvalue = (void *)&(((struct dbr_time_string *)pdata)->value);
		return(0);
	}
	return(1);
}

value_to_string(
char			*pval,
char			*pbuf,
int			type,
short			size,
struct ctrl_info	*pctrl_info)
{
	double	*pdouble;
	float	*pfloat;
	char	*pchar;
	int	i;
	char	temp[200];

/* what about arrays */
	switch (type){
	case DBR_STRING:
	    strcpy(temp,pval);
	    break;
	case DBR_SHORT: 
	    sprintf(temp,"%d",*(short *)pval);
	    break;
	case DBR_FLOAT:
	    pfloat = (float *)pval;
	    if((*pfloat < -10000000) || (*pfloat > 10000000))
		*pfloat = -50505;
	    sprintf(temp,"%-40.10f",*pfloat);
	    for(i=0;i<strlen(temp);i++){
		if (temp[i] == '.'){
			temp[i+pctrl_info->value.analog.prec] = 0;
		}
	    }
	    if (strlen(temp) > 30) strcpy(temp,"-50505*");
	    break;
	case DBR_ENUM:
	    pchar = (char *)&pctrl_info->value.index.state_strings;
	    for (i = 0; 
	      (i < *(short *)pval) && (i < pctrl_info->value.index.num_states); 
	      i++){
		pchar = pchar + strlen(pchar)+1;
	    }
	    if (i >= pctrl_info->value.index.num_states)
		strcpy(temp,"-50505");
	    else
	    	strcpy(temp,pchar);
	    break;
	case DBR_CHAR:
	    sprintf(temp,"%40x",*(char *)pval);
	    break;
	case DBR_LONG:
	    sprintf(temp,"%40x",*(long *)pval);
	    break;
	case DBR_DOUBLE:
	    pdouble = (double *)pval;
	    if((*pdouble < -100000000) || (*pdouble > 100000000))
		*pdouble = -50505;
	    sprintf(temp,"%-40.10f",*pdouble);
	    for(i=0;i<strlen(temp);i++){
		if (temp[i] == '.'){
			if (pctrl_info->value.analog.prec > 10)
				temp[i] = 0;
			else
				temp[i+pctrl_info->value.analog.prec] = 0;
		}
	    }
	    break;
	default:
	    strcpy(pbuf,"unknown type");
	    return(0);
	}

	/* blank pad */
	for (i = strlen(temp); i < size; i++)
		temp[i] = ' ';
	temp[size] = 0;
	strcpy(pbuf,temp);

	return(1);
}
to_ushort(
char		*pstr,
unsigned short	*pushort)
{
	*pushort = 0;
	while (*pstr){
		if (!isdigit(*pstr)) return(0);
		*pushort *= 10;
		*pushort += (*pstr - '0');
		pstr++;
	}
	return(1);
}
to_long(
char		*pstr,
long		*plong)
{
	short	negative = 0;

	*plong = 0;
	if (*pstr == '-'){
		negative = 1;
		pstr++;
	}
	while (*pstr){
		if (!isdigit(*pstr)) return(0);
		*plong *= 10;
		*plong += (*pstr - '0');
		pstr++;
	}
	if (negative) *plong = *plong * -1;
	return(1);
}
to_double(
char		*pstr,
double		*pdouble)
{
	double		frac_val = 0.0;
	double		frac = .1;
	unsigned short	negative = 0;

	*pdouble = 0.0;
	if (*pstr == '-'){
		negative = 1;
		pstr++;
	}
	while((*pstr) && (*pstr != '.')){
		if (!isdigit(*pstr)) return(0);
		*pdouble *= 10.0;
		*pdouble += (*pstr - '0');
		pstr++;
	}
	if (*pstr == '.'){
		pstr++;
		while((frac > .00001) && (*pstr)){
			if (!isdigit(*pstr)) return(0);
			frac_val /= 10.0;
			frac_val += ((*pstr - '0') * frac);
			frac /= 10.0;
			pstr++;
		}
	}
	*pdouble += frac_val;
	if (negative) *pdouble *= -1.0;
	if (*pdouble == -50505.0) *pdouble = 0.0;	/* ???should we do this???? */
	return(1);
}
string_to_value(
struct dbr_time_string	*pval,
char			*pbuf,
int			type,
struct ctrl_info	*pctrl_info)
{
	double	dval;
	long	lval;
	char	*pchar;
	int	i;
	char	temp[200];

/* what about arrays */
	switch (type){
	case DBR_TIME_STRING:
		strcpy(pval->value,pbuf);
		break;
	case DBR_TIME_SHORT:
		to_long(pbuf,&lval);
		((struct dbr_time_short *)(pval))->value = lval;
	    	break;
	case DBR_TIME_FLOAT:
		if (to_double(pbuf,&dval)){
			((struct dbr_time_float *)(pval))->value = dval;
		}else{
			((struct dbr_time_float *)(pval))->value = 0.0;
		}		
		break;
	case DBR_TIME_ENUM:
		pchar = (char *)&pctrl_info->value.index.state_strings;
		for (i = 0; 
		  (strcmp(pbuf,pchar) != 0) && (i < pctrl_info->value.index.num_states); 
		  i++){
			pchar = pchar + strlen(pchar)+1;
		}
		((struct dbr_time_enum *)(pval))->value = i;
		break;
	case DBR_TIME_CHAR:
		to_long(pbuf,&lval);
		((struct dbr_time_char *)(pval))->value = lval;
		break;
	case DBR_TIME_LONG:
		to_long(pbuf,&lval);
		((struct dbr_time_long *)(pval))->value = lval;
		break;
	case DBR_TIME_DOUBLE:
		to_double(pbuf,&((struct dbr_time_double *)(pval))->value);
		break;
	default:
printf("type mismatch %d %d\n",DBR_TIME_STRING,type);
		return(0);
	}
  	return(1);
}
val_to_short(
char			*pval,
short			*psval,
int			type)
{
        char    *pchar;
        short   *pshort;
        long    *plong;
        float   *pfloat;
        double  *pdouble;

	*psval = 0;
        switch (type){
        case DBR_STRING:
            break;
        case DBR_SHORT: 
        case DBR_ENUM:
            pshort = (short *)pval;
            *psval = *pshort;
            break;
        case DBR_FLOAT:
            pfloat = (float *)pval;
            *psval = *pfloat;
            break;
        case DBR_CHAR:
            pchar = (char *)pval;
            *psval = *pchar;
            break;
        case DBR_LONG:
            plong = (long *)pval;
            *psval = *plong;
            break;
        case DBR_DOUBLE:
            pdouble = (double *)pval;
            *psval = *pdouble;
            break;
        }
}

status_to_strings(
unsigned short	status,
unsigned short	severity,
char		*pstat_str,
char		*psevr_str)
{
	*pstat_str = *psevr_str = 0;

	severity &= 0xfff;
	if (severity == ARCH_EST_REPEAT){
		strcpy(psevr_str,"Est_Repeat");
		sprintf(pstat_str,"%d",status);
	}else if (severity == ARCH_REPEAT){
		strcpy(psevr_str,"Repeat");
		sprintf(pstat_str,"%d",status);
	}else if (severity == ARCH_DISCONNECT){
		strcpy(psevr_str,"Disconnected");
	}else if (severity == ARCH_STOPPED){
		strcpy(psevr_str,"Archive_Off");
	}else if (severity == ARCH_DISABLED){
		strcpy(psevr_str,"Archive_Disabled");
	/* put alarm status and severity strings here */
	}else if (severity == 1){
		strcpy(psevr_str,"MINOR");
		sprintf(pstat_str,"%d",status);
	}else if (severity == 2){
		strcpy(psevr_str,"MAJOR");
		sprintf(pstat_str,"%d",status);
	}else if (severity == 3){
		strcpy(psevr_str,"INVALID");
		sprintf(pstat_str,"%d",status);
	}else if (severity != 0){
		sprintf(psevr_str,"%d",severity);
		sprintf(pstat_str,"%d",status);
	}
	return(0);
}
strings_to_status(
unsigned short	*pstatus,
unsigned short	*pseverity,
char		*pstat_str,
char		*psevr_str)
{
	*pstatus = *pseverity = 0;

	if (strcmp(psevr_str,"Est_Repeat") == 0){
		*pseverity = ARCH_EST_REPEAT;
 	}else if (strcmp(psevr_str,"Repeat") == 0){
		*pseverity = ARCH_REPEAT;
  	}else if (strcmp(psevr_str,"Disconnected") == 0){
		*pseverity = ARCH_DISCONNECT;
	}else if (strcmp(psevr_str,"Archive_Off") == 0){
		*pseverity = ARCH_STOPPED;
 	}else if (strcmp(psevr_str,"Archive_Disabled") == 0){
		*pseverity = ARCH_DISABLED;
	/* put alarm status and severity strings here */
	}else if (strcmp(psevr_str,"MINOR") == 0){
		*pseverity = 1;
  	}else if (strcmp(psevr_str,"MAJOR") == 0){
		*pseverity = 2;
 	}else if (strcmp(psevr_str,"INVALID") == 0){
		*pseverity = 3;
 	}else if (*psevr_str){
		to_ushort(psevr_str,pseverity);
  	}
	to_ushort(pstat_str,pstatus);
	return(0);
}

value_to_strings(
unsigned short	type,
void		*pdata,
unsigned short	num_elements,
struct ctrl_info	*pctrl_info,
char		*pval_str,
char		*ptime_str,
char		*psevr_str,
char		*pstat_str,
struct disk_data_header	*pheader)
{
	void	*pvalue;
	unsigned short	status,severity;

	*pval_str = *ptime_str = *psevr_str = *pstat_str = 0;
/* verify the data is valid */
if (!data_ok(pheader,pdata)){
	strcpy(pval_str,"data bad");
	return(0);
}
	/* convert the value to a string */
	get_pvalue(type,pdata,&pvalue);
	value_to_string(pvalue,pval_str,type-DBR_TIME_STRING,12,pctrl_info);

	/* convert the time stamp */
	tsStampToText(&((struct dbr_time_string *)pdata)->stamp,
	    TS_TEXT_MMDDYY,ptime_str);

	/* convert status and severity to strings */
	severity = ((struct dbr_time_string *)pdata)->severity;
	status = ((struct dbr_time_string *)pdata)->status;
	status_to_strings(status,severity,pstat_str,psevr_str);
	return(1);
}
/* original find_channel with open and close fd for every search */
get_channel(
char			*pdir_name,	/* name of file with channel directory */
char			*pchan_name,	/* name of channel to find */
struct directory_entry	*pdir_entry)	/* channel information from directory */
{
	unsigned short		hash_offset;
	FILE			*dir_fd;
	unsigned short		found = 0;
	unsigned long		chan_offset;
	unsigned long		*pchan_offset = &chan_offset;

	/* open the directory */
	if ((dir_fd = fopen(pdir_name,"r")) == NULL){
		printf("couldn't open directory\n");
		return(-1);
	}

	/* find the record */
	hash_offset = hash(pchan_name,strlen(pchan_name));
	fseek(dir_fd,hash_offset*sizeof(unsigned long),SEEK_SET);
	fread(pchan_offset,1,sizeof(unsigned long),dir_fd);

	/* follow the chain of defined records that hashed to this value */
	while((*pchan_offset != 0xffffffff) && (!found)){
		fseek(dir_fd,*pchan_offset,SEEK_SET);
		fread(pdir_entry,1,sizeof(struct directory_entry),dir_fd);
		if (strcmp(pchan_name,pdir_entry->name) == 0) found = 1;
		else		*pchan_offset = pdir_entry->next_entry_offset;
	}
	fclose(dir_fd);
	if (found) return(0);
	return(-1);
}
find_time_range(
struct directory_entry	*pdir_entry,
TS_STAMP		*pstart_time,
TS_STAMP		*pend_time,
struct disk_data_header	*pdata_header,
char			**pfilename,
unsigned long		*poffset,
int			*pcount)
{
	TS_STAMP	time,day;
	TS_STAMP	*pday = &day;
	FILE		*data_fd;
	char		dwt[120];

	*pcount = 0;

	/* find the first file */
	day = *pstart_time;

	/* search backward */
	strcpy(*pfilename,pdir_entry->last_file);
	*poffset = pdir_entry->last_offset;
	
	/* search for the first buffer to contain this data */
	if ((data_fd = fopen(*pfilename,"r")) == NULL) return(-1);
	fseek(data_fd,*poffset,SEEK_SET);
	fread(pdata_header,sizeof(struct disk_data_header),1,data_fd);
	while((day.secPastEpoch < pdata_header->begin_time.secPastEpoch)
	  && (*pdata_header->prev_file)){
		if (pend_time->secPastEpoch > pdata_header->begin_time.secPastEpoch)
 			*pcount = *pcount + pdata_header->num_samples;
		if (strcmp(*pfilename,pdata_header->prev_file) != 0){
			fclose(data_fd);
			strcpy(*pfilename,pdata_header->prev_file);
			if ((data_fd = fopen(*pfilename,"r")) == NULL) return(0);
		}
		*poffset = pdata_header->prev_offset;
		fseek(data_fd,*poffset,SEEK_SET);
		fread(pdata_header,sizeof(struct disk_data_header),1,data_fd);
	}
	fclose(data_fd);
	return(1);
}


get_value(
char			*pfilename,
struct disk_data_header	*pbeg_data_header,
unsigned long		*pcurr_offset,
TS_STAMP		*ptime_array,
char			*pvalue_array,
unsigned short		*pseverity_array,
unsigned short		*pstatus_array,
TS_STAMP		*pfrom_time,
TS_STAMP		*pto_time,
unsigned long		*pstart_index,
unsigned long		*pend_index,
double			*pmin,
double			*pmax,
double			*pbuf_min,
double			*pbuf_max,
unsigned long		*pcount,
struct ctrl_info	*pctrl_info)
{
	TS_STAMP		*ptime;
	char			*pval_buf = pvalue_array;
	unsigned short		*pseverity = pseverity_array;
	unsigned short		*pstatus = pstatus_array;
	FILE			*data_fd;
	struct disk_data_header	data_header;
	struct disk_data_header	*pdata_header = &data_header;
	TS_STAMP		day;
	unsigned short		i;
	struct dbr_time_string	*pdisk_block;
	void			*pvalue;
	int			disk_block_size,last_disk_block_size,data_size;
	unsigned long		config_offset = 0;
	unsigned long		offset = 0;
	char			curr_filename[120];

	day = *pto_time;
	day.secPastEpoch++;
	ptime = ptime_array;
	if ((data_fd = fopen(pfilename,"r")) == NULL) return(-1);
	strcpy(curr_filename,pfilename);
	fseek(data_fd,*pcurr_offset,SEEK_SET);
	fread(pdata_header,sizeof(struct disk_data_header),1,data_fd);

	/* search for the first buffer to contain this data */
	last_disk_block_size = 0;
	*pstart_index = 0;
	*pend_index = 0;
	while (day.secPastEpoch > pdata_header->begin_time.secPastEpoch){

		/* get configuration information - contains display limits */
		if ((config_offset != pdata_header->config_offset)
		  && (pdata_header->config_offset != 0)){
			offset = ftell(data_fd);
			fseek(data_fd,pdata_header->config_offset,SEEK_SET);
			fread(pctrl_info,sizeof(struct ctrl_info),1,data_fd);
			fseek(data_fd,offset,SEEK_SET);
			config_offset = pdata_header->config_offset;
			if (pctrl_info->type == NUMBER_VALUE){
			    if (pctrl_info->value.analog.disp_high > *pmax){
				*pmax = pctrl_info->value.analog.disp_high;
				*pbuf_max = pctrl_info->value.analog.disp_high;
			    }
			    if (pctrl_info->value.analog.disp_low < *pmin){
				*pmin = pctrl_info->value.analog.disp_low;
				*pbuf_min = pctrl_info->value.analog.disp_low;
			    }
			}else{
			    if (pctrl_info->value.index.num_states > *pmax){
				*pmax = pctrl_info->value.index.num_states;
				*pbuf_max = pctrl_info->value.index.num_states;
			    }
			    *pmin = 0;
			    *pbuf_min = 0;
			}
		}

		disk_block_size = 
		  rounded_dbr_size_n(data_header.dbr_type,data_header.nelements);
		data_size = rounded_dbr_size_n
		  (data_header.dbr_type-DBR_TIME_STRING,data_header.nelements);
		if (disk_block_size > last_disk_block_size){
			if (last_disk_block_size > 0) 
				free(pdisk_block);
			pdisk_block = (struct dbr_time_string *)
			  (calloc(disk_block_size,1));
		}

		/* we get the pointer to the value part of the data */
		/* structure in pdisk_data_block based on data type */
		/* alignment causes the value offset to be different */
		/* for different data types			     */
		/* data type can only change betwee disk data blocks */
		/* so we do not do this inside the following loop    */
		get_pvalue(data_header.dbr_type,pdisk_block,&pvalue);

		/* copy out each value, time stamp, severity, status */
		for(i=0; i<pdata_header->num_samples; i++){
			fread(pdisk_block,disk_block_size,1,data_fd);
	
			/* take time and check if in desired range */
			/* set min and max for buffer and for in range as well */
			*ptime = pdisk_block->stamp;
			if ( (ptime->secPastEpoch < pfrom_time->secPastEpoch)
			  || ( (ptime->secPastEpoch == pfrom_time->secPastEpoch)
			  &&   (ptime->nsec <= pfrom_time->nsec) )){
				(*pstart_index)++;
				(*pend_index)++;
			}else if ((ptime->secPastEpoch < pto_time->secPastEpoch)
			  || ((ptime->secPastEpoch == pto_time->secPastEpoch)
			  &&  (ptime->nsec <= pto_time->nsec) )){
				(*pend_index)++;
			}
			ptime++;

			/* take value, status and severity */
			strncpy(pval_buf,(char *)pvalue,data_size);
			pval_buf += data_size;
			*pseverity = pdisk_block->severity;
			pseverity++;
			*pstatus = pdisk_block->status;
			pstatus++;
		}

		/* only open when next block is in a different file */
		if (strcmp(curr_filename,pdata_header->next_file) != 0){
			fclose(data_fd);
			if ((data_fd = fopen(pdata_header->next_file,"r")) == NULL){
				if (last_disk_block_size > 0) free(pdisk_block);
				return(0);
			}
			strcpy(curr_filename,pfilename);
		}
		fseek(data_fd,pdata_header->next_offset,SEEK_SET);
		fread(pdata_header,sizeof(struct disk_data_header),1,data_fd);
	}
	if (last_disk_block_size > 0) free(pdisk_block);
	fclose(data_fd);
	return(0);
}

open_value_context(
struct directory_entry	*pdir_entry,
struct fetch_context	*pcontext){
	struct ctrl_info	*pctrl_info;
	unsigned long		curr_offset;

/* make this use a common fd in global memory - instead of one per channel */
	if ((pcontext->curr_fd = fopen(pdir_entry->first_file,"r")) == NULL){
		printf("could not open data file *%s*\n",	pdir_entry->first_file);
		return;
	}
	strcpy(&pcontext->filename[0],pdir_entry->first_file);
	fseek(pcontext->curr_fd,pdir_entry->first_offset,SEEK_SET);
	fread(&pcontext->curr_header,1,sizeof(struct disk_data_header),pcontext->curr_fd);
	pcontext->value_size = 
	  rounded_dbr_size_n(pcontext->curr_header.dbr_type,pcontext->curr_header.nelements);
	pcontext->curr_inx = 0;
	pcontext->curr_offset = pdir_entry->first_offset;

	curr_offset = ftell(pcontext->curr_fd);
	fseek(pcontext->curr_fd,pcontext->curr_header.config_offset,SEEK_SET);
	fread(pcontext->config_info,1,sizeof(struct ctrl_info), pcontext->curr_fd);
	pctrl_info = (struct ctrl_info *)&pcontext->config_info[0];
	if (pctrl_info->size > sizeof(struct ctrl_info))
		fread(&pcontext->config_info[sizeof(struct ctrl_info)],
		  1,pctrl_info->size - sizeof(struct ctrl_info),
		  pcontext->curr_fd);
	fseek(pcontext->curr_fd,curr_offset,SEEK_SET);
}
open_curr_value_context(
struct directory_entry	*pdir_entry,
struct fetch_context	*pcontext){
	struct ctrl_info	*pctrl_info;
	unsigned long		curr_offset;

	if ((pcontext->curr_fd = fopen(pdir_entry->last_file,"r")) == NULL){
		printf("could not open data file *%s*\n",	pdir_entry->first_file);
		return;
	}
	strcpy(&pcontext->filename[0],pdir_entry->last_file);
	fseek(pcontext->curr_fd,pdir_entry->last_offset,SEEK_SET);
	fread(&pcontext->curr_header,1,sizeof(struct disk_data_header),pcontext->curr_fd);
	pcontext->value_size = 
	  rounded_dbr_size_n(pcontext->curr_header.dbr_type,pcontext->curr_header.nelements);
	pcontext->curr_inx = pcontext->curr_header.num_samples - 1;
	pcontext->curr_offset = pdir_entry->last_offset;
	curr_offset = ftell(pcontext->curr_fd);
	fseek(pcontext->curr_fd,pcontext->curr_header.config_offset,SEEK_SET);
	fread(pcontext->config_info,1,sizeof(struct ctrl_info), pcontext->curr_fd);
	pctrl_info = (struct ctrl_info *)&pcontext->config_info[0];
	if (pctrl_info->size > sizeof(struct ctrl_info))
		fread(&pcontext->config_info[sizeof(struct ctrl_info)],
		  1,pctrl_info->size - sizeof(struct ctrl_info),
		  pcontext->curr_fd);
	fseek(pcontext->curr_fd,curr_offset,SEEK_SET);
}
open_set_value_context(
struct directory_entry	*pdir_entry,
struct fetch_context	*pcontext,
TS_STAMP		*pstart_time,
TS_STAMP		*pend_time)
{
	struct ctrl_info	*pctrl_info;
	unsigned long		curr_offset;
	int			count;
	char			filename[120];
	char			*pfilename = &filename[0];

	/* get to the starting position */
	find_time_range(pdir_entry,pstart_time,pend_time,
	  &pcontext->curr_header,
	  &pfilename,
	  &pcontext->curr_offset,
	  &count);
	strcpy(pcontext->filename,pfilename);

	/* open the data fd */
	if ((pcontext->curr_fd = fopen(pcontext->filename,"r")) == NULL){
		printf("could not open data file *%s*\n",	pdir_entry->first_file);
		return;
	}

	/* compute value size */
	pcontext->value_size = 
	  rounded_dbr_size_n
	    (pcontext->curr_header.dbr_type,pcontext->curr_header.nelements);

	/* read in the config information */
	pcontext->curr_inx = 0;
	fseek(pcontext->curr_fd,
	  pcontext->curr_offset+sizeof(struct disk_data_header),SEEK_SET);
	curr_offset = ftell(pcontext->curr_fd);
	fseek(pcontext->curr_fd,pcontext->curr_header.config_offset,SEEK_SET);
	fread(pcontext->config_info,1,sizeof(struct ctrl_info), pcontext->curr_fd);
	pctrl_info = (struct ctrl_info *)&pcontext->config_info[0];
	if (pctrl_info->size > sizeof(struct ctrl_info))
		fread(&pcontext->config_info[sizeof(struct ctrl_info)],
		  1,pctrl_info->size - sizeof(struct ctrl_info),
		  pcontext->curr_fd);

	/* leave the file descriptor ready to get the next value */
	fseek(pcontext->curr_fd,curr_offset,SEEK_SET);
}

get_next_value(
struct fetch_context	*pcontext,
void			*pvalue){
	struct ctrl_info	*pctrl_info;
	unsigned long		curr_offset;

	if ((pcontext->curr_inx >= pcontext->curr_header.num_samples)
	  && (pcontext->curr_header.next_file[0] == 0)){
		/* try keeping the current filename- closing and reopenning it */
		fclose(pcontext->curr_fd);
		if ((pcontext->curr_fd = fopen(pcontext->filename,"r")) == NULL){
			printf("could not open data file *%s*\n",pcontext->curr_header.next_file);
			return(0);
		}
		fseek(pcontext->curr_fd,pcontext->curr_offset,SEEK_SET);
		fread(&pcontext->curr_header,1,sizeof(struct disk_data_header),pcontext->curr_fd);
		if (pcontext->curr_inx >= pcontext->curr_header.num_samples){
			/* reread the last one - it may be an estimated repeat count */
			fseek(pcontext->curr_fd,(pcontext->curr_inx-1)*pcontext->value_size,SEEK_CUR);
			fread(pvalue,1,pcontext->value_size,pcontext->curr_fd);
			return(0);
		}else{
			fseek(pcontext->curr_fd,pcontext->curr_inx*pcontext->value_size,SEEK_CUR);
		}
	}
	if (pcontext->curr_inx >= pcontext->curr_header.num_samples){ 
		fclose(pcontext->curr_fd);
		if ((pcontext->curr_fd = fopen(pcontext->curr_header.next_file,"r")) == NULL){
			printf("could not open data file *%s*\n",pcontext->curr_header.next_file);
			return(0);
		}
		strcpy(&pcontext->filename[0],pcontext->curr_header.next_file);
		pcontext->curr_inx = 0;
		pcontext->curr_offset = pcontext->curr_header.next_offset;
		fseek(pcontext->curr_fd,pcontext->curr_header.next_offset,SEEK_SET);
		fread(&pcontext->curr_header,1,sizeof(struct disk_data_header),pcontext->curr_fd);
		curr_offset = ftell(pcontext->curr_fd);
		fseek(pcontext->curr_fd,
		  pcontext->curr_header.config_offset,SEEK_SET);
		fread(pcontext->config_info,1,sizeof(struct ctrl_info),
		  pcontext->curr_fd);
		pctrl_info = (struct ctrl_info *)&pcontext->config_info[0];
		if (pctrl_info->size > sizeof(struct ctrl_info))
			fread(&pcontext->config_info[sizeof(struct ctrl_info)],
			  1,pctrl_info->size - sizeof(struct ctrl_info),
			  pcontext->curr_fd);
		fseek(pcontext->curr_fd,curr_offset,SEEK_SET);
	}
	if (pcontext->curr_inx < pcontext->curr_header.num_samples){
		fread(pvalue,1,pcontext->value_size,pcontext->curr_fd);
		pcontext->curr_inx++;
		return(1);
	}else{
		return(0);
	}
}

