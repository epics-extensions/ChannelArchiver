/*
 * fa_circbuf.c
 *
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
 * .01	06-28-97	lrd	added the ability to change the rate to write to disk -
 *				increases buffer allocation when write frequency is slower
 * .02	06-28-97	lrd	fixed a bug in chnaging archive frequency that required
 *				larger buffer - data would be repeated
 * .03	12-18-97	lrd	added archive all monitors (no repeat counts)
 * .04	01-21-98	lrd	put value into repeat events to make retrieval easier
 * .05	02-07-98	lrd	retry on .04
 * .06	02-12-98	lrd	clean up archiver events
 * .07	05-02-98	lrd	only save last value on disable - if there was one
 *						starting up disabled - caused a crash
 * .08	05-11-98	lrd	do not save new value to separate buffer - only mark UPDATED
 *						but not yet saved - as PENDING. On enable, reset all PENDING
 *						to UPDATED to be archived. Time stamp needs to be set to the
 *						current value
 * .09	07-11-98	lrd	disable by group
 */


#include	<stdlib.h>
#include	<cadef.h>
#include	<fdmgr.h>
#include 	"fa_display.h"
#include 	"freq_arch.h"
#include	"fa.h"
extern unsigned long	write_frequency;
unsigned long		archive_disable = 0;
extern struct arch_chan	*pchan_list;

/* return the next buffer in the circular buffer */
get_next_arch_buffer(
struct	arch_chan	*parch_chan,
struct	dbr_time_double	**pdbr_time)
{
	/* compute the place in the circular queue */
	if (++parch_chan->head_inx >= parch_chan->num_bufs)
		parch_chan->head_inx = 0;

	/* here is the over write of the queue */
	/* we slow it down in the event handler */
	/* we reset it in the archiving routine */
	if (parch_chan->head_inx == parch_chan->tail_inx){
		parch_chan->overwrites++;
		if (++parch_chan->tail_inx >= parch_chan->num_bufs)
			parch_chan->tail_inx = 0;
	}
	*pdbr_time = (struct dbr_time_double *)
	  ((parch_chan->pbuffer)+(parch_chan->head_inx*parch_chan->buf_size));
}
struct dbr_time_double	*pevent_buf = 0;
unsigned short		event_size = 0;
struct dbr_time_double	*prepeat_event = 0;
unsigned short		repeat_size = 0;

#define			LAST_VALUE 1

newGroupDisable(
struct arch_chan	*pdisabled_chan,
struct arch_chan	*pcheck_chan){
	struct group_list	*pcheck_group;
	struct group_list	*pdisable_group;

	/* are any of the group of the check channel already disabled */
	pcheck_group = pcheck_chan->pgroup_list;
	while(pcheck_group){
		if (pcheck_group->parch_group->disabled) return(0);
		pcheck_group = pcheck_group->pnext_group;
	}

	/* are the disabled and check channels in any of the same groups */
	pdisable_group = pdisabled_chan->pgroup_list;
	while(pdisable_group){
		pcheck_group = pcheck_chan->pgroup_list;
		while(pcheck_group){
			if (pcheck_group->parch_group == pdisable_group->parch_group)
 				return(1);
			pcheck_group = pcheck_group->pnext_group;
		}
		pdisable_group = pdisable_group->pnext_group;
	}
	return(0);
}
setGroupDisable(
struct arch_chan	*pdisabled_chan){
	struct group_list	*pdisable_group_list;
	struct group_list	*pcheck_group_list;
	struct arch_group	*pdisable_group;
	struct arch_chan	*pchan;
	int					found;

	pdisable_group_list = pdisabled_chan->pgroup_list;
	while(pdisable_group_list){
		pdisable_group = pdisable_group_list->parch_group;
		pdisable_group->disabled |= pdisabled_chan->disable_bit;
		pchan = pchan_list;
		while(pchan){
			if ((pchan->disabled & pdisabled_chan->disable_bit) == 0){
				pcheck_group_list = pchan->pgroup_list;
				found = 0;
				while(pcheck_group_list && (!found)){
					if (pcheck_group_list->parch_group == pdisable_group){
 						pchan->disabled |= pdisabled_chan->disable_bit;
						found = 1;
					}
					pcheck_group_list = pcheck_group_list->pnext_group;
				}
			}
			pchan = pchan->pnext;
		}
		pdisable_group_list = pdisable_group_list->pnext_group;
	}
}
clearGroupDisable(
struct arch_chan	*pdisabled_chan){
	struct group_list	*pdisable_group;
	struct arch_chan	*pchan;

