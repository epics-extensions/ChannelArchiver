/*
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
 * .01	lrd	06-28-97	added mod log, added extern for number of channels to use
 *				before displaying only set information
 * .02	lrd	06-28-97	channel update now only updates time and connected status
 * .03	lrd	06-29-97	added disable status to the display
 * .04	lrd	06-29-97	add exit button
 * .05	lrd	12-18-97	add monitor capability
 * .06	lrd	12-21-97	add list function for set display
 * .07	lrd	02-13-98	have channel update display last saved time
 *				instead of last queue time
 * .08	lrd	07-11-98	disable displayed by channel or group
 * .09	lrd	07-11-98	fixed listCb to check all groups containing a channel
 */
int	no_display = 0;

#include	<stdlib.h>
#include	<cadef.h>
#include	<fdmgr.h>
#include	"fa_display.h"
#include	"fa.h"
#include	"freq_arch.h"

/* top level window information for Motif */
Widget 			arScroll, arList, searchEntry, fileEntry;
Widget 			numchan_lbl, numconn_lbl, lastwrite_lbl, set_lbl;
struct faGuiInfo	arGuiInfo;
extern fdctx		*pfdctx;	/* file descriptor manager id */
extern char		directory_name[];
extern struct arch_chan	*pchan_list;
extern int		num_channels;
extern int		max_chan_displayed;
extern struct arch_group	*pgroup_list;
int			display_channels;
int			first_screen = 1;
extern int		archiver_exit;

void x_evhandler(void	*pparam)
{
	while (XtAppPending(arGuiInfo.arApp)) {
	  XtAppProcessEvent(arGuiInfo.arApp, XtIMAll);
	}
}
void	listCB(
Widget		widget, 
XtPointer	client_data, 
XtPointer	call_data)
{
	struct arch_group   *parch_group = (struct arch_group *)client_data;
	struct arch_chan	*parch_chan;
	struct group_list	*pcheck_group_list;
	char				ts_string[120];
	int					found;
 
	parch_chan = pchan_list;
	while(parch_chan){ 
		/* need to check each group in the group list */
		pcheck_group_list = parch_chan->pgroup_list;
		found = 0;
		while(pcheck_group_list && (!found)){
			if (pcheck_group_list->parch_group == parch_group){
				printf("%s:",parch_chan->pname);
				if (parch_chan->connected == 0)
					printf(" not connected");
				else
					printf(" connected    "); 
				tsStampToText(&parch_chan->last_saved,TS_TEXT_MMDDYY,ts_string);  
				printf(" saved %s",ts_string);
				if (parch_chan->monitor)
					printf(" archive on change\n");
				else
					printf(" archive at %6.2f interval\n",parch_chan->sample_freq);
				found = 1;
			}
			pcheck_group_list = pcheck_group_list->pnext_group;
		}
		parch_chan = parch_chan->pnext;
	}
}
/* frequency callback */
void freqCB(
Widget		widget, 
XtPointer	client_data, 
XtPointer	call_data)
{
	struct arch_chan	*parch_chan = (struct arch_chan *)client_data;
	char			buf[80];
	double			dfreq;
	float			new_freq;
	char			*endp;
	struct arch_event	arch_event;
	
	/* issue the channel access connection request for the new channel */
	strcpy(buf,XmTextFieldGetString(widget));
	dfreq = atof(&buf[0]);
	new_freq = parch_chan->sample_freq;
	parch_chan->monitor = 0;
	if ((buf[0] == 'M') || (buf[0] == 'm')){
		parch_chan->monitor = 1;
	}else if(dfreq <= 0.0){
		printf("bad number\n");
	}else {
/* need to determine if it is moved from ca_get list */
/* delete it from existing list and put it on new list */
/* or monitor it */
		new_freq = dfreq;
		/* add change frequency event */
		arch_event.event_type = ARCH_CHANGE_FREQ;
		arch_event.stamp = parch_chan->save_time;
		arch_event.frequency = new_freq;
		archiver_event(parch_chan,&arch_event);

	}
	if (parch_chan->monitor == 0){
		sprintf(buf,"%6.2f",new_freq);
		XmTextSetString(parch_chan->frequency_widget,buf);
	}else{
		XmTextSetString(parch_chan->frequency_widget,"Monitor");
	}
	sprintf(buf,"%6.2f",parch_chan->write_freq);
	XmTextSetString(parch_chan->wfrequency_widget,buf);

}

