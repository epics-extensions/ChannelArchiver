/* the X library */
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xos.h>
#include <X11/Xatom.h>
#include <X11/Xresource.h>
/* includes for Motif */
#include <X11/Intrinsic.h>
#include <Xm/Xm.h>
#include <Xm/MainW.h>
#include <Xm/PanedW.h>
#include <Xm/Command.h>
#include <Xm/ScrolledW.h>
#include <Xm/Form.h>
#include <Xm/List.h>
#include <Xm/MessageB.h>
#include <Xm/TextF.h>
#include <Xm/Text.h>
#include <Xm/Frame.h>
#include <Xm/LabelG.h>
#include <Xm/PushBG.h>
#include <Xm/CascadeB.h>
#include <Xm/BulletinB.h>
#include <Xm/RowColumn.h>
#include <Xm/SeparatoG.h>
#include <Xm/ToggleB.h>
#include <Xm/PushB.h>
#include <Xm/RowColumn.h>

struct  faGuiInfo{
	Widget topLevel;
	Widget mainShell;
	Widget pane;
	XtAppContext arApp;
};
#define	MAXITEMS	200
