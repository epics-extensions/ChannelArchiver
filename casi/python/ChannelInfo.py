from Tkinter import *
from ErrorDlg import ErrorDlg
import Pmw, casi, sys

class ChannelInfo:
    """ChannelInfo (root, archive name, channel name)

       Display dialog with information about channel
    """
    def __init__ (self, root, archiveName, channelName):
        # GUI
        root.option_readfile ('optionDB')

        self.__dlg = Pmw.Dialog (root,
                    buttons=('Restart', 'Close'),
                    defaultbutton = 'Close',
                    title = "Channel Info",
                    command = self.callback)
        root = self.__dlg.interior ()

        self.__archiveName = archiveName
        self.__channelName = Pmw.EntryField (root, labelpos='nw',
                                             label_text='Channel Name:',
                                             value = channelName,
                                             command=self.restart)
        self.__start = Pmw.EntryField (root, labelpos='nw', entry_width=30,
                                       label_text='First Time Stamp:')
        self.__end = Pmw.EntryField (root, labelpos='nw', entry_width=30,
                                     label_text='Last Time Stamp:')
        
        # Pack
        self.__channelName.pack (side=TOP, fill=X)
        self.__start.pack (side=TOP, fill=X)
        self.__end.pack (side=TOP, fill=X)

        self.restart ()
        self.__dlg.activate()


    def restart (self):
        archive = casi.archive()
        channel = casi.channel()

        if not archive.open (self.__archiveName):
            self.__start.setentry ('')
            self.__end.setentry ('')
            ErrorDlg (self.__root,
                      "Cannot open archive '%s'" % self.__archiveName)
            return
        if not archive.findChannelByName (self.__channelName.get(), channel):
            self.__start.setentry ('channel not found')
            self.__end.setentry ('')
            return
        self.__start.setentry (channel.getFirstTime())
        self.__end.setentry (channel.getLastTime())

    def callback (self, button):
        if button == 'Restart':
            self.restart()
        else:
	    self.__dlg.deactivate(button)
        
def usage ():
    print "USAGE: %s archive channelName" % sys.argv[0]
    sys.exit (1)

if __name__ == '__main__':
    # Parse args
    if 0:
        archiveName = "../../Engine/Test/freq_directory"
        channelName = "fred"
    else:
        argc = len(sys.argv)
        if argc != 3:
            usage()
        archiveName = sys.argv[1]
        channelName = sys.argv[2]
        
    root = None
    ChannelInfo (Tk(), archiveName, channelName)