	pdisable_group = pdisabled_chan->pgroup_list;
	while(pdisable_group){
		pdisable_group->parch_group->disabled &= ~pdisabled_chan->disable_bit;
		pdisable_group = pdisable_group->pnext_group;
	}
	pchan = pchan_list;
	while(pchan){
		pchan->disabled &= ~pdisabled_chan->disable_bit;
		pchan = pchan->pnext;
	}
}


make_event_buf(
struct arch_chan	*parch_chan,
unsigned short		severity,
unsigned short		status,
TS_STAMP		time,
unsigned short		last_val,
struct dbr_time_double	**pevent_buf,
unsigned short		*size)
{
	char	*pvalue;
	unsigned short i;

	if (parch_chan->buf_size > *size){
		if (*size > 0){
			free(*pevent_buf);
		}
		*pevent_buf = (struct dbr_time_double *)
		  calloc(parch_chan->buf_size,1);
		*size = parch_chan->buf_size;
	}

	/* either copy in the current value - or zero fill */
	/* repeat events need last value - others 0 fill */
	if (last_val){
 		bcopy((char *)(parch_chan->pcurr_value),(char *)(*pevent_buf),
		  parch_chan->buf_size);
	}else{
		for (i=0, pvalue = (char *)(*pevent_buf); i<parch_chan->buf_size; i++,pvalue++)
 		  *pvalue = 0;
	}

 	(*pevent_buf)->severity = severity;
	(*pevent_buf)->status =status;
	(*pevent_buf)->stamp.secPastEpoch = time.secPastEpoch;
	(*pevent_buf)->stamp.nsec = time.nsec;
}
/* a new value has been received through a monitor - or a get */
add_new_value(
struct arch_chan	*parch_chan,
struct dbr_time_double	*pvalue)
{
	struct arch_event	arch_event;
	void		*pval;
	short		disable = 0;
	struct arch_chan	*pdisable_chan;

	/* for non-disable records - add the event */
	if (parch_chan->disable_bit == 0){
		add_freq_mon(parch_chan,pvalue);
		return;
	}

	/* get the value of this record */
	get_pvalue(parch_chan->dbr_type,pvalue,&pval);
	val_to_short(pval,&disable,parch_chan->dbr_type-DBR_TIME_STRING);
	if (disable && ((archive_disable & parch_chan->disable_bit) == 0)){
		add_freq_mon(parch_chan,pvalue);
		pdisable_chan = pchan_list;
		while(pdisable_chan){
			if (newGroupDisable(parch_chan,pdisable_chan)){
				/* issue the disable event to all records */
				arch_event.event_type = ARCH_DISABLED;
				arch_event.stamp = pvalue->stamp;
				arch_event.stamp.nsec += 1000000; /* set 1 msec after disable */
				archiver_event(pdisable_chan,&arch_event);
			}
			pdisable_chan = pdisable_chan->pnext;
		}
		setGroupDisable(parch_chan);
		archive_disable |= parch_chan->disable_bit;
		update_time_conn();		/* update display */
	}else if (!disable && (archive_disable & parch_chan->disable_bit)){
		pdisable_chan = pchan_list;
		while(pdisable_chan){
			if (((pdisable_chan->disabled & ~parch_chan->disable_bit) == 0)
			 && ((pdisable_chan->disabled &  parch_chan->disable_bit) == 1)){
				if (pdisable_chan->pend == 1){
					pdisable_chan->ppend_value->stamp = pvalue->stamp;
					add_freq_mon(parch_chan,pdisable_chan->ppend_value);
					pdisable_chan->pend = 0;
				}
 			}
			pdisable_chan = pdisable_chan->pnext;
		}
		clearGroupDisable(parch_chan);
		archive_disable &= ~parch_chan->disable_bit;
		add_freq_mon(parch_chan,pvalue);
		update_time_conn();		/* update display */
 	}
}
/* add a new monitor */
add_freq_mon(
struct arch_chan	*parch_chan,
struct dbr_time_double	*pvalue)
{
	unsigned short	repeat_count;
	TS_STAMP	temp_time;

	if (parch_chan->save_time.secPastEpoch == 0){
		if (pvalue->stamp.secPastEpoch != 0){
			set_save_time(parch_chan,&pvalue->stamp);
		}else{
			printf("%s: event - with no time stamp\n",parch_chan->pname);
			return;
		}
	}

