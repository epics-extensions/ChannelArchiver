; CLW file contains information for the MFC ClassWizard

[General Info]
Version=1
LastClass=CBrowserDlg
LastTemplate=CDialog
NewFileInclude1=#include "stdafx.h"
NewFileInclude2=#include "WinBrowser.h"

ClassCount=4
Class1=CBrowserApp
Class2=CBrowserDlg
Class3=CAboutDlg

ResourceCount=4
Resource1=IDD_WINBROWSER_DIALOG
Resource2=IDR_MAINFRAME
Resource3=IDD_ABOUTBOX
Class4=CDlgExport
Resource4=IDD_EXPORT

[CLS:CBrowserApp]
Type=0
HeaderFile=WinBrowser.h
ImplementationFile=WinBrowser.cpp
Filter=N

[CLS:CBrowserDlg]
Type=0
HeaderFile=BrowserDlg.h
ImplementationFile=BrowserDlg.cpp
Filter=D
BaseClass=CDialog
VirtualFilter=dWC
LastObject=IDC_USE_REGEX

[CLS:CAboutDlg]
Type=0
HeaderFile=BrowserDlg.h
ImplementationFile=BrowserDlg.cpp
Filter=D

[DLG:IDD_ABOUTBOX]
Type=1
Class=CAboutDlg
ControlCount=4
Control1=IDC_STATIC,static,1342177283
Control2=IDC_STATIC,static,1342308480
Control3=IDC_STATIC,static,1342308352
Control4=IDOK,button,1342373889

[DLG:IDD_WINBROWSER_DIALOG]
Type=1
Class=CBrowserDlg
ControlCount=32
Control1=IDC_STATIC,static,1342308352
Control2=IDC_DIR_NAME,static,1342312448
Control3=IDC_SELECT_DIR,button,1342242816
Control4=IDC_OK,button,1073807361
Control5=IDC_STATIC,static,1342308352
Control6=IDC_PATTERN,edit,1350635652
Control7=IDC_STATIC,static,1342308352
Control8=IDC_START_DATE,SysDateTimePick32,1342242848
Control9=IDC_START_TIME,SysDateTimePick32,1342242841
Control10=IDC_STATIC,static,1342308352
Control11=IDC_END_DATE,SysDateTimePick32,1342242848
Control12=IDC_END_TIME,SysDateTimePick32,1342242841
Control13=IDC_CHANNEL_LIST,listbox,1353779459
Control14=IDC_SHOW,button,1342242816
Control15=IDC_STATIC,static,1342308352
Control16=IDC_ZOOM_IN_Y,button,1342242816
Control17=IDC_ZOOM_IN,button,1342242816
Control18=IDC_ZOOM_OUT_Y,button,1342242816
Control19=IDC_ZOOM_OUT,button,1342242816
Control20=IDC_AUTOZOOM,button,1342242816
Control21=IDC_UP,button,1342242816
Control22=IDC_BACK,button,1342242816
Control23=IDC_FORW,button,1342242816
Control24=IDC_DOWN,button,1342242816
Control25=IDC_CLEAR,button,1342242816
Control26=IDC_PRINT,button,1342242816
Control27=IDC_EXPORT,button,1342242816
Control28=IDC_PLOT,static,1342177543
Control29=IDC_STATIC,static,1342308352
Control30=IDC_CHANNEL_STATUS,static,1342308352
Control31=IDC_USE_REGEX,button,1342242819
Control32=IDC_ADJUST_LIMITS,button,1342242816

[DLG:IDD_EXPORT]
Type=1
Class=CDlgExport
ControlCount=14
Control1=IDC_STATIC,static,1342308352
Control2=IDC_EXPORT_FILENAME,edit,1350631552
Control3=IDC_PICK_EXPORT_NAME,button,1342242816
Control4=IDC_STATIC,button,1342177287
Control5=IDC_GNUPLOT,button,1342308361
Control6=IDC_SPREADSHEET,button,1342177289
Control7=IDC_MATLAB,button,1342177289
Control8=IDC_RAW,button,1342308361
Control9=IDC_INTERPOL,button,1342177289
Control10=IDC_SECONDS,edit,1350631552
Control11=IDC_FILLED,button,1342177289
Control12=IDOK,button,1342373889
Control13=IDCANCEL,button,1342242816
Control14=IDC_STATIC,static,1342308352

[CLS:CDlgExport]
Type=0
HeaderFile=DlgExport.h
ImplementationFile=DlgExport.cpp
BaseClass=CDialog
Filter=D
LastObject=CDlgExport
VirtualFilter=dWC

