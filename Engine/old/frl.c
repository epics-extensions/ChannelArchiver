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
 */
#include	<cadef.h>
#include	<fdmgr.h>
#include	"freq_arch.h"
unsigned short		bad_status = 0x5555;

#define NSEC_PER_SEC	1000000000	/* nano seconds per second */
#define MSEC_PER_SEC	1000		/* milli seconds per second */

static unsigned short hash( char *pname, int length)
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

get_channel(
char			*pdir_name,
char			*pchan_name,
struct directory_entry	*pdir_entry)
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
printf("chan %s found %d\n",pdir_entry->name,found);
	if (found) return(0);
	return(-1);
}

list_chans(
char			*pdir_name)
{
	unsigned short		hash_offset;
	FILE			*dir_fd;
	int			i;
	unsigned long		chan_offset;
	struct directory_entry	dir_entry;

	/* open the directory */
	if ((dir_fd = fopen(pdir_name,"r")) == NULL){
		printf("couldn't open directory\n");
		return(-1);
	}

	/* find the record */
	for (i=0; i < hashTableSize; i++){
		fseek(dir_fd,i*sizeof(unsigned long),SEEK_SET);
		fread(&chan_offset,1,sizeof(unsigned long),dir_fd);

		/* follow the chain of defined records that hashed to here */
		while(chan_offset != 0xffffffff){
			fseek(dir_fd,chan_offset,SEEK_SET);
			fread(&dir_entry,1,sizeof(struct directory_entry),dir_fd);
			printf("%s\n",dir_entry.name);
			chan_offset = dir_entry.next_entry_offset;
		}
	}
printf("%d names found\n",i);
	fclose(dir_fd);
}

write_directory_info(
struct directory_entry	*pdir_entry)
{
	char	time_string[120];

	printf("%s:\n",pdir_entry->name);
	tsStampToText(&pdir_entry->create_time,TS_TEXT_MMDDYY,time_string);
	printf("	created:\t%s\n",time_string);
	tsStampToText(&pdir_entry->first_save_time,TS_TEXT_MMDDYY,time_string);
	printf("	first saved:\t%s\n",time_string);
	printf("	begins: %s \tat offset %x\n",
	  pdir_entry->first_file,pdir_entry->first_offset);
	printf("	ends:   %s \tat offset %x\n",
	  pdir_entry->last_file,pdir_entry->last_offset);
}
write_disk_data_header(
struct disk_data_header	*pdata_header)
{
	char time_string[80];

	printf("Disk Header Info:\n");
	tsStampToText(&pdata_header->begin_time,TS_TEXT_MMDDYY,time_string);
	printf("\t%d samples %d freqs from %s",
	  pdata_header->num_samples,pdata_header->num_freqs,time_string);
	tsStampToText(&pdata_header->next_file_time,TS_TEXT_MMDDYY,time_string);
	printf("\tto %s\n",time_string);
	printf("\n\t%f second intervals\n",pdata_header->save_frequency);
	printf("\tType: %d\tNelements: %d\n",
	  pdata_header->dbr_type,pdata_header->nelements);
	printf("\tBuf Size %d Buf Free %d\n",
	  pdata_header->buf_size,pdata_header->buf_free);
	printf("\tNext File: %s\tNext Offset: %x\n",
	  &pdata_header->next_file[0],pdata_header->next_offset);
	printf("\tPrev File: %s\tPrev Offset: %x\n",
	  &pdata_header->prev_file[0],pdata_header->prev_offset);
	printf("\tCurr Offset: %x\n",pdata_header->curr_offset);
	tsStampToText(&pdata_header->last_wrote_time,TS_TEXT_MMDDYY,time_string);
	printf("\tLast Updated %s\n",time_string);
}

