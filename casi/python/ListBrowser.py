# --------------------------------------------------------
# $Id$
#
# Please refer to NOTICE.txt,
# included as part of this distribution,
# for legal information.
#
# Kay-Uwe Kasemir, kasemir@lanl.gov
# --------------------------------------------------------

from Tkinter import *
import Pmw, casiTools, casi, sys
from ErrorDlg import ErrorDlg
from ChannelInfo import ChannelInfo

class ListBrowser:
    """ListBrowser (root, archive name, channel name, end time stamp)

       Display dialog for browsing archived samples as text
       in ListBox.
    """
    def __init__ (self, root, archiveName, channelName, end='0'):
        """Constructor: root, archiveName and channelName
           have to be provided.
           end default to '0' which means last value
        """
        self.__GUI (root, archiveName, channelName, end)

    def activate (self):
        "Call activate to display & run dialog"
        self.__restart ()
        if self.__dlg.activate () == 'Ok':
            return self.__selectedItems()
        return []

    def __GUI (self, root, archiveName, channelName, end):
        # GUI
        root.option_readfile ('optionDB')
        self.__dlg = Pmw.Dialog (root, buttons=('Ok', 'Cancel'),
                                title='Value Browser (List)')
        root = self.__dlg.interior()
        self.__archiveName = Pmw.EntryField (root, labelpos='nw',
                                             label_text='Archive:',
                                             value = archiveName)
        self.__channelName = Pmw.EntryField (root, labelpos='nw',
                                             label_text='Channel:',
                                             value = channelName)
        self.__end = Pmw.EntryField (root, labelpos='nw',
                                     label_text='End:',
                                     value = end)
        data = Frame (root)
        scroll = Scrollbar (data, orient=VERTICAL)
        self.__times  = Listbox (data, yscrollcommand=scroll.set,
                                 selectmode=EXTENDED,
                                 width=28)
        self.__values = Listbox (data, yscrollcommand=scroll.set,
                                 selectmode=EXTENDED)
        scroll.configure (command=self.__scroll)
        
        self.__times.pack (side=LEFT, fill=Y)
        scroll.pack (side=RIGHT, fill=Y)
        self.__values.pack (side=LEFT, expand=YES, fill=BOTH)

        buttons=Pmw.ButtonBox (root)
        buttons.add ("Restart", command = self.__restart)
        buttons.add ("Info",    command =
                     lambda
                     r=root, a=self.__archiveName, c=self.__channelName:
                     ChannelInfo (r, a.get(), c.get()))
        buttons.add ("<<<",     command=self.__backward)
        buttons.add (">>>",     command=self.__forward)
        
        # Pack
        self.__archiveName.pack (side=TOP, fill=X)
        self.__channelName.pack (side=TOP, fill=X)
        self.__end.pack (side=TOP, fill=X)
        buttons.pack (side=BOTTOM, fill=X)
        data.pack (side=TOP, expand=YES, fill=BOTH)

    def __selectedItems (self):
        "Return selected items (time stamps) as list"
        items = self.__times.curselection()
        return map (lambda i, lb=self.__times: lb.get(i), items)

    def __scroll (self, *args):
        "Callback for scrollbar that scroll multiple listboxes"
        apply (self.__times.yview, args)
        apply (self.__values.yview, args)

    def __insertValue (self, value, where):
        "format value and add to listboxes"
        self.__times.insert (where, value.time())
        if value.isInfo():
            self.__values.insert (where, value.status())
        else:
            self.__values.insert (where,
                                  "%s %s" % (value.text(), value.status()))

    def __listBackward (self, endStamp):
        archive = casi.archive()
        channel = casi.channel()
        value   = casi.value()
        
        name = self.__archiveName.get()
        if not archive.open (name):
            ErrorDlg (self.__dlg.interior(), "Cannot open %s" % name)
            return
        
        name = self.__channelName.get()
        if not archive.findChannelByName (name, channel):
            ErrorDlg (self.__dlg.interior(), "Not found: %s" % name)
            return
                    
        if endStamp > '0':
            channel.getValueAfterTime (endStamp, value)
            # Skip the value that we already have:
            if value.valid(): value.prev()
        else:
            channel.getLastValue (value)
                
        count = int(self.__times['height'])
        while value.valid() and count > 0:
            self.__insertValue (value, 0)
            value.prev()
            count = count - 1

    def __listForward (self, startStamp):
        archive = casi.archive()
        channel = casi.channel()
        value   = casi.value()
        
        name = self.__archiveName.get()
        if not archive.open (name):
            ErrorDlg (self.__root, "Cannot open %s" % name)
            return
        
        name = self.__channelName.get()
        if not archive.findChannelByName (name, channel):
            ErrorDlg (self.__root, "Not found: %s" % name)
            return
    
        if startStamp > '0':
            channel.getValueAfterTime (startStamp, value)
            # Skip the value that we already have:
            if value.valid(): value.next()
        else:
            channel.getFirstValue (value)

        count = int(self.__times['height'])
        while value.valid() and count > 0:
            self.__insertValue (value, END)
            value.next()
            count = count - 1

    def __backward (self):
        "Button procedure"
        endStamp = self.__times.get (0)
        self.__listBackward (endStamp)
        self.__times.see (0)
        self.__values.see (0)

    def __forward (self):
        "Button procedure"
        startStamp = self.__times.get (END)
        self.__listForward (startStamp)
        self.__times.see (END)
        self.__values.see (END)

    def __restart (self):
        "Button procedure"
        self.__times.delete (0, END)
        self.__values.delete (0, END)
        endStamp = self.__end.get()
        self.__listBackward (endStamp)


                
## #################################
## #	Button-procedures
## #
## proc showInfo {} {
##     global archiveName channelName
##     createChannelInfo $archiveName $channelName
## }


def usage ():
    global argv0
    print "USAGE: %s archive channelName \[ endTime \]" % sys.argv[0]
    print "       endTime as \"YYYY/MM/DD hh:mm:ss\" in 24h format"
    
    sys.exit (1)

if __name__ == '__main__':
    # Parse args
    if 0:
        archiveName = '../../Engine/Test/freq_directory'
        channelName = 'fred'
        end = 0
    else:
        argc = len(sys.argv)
        if argc < 3: usage()

        archiveName = sys.argv[1]
        channelName = sys.argv[2]
        if argc > 3:
            end = sys.argv[3]
        else:
            end = 0
    
    root = None
    dlg = ListBrowser (Tk(), archiveName, channelName, end)
    print dlg.activate ()
