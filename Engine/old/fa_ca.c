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
 *
 * Modification Log:
 * -----------------
 * .01	lrd	08-24-97	support ca_get for slower channels instead of monitors
 * .02	lrd	01-10-98	modified to fetch the control data whenever the
 *						channel is reconnected
 * .03	lrd	05-30-98	only put ca_gets into buffer if the value and
 *				time stamp are new
 */
#include	<stdlib.h>
#include	<cadef.h>
#include	<fdmgr.h>
#include 	"fa_display.h"
#include	"fa.h"
#include	"freq_arch.h"

/* channel access parameters */
#ifndef ca_poll		/* timeout handler stuff */
#define ca_poll ca_pend_event(.000000000001)
#endif
#define FALSE 0

extern int		debug;
extern fdctx	*pfdctx;	/* file descriptor manager id */
extern float		get_threshhold;	/* scans >= to this use ca_get - less are monitored */
unsigned short		connections_changed = 0;

/* scan lists for channels that use ca_get instead of monitors */
struct freq_scan_entry{
	float					frequency;
	unsigned long			nextscan;
	struct freq_scan_entry	*pnext_freq_scan;
	struct arch_chan		*plast_chan;
	struct arch_chan		*pfirst_chan;
};
struct freq_scan_entry	*pfreq_scan_list;

/* capollfunc - is started by a fdmgr_add_timeout at init and reschedules itself */
/* outgoing channel access events need to be scheduled through a                */
/* periodic excursion to channel access -                                       */
/* this allows channel access to broadcast channel searches and service puts    */
static void capollfunc(void *pParam)
{
	ca_pend_event(.000000000001);
	fdmgr_add_timeout(pfdctx, &tenth_second, capollfunc, (void *)NULL);
}

/* caFDCallBack -                                          */
/* this stub is called when a channel access file descriptor is created or deleted */
/* every task will have one fd for the broadcast of channel names                  */
/* it will also have a file descriptor for each IOC that it connects to            */
/* as an fd activity will cause the fdmgr_pend_event to wake up   -                */
/* and we do a ca_pend_event there to allow these fd changes to be serviced -      */
/* this stub performs no action                                                    */
/* it needs to exist - as registering ca fd events requires a routine              */
void caFDCallBack(void *pUser)
{
}

/* fd_register */
void fd_register(pfdctx, fd, condition)
void    *pfdctx;
int     fd;
int     condition;
{
        if(condition){
                fdmgr_add_fd(pfdctx, fd, caFDCallBack, NULL);
        }else{
                fdmgr_clear_fd(pfdctx, fd);
        }
}

/* caException */
void caException(
struct exception_handler_args	arg)
{/* what happens here */
}

char	get_buf[1024];

static void caControlHandler(
struct event_handler_args arg)
{
	struct arch_chan	*parch_chan;
	struct dbr_ctrl_int	*pdbr_ctrl_int;
	struct dbr_ctrl_float	*pdbr_ctrl_float;
	struct dbr_ctrl_double	*pdbr_ctrl_dbl;
	struct dbr_ctrl_enum	*pdbr_ctrl_enum;
	struct dbr_ctrl_char	*pdbr_ctrl_char;
	struct dbr_ctrl_long	*pdbr_ctrl_long;
	char			*pchar;
	unsigned short		size;
	struct ctrl_info	*pinfo;
	short			i;

	if (arg.status != ECA_NORMAL)	return;

	/* verify the buffer is initialized */
	pinfo = (struct ctrl_info *)&get_buf[0];
	parch_chan = (struct arch_chan *)arg.usr;

