# $Id$
#
# Example for a scalar browser.
# Certainly not done, but I'm working on it.
# If you have ideas or - even better - code snippets,
# let me know.

from Tkinter import *
import tkFileDialog, Pmw
import sys, string, time, casi
from casiTools import *
from ErrorDlg import ErrorDlg

class Plot:

    def _up (self):
        "Buttonbar response"
        (y0, y1) = self.graph.yaxis_limits()
        dy = (y1-y0)/2
        self.graph.yaxis_configure(min=y0+dy, max=y1+dy)
    
    def _down (self):
        "Buttonbar response"
        (y0, y1) = self.graph.yaxis_limits()
        dy = -(y1-y0)/2
        self.graph.yaxis_configure(min=y0+dy, max=y1+dy)

    def _zoom (self, x0, y0, x1, y1):
        "Rubberband-zoom-helper"
        self.graph.xaxis_configure(min=x0, max=x1)
        self.graph.yaxis_configure(min=y0, max=y1)
    
    def _mouseUp (self, event):
        "Rubberband-zoom-helper"
        if not self._rubberbanding:
            return
        self._rubberbanding = 0
        self.graph.marker_delete("marking rectangle")
        
        if self._x0 <> self._x1 and self._y0 <> self._y1:
            # make sure the coordinates are sorted
            if self._x0 > self._x1: self._x0, self._x1 = self._x1, self._x0
            if self._y0 > self._y1: self._y0, self._y1 = self._y1, self._y0
     
            if event.num == 1:
               self._zoom (self._x0, self._y0, self._x1, self._y1) # zoom in
            else:
               (X0, X1) = self.graph.xaxis_limits()
               k  = (X1-X0)/(self._x1-self._x0)
               x0 = X0 -(self._x0-X0)*k
               x1 = X1 +(X1-self._x1)*k
               
               (Y0, Y1) = self.graph.yaxis_limits()
               k  = (Y1-Y0)/(self._y1-self._y0)
               self._y0 = Y0 -(self._y0-Y0)*k
               self._y1 = Y1 +(Y1-self._y1)*k
               
               self._zoom(self._x0, self._y0, self._x1, self._y1) # zoom out
        self.graph.crosshairs_on ()

    def _mouseMove (self, event):
        "Rubberband-zoom-helper"
        (self._x1, self._y1) = self.graph.invtransform (event.x, event.y)
        if self._rubberbanding:
            coords = (self._x0, self._y0, self._x1, self._y0,
                      self._x1, self._y1, self._x0, self._y1, self._x0, self._y0)
            self.graph.marker_configure("marking rectangle", coords = coords)
        else:
            pos = "@" +str(event.x) +"," +str(event.y)
            self.graph.crosshairs_configure(position = pos)
            pos = "Time: %s, X: %f" % (secs2stamp(self._x1), self._y1)
            self._messagebar.message ('state', pos)
                            
    def _mouseDown (self, event):
        "Rubberband-zoom-helper"
        self._rubberbanding = 0
        if self.graph.inside (event.x, event.y):
            self.graph.crosshairs_off ()
            (self._x0, self._y0) = self.graph.invtransform (event.x, event.y)
            self.graph.marker_create ("line", name="marking rectangle",
                                      dashes=(2, 2))
            self._rubberbanding = 1

    def timeLabel (graph, noclue, xvalue):
        "Helper for axis: Format xvalue from seconds to time"
        return string.replace (secs2stamp (float(xvalue)), " ", "\n")

    def _getColor (self):
        i = self._next_color
        color = (
            "#0000FF", # 0
            "#FF0000",
            "#000000",
            "#FF00FF",
            "#00FF00",
            "#505050",
            "#FFFF00",
            "#00FFFF", # 7
            )[i];
        if i<7:
            self._next_color = i+1
        else:
            self._next_color = 0
        return color

    def addData (self, channelName, start, end):
        "Add data for channel from start to end stamp"
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
        return valueCount
            
    def openArchive (self, archiveName=None):
        "Menu command"
        if not archiveName:
            archiveName = tkFileDialog.askopenfilename ()
        if archiveName:
            if not self.archive.open (archiveName):
                ErrorDlg (root, "Cannot open %s" % archiveName)
    
    def __init__ (self, root, archiveName):
        # Instance variables
        self._next_color = 0
        self._root=root
        self._rubberbanding = 0

        # Init GUI
        menu = Pmw.MenuBar (root, hull_relief=RAISED, hull_borderwidth=1)
        menu.addmenu ('File', '')
        menu.addmenuitem ('File', 'command', '',
                          label='Open Archive', command=self.openArchive)
        menu.addmenuitem ('File', 'command', '',
                          label='Quit', command=root.quit)
        
        self.graph = Pmw.Blt.Graph (root)
	self.graph.xaxis_configure (command=self.timeLabel)
        self._buttons = Pmw.ButtonBox (root)
        self._buttons.add ('Up', command=self._up)
        self._buttons.add ('Dn', command=self._down)
        self._messagebar = Pmw.MessageBar (root, 
                                           entry_width = 40,
                                           entry_relief='groove',
                                           labelpos = 'w',
                                           label_text = 'Info:')

        # Packing
        menu.pack             (side=TOP, fill=X)
        self._messagebar.pack (side=BOTTOM, fill=X)
        self._buttons.pack    (side=BOTTOM, fill=X)
        self.graph.pack       (side=TOP, expand=YES, fill=BOTH)

        # Bindings
        self.graph.crosshairs_configure (dashes="1", hide=0,
                                         linewidth=1,
                                         color="lightblue")
        self.graph.bind("<ButtonPress-1>",   self._mouseDown)
        self.graph.bind("<ButtonRelease-1>", self._mouseUp)
        self.graph.bind("<Motion>", self._mouseMove)
   
        # Init archive
        self.archive = casi.archive ()
        self.channel = casi.channel ()
        self.value   = casi.value ()

        self.openArchive ("../../Engine/Test/freq_directory")
        # Test Data
        channelName = "fred"
        if not self.archive.findChannelByName (channelName, self.channel):
            ErrorDlg (root, "Cannot find %s" % channelName)
            sys.exit ()
        # Last day
	(Year, Month, Day, H, M, S, N) = stamp2values (self.channel.getLastTime())
        start=values2stamp ((Year, Month, Day-1, 0, 0, 0, 0))
        end=values2stamp ((Year, Month, Day, 23, 59, 59, 0))
    
        t0 = time.clock()
        total = self.addData ("fred", start, end)
        total = total + self.addData ("jane", start, end)
        total = total + self.addData ("freddy", start, end)
        total = total + self.addData ("janet", start, end)
        t1 = time.clock()
        dt = t1-t0
        if dt:
            self._messagebar.message ('state', "%f values/second" % (total/dt))
        else:
            self._messagebar.message ('state', "%d values" % total)

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