print_value(
unsigned short	type,
void		*pvalue)
{
	char	*pchar;
	short	*pshort;
	long	*plong;
	float	*pfloat;
	double	*pdouble;

	switch (type){
	case DBR_TIME_STRING:
	    pchar = (char *)&(((struct dbr_time_string *)pvalue)->value);
	    printf("%s ",pchar);
	    break;
	case DBR_TIME_SHORT: 
	    pshort = (short *)&(((struct dbr_time_short *)pvalue)->value);
	    printf("%d ",*pshort);
	    break;
	case DBR_TIME_FLOAT:
	    pfloat = (float *)&(((struct dbr_time_float *)pvalue)->value);
	    printf("%6.0f ",*pfloat);
	    break;
	case DBR_TIME_ENUM:
	    pshort = (short *)&(((struct dbr_time_short *)pvalue)->value);
	    printf("%d ",*pshort);
	    break;
	case DBR_TIME_CHAR:
	    pchar = (char *)&(((struct dbr_time_string *)pvalue)->value);
	    printf("%x ",*pchar);
	    break;
	case DBR_TIME_LONG:
	    plong = (long *)&(((struct dbr_time_long *)pvalue)->value);
	    printf("%x ",*plong);
	    break;
	case DBR_TIME_DOUBLE:
	    pdouble = (double *)&(((struct dbr_time_double *)pvalue)->value);
	    printf("%6.0f ",*pdouble);
	    break;
	default:
	    printf(". ");
	}
}

write_data(
struct directory_entry	*pdir_entry)
{
	FILE			*data_fd;
	struct disk_data_header	disk_data_header;
	struct disk_data_header	*pdata_header = &disk_data_header;
	struct dbr_time_string	*pdbr_time_string;
	unsigned long		i,value_size;
char	dwt[120];

	if ((data_fd = fopen(pdir_entry->first_file,"r")) == NULL){
		printf("could not open data file *%s*\n",	pdir_entry->first_file);
		return;
	}
	fseek(data_fd,pdir_entry->first_offset,SEEK_SET);
	fread(pdata_header,1,sizeof(struct disk_data_header),data_fd);
	write_disk_data_header(pdata_header);
	value_size = dbr_size_n(pdata_header->dbr_type,pdata_header->nelements);
	pdbr_time_string = (struct dbr_time_string *)calloc(1,value_size);
	printf("\t");
	while(1){
		for (i = 0; i < pdata_header->num_samples; i++){
			fread(pdbr_time_string,1,value_size,data_fd);
			if (pdbr_time_string->severity & 0xff00){
				tsStampToText(&pdbr_time_string->stamp,TS_TEXT_MMDDYY,dwt);
				printf("\n\t%s\t",dwt);
				printf("R%x:%d ",
			pdbr_time_string->severity,pdbr_time_string->status);
			}else{
				tsStampToText(&pdbr_time_string->stamp,TS_TEXT_MMDDYY,dwt);
				printf("\n\t%s\t",dwt);
				print_value(pdata_header->dbr_type,pdbr_time_string);
			}
		}
		printf("\n");
		fclose(data_fd);
		if ((data_fd = fopen(pdata_header->next_file,"r")) == NULL){
			return;
		}
		fseek(data_fd,pdata_header->next_offset,SEEK_SET);
		fread(pdata_header,1,sizeof(struct disk_data_header),data_fd);
		write_disk_data_header(pdata_header);
	}
}

check_interval(
struct directory_entry	*pdir_entry)
{
	FILE			*data_fd;
	struct disk_data_header	disk_data_header;
	struct disk_data_header	*pdata_header = &disk_data_header;
	struct dbr_time_string	*pdbr_time_string;
	unsigned long		i,value_size;
	double			interval = 0,last_interval = 0;
	TS_STAMP		last_time = {0,0};
	char			time_string[120];