	/* check for repeat events - compute the next save time */
	/* repeat events for frequency channels */
	if ((parch_chan->monitor == 0) && (parch_chan->disabled == 0)){
		temp_time = parch_chan->save_time;
		repeat_count = 0;
		while((temp_time.secPastEpoch < pvalue->stamp.secPastEpoch) 
		  || ((temp_time.secPastEpoch == pvalue->stamp.secPastEpoch) && (temp_time.nsec < pvalue->stamp.nsec))){
			tsAddDouble(&temp_time,&temp_time,parch_chan->sample_freq);
			repeat_count++;
		}
		/* put the repeat event into the circular buffer */
		repeat_count--;
		if (repeat_count > 0){
			tsAddDouble(&temp_time,&temp_time,-parch_chan->sample_freq);
			make_event_buf(parch_chan,(unsigned short)(ARCH_REPEAT),repeat_count,temp_time,
			  (unsigned short)LAST_VALUE,&prepeat_event,&repeat_size);

			/* put the new value into the circular buffer */
			add_event(parch_chan,prepeat_event);
		}
	}

	/* put the new event into the current buffer */
	add_event(parch_chan,pvalue);
}

add_event(
struct arch_chan	*parch_chan,
struct dbr_time_double	*pvalue)
{
	struct dbr_time_double	*pdbr_time;

	/* event has the time stamp of the next save time - write it */
	/* if the frequency changed - the next save time changed already */
	/* put the frequency changed event directly into the circular buffer */
	/* this causes a new data header in the archive file with the new */
	/* frequency. Next save time was computed from the new frequency */
	/* As a result - the curr_buffer needs to be checked next */
	if ((pvalue->severity & ARCH_CHANGE_FREQ) == ARCH_CHANGE_FREQ) {
		get_next_arch_buffer(parch_chan,&pdbr_time);
		bcopy(pvalue,pdbr_time,sizeof(struct dbr_time_double));
		parch_chan->sample_freq = pvalue->value;
		return;
	}

	/* place events in the pend buffer if the archiver is disabled */
	if (parch_chan->disabled){
 		bcopy(pvalue,parch_chan->ppend_value,parch_chan->buf_size);
		parch_chan->pend = 1;
		return;
	}

	/* place curr value into the circular buffer for archiving if
	 * the curr value is an archiver event (ARCH_NO_VALUE on severity) and is new
	 * or it is an archive on change channel (monitor) and not disabled
	 * or the new time stamp exceeds the archive frequency and not disabled */
 	if ( (parch_chan->curr_written == UPDATED)
	  && ((parch_chan->monitor)
	   || (parch_chan->save_time.secPastEpoch < pvalue->stamp.secPastEpoch)
	   || ((parch_chan->save_time.secPastEpoch == pvalue->stamp.secPastEpoch) 
	   && (parch_chan->save_time.nsec < pvalue->stamp.nsec)) )){
		parch_chan->curr_written = WRITTEN;
		get_next_arch_buffer(parch_chan,&pdbr_time);
		bcopy(parch_chan->pcurr_value,pdbr_time,parch_chan->buf_size);
		/* increment next save time by one period or repeat interval */
		if (parch_chan->pcurr_value->severity == ARCH_REPEAT)
			tsAddDouble(&parch_chan->save_time,&parch_chan->save_time,
			  parch_chan->sample_freq * parch_chan->pcurr_value->status);
		else
			tsAddDouble(&parch_chan->save_time,&parch_chan->save_time,parch_chan->sample_freq);
 	}

	/* put the new event into the current value buffer */
 	bcopy(pvalue,parch_chan->pcurr_value,parch_chan->buf_size);
	parch_chan->last_queue_time = pvalue->stamp;
	parch_chan->curr_written = UPDATED;
}
bindump(
unsigned char	*pchar,
int	count)
{
	int i;
	for (i=0;i<count;i++) printf("%x ",*(pchar+i));
	printf("\n");
}

make_arch_bufs(
struct arch_chan	*parch_chan)
{
	char 		*pnew_buffer;
	unsigned short	new_size;

	/* make the buffer size be a properly structure aligned number */
	/* for pointer references later !!!!!!!!!!!!!!!!!              */
	new_size = dbr_size_n(parch_chan->dbr_type,parch_chan->nelements);
	if (new_size % 8) new_size += 8 - (new_size % 8);
	if (new_size == parch_chan->buf_size) return(1);

