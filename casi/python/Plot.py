# $Id$
#

from Tkinter import *
import tkFileDialog, Pmw
import sys, string, casi
from casiTools import *
from ErrorDlg import ErrorDlg

class Plot:

    def timeLabel (graph, noclue, xvalue):
        "Helper for axis: Format xvalue from seconds to time"
        return string.replace (secs2stamp (float(xvalue)), " ", "\n")

    def _getColor (self):
        i = self._next_color
        color = (
            "#0000FF", # 0
            "#000000",
            "#FF00FF",
            "#FF0000",
            "#00FF00",
            "#505050",
            "#FFFF00",
            "#00FFFF", # 7
            )[i];
        if i<7:
            self._next_color = i+1
        else:
            self._next_color = 0

    def addData (self, channelName, start, end):
        "Add data for channel from start to end stamp"

        print "Channel %s, %s - %s" % (channelName, start, end)

        if not self.archive.findChannelByName (channelName, self.channel):
            ErrorDlg (root, "Cannot find %s" % channelName)
            return
        value = self.value # shortcut
        if not self.channel.getValueAfterTime (start, value):
            return
        segment = 0
        color = self._getColor ()
        label = channelName
        times=[]
        values=[]
        valueCount = 0
        while value.valid()  and  value.time() <= end:
            if value.isInfo() and len(values) : # start new segment
                self.graph.line_create ("%s%d" % (channelName, segment),
                                symbol='',label=label, color=color,
                                xdata=tuple(times), ydata=tuple(values))
                segment = segment + 1
                label = ''
                valueCount = valueCount + len(values)
                times=[]
                values=[]
            else:
                times.append (stamp2secs(value.time()))
                values.append (value.get())
            value.next()

        if len(values):
            self.graph.line_create ("%s%d" % (channelName, segment),
                                    symbol='',label=label, color=color,
                                    xdata=tuple(times), ydata=tuple(values))
            valueCount = valueCount + len(values)
        print "--- %d values" % valueCount
            

    def fill (self):
        self.graph.line_create ("x1", symbol='',label='x', color=_colors[0],
                                xdata=(1, 2, 3),
                                ydata=(5, 6, 6.5))
        self.graph.line_create ("x2", symbol='',label='', color=_colors[1],
                                xdata=(4,5,6),
                                ydata=(7, 6, 6.5))
        self.graph.line_create ("y1", symbol='',label='y', color=_colors[2],
                                xdata=(1, 2, 3),
                                ydata=(2, 4, 3))
        self.graph.line_create ("y2", symbol='',label='', color=_colors[3],
                                xdata=(4,5,6),
                                ydata=(4, 5, 6))
    
    def __init__ (self, root, archiveName):
        self._next_color = 0

        self._root=root

        # Init GUI
        self.graph = Pmw.Blt.Graph (root)
	self.graph.xaxis_configure (command=self.timeLabel)

        self.graph.pack (expand=Y, fill=BOTH)

        # Init archive
        self.archive = casi.archive ()
        self.channel = casi.channel ()
        self.value   = casi.value ()
        name = "../../Engine/Test/freq_directory"
        if not self.archive.open (name):
            ErrorDlg (root, "Cannot open %s" % name)
            sys.exit ()

        # Test Data
        channelName = "fred"
        if not self.archive.findChannelByName (channelName, self.channel):
            ErrorDlg (root, "Cannot find %s" % channelName)
            sys.exit ()
        # Last day
	(Year, Month, Day, Hours, Minutes, Seconds, Nano) = stamp2values (self.channel.getLastTime())
        start=values2stamp ((Year, Month, Day-1, 0, 0, 0, 0))
        end=values2stamp ((Year, Month, Day, 23, 59, 59, 0))
    
        self.addData (channelName, start, end)

        # run
        root.mainloop()



if __name__ == "__main__":
    if len(sys.argv) == 2:
        archiveName = sys.argv[1]
    else:
        #archiveName = tkFileDialog.askopenfilename ()
        archiveName = "../../Engine/Test/freq_directory"
    root=Tk()
    Pmw.initialise()
    if not archiveName: sys.exit()
    Plot (root, archiveName)