	if ((data_fd = fopen(pdir_entry->first_file,"r")) == NULL){
		printf("could not open data file %s\n",	pdir_entry->first_file);
		return;
	}
	fseek(data_fd,pdir_entry->first_offset,SEEK_SET);
	fread(pdata_header,1,sizeof(struct disk_data_header),data_fd);
	value_size = dbr_size_n(pdata_header->dbr_type,pdata_header->nelements);
	pdbr_time_string = (struct dbr_time_string *)calloc(1,value_size);
	while(1){
		for (i = 0; i < pdata_header->num_samples; i++){
			fread(pdbr_time_string,1,value_size,data_fd);
			TsDiffAsDouble(&interval, &pdbr_time_string->stamp, &last_time);
			if ((last_interval != 0)
			  && ( ((interval - last_interval) > .0001)
			    || ((interval - last_interval) < -.0001) )){
				tsStampToText(&last_time,TS_TEXT_MMDDYY,time_string);
				printf("\t%s to ",time_string);
				tsStampToText(&pdbr_time_string->stamp,TS_TEXT_MMDDYY,time_string);
				printf("%s - interval %f\n",time_string,interval);
			}
			last_time = pdbr_time_string->stamp;
			last_interval = interval;
		}
		fclose(data_fd);
		if ((data_fd = fopen(pdata_header->next_file,"r")) == NULL){
			tsStampToText(&pdbr_time_string->stamp,TS_TEXT_MMDDYY,time_string);
			printf("\t last time: %s - last interval: %f\n",time_string,interval);
			return;
		}
		fseek(data_fd,pdata_header->next_offset,SEEK_SET);
		fread(pdata_header,1,sizeof(struct disk_data_header),data_fd);
	}
}

go_back_to_start(
struct directory_entry	*pdir_entry,
TS_STAMP		*pstart_time,
struct disk_data_header	*pdata_header,
char			**pfilename,
unsigned long		*poffset)
{
	TS_STAMP	time,day;
	TS_STAMP	*pday = &day;
	FILE		*data_fd;
	char		dwt[120];

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
		fclose(data_fd);
		strcpy(*pfilename,pdata_header->prev_file);
		*poffset = pdata_header->prev_offset;
		if ((data_fd = fopen(*pfilename,"r")) == NULL) return(-1);
		fseek(data_fd,*poffset,SEEK_SET);
		fread(pdata_header,sizeof(struct disk_data_header),1,data_fd);
	}
printf("start at %s\n",*pfilename);
	fclose(data_fd);
	return(0);
}