	/* put related information into the arch_channel structure for later */
	switch (arg.type){
	case DBR_CTRL_STRING:	/* no control information */
	    size =  sizeof(struct arch_enum);
	    pinfo->size = size;
	    pinfo->type = ENUM_VALUE;
	    pinfo->value.index.num_states = 0;
	    break;
	case DBR_CTRL_SHORT:
	    pdbr_ctrl_int = (struct dbr_ctrl_int *)arg.dbr;
	    size =  sizeof(struct arch_number) + strlen(pdbr_ctrl_int->units);
	    pinfo->size = size;
	    pinfo->type = NUMBER_VALUE;
	    pinfo->value.analog.disp_high = pdbr_ctrl_int->upper_disp_limit;
	    pinfo->value.analog.disp_low = pdbr_ctrl_int->lower_disp_limit;
	    pinfo->value.analog.low_warn = pdbr_ctrl_int->lower_warning_limit;
	    pinfo->value.analog.low_alarm = pdbr_ctrl_int->lower_alarm_limit;
	    pinfo->value.analog.high_warn = pdbr_ctrl_int->upper_warning_limit;
	    pinfo->value.analog.high_alarm = pdbr_ctrl_int->upper_alarm_limit;
	    pinfo->value.analog.prec = 0;
	    strcpy((char *)&pinfo->value.analog.units,pdbr_ctrl_int->units);
	    break;
	case DBR_CTRL_ENUM:
	    pdbr_ctrl_enum = (struct dbr_ctrl_enum *)arg.dbr;
	    size =  sizeof(struct arch_enum);
	    for (i=0; i < pdbr_ctrl_enum->no_str; i++)
		size = size + 1 + strlen(pdbr_ctrl_enum->strs[i]);
	    pinfo->size = size;
	    pinfo->type = ENUM_VALUE;
	    pinfo->value.index.num_states = pdbr_ctrl_enum->no_str;
	    pchar = (char *)&pinfo->value.index.state_strings;
	    for (i=0; i<pdbr_ctrl_enum->no_str; i++){
		strcpy(pchar,pdbr_ctrl_enum->strs[i]);
		pchar += strlen(pdbr_ctrl_enum->strs[i]);
		*pchar = 0;
		pchar++;
	    }
	    break;
	case DBR_CTRL_FLOAT:
	    pdbr_ctrl_float = (struct dbr_ctrl_float *)arg.dbr;
	    size =  sizeof(struct arch_number) + strlen(pdbr_ctrl_float->units);
	    pinfo->size = size;
	    pinfo->type = NUMBER_VALUE;
	    pinfo->value.analog.disp_high = pdbr_ctrl_float->upper_disp_limit;
	    pinfo->value.analog.disp_low = pdbr_ctrl_float->lower_disp_limit;
	    pinfo->value.analog.low_warn = pdbr_ctrl_float->lower_warning_limit;
	    pinfo->value.analog.low_alarm = pdbr_ctrl_float->lower_alarm_limit;
	    pinfo->value.analog.high_warn = 
	      pdbr_ctrl_float->upper_warning_limit;
	    pinfo->value.analog.high_alarm = pdbr_ctrl_float->upper_alarm_limit;
	    pinfo->value.analog.prec = pdbr_ctrl_float->precision;
	    strcpy((char *)&pinfo->value.analog.units,pdbr_ctrl_float->units);
	    break;
	case DBR_CTRL_CHAR:
	    pdbr_ctrl_char = (struct dbr_ctrl_char *)arg.dbr;
	    size =  sizeof(struct arch_number) + strlen(pdbr_ctrl_char->units);
	    pinfo->size = size;
	    pinfo->type = NUMBER_VALUE;
	    pinfo->value.analog.disp_high = pdbr_ctrl_char->upper_disp_limit;
	    pinfo->value.analog.disp_low = pdbr_ctrl_char->lower_disp_limit;
	    pinfo->value.analog.low_warn = pdbr_ctrl_char->lower_warning_limit;
	    pinfo->value.analog.low_alarm = pdbr_ctrl_char->lower_alarm_limit;
	    pinfo->value.analog.high_warn = pdbr_ctrl_char->upper_warning_limit;
	    pinfo->value.analog.high_alarm = pdbr_ctrl_char->upper_alarm_limit;
	    pinfo->value.analog.prec = 0;
	    strcpy((char *)&pinfo->value.analog.units,pdbr_ctrl_char->units);
	    break;
	case DBR_CTRL_LONG:
	    pdbr_ctrl_long = (struct dbr_ctrl_long *)arg.dbr;
	    size =  sizeof(struct arch_number) + strlen(pdbr_ctrl_long->units);
	    pinfo->size = size;
	    pinfo->type = NUMBER_VALUE;
	    pinfo->value.analog.disp_high = pdbr_ctrl_long->upper_disp_limit;
	    pinfo->value.analog.disp_low = pdbr_ctrl_long->lower_disp_limit;
	    pinfo->value.analog.low_warn = pdbr_ctrl_long->lower_warning_limit;
	    pinfo->value.analog.low_alarm = pdbr_ctrl_long->lower_alarm_limit;
	    pinfo->value.analog.high_warn = pdbr_ctrl_long->upper_warning_limit;
	    pinfo->value.analog.high_alarm = pdbr_ctrl_long->upper_alarm_limit;
	    pinfo->value.analog.prec = 0;
	    strcpy((char *)&pinfo->value.analog.units,pdbr_ctrl_long->units);
	    break;
	case DBR_CTRL_DOUBLE:
	    pdbr_ctrl_dbl = (struct dbr_ctrl_double *)arg.dbr;
	    size =  sizeof(struct arch_number) + strlen(pdbr_ctrl_dbl->units);
	    pinfo->size = size;
	    pinfo->type = NUMBER_VALUE;
	    pinfo->value.analog.disp_high = pdbr_ctrl_dbl->upper_disp_limit;
	    pinfo->value.analog.disp_low = pdbr_ctrl_dbl->lower_disp_limit;
	    pinfo->value.analog.low_warn = pdbr_ctrl_dbl->lower_warning_limit;
	    pinfo->value.analog.low_alarm = pdbr_ctrl_dbl->lower_alarm_limit;
	    pinfo->value.analog.high_warn = pdbr_ctrl_dbl->upper_warning_limit;
	    pinfo->value.analog.high_alarm = pdbr_ctrl_dbl->upper_alarm_limit;
	    pinfo->value.analog.prec = pdbr_ctrl_dbl->precision;
	    strcpy((char *)&pinfo->value.analog.units,pdbr_ctrl_dbl->units);
	    break;
	}

