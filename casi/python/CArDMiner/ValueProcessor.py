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


from ListBrowser import ListBrowser

def SpawnValueDump(self, channelName, markForRemoval = "NO"):
    """
    SpawnValueDump dumps all values from the given channel
    into a listbox for further processing
    """
        
    if channelName == "MUST_SET":
        try:
            channelName = self.processing_channel
        except:
            self.theHandler.Raise(2)
            return
        

    try:
        self.archivePtr.findChannelByName(channelName, self.chanI)
    except AttributeError:
        self.theHandler.Raise(2)
        return

    if markForRemoval == "YES":
        dlg = ListBrowser (self.root, self.archiveRecord.archive_name, channelName)
        valueList = dlg.activate()

        try:    
            channelPtr = self.archiveTo.copy_dict[channelName]
            channelPtr.AddMarkedValues(valueList)
            self.ValuesToRemove.setlist(channelPtr.markedValueList)
        except KeyError:
            self.theHandler.Raise(2)

    else:
        try:
            channelPtr = self.archiveTo.copy_dict[channelName]
            valueList = self.ValuesToRemove.getcurselection()
            channelPtr.RemoveMarkedValues(valueList)
            self.ValuesToRemove.setlist(channelPtr.markedValueList)
        except KeyError:
            self.theHandler.Raise(2)
            
    return


def ShowValueProcessing(self):
    """
    ShowValueProcessing fills the Value Processing page
    with info on the channel just clicked on
    """

    try:
        self.processing_channel = self.BChannelList.getcurselection()[0]
    except IndexError:
        pass
        return

    self.ValuesToRemove.setlist(self.archiveTo.copy_dict[self.processing_channel].markedValueList)
    self.ValueRangeFrame.configure(tag_text = "Remove Values from Channel: " + self.processing_channel)
    self.ValuePopupListBrowser.configure(text = "Select Values to\nRemove from " + self.processing_channel)
    return