/* add a new config file - callback */
void configCB(
Widget		widget, 
XtPointer	client_data, 
XtPointer	call_data)
{
	char	buf[120];

	strcpy(buf,XmTextFieldGetString(widget));
	create_data_set(buf,directory_name);
	update_display();
}

/* exit the archiver */
void exitCB(
Widget		widget, 
XtPointer	client_data, 
XtPointer	call_data)
{
	char	buf[120];

	archiver_exit = 1;

}

/* channel search callback */
char			last_buf[120];
struct arch_chan	*last_parch_chan;
void searchCB(
Widget		widget, 
XtPointer	client_data, 
XtPointer	call_data)
{
	char	buf[120];
	struct arch_chan	*parch_chan;
	short			round_once = 0;

	strcpy(buf,XmTextFieldGetString(widget));
	if (strcmp(buf,last_buf) == 0){
		parch_chan = last_parch_chan;
		round_once = 1;
	}else{
		parch_chan = pchan_list;
		strcpy(last_buf,buf);
	}
	while ((parch_chan != 0) || round_once){
		if ((parch_chan == 0) && round_once){
			parch_chan = pchan_list;
			round_once = 0;
		}
		if (strncmp(parch_chan->pname,buf,strlen(buf)) == 0){
			XmScrollVisible(arScroll,parch_chan->chan_widget,0,0);
			last_parch_chan = parch_chan->pnext;
			return;
		}
		parch_chan = parch_chan->pnext;
	}
	last_parch_chan = pchan_list;
}

