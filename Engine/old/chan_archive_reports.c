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
 */
#include	<cadef.h>
#include	<fdmgr.h>
#include	<malloc.h>
#include	<stdlib.h>
#include	<string.h>
#include	"freq_arch.h"


report_directory_info(
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
report_disk_data_header(
struct disk_data_header	*pdata_header)
{
	char time_string[80];

	printf("Disk Header Info:\n");
	tsStampToText(&pdata_header->begin_time,TS_TEXT_MMDDYY,time_string);
	printf("\t%d samples from %s",
	  pdata_header->num_samples,time_string);
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
report_config_data(
struct ctrl_info	*pctrl_info)
{
	char	*pchar;
	short	i;

	printf("Config Info:\n");
	if (pctrl_info->type == NUMBER_VALUE){
		printf("\tDisplay limits %f %f\n",
		  pctrl_info->value.analog.disp_high,
		  pctrl_info->value.analog.disp_low);
		printf("\tWarning limits %f %f\n",
		  pctrl_info->value.analog.high_warn,
		  pctrl_info->value.analog.low_warn);
		printf("\tAlarm limits %f %f\n",
		  pctrl_info->value.analog.high_alarm,
		  pctrl_info->value.analog.low_alarm);
		pchar = (char *)&pctrl_info->value.analog.units;
		printf("\tPrec %d %s\n",
		  pctrl_info->value.analog.prec,pchar);
	}else{
		printf("\t%d Strings\n",pctrl_info->value.index.num_states);
		pchar = (char *)&pctrl_info->value.index.state_strings;
		for (i = 0; i < pctrl_info->value.index.num_states; i++){
			printf("\t%s\n",pchar);
			pchar = pchar + strlen(pchar)+1;
		}
	}
}

report_context(
struct	fetch_context	*pcontext)
{
	printf("%s %d %x %x\n",pcontext->filename,pcontext->value_size,
	  pcontext->curr_inx,pcontext->curr_offset);


}