count_events(
struct disk_data_header	*pbeg_data_header,
unsigned long		*pcount,
TS_STAMP		*pend_time)
{
	FILE			*data_fd;
	struct disk_data_header	data_header;
	struct disk_data_header	*pdata_header = &data_header;
	TS_STAMP		day;
	TS_STAMP		*pday = &day;
	char			dwt[120];

	day = *pend_time;
	day.secPastEpoch++;

	*pcount = 0;
	if (day.secPastEpoch > pbeg_data_header->begin_time.secPastEpoch)
	  (*pcount) += pbeg_data_header->num_samples;
printf("count : %d\n",*pcount);
	    
	/* count forward */
	if ((data_fd = fopen(pbeg_data_header->next_file,"r")) == NULL){
		return(-1);
	}
	fseek(data_fd,pbeg_data_header->next_offset,SEEK_SET);
	fread(pdata_header,sizeof(struct disk_data_header),1,data_fd);

	/* search for the last buffer to contain this data */
	while ((day.secPastEpoch > pdata_header->begin_time.secPastEpoch)
	  && (*pdata_header->next_file)){
		fclose(data_fd);
		(*pcount) += pdata_header->num_samples;
printf("count + %d\n",pdata_header->num_samples);
		if ((data_fd = fopen(pdata_header->next_file,"r")) == NULL) 
			return(-1);
		fseek(data_fd,pdata_header->next_offset,SEEK_SET);
		fread(pdata_header,sizeof(struct disk_data_header),1,data_fd);
	}
	fclose(data_fd);
printf("count = %d\n",*pcount);
	return(0);
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
unsigned long		*pcount)
{
  TS_STAMP			*ptime;
  char				*pval_buf = pvalue_array;
  unsigned short		*pseverity = pseverity_array;
  unsigned short		*pstatus = pstatus_array;
  FILE				*data_fd;
  struct disk_data_header	data_header;
  struct disk_data_header	*pdata_header = &data_header;
  TS_STAMP			day;
  long				i, n, r;
  struct dbr_time_string	*pdisk_block;
  char				*pvalue;
  unsigned short		disk_block_size,last_disk_block_size,data_size;
  char				dwt[120];
  char				*pchar;
  int				gte, lte;
  int				ret = 0;

  day = *pto_time;
  day.secPastEpoch++;
  ptime = ptime_array;

  if ((data_fd = fopen(pfilename,"r")) == NULL) {
    ret = -1;
    goto done;
  }
  fseek(data_fd,*pcurr_offset,SEEK_SET);
  fread(pdata_header,sizeof(struct disk_data_header),1,data_fd);

  /* search for the first buffer to contain this data */
  *pmin = 1.0; *pmax = -1.0;
  *pstart_index = 0;
  *pend_index = 0;
  last_disk_block_size = 0;
  n = 0;
  r = 0;
  
  while ((day.secPastEpoch > pdata_header->begin_time.secPastEpoch) &&
	 (n < *pcount))
    {
      disk_block_size = 
	dbr_size_n(data_header.dbr_type,data_header.nelements);
      data_size = dbr_size_n
	(data_header.dbr_type-DBR_TIME_STRING,data_header.nelements);
      
      if (disk_block_size > last_disk_block_size)
	{
	  if (last_disk_block_size > 0) 
	    free(pdisk_block);
	  pdisk_block = (struct dbr_time_string *)malloc(disk_block_size,1);
	}
      
      switch (pbeg_data_header->dbr_type)
	{
	case DBR_TIME_STRING:
	  pvalue = (char *) &(((struct dbr_time_string *)pdisk_block)->value);
	  break;
	case DBR_TIME_SHORT: 
	  pvalue = (char *) &(((struct dbr_time_short *)pdisk_block)->value);
	  break;
	case DBR_TIME_FLOAT:
	  pvalue = (char *) &(((struct dbr_time_float *)pdisk_block)->value);
	  break;
	case DBR_TIME_ENUM:
	  pvalue = (char *) &(((struct dbr_time_short *)pdisk_block)->value);
	  break;
	case DBR_TIME_CHAR:
	  pvalue = (char *) &(((struct dbr_time_string *)pdisk_block)->value);
	  break;
	case DBR_TIME_LONG:
	  pvalue = (char *) &(((struct dbr_time_long *)pdisk_block)->value);
	  break;
	case DBR_TIME_DOUBLE:
	  pvalue = (char *) &(((struct dbr_time_double *)pdisk_block)->value);
	  break;
	default:
	  printf("unknown type\n");
	  ret = -1;
	  goto done;
	}
      
      for (i=0; (i<pdata_header->num_samples) && (n < *pcount); i++)
	{
	  fread (pdisk_block,disk_block_size,1,data_fd);
	  *ptime = pdisk_block->stamp;
	  
	  /* gte is true if current time >= requested first time */
	  gte = ((ptime->secPastEpoch > pfrom_time->secPastEpoch)
		 || ((ptime->secPastEpoch == pfrom_time->secPastEpoch)
		     && (ptime->nsec >= pfrom_time->nsec)));
	  /* lte is true if current time <= requested end time */
	  lte = ((ptime->secPastEpoch < pto_time->secPastEpoch)
		 || ((ptime->secPastEpoch == pto_time->secPastEpoch)
		     && (ptime->nsec <= pto_time->nsec)));
	  
	  /* If the current sample falls in the specified range gather min/max
	     data.  Otherwise, if the current sample time is still prior to the
	     requested first time, increment the returned begin index */
	  if (gte && lte)
	    {
	      if (*pmin <= *pmax)
		{
		  r++;
		  if (*(double *)pvalue > *pmax) *pmax = *(double *)pvalue;
		  if (*(double *)pvalue < *pmin) *pmin = *(double *)pvalue;
		}
	      else *pmin = *pmax = *(double *)pvalue;
	    }
	  else if (!gte) (*pstart_index)++;
	  
	  ptime++;
	  memcpy(pval_buf,pvalue,data_size);
	  pval_buf += data_size;
	  *pseverity = pdisk_block->severity;
	  pseverity++;
	  *pstatus = pdisk_block->status;
	  pstatus++;
	  n++;
	}
      
      /* now we know what the returned end index should be */
      *pend_index = (*pstart_index) + (r-1);
      
      fclose(data_fd);
      if ((data_fd = fopen(pdata_header->next_file,"r")) == NULL)
	{
	  if (last_disk_block_size > 0) free(pdisk_block);
	  ret = 0;
	  goto done;
	}
      fseek(data_fd,pdata_header->next_offset,SEEK_SET);
      fread(pdata_header,sizeof(struct disk_data_header),1,data_fd);
    }
  
  if (last_disk_block_size > 0) free(pdisk_block);
  fclose(data_fd);

done:
  *pcount = n;
  return ret;
}