	/* put the control information into the channel structure */
	/* it will be written to disk later			  */
	if (size % 8)	size = size - (size % 8) + 8;
	if (size != parch_chan->control_size){
		parch_chan->control_size = size;
		if (parch_chan->pctrl_info != 0)
			free(parch_chan->pctrl_info);
		parch_chan->pctrl_info = calloc(size,1);
	}
	pdbr_ctrl_int = (struct dbr_ctrl_int *)arg.dbr;
	bcopy(pinfo,parch_chan->pctrl_info,size);
}

/* caEventHandler */
/* channel access monitor events are handled here                 */
/* this call back is set up by the ca_add_array_event call in the */
/* connection handler routine                                     */
static void caEventHandler(
struct event_handler_args arg)
{
	struct arch_chan	*parch_chan;
	struct dbr_time_string	*pdbr_time;

	/* verify the buffer is initialized */
	parch_chan = (struct arch_chan *)arg.usr;
	if (parch_chan->pbuffer == NULL) return;
	if (parch_chan->overwrites) return;
	if (((struct dbr_time_double *)arg.dbr)->stamp.secPastEpoch == 0) return;
 	add_new_value(parch_chan,(struct dbr_time_double *)arg.dbr);
 }
dump_buf(
char	*pbuf,
int		cnt)
{
	int	i;
	for (i=0;i<cnt;i++,pbuf++) printf("%x ",*pbuf);
	printf("\n");
}

/* dbCaLinkConnectionHandler */
/* ca_search_and_connect calls use this handle when a channel is found */
/* channel access also calls this routine when a connection is broken */
/* it is called for each channel that connects or disconnects         */
void dbCaLinkConnectionHandler(
struct connection_handler_args	arg)
{
	struct arch_chan	*parch_chan;
	struct arch_event	arch_event;
	unsigned short		nelements,dbr_type;
	char			buf[120];
	struct group_list	*pgroup_list;

	parch_chan = ca_puser(arg.chid);

