# $Id$

# Please refer to NOTICE.txt,
# included as part of this distribution,
# for legal information.
#
# Kay-Uwe Kasemir, kasemir@lanl.gov
# --------------------------------------------------------

from Tkinter import *
import tkFileDialog, Pmw
import sys, string, time, casi
from casiTools import *
from ErrorDlg import ErrorDlg

class XYPlot:
    def __init__ (self, root, archiveName, xChannelName, yChannelName, time):
        self.__GUI (root, archiveName, xChannelName, yChannelName, time)
        self.__restart ()
        self.__dlg.activate ()

    def __GUI (self, root, archiveName, xChannelName, yChannelName, time):
        root.option_readfile ('optionDB')
        self.__dlg = Pmw.Dialog (root,
                                 buttons=('Restart', '<<<', '>>>', 'Exit'),
                                 title='Value Browser (List)',
                                 command=self.__buttoncallback)
        root = self.__dlg.interior()
        self.__archiveName = Pmw.EntryField (root, labelpos='nw',
                                             label_text='Archive:',
                                             value = archiveName)
        f1=Frame (root)
        self.__xChannelName = Pmw.EntryField (f1, labelpos='nw',
                                             label_text='X Channel:',
                                             value = xChannelName)
        self.__yChannelName = Pmw.EntryField (f1, labelpos='nw',
                                             label_text='Y Channel:',
                                             value = yChannelName)
        self.__start = Pmw.EntryField (f1, labelpos='nw',
                                       label_text='Time:',
                                       value = time)
        
        self.__graph = Pmw.Blt.Graph (root, title = "X/Y Plot")
        self.__graph.line_create ('data', xdata=(), ydata=(),
                                  symbol='', label='')
        f2=Frame (root)
        self.__xTime = Pmw.EntryField (f2, labelpos='nw',
                                       label_text='X Time:')
        self.__yTime = Pmw.EntryField (f2, labelpos='nw',
                                       label_text='Y Time:')
        self.__gap = Pmw.EntryField (f2, labelpos='nw',
                                     label_text='Gap:')
        

        self.__xChannelName.pack (side=LEFT, expand=YES, fill=X)
        self.__yChannelName.pack (side=LEFT, expand=YES, fill=X)
        self.__start.pack        (side=LEFT, expand=YES, fill=X)
        self.__xTime.pack (side=LEFT, expand=YES, fill=X)
        self.__yTime.pack (side=LEFT, expand=YES, fill=X)
        self.__gap.pack   (side=LEFT, expand=YES, fill=X)

        f2.pack (side=BOTTOM, expand=YES, fill=X)
        f1.pack (side=BOTTOM, expand=YES, fill=X)
        self.__archiveName.pack (side=BOTTOM, expand=YES, fill=X)
        self.__graph.pack (side=TOP, expand=YES, fill=BOTH)

        
    def __buttoncallback (self, command):
        if command=='Exit':
            self.__dlg.deactivate ()
        elif command=='Restart':
            self.__restart()
        elif command=='>>>':
            self.__forward()
        elif command=='<<<':
            self.__backward()

    def __restart (self):
        "Button callback"
        self.__closeArchive()
        if self.__openArchive():
            self.__getXY (self.__start.get())

    def __forward (self):
        "Button callback"
        if not self.__xValue.valid():
            return
        self.__xValue.next ()
        if not self.__xValue.valid():
            self.__xValue.prev ()
            return
        self.__getXY (self.__xValue.time())

    def __backward (self):
        "Button callback"
        if not self.__xValue.valid():
            return
        self.__xValue.prev ()
        if not self.__xValue.valid():
            self.__xValue.next ()
            return
        self.__getXY (self.__xValue.time())

    def __openArchive (self):
        self.__archive  = casi.archive()
        self.__xChannel = casi.channel()
        self.__yChannel = casi.channel()
        self.__xValue   = casi.value()
        self.__yValue   = casi.value()
        name = self.__archiveName.get()
        if not self.__archive.open (name):
            ErrorDlg (self.__dlg.interior(), "Cannot open archive %s" % name)
            return 0
        name = self.__xChannelName.get()
        if not self.__archive.findChannelByName (name, self.__xChannel):
            ErrorDlg (self.__dlg.interior(), "Cannot find channel %s" % name)
            return 0
        name = self.__yChannelName.get()
        if not self.__archive.findChannelByName (name, self.__yChannel):
            ErrorDlg (self.__dlg.interior(), "Cannot find channel %s" % name)
            return 0
        return 1

    def __closeArchive (self):
        try:   # Note: Called in restart when all Ids are invalid!
            del self.__yValue
            del self.__xValue
            del self.__yChannel
            del self.__xChannel
            del self.__archive
        except:
            pass

    def __getXY (self, when):
        """Look for x/y pair close to "when"."""
        if when < '1999':
            when=self.__xChannel.getFirstTime()

        # set xValue to something close to when (maybe it's already there)
        if not self.__xValue.valid() or self.__xValue.time() != when:
            self.__xChannel.getValueNearTime (when, self.__xValue)

        if not self.__xValue.valid():
            return

        xStamp = self.__xValue.time()

        if not self.__yChannel.getValueNearTime (xStamp, self.__yValue):
            return
	
        yStamp = self.__yValue.time()
        # Complain if arrays are of different size?
        count = min (self.__xValue.count(), self.__yValue.count())
        i=range(count)
        x = map (self.__xValue.getidx, i)
        y = map (self.__yValue.getidx, i)

        ysec = stamp2secs (yStamp)
        gap = stamp2secs (xStamp) - ysec
        time= secs2stamp(ysec + gap/2)
        gap = abs(gap)

        self.__start.setentry (time)
        self.__xTime.setentry (xStamp)
        self.__yTime.setentry (yStamp)
        self.__gap.setentry (gap)
        self.__graph.element_configure ('data', xdata=tuple(x), ydata=tuple(y))



def usage ():
    print "USAGE: %s archive xChannelName yChannelName \[ time \]" % sys.argv[0]
    print "       time as \"YYYY/MM/DD hh:mm:ss\" in 24h format"
    sys.exit (1)


if __name__ == "__main__":
    if 1:
        if not (4 <= len(sys.argv) <= 5):
            usage()
        
        archiveName  = sys.argv[1]
        xChannelName = sys.argv[2]
        yChannelName = sys.argv[3]
        if len(sys.argv) == 5:
            time = sys.argv[4]
        else:
            time='0'
    else:
        archiveName  = '/home/kasemir/Epics/Halo/dir'
        xChannelName = 'ExamplePosition'
        yChannelName = 'ExampleArray'
        time='0'
        
    root=Tk()
    Pmw.initialise()
    XYPlot (root, archiveName, xChannelName, yChannelName, time)