	/* get new current buffer */
	if ((pnew_buffer = calloc(1,new_size)) == 0) return(0);
	if (parch_chan->pcurr_value){
		if (new_size < parch_chan->buf_size)
			bcopy(parch_chan->pcurr_value,pnew_buffer,new_size);
		else
			bcopy(parch_chan->pcurr_value,pnew_buffer,parch_chan->buf_size);
		free(parch_chan->pcurr_value);
	}
	parch_chan->pcurr_value = (struct dbr_time_double *)pnew_buffer;
	parch_chan->curr_written = WRITTEN;	/* initial condition - not ready to write */
	parch_chan->buf_size  = new_size;

	/* new get buffer */
	if (parch_chan->pget_buffer) free(parch_chan->pget_buffer);
	if ((parch_chan->pget_buffer = (struct dbr_time_double *)calloc(1,new_size)) == 0) return(0);

	/* new pend buffer */
	if (parch_chan->ppend_value) free(parch_chan->ppend_value);
	if ((parch_chan->ppend_value = (struct dbr_time_double *)calloc(1,new_size)) == 0) return(0);

	return(1);
}
make_circ_buf(
struct arch_chan	*parch_chan)
{
	unsigned short	num_bufs,num_to_end;
	char		*pbuffer;

	/* allocate a circular buffer */
	num_bufs = (parch_chan->write_freq * 2) / parch_chan->sample_freq;
	if (num_bufs < 3) num_bufs = 3;
	if (parch_chan->num_bufs == num_bufs) return(1);
	if ((pbuffer = (char *)calloc(num_bufs,parch_chan->buf_size)) == 0) return(0);

	/* as this circular buffer has more entries of the same size */
	/* copy it all over and keep the head and tail ptrs the same */
	if (parch_chan->pbuffer != 0) {
	    if (num_bufs > parch_chan->num_bufs){
			if (parch_chan->tail_inx < parch_chan->head_inx){
		 	   bcopy(parch_chan->pbuffer,pbuffer,parch_chan->num_bufs*parch_chan->buf_size);
			}else{
		 		if (parch_chan->tail_inx == parch_chan->head_inx){
					parch_chan->tail_inx = 0;
					parch_chan->head_inx = 0;
		  		}else if (++parch_chan->tail_inx >= parch_chan->num_bufs){
					parch_chan->tail_inx = 0;
					bcopy(parch_chan->pbuffer,pbuffer,parch_chan->num_bufs*parch_chan->buf_size);
					parch_chan->tail_inx = num_bufs;
					parch_chan->head_inx += num_to_end;
		 		}else{
					num_to_end = parch_chan->num_bufs-parch_chan->tail_inx;
					bcopy(parch_chan->pbuffer+(parch_chan->tail_inx*parch_chan->buf_size),pbuffer,
					  num_to_end * parch_chan->buf_size);
					bcopy(parch_chan->pbuffer,pbuffer+(parch_chan->buf_size * num_to_end),
					  (parch_chan->head_inx+1)*parch_chan->buf_size);
					parch_chan->tail_inx = num_bufs;
					parch_chan->head_inx += num_to_end;
		  		}

			}
		}
	    free(parch_chan->pbuffer);
	}
	parch_chan->num_bufs = num_bufs;
	parch_chan->pbuffer = pbuffer;
	return(1);
}			  
archiver_event(
struct arch_chan	*parch_chan,
struct arch_event	*pevent)
{
    struct dbr_time_double	dbr_time;
    unsigned short		num_bufs,num_to_end;
    char			*pbuffer;

    /* any extra handling for these events - like changing buffer size, number etc.. */
    if ((pevent->event_type & ARCH_CHANGE_SIZE) == ARCH_CHANGE_SIZE){
		parch_chan->dbr_type = pevent->dbr_type;
		parch_chan->nelements = pevent->nelements;

		if (!make_arch_bufs(parch_chan)) return(0);
		if (!make_circ_buf(parch_chan)) return(0);
		return;
    }else if (pevent->event_type == ARCH_CHANGE_WRITE_FREQ){
		/* changed to a faster frequency - do we need a bigger circ buffer */
		if (parch_chan->write_freq < pevent->frequency){
			parch_chan->write_freq = pevent->frequency;
			if (!make_circ_buf(parch_chan)) return(0);
		}else
			parch_chan->write_freq = pevent->frequency;
			return;
	}else if (pevent->event_type == ARCH_CHANGE_FREQ){
		/* changed to a faster frequency - do we need a bigger circ buffer */
		if (pevent->frequency < parch_chan->sample_freq){
			if ((parch_chan->write_freq > write_frequency) && (pevent->frequency < write_frequency))
				parch_chan->write_freq = write_frequency;
			if (!make_circ_buf(parch_chan)) return(0);
		/* if we slowed down - are we slower than the disk write period */
		}else if (pevent->frequency > parch_chan->write_freq){
			parch_chan->write_freq = pevent->frequency;
		}
		tsAddDouble(&parch_chan->save_time,&parch_chan->save_time, -parch_chan->sample_freq);
		while((parch_chan->save_time.secPastEpoch < parch_chan->last_queue_time.secPastEpoch)
		  || ((parch_chan->save_time.secPastEpoch == parch_chan->last_queue_time.secPastEpoch)
		  && (parch_chan->save_time.nsec < parch_chan->last_queue_time.nsec)) )
			tsAddDouble(&parch_chan->save_time,&parch_chan->save_time,pevent->frequency);
	}