	/* add a disconnect event */
	if (ca_state(arg.chid) != cs_conn){
		arch_event.event_type = ARCH_DISCONNECT;
		arch_event.stamp = parch_chan->save_time; 
		pgroup_list = parch_chan->pgroup_list;
		while (pgroup_list){
			pgroup_list->parch_group->num_connected--;
			pgroup_list = pgroup_list->pnext_group;
		}
		archiver_event(parch_chan,&arch_event);
		parch_chan->connected = 0;
		parch_chan->connected_changed = 1;
		connections_changed = 1;
		return;
	}

	/* get the control information for this channel */
	SEVCHK(ca_array_get_callback(ca_field_type(arg.chid)+DBR_CTRL_STRING,1,
	  arg.chid,caControlHandler,parch_chan),NULL);

	dbr_type = ca_field_type(arg.chid)+DBR_TIME_STRING;
	nelements = ca_element_count(arg.chid);

	/* if there is no circular buffer - first connection */
	if (!parch_chan->ca_monitored){
		parch_chan->ca_monitored = 1;
		if (parch_chan->sample_freq >= get_threshhold){
			add_to_get_list(parch_chan);
		}else{
			/* add a monitor for this channel */
			SEVCHK(ca_add_array_event(dbr_type,nelements,
			  arg.chid,caEventHandler,parch_chan,0.0,0.0,0.0,(evid *)NULL),NULL);
		}
	}
	pgroup_list = parch_chan->pgroup_list;
	while (pgroup_list){
		pgroup_list->parch_group->num_connected++;
		pgroup_list = pgroup_list->pnext_group;
	}
	parch_chan->connected = 1;
	parch_chan->connected_changed = 1;
	connections_changed = 1;

	/* did the variable being monitored change size or type ? */
	if ((parch_chan->pbuffer == NULL)
	  || (parch_chan->nelements != nelements)
	  || (parch_chan->dbr_type != dbr_type)){
		arch_event.dbr_type = dbr_type;
		arch_event.nelements = nelements;
		arch_event.event_type = ARCH_CHANGE_SIZE;
		arch_event.stamp = parch_chan->save_time;
		archiver_event(parch_chan,&arch_event);
	}
}

/* INIT*/
ca_init(){
	long	stat;

	/* initialize channel access */
	SEVCHK(ca_task_initialize(), "ca_task_initialize: CA init failure.\n");

	/* Add exception handler to avoid aborts from CA server*/
	stat = ca_add_exception_event(caException, NULL);
	if (stat != ECA_NORMAL) printf("Bad stat from ca_add_exception\n");

	/* Set up file descriptor mngr(fdmgr) */
	pfdctx =  fdmgr_init();
	if (!pfdctx) {
		printf("ca_task_initialize: fdmgr_init failed.\n");
		abort();
	}

	/* Get CA file descriptor for fdmgr */
	/* all file descr activity for channel access is now be registered with fdmgr */
	SEVCHK(ca_add_fd_registration(fd_register, pfdctx),
	  "gsdl_macro_ca_task_initialize:   Fd registration failed.\n");

	/* incoming channel access acitivity will cause the fdmgr_pend_event to wake up */
	/* however - outgoing channel access events need to be scheduled through a      */
	/* periodic excursion to channel access -                                       */
	/* this allows channel access to broadcast channel searches and service puts    */
	fdmgr_add_timeout(pfdctx, &one_second, capollfunc, (void *)NULL);
}

add_to_get_list(
struct arch_chan	*parch_chan){
	struct freq_scan_entry	*pfreq_scan_entry;
	unsigned short		not_found;
	TS_STAMP		time;
	unsigned long		sec_round;

	pfreq_scan_entry = pfreq_scan_list;
	not_found = 1;
	while (pfreq_scan_entry && not_found){
		if (pfreq_scan_entry->frequency == parch_chan->sample_freq)
			not_found = 0;
		else
			pfreq_scan_entry = pfreq_scan_entry->pnext_freq_scan;
	}