print_values(
struct disk_data_header	*pdata_header,
TS_STAMP		*ptime_array,
char			*pvalue_array,
unsigned short		*pseverity_array,
unsigned short		*pstatus_array,
unsigned long		start_index,
unsigned long		end_index,
unsigned long 		count)
{
	unsigned short	i;
	char		dwt[120];
	TS_STAMP	*ptime = ptime_array;
	char		*pval_buf = pvalue_array;
	unsigned long	data_size;
	double		*pdouble;
	unsigned short	*pseverity = pseverity_array;
	unsigned short	*pstatus = pstatus_array;

	data_size = 
	  dbr_size_n(pdata_header->dbr_type-DBR_TIME_STRING,pdata_header->nelements);

	ptime += start_index;
	pseverity += start_index;
	pstatus += start_index;
	pval_buf += (data_size * start_index);
	for (i = start_index; i <= end_index; i++){
		tsStampToText(ptime,TS_TEXT_MMDDYY,dwt);
		printf("%s\t",dwt);

		if ((*pseverity & 0xff00) == 0xff00){
			printf("%x cnt %d\n",*pseverity,*pstatus);
		}else{
			switch (pdata_header->dbr_type){
			case DBR_TIME_STRING:
			    printf("%s\n",pval_buf);
			    break;
			case DBR_TIME_SHORT: 
			    printf("%d\n",*(short *)pval_buf);
			    break;
			case DBR_TIME_FLOAT:
			    printf("%f\n",*(float *)pval_buf);
			    break;
			case DBR_TIME_ENUM:
			    printf("%d\n",*(short *)pval_buf);
	 		    break;
			case DBR_TIME_CHAR:
			    printf("%x\n",*(char *)pval_buf);
			    break;
			case DBR_TIME_LONG:
			    printf("%x\n",*(long *)pval_buf);
			    break;
			case DBR_TIME_DOUBLE:
			    pdouble = (double *)pval_buf;
			    printf("%f\n",*pdouble);
			    break;
			default:
			    printf("unknown type\n");
			}
		}
		ptime++;
		pseverity++;
		pstatus++;
		pval_buf += data_size;
	}
}
unsigned short final = 0;
unsigned short debug = 1;



