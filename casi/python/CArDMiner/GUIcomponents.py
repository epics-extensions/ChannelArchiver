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
from CArDMiner_utilities import Command
import ChannelBrowser
import ValueProcessor
import BatchProcessor


def createMain(self):
    """
    This is a REALLY LONG method that does all of the GUI instantiation and packing within
    the frames that are packed and unpacked in CArDMiner.py
    """
    
    self.all = "NO"
    
    self.createButtons()
    
    self.ArchiveSummaryFrame = self.createcomponent('ArchiveSummary',
                                                    (), None,
                                                    Frame,
                                                    (self.interior(),),)
    
    self.ChannelBrowserFrame = self.createcomponent('ChannelBrowser',
                                                    (), None,
                                                    Frame,
                                                    (self.interior(),),)
    
    self.ValueProcessingFrame = self.createcomponent('ValueProcessor',
                                                     (), None,
                                                     Frame,
                                                     (self.interior(),),)
    
    self.BatchProcessingFrame = self.createcomponent('BatchProcessor',
                                                     (), None,
                                                     Frame,
                                                     (self.interior(),),)
    
    self.CopyEngineFrame = self.createcomponent('CopyEngine',
                                                (), None,
                                                Frame,
                                                (self.interior(),),)
    
    
    self.SummaryText = self.createcomponent('archiveInfo',
                                            (), None,
                                            Text,
                                            (self.ArchiveSummaryFrame,), bg='white')
    
    self.SummaryText.pack()
    
    
    #### From Frame ####
    
    self.ChannelBrowserFromFrame = self.createcomponent('FromFrame',
                                                        (), None,
                                                        Pmw.Group,
                                                        (self.ChannelBrowserFrame,),
                                                        tag_text = 'Origin Archive')
    
    self.ChannelBrowserToFrame = self.createcomponent('ToFrame',
                                                      (), None,
                                                      Pmw.Group,
                                                      (self.ChannelBrowserFrame,),
                                                      tag_text = 'Destination Archive')
    
    
    ####### ListBoxes for Both Frames ###########

    self.ChannelBrowserSelectChannelFrame = self.createcomponent('selectChannelFrame',
                                                                 (), None,
                                                                 Frame,
                                                                 (self.ChannelBrowserFromFrame.interior(),),)
    
    self.ChannelBrowserSelectChannel = self.createcomponent('selectChannel',
                                                            (), None,
                                                            Pmw.ScrolledListBox,
                                                            (self.ChannelBrowserSelectChannelFrame,),
                                                            dblclickcommand=Command(ChannelBrowser.SpawnChannelSummary,
                                                                                    self))
    
    self.ChannelBrowserSelectChannel._listbox.configure(selectmode = EXTENDED)
    
    self.ChannelBrowserSelectChannelFrame2 = self.createcomponent('selectChannelFrame2',
                                                                  (), None,
                                                                  Frame,
                                                                  (self.ChannelBrowserToFrame.interior(),),)
    
    self.ChannelBrowserSelectChannel2 = self.createcomponent('selectChannel2',
                                                             (), None,
                                                             Pmw.ScrolledListBox,
                                                             (self.ChannelBrowserSelectChannelFrame2,),
                                                             dblclickcommand=Command(ChannelBrowser.SpawnChannelSummary,
                                                                                     self))
    self.ChannelBrowserSelectChannel2._listbox.configure(selectmode = EXTENDED)
    
    ####### End ListBoxes for both frames ############
    
    self.ChannelBrowserRegExpFrame = self.createcomponent('regExpFrame',
                                                          (), None,
                                                          Frame,
                                                          (self.ChannelBrowserFromFrame.interior(),),)
    
    
    self.ChannelBrowserRegExpLabel = self.createcomponent('regExpLabel',
                                                          (), None,
                                                          Label,
                                                          (self.ChannelBrowserRegExpFrame,),
                                                          text="Regular Expression\nChannel Selection",
                                                          pady=10)
    
    self.ChannelBrowserRegExpInclude = self.createcomponent('regExpInclude',
                                                            (), None,
                                                            Pmw.EntryField,
                                                            (self.ChannelBrowserRegExpFrame,),
                                                            label_text="Include ",
                                                            labelpos=W)
    
    self.ChannelBrowserRegExpExclude = self.createcomponent('regExpExclude',
                                                            (), None,
                                                            Pmw.EntryField,
                                                            (self.ChannelBrowserRegExpFrame,),
                                                            label_text="Exclude",
                                                            labelpos=W)
    
    self.ChannelBrowserRegExpUpdateList = self.createcomponent('regExpUpdateList',
                                                               (), None,
                                                               Button,
                                                               (self.ChannelBrowserRegExpFrame,),
                                                               text="Update Channel List",
                                                               command = Command(ChannelBrowser.UpdateList,
                                                                                 self,
                                                                                 "window1"))
    
    self.ChannelBrowserRegExpListAllChannels = self.createcomponent('regExpListAllChannels',
                                                                    (), None,
                                                                    Button,
                                                                    (self.ChannelBrowserRegExpFrame,),
                                                                    text="ReList all Channels",
                                                                    command = Command(ChannelBrowser.UpdateList,
                                                                                      self,
                                                                                      "window1",
                                                                                      all = "YES"))
    
    self.ChannelBrowserRegExpFrame.pack(side=BOTTOM, anchor=S,  padx=10)
    self.ChannelBrowserRegExpLabel.pack()
    self.ChannelBrowserRegExpInclude.pack()
    self.ChannelBrowserRegExpExclude.pack()
    self.ChannelBrowserRegExpUpdateList.pack()
    self.ChannelBrowserRegExpListAllChannels.pack()
    
    self.ChannelBrowserSelectChannel.pack(fill=BOTH)
    self.ChannelBrowserSelectChannelFrame.pack(side=TOP, fill=BOTH)
    
    ### End From Frame ###
    
    
    self.ChannelBrowserXferFrame = self.createcomponent('XferFrame',
                                                        (), None,
                                                        Frame,
                                                        (self.ChannelBrowserFrame,),)
    
    self.ChannelBrowserUnIncludeChannel = self.createcomponent('UnIncludeChannel',
                                                               (), None,
                                                               Button,
                                                               (self.ChannelBrowserXferFrame,),
                                                               text="<-",
                                                               command = Command(ChannelBrowser.CopyChannel,
                                                                                 self,
                                                                                 "UnInclude"))
    
    self.ChannelBrowserIncludeChannel = self.createcomponent('IncludeChannel',
                                                             (), None,
                                                             Button,
                                                             (self.ChannelBrowserXferFrame,),
                                                             text="->",
                                                             command = Command(ChannelBrowser.CopyChannel,
                                                                               self,
                                                                               "Include"))
    
    
    
    self.ChannelBrowserUnIncludeChannel.pack(anchor=N)
    self.ChannelBrowserIncludeChannel.pack(anchor=N)
    self.ChannelBrowserUnIncludeChannel.configure(default=DISABLED)
    self.ChannelBrowserIncludeChannel.configure(default=DISABLED)
    
    #### To Frame ####
    self.ChannelBrowserSelectChannel2.pack(fill=BOTH)
    self.ChannelBrowserSelectChannelFrame2.pack(side=TOP, fill=BOTH)
    ### End To Frame ###
    
    self.ChannelBrowserFromFrame.pack(side=LEFT)
    self.ChannelBrowserXferFrame.pack(side=LEFT, padx=10)
    self.ChannelBrowserToFrame.pack(side=LEFT)
    
    ##### Start Value Processor ########

    self.BChannelListFrame = self.createcomponent('BatchChannelListFrame',
                                                 (), None,
                                                 Pmw.Group,
                                                 (self.ValueProcessingFrame,),
                                                 tag_text = 'Channels in Copy Queue')
    
    self.BChannelList = self.createcomponent('BChannelList',
                                            (), None,
                                            Pmw.ScrolledListBox,
                                            (self.BChannelListFrame.interior(),),
                                            selectioncommand = Command(ValueProcessor.ShowValueProcessing,
                                                                       self))
    
    self.BChannelList._listbox.configure(selectmode = EXTENDED, height=4)

    self.BatchName = self.createcomponent('BatchName',
                                          (), None,
                                          Pmw.EntryField,
                                          (self.BChannelListFrame.interior(),),
                                          label_text='Batch Name: ',
                                          labelpos = W)

    self.GroupIntoBatch = self.createcomponent('GroupIntoBatch',
                                               (), None,
                                               Button,
                                               (self.BChannelListFrame.interior(),),
                                               text="Group Selected Channels\nInto a Batch",
                                               command = Command(BatchProcessor.MakeBatch,
                                                                 self))

    self.BatchName.pack(side = BOTTOM)
    self.GroupIntoBatch.pack(side = BOTTOM)
    self.BChannelList.pack(side = BOTTOM, expand = YES, fill = BOTH)
    self.BChannelListFrame.pack(side = LEFT)
    
    self.ValueRangeFrame = self.createcomponent('ValueRangeFrame',
                                                (), None,
                                                Pmw.Group,
                                                (self.ValueProcessingFrame,),
                                                tag_text = 'Remove Values from Channel')
    
    self.ValuesToRemove = self.createcomponent('ValuesToRemove',
                                               (), None,
                                               Pmw.ScrolledListBox,
                                               (self.ValueRangeFrame.interior(),),)
    
    self.ValuesToRemove._listbox.configure(selectmode = EXTENDED, height=5)
    
    self.ValuePopupListBrowser = self.createcomponent('PopupListBrowser',
                                                      (), None,
                                                      Button,
                                                      (self.ValueRangeFrame.interior(),),
                                                      text="Select Values to\nRemove from Channel",
                                                      command = Command(ValueProcessor.SpawnValueDump,
                                                                        self,
                                                                        channelName = "MUST_SET",
                                                                        markForRemoval = "YES"))
    
    self.ValueRemoveRange = self.createcomponent('RemoveRangeFromList',
                                                 (), None,
                                                 Button,
                                                 (self.ValueRangeFrame.interior(),),
                                                 text="UnDelete highlighted values",
                                                 command = Command(ValueProcessor.SpawnValueDump,
                                                                   self,
                                                                   channelName = "MUST_SET"))
    
    self.ValuesToRemove.pack(side = TOP, fill=BOTH)
    self.ValuePopupListBrowser.pack(side=LEFT)
    self.ValueRemoveRange.pack(side=LEFT)    
    self.ValueRangeFrame.pack(side = TOP)        

    ### Batch Processor

    self.BatchListFrame = self.createcomponent('BatchListFrame',
                                                 (), None,
                                                 Pmw.Group,
                                                 (self.ValueProcessingFrame,),
                                                 tag_text = 'Batch List')
    
    self.BatchList = self.createcomponent('BatchList',
                                            (), None,
                                            Pmw.ScrolledListBox,
                                            (self.BatchListFrame.interior(),),)

    
    self.BatchList._listbox.configure(selectmode = SINGLE, height=7)

    self.RemoveBatch = self.createcomponent('RemoveBatch',
                                               (), None,
                                               Button,
                                               (self.BatchListFrame.interior(),),
                                               text="Delete Batch",
                                               command = Command(BatchProcessor.RemoveBatch,
                                                                 self))

    self.BatchList.pack()
    self.RemoveBatch.pack()
    self.BatchListFrame.pack(side = LEFT)

    self.BatchEditorFrame = self.createcomponent('BatchCreationFrame',
                                               (), None,
                                               Frame,
                                               (self.BatchProcessingFrame,),)

    self.BatchApplyFrame = self.createcomponent('BatchApplyFrame',
                                                        (), None,
                                                        Frame,
                                                        (self.BatchEditorFrame,),)

    self.CodeSnipBatchFrame = self.createcomponent('CodeSnipBatchFrame',
                                                 (), None,
                                                 Pmw.Group,
                                                 (self.BatchApplyFrame,),
                                                 tag_text = 'Batch List')
    
    self.CodeSnipBatchList = self.createcomponent('CodeSnipBatchList',
                                            (), None,
                                            Pmw.ScrolledListBox,
                                            (self.CodeSnipBatchFrame.interior(),),
                                            selectioncommand = Command(BatchProcessor.ShowBatchProcessing,
                                                                       self))
    
    self.CodeSnipBatchList._listbox.configure(selectmode = EXTENDED, height=6)

    self.CodeSnipBatchList.pack()
    self.CodeSnipBatchFrame.pack(side = LEFT)

    self.AppliedCodeSnippetsFrame = self.createcomponent('AppliedCodeSnippetsFrame',
                                                         (), None,
                                                         Pmw.Group,
                                                         (self.BatchApplyFrame,),
                                                         tag_text = 'Applied Code Snippets')
    
    self.AppliedCodeSnippetsList = self.createcomponent('AppliedCodeSnippetsList',
                                                    (), None,
                                                    Pmw.ScrolledListBox,
                                                    (self.AppliedCodeSnippetsFrame.interior(),),
                                                        selectioncommand = Command(BatchProcessor.ShowSnippet,
                                                                                   self,
                                                                                   window="Applied"))
                                                                                   
    
    self.AppliedCodeSnippetsList._listbox.configure(selectmode = EXTENDED, height=5)

    self.CodeSnipUnapplySnippet = self.createcomponent('Unapply Snippet',
                                               (), None,
                                               Button,
                                               (self.AppliedCodeSnippetsFrame.interior(),),
                                               text="Unapply Snippet",
                                               command = Command(BatchProcessor.UnApplySnippet,
                                                                 self))

    self.AppliedCodeSnippetsList.pack()
    self.CodeSnipUnapplySnippet.pack()
    self.AppliedCodeSnippetsFrame.pack(side = LEFT)

    self.CodeSnippetsFrame = self.createcomponent('CodeSnippetsFrame',
                                                         (), None,
                                                         Pmw.Group,
                                                         (self.BatchApplyFrame,),
                                                         tag_text = 'Code Snippets')
    
    self.CodeSnippetsList = self.createcomponent('CodeSnippetsList',
                                                    (), None,
                                                    Pmw.ScrolledListBox,
                                                    (self.CodeSnippetsFrame.interior(),),
                                                    selectioncommand = Command(BatchProcessor.ShowSnippet,
                                                                               self,
                                                                               window="Master"))
                                                                               
    
    self.CodeSnippetsList._listbox.configure(selectmode = EXTENDED, height=5)

    self.CodeSnipApplySnippet = self.createcomponent('Apply Snippet',
                                               (), None,
                                               Button,
                                               (self.CodeSnippetsFrame.interior(),),
                                               text="Apply Snippet",
                                               command = Command(BatchProcessor.ApplySnippet,
                                                                 self))
    
    self.CodeSnippetsList.pack()
    self.CodeSnipApplySnippet.pack()
    self.CodeSnippetsFrame.pack(side = LEFT)

    self.BatchSummary = self.createcomponent('BatchSummary',
                                            (), None,
                                            Pmw.ScrolledText,
                                            (self.BatchEditorFrame,),)
    
    self.BatchApplyFrame.pack()
    self.BatchSummary.pack(side = BOTTOM)

    self.BatchEditorFrame.pack(side = LEFT)
        
    
    self.CopyEngineControlFrame = self.createcomponent('CopyEngineControlFrame',
                                                       (), None,
                                                       Frame,
                                                       (self.CopyEngineFrame,),)
    
    self.TheLogFrame = self.createcomponent('TheLogFrame',
                                            (), None,
                                            Pmw.Group,
                                            (self.CopyEngineFrame,),
                                            tag_text = 'Copy Engine Log')
    
    self.TheLog = self.createcomponent('TheLog',
                                       (), None,
                                       Pmw.ScrolledText,
                                           (self.TheLogFrame.interior(),),)

    self.ThePickDestination = self.createcomponent('ThePickDestination',
                                                   (), None,
                                                   Button,
                                                   (self.CopyEngineControlFrame,),
                                                   text='Pick Destination Filename',
                                                   command = Command(self.setDestName))
    
    self.TheEngineCancel = self.createcomponent('TheEngineCancel',
                                                (), None,
                                                Button,
                                                (self.CopyEngineControlFrame,),
                                                text='CANCEL',
                                                command = Command(self.CancelEngine))
    
    self.TheLogWrite = self.createcomponent('TheLogWrite',
                                            (), None,
                                            Button,
                                            (self.CopyEngineControlFrame,),
                                            text='Write the Archive NOW',
                                            command = Command(self.InteractiveArchiveEngine))
    
    self.ThePickDestination.pack()
    self.TheLogWrite.pack()
    self.CopyEngineControlFrame.pack(side=BOTTOM)
    
    self.TheLog.pack()
    self.TheLogFrame.pack()
    
    return