set_init(
unsigned short	pass
){
    Widget	arForm,temp;
    char	buf[120];
    struct arch_group	*parch_group;
    XmString                save_text;

    parch_group = pgroup_list;
    while (parch_group != 0){
	if (parch_group->time_widget && (pass != 0)){
		parch_group = parch_group->pnext;
		continue;
	}
	arForm = XtVaCreateWidget (NULL,xmFormWidgetClass, arList, NULL);

	save_text = XmStringCreateLocalized("List");
        temp = XtVaCreateManagedWidget("button",
                xmPushButtonWidgetClass,        arForm,
                XmNlabelString,                 save_text,
                XmNtopAttachment,       XmATTACH_FORM,
                XmNbottomAttachment,    XmATTACH_FORM,
                XmNleftAttachment,      XmATTACH_FORM,
                XmNrightAttachment,     XmATTACH_POSITION,
                XmNrightPosition,       5,
                NULL);
        XmStringFree(save_text);
        XtAddCallback (temp, XmNactivateCallback,listCB,(XtPointer)parch_group);

	temp = XtVaCreateManagedWidget("chan", 
		xmTextFieldWidgetClass, arForm,
		XmNeditable,			False,
		XmNcursorPositionVisible,	False,
		XmNalignment,		XmALIGNMENT_BEGINNING,
		XmNtopAttachment,	XmATTACH_FORM,
		XmNbottomAttachment,	XmATTACH_FORM,
		XmNleftAttachment,	XmATTACH_POSITION,
		XmNleftPosition,	5,
		XmNrightAttachment,	XmATTACH_POSITION,
		XmNrightPosition,	25,
		NULL);
	XmTextSetString(temp,parch_group->name);

	parch_group->time_widget
	  = XtVaCreateManagedWidget("", xmTextFieldWidgetClass, arForm,
		XmNeditable,			False,
		XmNcursorPositionVisible,	False,
		XmNtopAttachment,		XmATTACH_WIDGET,
		XmNleftAttachment,		XmATTACH_POSITION,
		XmNleftPosition,		25,
		XmNrightAttachment,		XmATTACH_POSITION,
		XmNrightPosition,		35,
		NULL);
	if (parch_group->last_saved.secPastEpoch != 0)
		tsStampToText(&parch_group->last_saved,TS_TEXT_MMDDYY,buf);
	else strcpy(buf,"Not yet saved");
	XmTextSetString(parch_group->time_widget,buf);
	parch_group->enable_widget
	  = XtVaCreateManagedWidget("", xmTextFieldWidgetClass, arForm,
		XmNeditable,			False,
		XmNcursorPositionVisible,	False,
		XmNtopAttachment,		XmATTACH_WIDGET,
		XmNleftAttachment,		XmATTACH_POSITION,
		XmNleftPosition,		35,
		XmNrightAttachment,		XmATTACH_POSITION,
		XmNrightPosition,		45,
		NULL);
	temp 
	  = XtVaCreateManagedWidget("Frequency", xmTextFieldWidgetClass, arForm,
		XmNtopAttachment,		XmATTACH_FORM,
		XmNbottomAttachment,		XmATTACH_FORM,
		XmNleftAttachment,		XmATTACH_POSITION,
		XmNleftPosition,		45,
		XmNrightAttachment,		XmATTACH_POSITION,
		XmNrightPosition,		55,
		NULL);
	sprintf(buf,"%d",parch_group->num_channels);
	XmTextSetString(temp,buf);
	parch_group->connected_widget
	  = XtVaCreateManagedWidget("Write Frequency", 
		xmTextFieldWidgetClass, 	arForm,
		XmNeditable,			False,
		XmNcursorPositionVisible,	False,
		XmNtopAttachment,		XmATTACH_FORM,
		XmNbottomAttachment,		XmATTACH_FORM,
		XmNleftAttachment,		XmATTACH_POSITION,
		XmNleftPosition,		55,
		XmNrightAttachment,		XmATTACH_POSITION,
		XmNrightPosition,		65,
		NULL);
	sprintf(buf,"%d",parch_group->num_connected);
	XmTextSetString(parch_group->connected_widget,buf);
	XtManageChild (arForm);

	parch_group = parch_group->pnext;
    }
}
set_display()
{
	Widget	arForm;

	/* labels for each column */
	if (!first_screen){
		XtUnmanageChild(arScroll,arList,NULL);
		first_screen = 1;
	}
	/* create scrolled window with rows of form widgets for each archive channel */
	if (first_screen)
		arScroll = XtVaCreateManagedWidget("scroll window",
			xmScrolledWindowWidgetClass,	arGuiInfo.pane,
			XmNwidth,			600,
			XmNheight,			300,
			XmNscrollingPolicy,		XmAUTOMATIC,
			NULL);

	arList = XtVaCreateWidget("arList",
		xmRowColumnWidgetClass, arScroll,
		XmNpacking,		XmPACK_COLUMN,
		XmNnumColumns,		MAXITEMS,
		XmNorientation,		XmHORIZONTAL,
		XmNisAligned,		True,
		XmNentryAlignment,	XmALIGNMENT_END,
		NULL);
	arForm = XtVaCreateWidget (NULL,xmFormWidgetClass, arList, 
		XmNwidth,			600,
		NULL);
	XtVaCreateManagedWidget("Set_name", xmLabelGadgetClass, arForm,
		XmNalignment,		XmALIGNMENT_BEGINNING,
		XmNtopAttachment,	XmATTACH_FORM,
		XmNbottomAttachment,	XmATTACH_FORM,
		XmNleftAttachment,	XmATTACH_FORM,
		XmNrightAttachment,	XmATTACH_POSITION,
		XmNrightPosition,	25,
		NULL);
	XtVaCreateManagedWidget("Last Saved Time", xmLabelGadgetClass, arForm,
		XmNeditable,			False,
		XmNcursorPositionVisible,	False,
		XmNtopAttachment,		XmATTACH_FORM,
		XmNbottomAttachment,		XmATTACH_FORM,
		XmNleftAttachment,		XmATTACH_POSITION,
		XmNleftPosition,		25,
		XmNrightAttachment,		XmATTACH_POSITION,
		XmNrightPosition,		35,
		NULL);
	XtVaCreateManagedWidget("Disabled", xmLabelGadgetClass, arForm,
		XmNeditable,			False,
		XmNcursorPositionVisible,	False,
		XmNtopAttachment,		XmATTACH_FORM,
		XmNbottomAttachment,		XmATTACH_FORM,
		XmNleftAttachment,		XmATTACH_POSITION,
		XmNleftPosition,		35,
		XmNrightAttachment,		XmATTACH_POSITION,
		XmNrightPosition,		45,
		NULL);
	XtVaCreateManagedWidget("Num Channels", xmLabelGadgetClass, arForm,
		XmNeditable,			False,
		XmNcursorPositionVisible,	False,
		XmNtopAttachment,		XmATTACH_FORM,
		XmNbottomAttachment,		XmATTACH_FORM,
		XmNleftAttachment,		XmATTACH_POSITION,
		XmNleftPosition,		45,
		XmNrightAttachment,		XmATTACH_POSITION,
		XmNrightPosition,		55,
		NULL);
	XtVaCreateManagedWidget("Num Connected", xmLabelGadgetClass, arForm,
		XmNeditable,			False,
		XmNcursorPositionVisible,	False,
		XmNtopAttachment,		XmATTACH_FORM,
		XmNbottomAttachment,		XmATTACH_FORM,
		XmNleftAttachment,		XmATTACH_POSITION,
		XmNleftPosition,		55,
		XmNrightAttachment,		XmATTACH_POSITION,
		XmNrightPosition,		65,
		NULL);

	XtManageChild(arForm);
	XtManageChild(arList);
	if (first_screen) XtManageChild(arScroll);
	first_screen = 0;

	set_init(0);
}
set_update()
{
    char		buf[256];
    struct arch_group	*parch_group;

    parch_group = pgroup_list;
    while (parch_group != 0){
	if (parch_group->time_widget == 0){
		parch_group = parch_group->pnext;
		continue;
	}
	tsStampToText(&parch_group->last_saved,TS_TEXT_MMDDYY,buf);
	XmTextSetString(parch_group->time_widget,buf);
	sprintf(buf,"%d",parch_group->num_connected);
	XmTextSetString(parch_group->connected_widget,buf);
	if (parch_group->disabled)
		sprintf(buf,"Disabled 0x%x",parch_group->disabled);
	else
		strcpy(buf,"Enabled");
	XmTextSetString(parch_group->enable_widget,buf);
	sprintf(buf,"%d",parch_group->num_connected);
	XmTextSetString(parch_group->connected_widget,buf);
 	parch_group = parch_group->pnext;
   }
}
/* one line in the form for each channel */
channel_init(
unsigned short	pass)
{
    char		buf[120];
    Widget		arForm;
    struct arch_chan	*parch_chan;

    parch_chan = pchan_list;
    while (parch_chan != 0){
	if (parch_chan->chan_widget && (pass != 0)){
		/* may have changed the write frequency through the config file */
		sprintf(buf,"%6.2f",parch_chan->write_freq);
		XmTextSetString(parch_chan->wfrequency_widget,buf);
		parch_chan = parch_chan->pnext;
		continue;
	}
	arForm = XtVaCreateWidget (NULL,xmFormWidgetClass, arList, NULL);

	parch_chan->chan_widget = XtVaCreateManagedWidget("chan", 
		xmLabelGadgetClass, 	arForm,
		XmNalignment,		XmALIGNMENT_BEGINNING,
		XmNtopAttachment,	XmATTACH_FORM,
		XmNbottomAttachment,	XmATTACH_FORM,
		XmNleftAttachment,	XmATTACH_FORM,
		XmNrightAttachment,	XmATTACH_POSITION,
		XmNrightPosition,	25,
		NULL);
	XtVaSetValues(parch_chan->chan_widget, 
		XmNlabelString, XmStringCreateLocalized(parch_chan->pname), NULL);

	parch_chan->time_widget 
	  = XtVaCreateManagedWidget("Save Time", xmTextFieldWidgetClass, arForm,
		XmNeditable,			False,
		XmNcursorPositionVisible,	False,
		XmNtopAttachment,		XmATTACH_FORM,
		XmNbottomAttachment,		XmATTACH_FORM, 
		XmNleftAttachment,		XmATTACH_POSITION,
		XmNleftPosition,		25,
		XmNrightAttachment,		XmATTACH_POSITION,
		XmNrightPosition,		35,
		NULL);
	tsStampToText(&parch_chan->last_saved,TS_TEXT_MMDDYY,buf);
	XmTextSetString(parch_chan->time_widget,buf);
	parch_chan->disable_widget 
	  = XtVaCreateManagedWidget("Disable", xmTextFieldWidgetClass, arForm,
		XmNeditable,			False,
		XmNcursorPositionVisible,	False,
		XmNtopAttachment,		XmATTACH_FORM,
		XmNbottomAttachment,		XmATTACH_FORM, 
		XmNleftAttachment,		XmATTACH_POSITION,
		XmNleftPosition,		35,
		XmNrightAttachment,		XmATTACH_POSITION,
		XmNrightPosition,		45,
		NULL);
	if (parch_chan->disabled)
		sprintf(buf,"Disabled 0x%x",parch_chan->disabled);
	else
		strcpy(buf,"Enabled");
	XmTextSetString(parch_chan->disable_widget,buf);
	parch_chan->frequency_widget 
	  = XtVaCreateManagedWidget("Frequency", xmTextFieldWidgetClass, arForm,
		XmNtopAttachment,		XmATTACH_FORM,
		XmNbottomAttachment,		XmATTACH_FORM,
		XmNleftAttachment,		XmATTACH_POSITION,
		XmNleftPosition,		45,
		XmNrightAttachment,		XmATTACH_POSITION,
		XmNrightPosition,		55,
		NULL);
	if (parch_chan->monitor == 0){
		sprintf(buf,"%6.2f",parch_chan->sample_freq);
		XmTextSetString(parch_chan->frequency_widget,buf);
	}else{
		XmTextSetString(parch_chan->frequency_widget,"Monitor");
	}
	XtAddCallback(parch_chan->frequency_widget, XmNactivateCallback, freqCB, (XtPointer)parch_chan);
	parch_chan->wfrequency_widget 
	  = XtVaCreateManagedWidget("Write Frequency", xmTextFieldWidgetClass, arForm,
		XmNeditable,			False,
		XmNcursorPositionVisible,	False,
		XmNtopAttachment,		XmATTACH_FORM,
		XmNbottomAttachment,		XmATTACH_FORM,
		XmNleftAttachment,		XmATTACH_POSITION,
		XmNleftPosition,		55,
		XmNrightAttachment,		XmATTACH_POSITION,
		XmNrightPosition,		65,
		NULL);
	sprintf(buf,"%6.2f",parch_chan->write_freq);
	XmTextSetString(parch_chan->wfrequency_widget,buf);
	parch_chan->status_widget 
	  = XtVaCreateManagedWidget("Connection Status", 
		xmTextFieldWidgetClass, 	arForm,
		XmNeditable,			False,
		XmNcursorPositionVisible,	False,
		XmNtopAttachment,		XmATTACH_FORM,
		XmNbottomAttachment,		XmATTACH_FORM,
		XmNleftAttachment,		XmATTACH_POSITION,
		XmNleftPosition,		65,
		XmNrightAttachment,		XmATTACH_POSITION,
		XmNrightPosition,		75,
		NULL);
	XmTextSetString(parch_chan->status_widget,"Not Connected");

/*	parch_chan->filename_widget 
	  = XtVaCreateManagedWidget("config", xmTextFieldWidgetClass, arForm,
		XmNeditable,			False,
		XmNcursorPositionVisible,	False,
		XmNtopAttachment,		XmATTACH_FORM,
		XmNbottomAttachment,		XmATTACH_FORM,
		XmNleftAttachment,		XmATTACH_POSITION,
		XmNleftPosition,		75,
		XmNrightAttachment,		XmATTACH_POSITION,
		XmNrightPosition,		100,
		NULL);
*/
	XtManageChild (arForm);
	parch_chan = parch_chan->pnext;
/*	XmTextSetString(parch_chan->filename_widget,pfile_name);
*/
/* where do we get set name from ?*/

    }
}


