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

from ChannelInfo import ChannelInfo
import Pmw 


def FillChannelBrowser(self, window, intendedTopView=0, intendedTopView2=0):
    """
    FillChannelBrowser simply sets the the list parameter
    in the Channel List Listbox
    """

    if window == "window1":
        self.ChannelBrowserSelectChannel.setlist(self.archiveRecord.channelList)
        self.ChannelBrowserSelectChannel.yview_scroll(intendedTopView, 'units')
    elif window == "window2":
        self.ChannelBrowserSelectChannel2.setlist(self.archiveTo.copy_list)
        self.BChannelList.setlist(self.archiveTo.copy_list)
        self.ChannelBrowserSelectChannel2.yview_scroll(intendedTopView2, 'units')

    return
        
def UpdateList(self, window, all = "NO"):
    """
    UpdateList simpy updates the ChannelList to reflect
    display additions, display removals, and deletion marks
    """

    intendedTopView = self.ChannelBrowserSelectChannel.nearest(0)
    intendedTopView2 = self.ChannelBrowserSelectChannel2.nearest(0)

    if window == "window1":
        if all == "YES":
            self.ChannelBrowserRegExpInclude.clear(),
            self.ChannelBrowserRegExpExclude.clear()

        self.archiveRecord.UpdateChannelList(self.archivePtr,
                                             self.ChannelBrowserRegExpInclude.get(),
                                             self.ChannelBrowserRegExpExclude.get())
        FillChannelBrowser(self, window, intendedTopView)
        
        return

    elif window == "window2":

        FillChannelBrowser(self, window, intendedTopView2)
        return   


def CopyChannel(self, UnOrIn):
    """
    CopyChannel tells the archiveRecord to reflect intent
    to copy channel specified in the channel ListBox,
    and tells archiveTo to add the channel to it's copy_list
    """

    if UnOrIn == "Include":
        if self.ChannelBrowserSelectChannel.getcurselection():
            self.archiveTo.ToggleCopyChannel(self.ChannelBrowserSelectChannel.getcurselection())
            UpdateList(self, "window2")
        return

    elif UnOrIn == "UnInclude":
        if self.ChannelBrowserSelectChannel2.getcurselection():
            self.archiveTo.ToggleCopyChannel(self.ChannelBrowserSelectChannel2.getcurselection(), delete="YES")
            UpdateList(self, "window2")
        return

def SpawnChannelSummary(self):
    """
    SpawnChannelSummary spawns a window containing simple
    begin/end times for channel 
    """

    channelName = self.ChannelBrowserSelectChannel.getcurselection()
    for channelName in channelName:
        info = ChannelInfo(self.root, self.archiveRecord.archive_name, channelName)
    return
