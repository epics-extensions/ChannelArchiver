#$Id$
#######################################################################################
#Copyright 2000. The Regents of the University of California.
#This software was produced under U.S. Government contract W-7405-ENG-36
#for Los Alamos National Laboratory, which is operated by the University of California
#for the U.S. Department of Energy.
#The Government is granted for itself and others acting on its behalf a paid-up,
#nonexclusive irrevocable worldwide license in this software to reproduce,
#prepare derivative works, and perform publicly and display publicly.
#NEITHER THE UNITED STATES NOR THE UNITED STATES DEPARTMENT OF ENERGY,
#NOR THE UNIVERSITY OF CALIFORNIA, NOR ANY OF THEIR EMPLOYEES,
#MAKES ANY WARRANTY, EXPRESS OR IMPLIED,
#OR ASSUMES ANY LEGAL LIABILITY OR RESPONSIBILITY FOR THE ACCURACY,
#COMPLETENESS, OR USEFULNESS OF ANY INFORMATION, APPARATUS,
#PRODUCT OR PROCESS DISCLOSED,
#OR REPRESENTS THAT ITS USE WOULD NOT INFRINGE PRIVATELY OWNED RIGHTS.          
#######################################################################################

from Tkinter import *
import Pmw
import ProgressBar
from CArDMiner_utilities import Command