channel_display()
{
	Widget	rowcol,arForm;
	/* create scrolled window with rows of form widgets for each archive channel */
	if (first_screen)
		arScroll = XtVaCreateManagedWidget("scroll window",
			xmScrolledWindowWidgetClass,	arGuiInfo.pane,
			XmNwidth,			600,
			XmNheight,			300,
			XmNscrollingPolicy,		XmAUTOMATIC,
			NULL);

	/* labels for each column */
	if (!first_screen){
		XtUnmanageChild(arList);
		first_screen = 1;
	}
	arList = XtVaCreateWidget("arList",
		xmRowColumnWidgetClass, arScroll,
		XmNpacking,		XmPACK_COLUMN,
		XmNnumColumns,		MAXITEMS,
		XmNorientation,		XmHORIZONTAL,
		XmNisAligned,		True,
		XmNentryAlignment,	XmALIGNMENT_END,
		NULL);
	arForm = XtVaCreateWidget (NULL,xmFormWidgetClass, arList, 
		XmNwidth,			600,
		NULL);
	XtVaCreateManagedWidget("Channel Name", xmLabelGadgetClass, arForm,
		XmNalignment,		XmALIGNMENT_BEGINNING,
		XmNtopAttachment,	XmATTACH_FORM,
		XmNbottomAttachment,	XmATTACH_FORM,
		XmNleftAttachment,	XmATTACH_FORM,
		XmNrightAttachment,	XmATTACH_POSITION,
		XmNrightPosition,	25,
		NULL);
	XtVaCreateManagedWidget("Last Saved Time", xmLabelGadgetClass, arForm,
		XmNeditable,			False,
		XmNcursorPositionVisible,	False,
		XmNtopAttachment,		XmATTACH_FORM,
		XmNbottomAttachment,		XmATTACH_FORM,
		XmNleftAttachment,		XmATTACH_POSITION,
		XmNleftPosition,		25,
		XmNrightAttachment,		XmATTACH_POSITION,
		XmNrightPosition,		35,
		NULL);
	XtVaCreateManagedWidget("Disable Status", xmLabelGadgetClass, arForm,
		XmNeditable,			False,
		XmNcursorPositionVisible,	False,
		XmNtopAttachment,		XmATTACH_FORM,
		XmNbottomAttachment,		XmATTACH_FORM,
		XmNleftAttachment,		XmATTACH_POSITION,
		XmNleftPosition,		35,
		XmNrightAttachment,		XmATTACH_POSITION,
		XmNrightPosition,		45,
		NULL);
	XtVaCreateManagedWidget("Archive Frequency", xmLabelGadgetClass, arForm,
		XmNeditable,			False,
		XmNcursorPositionVisible,	False,
		XmNtopAttachment,		XmATTACH_FORM,
		XmNbottomAttachment,		XmATTACH_FORM,
		XmNleftAttachment,		XmATTACH_POSITION,
		XmNleftPosition,		45,
		XmNrightAttachment,		XmATTACH_POSITION,
		XmNrightPosition,		55,
		NULL);
	XtVaCreateManagedWidget("Write Frequency", xmLabelGadgetClass, arForm,
		XmNeditable,			False,
		XmNcursorPositionVisible,	False,
		XmNtopAttachment,		XmATTACH_FORM,
		XmNbottomAttachment,		XmATTACH_FORM,
		XmNleftAttachment,		XmATTACH_POSITION,
		XmNleftPosition,		55,
		XmNrightAttachment,		XmATTACH_POSITION,
		XmNrightPosition,		65,
		NULL);
	XtVaCreateManagedWidget("Connection Status", xmLabelGadgetClass, arForm,
		XmNeditable,			False,
		XmNcursorPositionVisible,	False,
		XmNtopAttachment,		XmATTACH_FORM,
		XmNbottomAttachment,		XmATTACH_FORM,
		XmNleftAttachment,		XmATTACH_POSITION,
		XmNleftPosition,		65,
		XmNrightAttachment,		XmATTACH_POSITION,
		XmNrightPosition,		75,
		NULL);

	XtManageChild(arForm);
	XtManageChild(arList);
	if (first_screen) XtManageChild(arScroll);
	first_screen = 0;
	channel_init(0);
}
/* one line in the form for each channel */
chan_update()
{
    char		buf[120];
    Widget		arForm;
    struct arch_chan	*parch_chan;
unsigned short i;

    parch_chan = pchan_list;
    while (parch_chan != 0){
	if (parch_chan->chan_widget == 0){
		parch_chan = parch_chan->pnext;
		continue;
	}

	tsStampToText(&parch_chan->last_saved,TS_TEXT_MMDDYY,buf);
	XmTextSetString(parch_chan->time_widget,buf);
	if (parch_chan->disabled)
		sprintf(buf,"Disabled 0x%x",parch_chan->disabled);
	else
		strcpy(buf,"Enabled");
	XmTextSetString(parch_chan->disable_widget,buf);
	if (parch_chan->connected_changed){
	    if (parch_chan->connected){
		XmTextSetString(parch_chan->status_widget,"Connected");
	    }else{
		XmTextSetString(parch_chan->status_widget,"Not Connected");
	    }
	    parch_chan->connected_changed = 0;
	}
	parch_chan = parch_chan->pnext;
    }
}