	/* create a new scan list */
	if (not_found){
		pfreq_scan_entry = (struct freq_scan_entry *)malloc(sizeof(struct freq_scan_entry));
		if (pfreq_scan_entry == 0){
			printf("Could not create new freq_scan for %s\n",parch_chan->pname);
			return(-1);
		}

		/* next time we will get the data and archive it - and frequency */
		tsLocalTime(&time);
		sec_round = parch_chan->sample_freq;
		pfreq_scan_entry->nextscan = time.secPastEpoch;
		pfreq_scan_entry->nextscan -= time.secPastEpoch % sec_round;
		pfreq_scan_entry->nextscan += sec_round;
		pfreq_scan_entry->frequency = parch_chan->sample_freq;

		/* init this channel to be first and last channel */
		pfreq_scan_entry->plast_chan = pfreq_scan_entry->pfirst_chan = parch_chan;
		pfreq_scan_entry->pnext_freq_scan = pfreq_scan_list;
		pfreq_scan_list = pfreq_scan_entry;

	/* add this channel to the list - and make it the last channel */
	}else{
		pfreq_scan_entry->plast_chan->pnext_get = parch_chan;
		pfreq_scan_entry->plast_chan = parch_chan;
	}
}
check_get_list(
unsigned long	secPastEpoch)
{
	struct freq_scan_entry	*pfreq_scan_entry;
	unsigned long		sec_round;

	pfreq_scan_entry = pfreq_scan_list;
	while (pfreq_scan_entry){
		if (secPastEpoch >= pfreq_scan_entry->nextscan){
			sec_round = pfreq_scan_entry->frequency;
			while (secPastEpoch >= pfreq_scan_entry->nextscan)
				pfreq_scan_entry->nextscan += sec_round;
			if (take_data(pfreq_scan_entry->pfirst_chan) < 0)
				printf("scan %f - timeout on ca_pend_io\n",pfreq_scan_entry->frequency);
		}
		pfreq_scan_entry = pfreq_scan_entry->pnext_freq_scan;
	}
}
take_data(
struct arch_chan	*pfirst_arch_chan)
{
	int			status;
	struct dbr_time_string	*pdbr_time_string;
	struct arch_chan	*parch_chan;
	long		diff_sec,diff_nsec,diff_msec;
	float		longest_pend = 0;
	float		end_pend = 10.0;
	float		pend = .3;
	int		size;
float		*pfloat;

	/* fetch the channels */
	/* we back off the timeout on subsequent attempts */
if (debug) printf("take_data\n");
	status = ECA_TIMEOUT;
	while ((status != ECA_NORMAL) && (pend <= end_pend)){
		parch_chan = pfirst_arch_chan;
		while(parch_chan){
			if (parch_chan->connected){
				SEVCHK(ca_array_get(parch_chan->dbr_type,
				    parch_chan->nelements,
				    parch_chan->chid,parch_chan->pget_buffer),"failed get");
			}
			parch_chan = parch_chan->pnext_get;
		}
		status = ca_pend_io(pend);
		pend *= 2.0;
	}
	if (pend > end_pend){
		return(-1);
	}

	/* add the events to the circular buffer */
	parch_chan = pfirst_arch_chan;
	while(parch_chan){
	    size = dbr_size_n(parch_chan->dbr_type-DBR_TIME_STRING,
	      parch_chan->nelements);
	    if ((parch_chan->connected) 
	      && (parch_chan->pget_buffer->stamp.secPastEpoch != 0)){
		if ((parch_chan->pcurr_value == 0)
		  || (parch_chan->pcurr_value->stamp.secPastEpoch == 0)){
			add_new_value(parch_chan,parch_chan->pget_buffer);
		/* value and time stamp are different - or status/severity are different */
		}else if(((bcmp(&parch_chan->pcurr_value->value,
		      &parch_chan->pget_buffer->value,size) != 0)
		  && (parch_chan->pget_buffer->stamp.secPastEpoch >
		  parch_chan->pcurr_value->stamp.secPastEpoch))
		  || (parch_chan->pcurr_value->status != parch_chan->pget_buffer->status)
		  || (parch_chan->pcurr_value->severity != parch_chan->pget_buffer->severity)){
			add_new_value(parch_chan,parch_chan->pget_buffer);
		}
	    }
	    parch_chan = parch_chan->pnext_get;
	}
if (debug) printf("done take_data\n");

	return(0);
}