    /* first one ever - compute the next save time */
	if (parch_chan->save_time.secPastEpoch == 0){
		/* do not save connection events here - there is no time stamp */
		/* this should only occur on the first connection */
		if (pevent->stamp.secPastEpoch == 0){
		}else{
			set_save_time(parch_chan,&pevent->stamp);
		}
	}else if (parch_chan->pbuffer == 0){
		return;
	}

    /* add the event to the monitor queue */
    /* create a monitor buffer for this event */
	make_event_buf(parch_chan,pevent->event_type,(unsigned short)0,pevent->stamp,
	  (unsigned short)0,&pevent_buf,&event_size);

	if ((pevent->event_type & ARCH_CHANGE_FREQ) == ARCH_CHANGE_FREQ)
		pevent_buf->value = pevent->frequency;
	add_freq_mon(parch_chan,pevent_buf);
}


get_freq_event(
struct arch_chan	*parch_chan,
struct dbr_time_double	**pvalue,
unsigned short		first_call,
struct dbr_time_double	*pestimate)
{
	struct dbr_time_double	*pdbr_time;
	struct dbr_time_double	*pdbr_repeat_estimate;
	float			time_sum;
	TS_STAMP		last_time;
	unsigned short		repeat_count;

	/* return the next available archive block */
	if (parch_chan->tail_inx != parch_chan->head_inx){
		/* return the next event */
		if (++parch_chan->tail_inx >= parch_chan->num_bufs)
			parch_chan->tail_inx = 0;
		*pvalue = (struct dbr_time_double *)((parch_chan->pbuffer)
		   +(parch_chan->tail_inx*parch_chan->buf_size));
		return(1);

	/* write frequency past - and current buffer not written */
	}else if (first_call){
		if (parch_chan->pbuffer == 0) return(0);
		if (parch_chan->pcurr_value->stamp.secPastEpoch == 0) return(0);

		/* write the current buffer if it was not yet */
		if (parch_chan->curr_written == UPDATED){
			/* add the current buffer to the circular buffer */
			get_next_arch_buffer(parch_chan,&pdbr_time);
			bcopy(parch_chan->pcurr_value,pdbr_time,parch_chan->buf_size);
			tsAddDouble(&parch_chan->save_time,&parch_chan->save_time,parch_chan->sample_freq);
			parch_chan->curr_written = WRITTEN;
			/* return the next event */
			if (++parch_chan->tail_inx >= parch_chan->num_bufs)
				parch_chan->tail_inx = 0;
			*pvalue =(struct dbr_time_double *)((parch_chan->pbuffer)+(parch_chan->tail_inx*parch_chan->buf_size));
			return(1);

		/* write frequency past and current buffer is written - repeat time */
		/* no estimated repeat counts during archive disable */
		}else if ((parch_chan->monitor == 0) && (archive_disable == 0)){
 			/* make an estimated repeat event */
			tsAddDouble(&parch_chan->pcurr_value->stamp,&parch_chan->pcurr_value->stamp,parch_chan->write_freq);
			repeat_count = parch_chan->write_freq / parch_chan->sample_freq;
			tsAddDouble(&parch_chan->save_time,&parch_chan->save_time,parch_chan->write_freq);
 			make_event_buf(parch_chan,(unsigned short)ARCH_EST_REPEAT,repeat_count,
			  parch_chan->pcurr_value->stamp,(unsigned short)LAST_VALUE,&prepeat_event,&repeat_size);
			*pvalue = prepeat_event;
			return(1);
		}else{
			return(0);
		}

	/* no more events to archive */
	}else{
		return(0);
	}

}