update_time_conn()
{
	if (no_display){
printf("no updates - no display\n");
 return;
}
	if (display_channels){
		chan_update();
	}else{
		set_update();
	}
}


my_display()
{
	register 		i;
	struct arch_chan	*parch_chan;
	char			buf[120];
	XmString		exit_text;
	Widget			rowcol,arForm,arForm1,dirLabel,dirEntry;
	Widget			fileLabel,fileEntry,searchLabel,searchEntry,exitButton;

	if (no_display) return;

	/* create the main window shell */
	arGuiInfo.mainShell = XtVaCreateWidget("main_window",
		xmMainWindowWidgetClass,arGuiInfo.topLevel,NULL);

	arGuiInfo.pane = XtVaCreateWidget("pane", xmPanedWindowWidgetClass,
		arGuiInfo.mainShell,NULL);

	/* exit button */
	arForm1 = XtVaCreateWidget (NULL,xmFormWidgetClass, arGuiInfo.pane, NULL);
	exit_text = XmStringCreateLocalized("Exit");
	exitButton = XtVaCreateManagedWidget("button",
                xmPushButtonWidgetClass,        arForm1,
                XmNlabelString,                 exit_text,
                XmNtopAttachment,               XmATTACH_FORM,
                XmNrightAttachment,              XmATTACH_FORM,   

		NULL);
	XmStringFree(exit_text);
	XtAddCallback (exitButton, XmNactivateCallback, exitCB, NULL);
	XtManageChild(arForm1);

	/* put command interaction in the top pane */
	rowcol = XtVaCreateWidget("rowcol",
		xmRowColumnWidgetClass, arGuiInfo.pane,
		XmNpacking,		XmPACK_COLUMN,
		XmNnumColumns,		1,
		XmNorientation,		XmVERTICAL,
		NULL);
	arForm1 = XtVaCreateWidget (NULL,xmFormWidgetClass, rowcol, NULL);

	dirLabel = XtVaCreateManagedWidget( "Directory File Name:",
		xmLabelGadgetClass,	arForm1,
		XmNalignment,		XmALIGNMENT_BEGINNING,
		XmNtopAttachment,	XmATTACH_FORM,
		XmNleftAttachment,	XmATTACH_FORM,
		XmNrightAttachment,	XmATTACH_POSITION,
		XmNrightPosition,	25,
		NULL);
	dirEntry = XtVaCreateManagedWidget(directory_name,
		xmLabelGadgetClass, 		arForm1,
		XmNalignment,			XmALIGNMENT_BEGINNING,
		XmNeditable,			False,
		XmNcursorPositionVisible,	False,
		XmNtopAttachment,		XmATTACH_FORM,
		XmNleftAttachment,		XmATTACH_POSITION,
		XmNleftPosition,		25,
		XmNrightAttachment,		XmATTACH_FORM,
		NULL);
/*	XmTextSetString(dirEntry,directory_name); */
	XtManageChild(arForm1);

	arForm1 = XtVaCreateWidget (NULL,xmFormWidgetClass, rowcol, NULL);
	searchLabel = XtVaCreateManagedWidget( "Channel Search:",
		xmLabelGadgetClass,	arForm1,
		XmNalignment,		XmALIGNMENT_BEGINNING,
		XmNtopAttachment,	XmATTACH_WIDGET,
		XmNtopWidget,		dirLabel,
		XmNleftAttachment,	XmATTACH_FORM,
		XmNrightAttachment,	XmATTACH_POSITION,
		XmNrightPosition,	25,
		NULL);
	searchEntry = XtVaCreateManagedWidget("chan", 
		xmTextFieldWidgetClass, arForm1,
		XmNtopAttachment,	XmATTACH_WIDGET,
		XmNtopWidget,		dirEntry,
		XmNleftAttachment,	XmATTACH_POSITION,
		XmNleftPosition,	25,
		XmNrightAttachment,	XmATTACH_FORM,
		NULL);
	XtAddCallback(searchEntry, XmNactivateCallback, searchCB, (XtPointer)NULL);
	XtManageChild(arForm1);

	arForm1 = XtVaCreateWidget (NULL,xmFormWidgetClass, rowcol, NULL);
	fileLabel = XtVaCreateManagedWidget("Add Config File:",
		xmLabelGadgetClass,	arForm1,
		XmNalignment,		XmALIGNMENT_BEGINNING,
		XmNtopAttachment,	XmATTACH_WIDGET,
		XmNtopWidget,		searchLabel,
		XmNleftAttachment,	XmATTACH_FORM,
		XmNrightAttachment,	XmATTACH_POSITION,
		XmNrightPosition,	25,
		NULL);
	fileEntry = XtVaCreateManagedWidget("file", 
		xmTextFieldWidgetClass, arForm1,
		XmNtopAttachment,	XmATTACH_WIDGET,
		XmNtopWidget,		searchEntry,
		XmNleftAttachment,	XmATTACH_POSITION,
		XmNleftPosition,	25,
		XmNrightAttachment,	XmATTACH_FORM,
		NULL);
	XtAddCallback(fileEntry, XmNactivateCallback, configCB, (XtPointer)NULL);
	XtManageChild(arForm1);

	XtManageChild(rowcol);
	XtManageChild(arGuiInfo.pane);

	display();

	XtManageChild(arGuiInfo.mainShell);
	XtRealizeWidget(arGuiInfo.topLevel);
}
display()
{
	if (num_channels < max_chan_displayed){
		channel_display();
		display_channels = 1;
	}else{
		set_display();
		display_channels = 0;
	}
}

update_display()
{
	if (num_channels < max_chan_displayed){
		/* transition to channel display requires redraw */
		if (!display_channels)
			channel_display();
		/* new channels may have been added */
		else
			channel_init(1);
		display_channels = 1;
	}else{
		/* transition to set display requires redraw */
		if (display_channels)
			set_display();
		/* new set may have been added */
		else
			set_init(1);
		display_channels = 0;
	}
}

/* initialize the motif display */
motif_init(
int *pargc, 
char *argv[])
{
	int	x_fd;
	void	*dummy;

	if (no_display) return;
	XtSetLanguageProc(NULL ,NULL, NULL);
	arGuiInfo.topLevel = XtVaAppInitialize (&(arGuiInfo.arApp), 
		"topLevel", NULL, 0, pargc, argv, NULL,/* user function */ NULL /* user arg */);

	x_fd = ConnectionNumber(XtDisplay(arGuiInfo.topLevel));
	fdmgr_add_fd(pfdctx,x_fd,x_evhandler,dummy);
}