#define	MAX_CHANNELS	10
report_last_day(
char		*pdir_name,
TS_STAMP	present_time)
{
	unsigned short		hash_offset;
	FILE			*dir_fd;
	int			i;
	unsigned long		chan_offset;
	struct directory_entry	dir_entry;
	TS_STAMP		from_time,to_time;
	unsigned short		num_channels;
	TS_STAMP		*ptime_array[MAX_CHANNELS];
	double			*pvalue_array[MAX_CHANNELS];
	unsigned short		*pseverity_array[MAX_CHANNELS];
	unsigned short		*pstatus_array[MAX_CHANNELS];
	unsigned long		start_index,end_index,count;
	double			min,max,bufmin,bufmax;
	struct disk_data_header	data_header;
	char			curr_filename[120];
	char			*pcurr_filename = &curr_filename[0];
	unsigned long		curr_offset;

	/* open the directory */
	if ((dir_fd = fopen(pdir_name,"r")) == NULL){
		printf("couldn't open directory\n");
		return(-1);
	}
	to_time = from_time = present_time;
	tsRoundDownLocal(&from_time,(unsigned long)(60*60*8)); /* last 8 hours */

	/* find the record */
	num_channels = 0;
	for (i=0; i < hashTableSize; i++){
		fseek(dir_fd,i*sizeof(unsigned long),SEEK_SET);
		fread(&chan_offset,1,sizeof(unsigned long),dir_fd);

		/* follow the chain of defined records that hashed to here */
		while((chan_offset != 0xffffffff) && (num_channels < MAX_CHANNELS)){
			fseek(dir_fd,chan_offset,SEEK_SET);
			fread(&dir_entry,1,sizeof(struct directory_entry),dir_fd);
			printf("\t%s",dir_entry.name);
			chan_offset = dir_entry.next_entry_offset;


			num_channels++;



			/* get the data file for the start of the data */
			go_back_to_start(&dir_entry,&from_time,&data_header,&pcurr_filename,
			  &curr_offset);
if (debug) printf("at start\n");

			/* get the number of buffers that will be needed */
			count_events(&data_header,&count,&to_time);
if (debug) printf("count %d\n",count);

			/* allocate the buffer for time stamps and values */
			ptime_array[num_channels] = (TS_STAMP *)calloc(sizeof(TS_STAMP),count);
			pvalue_array[num_channels] = (double *)
			  calloc(dbr_size_n((data_header.dbr_type-DBR_TIME_STRING),
			  data_header.nelements),count);
			pseverity_array[num_channels] = 
			  (unsigned short *)calloc(sizeof(unsigned short),count);
			pstatus_array[num_channels] = 
			  (unsigned short *)calloc(sizeof(unsigned short),count);

if (debug) printf("alloc'd memory\n");
			/* get time stamp / value pairs */
			get_value(pcurr_filename,&data_header,&curr_offset,
			  ptime_array[num_channels],
			  (char *)pvalue_array[num_channels],
			  pseverity_array[num_channels],pstatus_array[num_channels],
			  &from_time,&to_time,&start_index,&end_index, &min, &max,
			  &bufmin,&bufmax,&count);
			num_channels++;
if (debug) printf("done one\n");
		}
	}
	fclose(dir_fd);


}


#ifdef FREQ_RETRIEVER_APP

