# $Id$
#
# Attempt on a GUI for archive information,
# in definite need of more work...

from Tkinter import *
import tkFileDialog
import Pmw
import sys
import casi
from ListBrowser import ListBrowser

class Browser:
    def showError (self, text):
        Pmw.MessageDialog (self.root, title="Error",
                           buttons=('I see...',),
                           message_text=text).active()

    def menuAbout (self):
        Pmw.aboutversion ('0.1')
        Pmw.aboutcontact ("kasemir@lanl.gov")
        Pmw.AboutDialog (self.root, applicationname="Archive Info")

    def menuOpen (self):
        archiveName = tkFileDialog.askopenfilename ()
        if (archiveName):
            self.archiveName.setentry(archiveName)
            self.listChannels()

    def menuShowChannel (self):
        if len(self.channelNames.curselection()) != 1:
            self.showError ("Please select single channel in channel list")
            return
        i = self.channelNames.curselection()[0]
        channelName = self.channelNames.get(i)
        dlg = ListBrowser (self.root, self.archiveName.get(), channelName)
        dlg.activate ()

    def listChannels (self):
        self.channelNames.delete(0,END)
        self.channelT0.delete(0,END)
        self.channelT1.delete(0,END)
        archive = casi.archive()
        if not archive.open (self.archiveName.get()):
            self.showError ("Cannot open '%s'" % self.archiveName.get())
            return
        channel = casi.channel()
        archive.findChannelByPattern (self.pattern.get(), channel)
	while channel.valid():
            self.channelNames.insert (END, channel.name())
            self.channelT0.insert (END, channel.getFirstTime())
            self.channelT1.insert (END, channel.getLastTime())
            channel.next()
      
    def scrollChannels (self, *args):
        "Callback for scrollbar that scroll multiple listboxes"
        apply (self.channelNames.yview, args)
        apply (self.channelT0.yview, args)
        apply (self.channelT1.yview, args)

    def makeGUI (self, archiveName):
        root = self.root # shortcut
        root.option_readfile ('optionDB')
        menu = Pmw.MenuBar (root)
        menu.addmenu ('File', 'File Functions')
        menu.addmenu ('Channel', 'Channel Functions')
        menu.addmenu ('Help', 'Channel Functions')
        menu.addmenuitem ('File', 'command', 'Open archive',
                          label='Open', command=self.menuOpen)
        menu.addmenuitem ('File', 'command', 'Exit program',
                          label='Quit', command=root.quit)
        menu.addmenuitem ('Channel', 'command', 'Show Channel',
                          label='Show', command=self.menuShowChannel)
        menu.addmenuitem ('Help', 'command', '???',
                          label='About', command=self.menuAbout)
      
        self.archiveName=Pmw.EntryField (root, labelpos=W,
                                         label_text="Archive:",
                                         entry_width=40,
                                         value = archiveName,
                                         command=self.listChannels)
        self.pattern=Pmw.EntryField (root, labelpos=W,
                                         label_text="Pattern: ",
                                         entry_width=10,
                                         value = "",
                                         command=self.listChannels)

        channels = Frame (root)
        channelScroll = Scrollbar (channels, orient=VERTICAL)
        self.channelNames = Listbox (channels, yscrollcommand=channelScroll.set)
        self.channelT0 = Listbox (channels, yscrollcommand=channelScroll.set,
                                  width=28)
        self.channelT1 = Listbox (channels, yscrollcommand=channelScroll.set,
                                  width=28)
        channelScroll.configure (command=self.scrollChannels)

        Label (channels, text="Channels:").pack (side=TOP,anchor=W)
        channelScroll.pack (side=RIGHT, fill=Y)
        self.channelT1.pack (side=RIGHT, expand=Y, fill=BOTH)
        self.channelT0.pack (side=RIGHT, expand=Y, fill=BOTH)
        self.channelNames.pack (side=RIGHT, expand=Y, fill=BOTH)
        # This could be done: self.channelNames.bind ('<ButtonRelease-1>', self.channelInfo)
        
        menu.pack (fill=X)
        self.archiveName.pack(side=TOP, fill=X)
        self.pattern.pack(side=TOP, fill=X)
        channels.pack(side=TOP, expand=Y, fill=BOTH)

    def __init__(self,archiveName):
        self.root=Tk()
        Pmw.initialise()
        self.makeGUI (archiveName)
        self.listChannels ()
        self.root.mainloop()

if __name__ == "__main__":
#    if len(sys.argv) == 2:
#        archiveName = sys.argv[1]
#    else:
#        archiveName = TkFileDialog.askopenfilename ()
    archiveName = "../../Engine/Test/freq_directory"
    if not archiveName: sys.exit()
    Browser (archiveName)