class AppShell(Pmw.MegaWidget):        
    """
    This is a really nifty app template class that I got
    from the book Python and Tkinter
    """

    appversion      = '1.0'
    appname         = 'CArDMiner'
    copyright       = 'Copyright 2000 LANL\nLANSCE-8 (Accelerator Controls and Automation)\nAll Rights Reserved'
    contactname     = 'Nick D. Pattengale'
    contactphone    = '(505) 665-2844'
    contactemail    = 'nickp@lanl.gov'
          
    frameWidth      = 640
    frameHeight     = 480
    padx            = 5
    pady            = 5
    usecommandarea  = 0
    balloonhelp     = 1
    
    busyCursor = 'watch'
    
    def __init__(self, **kw):
        optiondefs = (
            ('padx',           1,                   Pmw.INITOPT),
            ('pady',           1,                   Pmw.INITOPT),
            ('framewidth',     1,                   Pmw.INITOPT),
            ('frameheight',    1,                   Pmw.INITOPT),
            ('usecommandarea', self.usecommandarea, Pmw.INITOPT))
        self.defineoptions(kw, optiondefs)
        
        self.root = Tk()
        self.initializeTk(self.root)
        Pmw.initialise(self.root)
        self.root.title(self.appname)
        self.root.geometry('%dx%d' % (self.frameWidth, self.frameHeight))

        # Initialize the base class
        Pmw.MegaWidget.__init__(self, parent=self.root)
        
        # initialize the application
        self.appInit()

        # create the interface
        self.__createInterface()
        
        # create a table to hold the cursors for
        # widgets which get changed when we go busy
        self.preBusyCursors = None
        
        # pack the container and set focus
        # to ourselves
        self._hull.pack(side=TOP, fill=BOTH, expand=YES)
        self.focus_set()

        # initialize our options
        self.initialiseoptions(AppShell)
        
    def appInit(self):
        # Called before interface is created (should be overridden).
        pass
        
    def initializeTk(self, root):
        # Initialize platform-specific options
        if sys.platform == 'mac':
            self.__initializeTk_mac(root)
        elif sys.platform == 'win32':
            self.__initializeTk_win32(root)
        else:
            self.__initializeTk_unix(root)

    def __initializeTk_colors_common(self, root):
        root.option_readfile ('optionDB')
        #root.option_add('*background', 'grey')
        #root.option_add('*foreground', 'black')
        root.option_add('*EntryField.Entry.background', 'white')
        #root.option_add('*Entry.background', 'white')        
        #root.option_add('*MessageBar.Entry.background', 'gray85')
        root.option_add('*Listbox*background', 'white')
        root.option_add('*Listbox*selectBackground', 'dark slate blue')
        #root.option_add('*Listbox*selectForeground', 'white')
        root.option_add('*Button*background', 'gray')
        root.option_add('*ScrolledText*background', 'white')
                        
    def __initializeTk_win32(self, root):
        self.__initializeTk_colors_common(root)
        root.option_add('*Font', 'Verdana 10 bold')
        root.option_add('*EntryField.Entry.Font', 'Courier 10')
        root.option_add('*Listbox*Font', 'Courier 10')
        
    def __initializeTk_mac(self, root):
        self.__initializeTk_colors_common(root)
        
    def __initializeTk_unix(self, root):
        self.__initializeTk_colors_common(root)

    def busyStart(self, newcursor=None):
        if not newcursor:
            newcursor = self.busyCursor
        newPreBusyCursors = {}
        for component in self.busyWidgets:
            newPreBusyCursors[component] = component['cursor']
            component.configure(cursor=newcursor)
            component.update_idletasks()
        self.preBusyCursors = (newPreBusyCursors, self.preBusyCursors)
        
    def busyEnd(self):
        if not self.preBusyCursors:
            return
        oldPreBusyCursors = self.preBusyCursors[0]
        self.preBusyCursors = self.preBusyCursors[1]
        for component in self.busyWidgets:
            try:
                component.configure(cursor=oldPreBusyCursors[component])
            except KeyError:
                pass
            component.update_idletasks()
              
    def __createAboutBox(self):
        Pmw.aboutversion(self.appversion)
        Pmw.aboutcopyright(self.copyright)
        Pmw.aboutcontact(
          'For more information, contact:\n %s\n Phone: %s\n Email: %s' %\
                      (self.contactname, self.contactphone, 
                       self.contactemail))
        self.about = Pmw.AboutDialog(self._hull, 
                                     applicationname=self.appname)
        self.about.withdraw()
        return None
       
    def showAbout(self):
        # Create the dialog to display about and contact information.
        self.about.show()
        self.about.focus_set()
       
    def toggleBalloon(self):
	if self.toggleBalloonVar.get():
            self.__balloon.configure(state = 'both')
	else:
            self.__balloon.configure(state = 'status')

    def __createMenuBar(self):
        self.menuBar = self.createcomponent('menubar', (), None,
                                            Pmw.MenuBar,
                                            (self._hull,),
                                            hull_relief=RAISED,
                                            hull_borderwidth=1,
                                            balloon=self.balloon())

        self.menuBar.pack(fill=X)
        self.menuBar.addmenu('Help', 'About %s' % self.appname, side='right')
        self.menuBar.addmenu('File', 'File Operations Utilities')
                            
    def createMenuBar(self):
        self.menuBar.addmenuitem('Help', 'command',
                                 'Get information about CArDMiner', 
                                 label='About...', command=self.showAbout)
        self.toggleBalloonVar = IntVar()
        self.toggleBalloonVar.set(1)
        self.menuBar.addmenuitem('Help', 'checkbutton',
                                 'Toggle balloon help',
                                 label='Balloon help',
                                 variable = self.toggleBalloonVar,
                                 command=self.toggleBalloon)

        self.menuBar.addmenuitem('File', 'command', 'Open Channel Archive',
                                 label='Open Archive',
                                 command=self.OpenArchive)
        self.menuBar.addmenuitem('File', 'separator')
        self.menuBar.addmenuitem('File', 'command', 'Quit CArDMiner',
                                 label='Quit',
                                 command=self.ShutDown)
                                        
    def __createBalloon(self):
        # Create the balloon help manager for the frame.
        # Create the manager for the balloon help
        self.__balloon = self.createcomponent('balloon', (), None,
                                              Pmw.Balloon, (self._hull,))

    def balloon(self):
        return self.__balloon

    def __createArchiveTag(self):
        # Create data area where data entry widgets are placed.
        self.archiveTagFrame = self.createcomponent('archiveTagFrame',
                                                    (), None,
                                                    Frame, (self._hull,),)
        
        self.SRCarchiveTag = self.createcomponent('SRCarchiveTag',
                                                  (), None,
                                                  Label,
                                                  (self.archiveTagFrame,),
                                                  text="Source Archive: ")
        
        self.DESTarchiveTag = self.createcomponent('DESTarchiveTag',
                                                   (), None,
                                                   Label,
                                                   (self.archiveTagFrame,),
                                                   text="Destination Archive: ")

        self.SRCarchiveTag.pack(anchor=W)
        self.DESTarchiveTag.pack(anchor=W)
        self.archiveTagFrame.pack(side=TOP, anchor=W)

    def __createDataArea(self):
        # Create data area where data entry widgets are placed.
        self.dataArea = self.createcomponent('dataarea',
                                             (), None,
                                             Frame, (self._hull,),
                                             relief=GROOVE, 
                                             bd=1)
        self.dataArea.pack(side=TOP, fill=BOTH, expand=YES,
                           padx=self['padx'], pady=self['pady'])

    def __createCommandArea(self):
        # Create a command area for application-wide buttons.
        self.__commandFrame = self.createcomponent('commandframe', (), None,
                                                   Frame,
                                                   (self._hull,),
                                                   relief=SUNKEN,
                                                   bd=1)
        self.__buttonBox = self.createcomponent('buttonbox', (), None,
                                                Pmw.ButtonBox,
                                                (self.__commandFrame,),
                                                padx=0, pady=0)
        self.__buttonBox.pack(side=RIGHT, expand=NO, fill=X)
        if self['usecommandarea']:
            self.__commandFrame.pack(side=BOTTOM, 
                                     expand=NO, 
                                     fill=X,
                                     padx=self['padx'],
                                     pady=self['pady'])
        
    def __createMessageBar(self):
        # Create the message bar area for help and status messages.
        frame = self.createcomponent('bottomtray', (), None,
                                     Frame,(self._hull,), relief=SUNKEN)
        self.__messageBar = self.createcomponent('messagebar',
                                                  (), None,
                                                 Pmw.MessageBar, 
                                                 (frame,),
                                                 #entry_width = 40,
                                                 entry_relief=SUNKEN,
                                                 entry_bd=1,
                                                 labelpos=None)
        self.__messageBar.pack(side=LEFT, expand=YES, fill=X)

        self.__progressBar = ProgressBar.ProgressBar(frame,
                                                fillColor='slateblue',
                                                doLabel=1,
                                                width=150)
        self.__progressBar.frame.pack(side=LEFT, expand=NO, fill=NONE)

        self.updateProgress(0)
        frame.pack(side=BOTTOM, expand=NO, fill=X)
                   
        self.__balloon.configure(statuscommand = \
                                 self.__messageBar.helpmessage)

    def messageBar(self):
	return self.__messageBar

    def updateProgress(self, newValue=0, newMax=0):
        self.__progressBar.updateProgress(newValue, newMax)

    def bind(self, child, balloonHelpMsg, statusHelpMsg=None):
        # Bind a help message and/or status message to a widget.
        self.__balloon.bind(child, balloonHelpMsg, statusHelpMsg)

    def interior(self):
        # Retrieve the interior site where widgets should go.
        return self.dataArea
    
    def buttonBox(self):
        # Retrieve the button box.
        return self.__buttonBox

    def buttonAdd(self, buttonName, buttonAction, helpMessage=None,
                  statusMessage=None, **kw):
        # Add a button to the button box.
        newBtn = self.__buttonBox.add(buttonName)
        newBtn.configure(kw, command=Command(self.FrameSwitcher, buttonName))
        if helpMessage:
             self.bind(newBtn, helpMessage, statusMessage)
        return newBtn


    def __createInterface(self):
        self.__createBalloon()
        self.__createMenuBar()
        self.__createMessageBar()
        self.__createArchiveTag()
        self.__createCommandArea()        
        self.__createDataArea()
        self.__createAboutBox()
        #
        # Create the parts of the interface
        # which can be modified by subclasses
        #
        self.busyWidgets = ( self.root, )
        self.createMenuBar()
        self.createInterface()

    def createInterface(self):
        # Override this method to create the interface for the app.
        pass
        
    def main(self):
        # This method should be left intact!
        self.pack()
        self.mainloop()
        
    def run(self):
        self.main()
