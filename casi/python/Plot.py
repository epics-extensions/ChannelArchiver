# $Id$
#
# Please refer to NOTICE.txt,
# included as part of this distribution,
# for legal information.
#
# Kay-Uwe Kasemir, kasemir@lanl.gov
# --------------------------------------------------------
#
# Example for a scalar browser.
# Certainly not done, but I'm working on it.
# If you have ideas or - even better - code snippets,
# let me know.
#
# If nothing else this is a benchmark:
# 800MHz Win. NT  :  2550 values/sec
# 500MHz Linux box:  1430 values/sec
# 266MHz Linux box:   785 values/sec
# L8 "beowolf"    :   416 values/sec

from Tkinter import *
import tkFileDialog, Pmw
import sys, string, time, casi
from casiTools import *
from ErrorDlg import ErrorDlg
import ListChannels


# Global for next color index used by ChannelInfo
# (until I know how to use class 'static' vars)
_next_color=0

class ChannelInfo:
    "Helper class for Plot: hold info on single channel"

    def __init__ (self, name):
        self.name = name
        self.color = self.__getColor ()

    def __getColor (self):
        "Get (unique) color for this channel"
        global _next_color
        color = ( "#0000FF", "#FF0000", "#000000", "#FF00FF",
                  "#00FF00", "#505050", "#FFFF00", "#00FFFF", # 7
                )[_next_color];
        if _next_color<7:   _next_color = _next_color+1
        else:               _next_color = 0
        return color

    def loadAndPlot (self, root, graph, archive, channel, value, start, end):
        """Add data for channel from start to end stamp
           * root window
           * BLT graph
           * archive, channel, value form casi
           * start/end stamp
        """
        # Delete existing:
        for el in graph.element_names ("%s *" % self.name):
            graph.element_delete (el)

        # Create new:
        if not archive.findChannelByName (self.name, channel):
            ErrorDlg (root, "Cannot find %s" % self.name)
            return
        if not channel.getValueAfterTime (start, value):
            return
        label = self.name
        times, values =[], []
        segment, valueCount = 0, 0
        while value.valid()  and  value.time() <= end:
            if value.isInfo() and len(values) : # start new segment
                graph.line_create ("%s %d" % (self.name, segment), symbol='',
                                   label=label, color=self.color,
                                   xdata=tuple(times), ydata=tuple(values))
                segment = segment + 1
                label = ''
                valueCount = valueCount + len(values)
                times, values =[], []
            else:
                times.append (stamp2secs(value.time()))
                values.append (value.get())
            value.next()

        if len(values):
            graph.line_create ("%s %d" % (self.name, segment), symbol='',
                               label=label, color=self.color,
                               xdata=tuple(times), ydata=tuple(values))
            valueCount = valueCount + len(values)

        return valueCount


