#$Id$
######################################################################################
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
import sys, string
import casi
import ProgressBar
import tkFileDialog
from ListBrowser import ListBrowser
import thread

### internal modules ###
import ArchiveStructs
import GUIcomponents
from GUIstructure import AppShell
from CArDMiner_utilities import Command, CArDMinerProperties
import ArchiveSummary
import ChannelBrowser
import ValueProcessor
import BatchProcessor
import Exception_Handler
### internal modules ###

class BuildAppShell(AppShell):

    usecommandarea=1
    

    def initialInit(self):
        """
        initialInit does some basic structure initialization.  
        In particular, we instantiate our ArchiveStructs,
        get a pointer to old archive, clear out boxes.
        This is to be run every time a new archive is opened.
        """

        self.theHandler = Exception_Handler.theHandler()
        self.archiveRecord = ArchiveStructs.ArchiveFrom()
        self.archiveTo = ArchiveStructs.ArchiveTo(globals())
        self.archivePtr = casi.archive()
        self.chanI = casi.channel()
        self.SRCarchiveTag.configure(text = "Source Archive: ")
        self.DESTarchiveTag.configure(text = "Destination Archive: ")

        self.ValuesToRemove.clear()
        self.BatchList.clear()
        self.CodeSnipBatchList.clear()
        self.AppliedCodeSnippetsList.clear()
        self.CodeSnippetsList.clear()
        self.BatchSummary.clear()
        self.TheLog.clear()

        self.processing_channel = ''
        self.ValueRangeFrame.configure(tag_text = "Remove Values from Channel: " + self.processing_channel)
        self.ValuePopupListBrowser.configure(text = "Select Values to\nRemove from " + self.processing_channel)
        self.processing_batch = ''
        self.CodeSnipUnapplySnippet.configure(text="Unapply Snippet from:\n" + self.processing_batch)
        self.CodeSnipApplySnippet.configure(text="Apply Snippet to:\n" + self.processing_batch)
        

        return
        
    def OpenArchive(self):
        """
        OpenArchive receives filename from dialog, opens archive
        and spawns initialization of various windows.
        Here we instantiate our ArchiveStructs too.
        """        

        self.initialInit()

        archive_name = tkFileDialog.askopenfilename()
        self.archivePtr = self.archiveRecord.open(self.archivePtr, archive_name)

        if self.archivePtr == 0:
            return

        try:
            self.archiveRecord.get_info(self.archivePtr)

            self.archiveTo.start_date = self.archiveRecord.first_stamp
            self.archiveTo.end_date = self.archiveRecord.last_stamp

            ArchiveSummary.ShowArchiveSummary(self)
            self.SRCarchiveTag.configure(text = "Source Archive: " + self.archiveRecord.archive_name)
            ChannelBrowser.FillChannelBrowser(self, "window1")
            ChannelBrowser.FillChannelBrowser(self, "window2")
        except:
            self.theHandler.Raise(0)
        return 

    def setDestName(self):
        """
        This method should be called setDestStuff because it
        doesn't only set the name, it sets most of the variables
        in the ArchiveStructs.ArchiveTo structure based on the result
        of a properties configuration session.
        """
        if self.archiveRecord.archive_name != '':
            props = CArDMinerProperties(self.root,
                                        self.archiveTo.start_date,
                                        self.archiveTo.end_date,
                                        self.archiveTo.hours_per_file,
                                        self.archiveTo.archive_name,
                                        self.archiveTo.log_name)
            archiveTo_name = props.activate()

            self.archiveTo.setName(archiveTo_name)
            self.DESTarchiveTag.configure(text = "Destination Archive: " + self.archiveTo.archive_name)
            self.archiveTo.hours_per_file = props.hours_per_file
            self.archiveTo.log_name = props.archiveTo_log
            self.archiveTo.start_date = props.start_date
            self.archiveTo.end_date = props.end_date
        return


    def createInterface(self):
        """
        createInterface does the generic GUI initialization
        """

        AppShell.createInterface(self)
        GUIcomponents.createMain(self)
        self.initialInit()
        return

    def ShutDown(self):
        """
        ShutDown performs anything needed before exit and then
        exits the program
        """
        print "Thank You for using CArDMiner"
        sys.exit()
        return


        

    def InteractiveArchiveEngine(self):
        """
        This method is run when a user hits the Write Archive NOW
        buttn.  We simply exchange the button for a CANCEL button,
        instantiate the logger (which writes to a window and a log file,
        it is legal to substitute 'none' for the window, and it will be dumped
        to a Tkinter text box that is never packed), and start the CreateArchive
        method of the ArchiveTo structure.  The lock is used as a mean to replace
        the correct button when archive synthesis is done or cancelled.
        """
        
        self.TheLogWrite.pack_forget()
        self.TheEngineCancel.pack()

        the_logger = ArchiveStructs.Logger(self.TheLog, self.archiveTo.log_name)

        theLock = thread.allocate_lock()
        theLock.acquire()

        self.archiveTo.RunInitProcCode(theLock, the_logger)

        if theLock.locked() == 1:
            thread.start_new_thread(self.archiveTo.CreateArchive, (self.archivePtr, theLock, the_logger))
        thread.start_new_thread(self.archiveProgress, (theLock,))


        return

    def archiveProgress(self, theLock):

        """
        archiveProgress is a misleading name because this method doesn't
        have anything to do with archive progres.  It originally worked
        with the progress bar, but now it simply redraws the write archive now
        button when synthesis is done or cancelled.  THe following commented
        line is left so you see how to use the progress bar, but uncommenting it
        will not make the progress bar work since this method blocks until archive
        synthesis is stopped.
        """
        #self.updateProgress(100*self.archiveTo.progress/len(self.archiveTo.copy_list), 100)

        theLock.acquire(1)

        self.TheEngineCancel.pack_forget()
        self.TheLogWrite.pack()

        self.updateProgress(0, 100)
        
        return

    def CancelEngine(self):
        """
        the archiveTO.cancelled field is used in the CreateArchive engine
        to stop synthesis if it it set to YES
        """
        self.archiveTo.cancelled = "YES"
        return

    def createButtons(self):
        """
        This method creates the buttons in the command area, it
        calls upon this classes superclass, the AppShell contained
        in GUIstructure.py
        """
        
        self.buttonAdd("ArchiveSummary",
                       "View Achive Summary")
        self.buttonAdd("ChannelBrowser",
                       "View Channel Browser")
        self.buttonAdd("ValueProcessor",
                       "View Value Processor")
        self.buttonAdd("BatchProcessor",
                       "View Batch Processor")
        self.buttonAdd("CopyEngine",
                       "View Copy Engine")
        return


    def FrameSwitcher(self, frame):

        """
        this method simply does the packing and unpacking that
        allows the different screens to pop up as you move through
        the archive synthesis process
        """

        if frame == 'ArchiveSummary':
            self.ChannelBrowserFrame.pack_forget()
            self.ValueProcessingFrame.pack_forget()
            self.CopyEngineFrame.pack_forget()
            self.BatchProcessingFrame.pack_forget()
            self.ArchiveSummaryFrame.pack()
        elif frame == 'ChannelBrowser':
            self.ArchiveSummaryFrame.pack_forget()
            self.ValueProcessingFrame.pack_forget()
            self.CopyEngineFrame.pack_forget()
            self.BatchProcessingFrame.pack_forget()
            self.ChannelBrowserFrame.pack()
        elif frame == 'ValueProcessor':
            self.ArchiveSummaryFrame.pack_forget()
            self.ChannelBrowserFrame.pack_forget()
            self.CopyEngineFrame.pack_forget()
            self.BatchProcessingFrame.pack_forget()
            self.ValueProcessingFrame.pack()
        elif frame == 'CopyEngine':
            self.ArchiveSummaryFrame.pack_forget()
            self.ChannelBrowserFrame.pack_forget()
            self.ValueProcessingFrame.pack_forget()
            self.BatchProcessingFrame.pack_forget()
            self.CopyEngineFrame.pack()
        elif frame == 'BatchProcessor':
            self.ArchiveSummaryFrame.pack_forget()
            self.ValueProcessingFrame.pack_forget()
            self.CopyEngineFrame.pack_forget()
            self.ChannelBrowserFrame.pack_forget()
            self.BatchProcessingFrame.pack()
        return

        
CArDMiner = BuildAppShell(balloon_state='both')
CArDMiner.run()
    