/* main loop */
main(
unsigned int	argc,
char		**argv)
{
	TS_STAMP	beg_time,end_time;
	char		directory_name[80];
	char		chan_name[80];
	struct directory_entry	dir_entry;
	struct directory_entry	*pdir_entry = &dir_entry;
	char			dwt[120];
	struct disk_data_header	data_header;
	char			curr_filename[120];
	char			*pcurr_filename = &curr_filename[0];
	char			buf[120];
	char			*pbuf;
	TS_STAMP		from_time,to_time;
	unsigned long		curr_offset;
	TS_STAMP		*ptime_array;
	double			*pvalue_array;
	unsigned short		*pseverity_array;
	unsigned short		*pstatus_array;
	unsigned long		count;
	unsigned long		start_index,end_index;
	double			min,max,bufmin,bufmax;

printf("freq retrieval\n");
tsLocalTime(&beg_time);

	/* parse args for default frequency */
	if (argc < 2){
		strcpy(directory_name,"freq_directory");
		report_last_day(directory_name,beg_time);
	}else if (strcmp(argv[1],"help") == 0){
		printf("Usage: freq_retriever directory <chan_name/command>\n");
		printf("Commands: list, interval, bewteen\n");
		return;
	}
	strcpy(directory_name,argv[1]);
	strcpy(chan_name,argv[2]);
	if (strcmp(argv[2],"list") == 0){
/* create a link list */
		list_chans(directory_name);
	}else if (argc < 4){
		if (get_channel(directory_name,chan_name,pdir_entry) < 0){
			printf("get_channel failed\n");
			return;
		}
		write_directory_info(pdir_entry);
		write_data(pdir_entry);
	}else if (strcmp(argv[3],"interval") == 0){
		if (get_channel(directory_name,chan_name,pdir_entry) < 0){
			printf("get_channel failed\n");
			return;
		}
		write_directory_info(pdir_entry);
		check_interval(pdir_entry);


	}else if ((strcmp(argv[3],"between") == 0) && (argc >= 6)){
if (debug) printf("long way\n");
		if (get_channel(directory_name,chan_name,pdir_entry) < 0){
			printf("get_channel failed\n");
			return;
		}
if (debug) printf("got channel\n");

		/* convert time to tsStamp */
		strcpy(&buf[0],argv[4]);
		pbuf = &buf[0];
		if (argc > 6){
			strcat(&buf[0]," ");
			strcat(&buf[0],argv[5]);
if (debug) printf("got big time stamps from %s\n",&buf[0]);
			if (tsTextToStamp(&from_time,&pbuf) < 0) return;
			strcpy(&buf[0],argv[6]);
			strcat(&buf[0]," ");
			strcat(&buf[0],argv[7]);
		}else{
			strcat(&buf[0]," 00:00:00"); /* BRIAD */
			if (tsTextToStamp(&from_time,&pbuf) < 0) return;
if (debug) printf("got time stamps from %s\n",&buf[0]);
			strcpy(&buf[0],argv[5]);
			strcat(&buf[0]," 00:00:00"); /*BRIAD */
		}
		pbuf = &buf[0];
if (debug) printf("got time stamps to %s\n",&buf[0]);
		if (tsTextToStamp(&to_time,&pbuf) < 0) return;

		/* get the data file for the start of the data */
		go_back_to_start(&dir_entry,&from_time,&data_header,&pcurr_filename,
		  &curr_offset);
if (debug) printf("at start\n");

		/* get the number of buffers that will be needed */
		count_events(&data_header,&count,&to_time);
if (debug) printf("count %d\n",count);

		/* allocate the buffer for time stamps and values */
		ptime_array = (TS_STAMP *)calloc(sizeof(TS_STAMP),count);
		pvalue_array = (double *)
		  calloc(dbr_size_n((data_header.dbr_type-DBR_TIME_STRING),data_header.nelements)
		    ,count);
		pseverity_array = (unsigned short *)calloc(sizeof(unsigned short),count);
		pstatus_array = (unsigned short *)calloc(sizeof(unsigned short),count);
tsLocalTime(&end_time);

if (debug) printf("alloc'd memory\n");
		/* get time stamp / value pairs */
		get_value(pcurr_filename,&data_header,&curr_offset,
		  ptime_array,(char *)pvalue_array,pseverity_array,pstatus_array,
		  &from_time,&to_time,&start_index,&end_index, &min, &max,
		  &bufmin,&bufmax,&count);
if (debug) printf("start inx: %d end inx: %d min %f max %f bmin %f bmax %f\n",
start_index,end_index,min,max,bufmin,bufmax);
if (!final) {
write_directory_info(pdir_entry);
tsStampToText(&beg_time,TS_TEXT_MMDDYY,dwt);
printf("start %s ",dwt);
tsStampToText(&end_time,TS_TEXT_MMDDYY,dwt);
printf("end %s\n",dwt);
printf("%d events malloc'd of size %d \n",count,dbr_size_n((data_header.dbr_type - DBR_TIME_STRING),data_header.nelements));
printf("start inx: %d\tend inx: %d\n",start_index,end_index);

		/* print out the values */
		print_values(&data_header,
		  ptime_array,(char *)pvalue_array,
		  pseverity_array,pstatus_array,
		  start_index,end_index,count);
}
	}
}

#endif