class Plot:

    def _channels (self):
        "Return list of channel names currently displayed"
        return map (lambda info: info.name, self._channelInfos)

    def _findLastDay (self):
        "Find last day that has values for current channels"
        end=''
        for name in self._channels ():
            if self._archive.findChannelByName (name, self._channel):
                end = max (end, self._channel.getLastTime ())

        # Last day
	(Year, Month, Day, H, M, S, N) = stamp2values (end)
        start=values2stamp ((Year, Month, Day-1, 0, 0, 0, 0))
        end=values2stamp ((Year, Month, Day, 23, 59, 59, 0))
        return (start, end)

    def _currentTimeRange (self):
        "Returns current start...end time stamps"
        return map (secs2stamp, self._graph.xaxis_limits())

    def _yIn (self):
        "Buttonbar response"
        (y0, y1) = self._graph.yaxis_limits()
        dy = (y1-y0)/4
        if dy > 0.0000001: # prevent BLT crash
            self._graph.yaxis_configure(min=y0+dy, max=y1-dy)
    
    def _yOut (self):
        "Buttonbar response"
        (y0, y1) = self._graph.yaxis_limits()
        dy = -(y1-y0)/4
        self._graph.yaxis_configure(min=y0+dy, max=y1-dy)
    
    def _up (self):
        "Buttonbar response"
        (y0, y1) = self._graph.yaxis_limits()
        dy = (y1-y0)/2
        self._graph.yaxis_configure(min=y0+dy, max=y1+dy)
    
    def _down (self):
        "Buttonbar response"
        (y0, y1) = self._graph.yaxis_limits()
        dy = -(y1-y0)/2
        self._graph.yaxis_configure(min=y0+dy, max=y1+dy)

    def _zoom (self, x0, y0, x1, y1):
        "Rubberband-zoom-helper"
        self._graph.xaxis_configure(min=x0, max=x1)
        self._graph.yaxis_configure(min=y0, max=y1)
    
    def _mouseUp (self, event):
        "Rubberband-zoom-helper"
        if not self._rubberbanding:
            return
        self._rubberbanding = 0
        self._graph.marker_delete("marking rectangle")
        
        if self._x0 <> self._x1 and self._y0 <> self._y1:
            # make sure the coordinates are sorted
            if self._x0 > self._x1: self._x0, self._x1 = self._x1, self._x0
            if self._y0 > self._y1: self._y0, self._y1 = self._y1, self._y0
     
            if event.num == 1:
               self._zoom (self._x0, self._y0, self._x1, self._y1) # zoom in
            else:
               (X0, X1) = self._graph.xaxis_limits()
               k  = (X1-X0)/(self._x1-self._x0)
               x0 = X0 -(self._x0-X0)*k
               x1 = X1 +(X1-self._x1)*k
               
               (Y0, Y1) = self._graph.yaxis_limits()
               k  = (Y1-Y0)/(self._y1-self._y0)
               self._y0 = Y0 -(self._y0-Y0)*k
               self._y1 = Y1 +(Y1-self._y1)*k
               
               self._zoom(self._x0, self._y0, self._x1, self._y1) # zoom out
        self._graph.crosshairs_on ()

    def _mouseMove (self, event):
        "Rubberband-zoom-helper"
        (self._x1, self._y1) = self._graph.invtransform (event.x, event.y)
        if self._rubberbanding:
            coords = (self._x0, self._y0, self._x1, self._y0,
                      self._x1, self._y1, self._x0, self._y1, self._x0, self._y0)
            self._graph.marker_configure("marking rectangle", coords = coords)
        else:
            pos = "@" +str(event.x) +"," +str(event.y)
            self._graph.crosshairs_configure(position = pos)
            pos = "Time: %s,  Value: %f" % (secs2stamp(self._x1), self._y1)
            self._messagebar.message ('state', pos)
                            
    def _mouseDown (self, event):
        "Rubberband-zoom-helper"
        self._rubberbanding = 0
        if self._graph.inside (event.x, event.y):
            self._graph.crosshairs_off ()
            (self._x0, self._y0) = self._graph.invtransform (event.x, event.y)
            self._graph.marker_create ("line", name="marking rectangle",
                                      dashes=(2, 2))
            self._rubberbanding = 1

    def timeLabel (graph, noclue, xvalue):
        "Helper for axis: Format xvalue from seconds to time"
        return string.replace (secs2stamp (float(xvalue)), " ", "\n")

    def openArchive (self, archiveName=None):
        "Menu command"
        if not archiveName:
            archiveName = tkFileDialog.askopenfilename ()
        if archiveName:
            if not self._archive.open (archiveName):
                ErrorDlg (root, "Cannot open %s" % archiveName)
                return 0
        return 1

    def addChannels (self, names=()):
        "Menu command"
        # No names given -> pop up dialog
        if len(names) < 1:
            dlg = ListChannels.ChannelDialog (self._root, self._archive)
            names = dlg.activate()
        if len(names) < 1:
            return # Still no names? Well, I tried!

        first = len(self._channelInfos) == 0
        for name in names:
            found = filter (lambda info, n=name: info.name==n, self._channelInfos)
            if len(found) == 0:
                self._channelInfos.append (ChannelInfo (name))
        if first:
            (start, end) = self._findLastDay ()
        else: # snoop time range from current graph
            (start, end) = self._currentTimeRange ()

        t0 = time.clock()
        total = 0
        for info in self._channelInfos:
            total = total + info.loadAndPlot (self._root, self._graph,
                                              self._archive, self._channel, self._value,
                                              start, end)
        dt = time.clock()-t0
        if dt:
            self._messagebar.message ('state', "%f values/second" % (total/dt))

    def __init__ (self, root, archiveName):
        # Instance variables
        self._root=root           # Root window
        self._next_color = 0      # for _getColor
        self._rubberbanding = 0
        self._channelInfos = []

        # Init GUI
        menu = Pmw.MenuBar (root, hull_relief=RAISED, hull_borderwidth=1)
        menu.addmenu ('File', '')
        menu.addmenuitem ('File', 'command', '',
                          label='Open Archive', command=self.openArchive)
        menu.addmenuitem ('File', 'command', '',
                          label='Quit', command=root.quit)
        menu.addmenu ('Channel', '')
        menu.addmenuitem ('Channel', 'command', '',
                          label='Add Channels', command=self.addChannels)
        
        self._graph = Pmw.Blt.Graph (root)
	self._graph.xaxis_configure (command=self.timeLabel)
        self._buttons = Pmw.ButtonBox (root)
        self._buttons.add ('Zoom', command=self._yIn)
        self._buttons.add ('Out', command=self._yOut)
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
        self._buttons.pack    (side=BOTTOM)
        self._graph.pack       (side=TOP, expand=YES, fill=BOTH)

        # Bindings
        self._graph.crosshairs_configure (dashes="1", hide=0,
                                         linewidth=1,
                                         color="lightblue")
        self._graph.bind("<ButtonPress-1>",   self._mouseDown)
        self._graph.bind("<ButtonRelease-1>", self._mouseUp)
        self._graph.bind("<Motion>", self._mouseMove)
   
        # Init archive
        self._archive = casi.archive ()
        self._channel = casi.channel ()
        self._value   = casi.value ()

        if not self.openArchive ("../../Engine/Test/freq_directory"):
            sys.exit (1)
            
        # Test Data
        self.addChannels (('fred', 'freddy'))
        
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




