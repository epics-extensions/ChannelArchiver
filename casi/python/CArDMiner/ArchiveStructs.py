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

import casi
import string
import Exception_Handler
from os import listdir
from os.path import join
from Tkinter import *

theHandler = Exception_Handler.theHandler()

class ArchiveFrom:

    def __init__(self):
        ### most of these are set in ArchiveSummary.py
        self.archive_name = ''     # filename of source archive
        self.num_of_channels = 0   # number of channels in source archive
        self.first_stamp = 0       # first timestamp in source archive
        self.first_channel = ''    # channel with first timestamp
        self.last_stamp = 0        # last timestamp in source archive
        self.last_channel = ''     # channel with last timestamp
        self.channelList = []      # list of all channels in source archive
        return


    
    def open(self, archivePtr, archive_name):   
        """
        This function will open an archive and return
        it's pointer or 0 if no name is obtained
        """
        self.archive_name = archive_name

        if not self.archive_name:
            return 0

        try:
            archivePtr.open(self.archive_name)
        except:
            theHandler.Raise(1)
            archivePtr = 0
            
        return archivePtr

    def get_info(self, archivePtr):
        """
        This method simply fills the summary information.
        The information is used on the summary display
        and the first and last stamps are used as default
        time ranges for destination archive
        """
        
        channelIterator = casi.channel()
        channelCount = 0
        
        archivePtr.findFirstChannel(channelIterator)
        self.first_stamp = self.last_stamp = channelIterator.getFirstTime()
        self.first_channel = self.last_channel = channelIterator.name()
        while(channelIterator.valid()):
            if(channelIterator.getFirstTime() < self.first_stamp):
                self.first_stamp = channelIterator.getFirstTime()
                self.first_channel = channelIterator.name()
            if(channelIterator.getLastTime() > self.last_stamp):
                self.last_stamp = channelIterator.getLastTime()
                self.last_channel = channelIterator.name()
            channelCount = channelCount+1
            self.channelList.append(str(channelIterator.name()))
            channelIterator.next()

        self.num_of_channels = channelCount
        return

    
    def UpdateChannelList(self, archivePtr, include, exclude):
        """
        This method will take two strings to do regular expression
        based inclusion and exclusion in the display list.
        Note: this simply modifies the channel list, but will not
        affect archive synthesis because that is driven by the copy_list
        in the ArchiveTo structure
        """
        
        channelIterator = casi.channel()

        self.channelList = []

        archivePtr.findChannelByPattern(include, channelIterator)
            
        while channelIterator.valid():            
            self.channelList.append(channelIterator.name())
            channelIterator.next()

        try:
            if exclude:            
                archivePtr.findChannelByPattern(exclude, channelIterator)
        except AttributeError:
            theHandler.Raise(14)
            return
        
        while channelIterator.valid():
            if channelIterator.name() in self.channelList:
                del self.channelList[self.channelList.index(channelIterator.name())]
            channelIterator.next()

        return

class ArchiveTo:

    def __init__(self, globals_dict):
        self.archive_name = ''                     # destination archive filename
        self.log_name = './CArDMiner.log'          # archive synthesis log file
        self.hours_per_file = casi.HOURS_PER_MONTH # destination archive hours per file
        self.start_date = ''                       # destination archive start date/time
        self.end_date = ''                         # destination archive end date/time

        #self.num_of_channels = 0

        self.copy_list = []    # list of channels to include in destination archive
        self.copy_dict = {}    # dictionary to ChannelFrom objects referenced by channel name       

        self.batch_list = []   # list of batches to include in destinaton archive
        self.batch_dict = {}   # dictionary to BatchFrom objects referenced by batch name

        self.snippetLib = SnippetLibrary()  # class used to read snippet directory
        
        self.channel_done = "NO"       # flag used in ArchiveEngine control flow
        self.cancelled = "NO"          # "                                     "

        self.progress = 0      # a counter for use with the progress bar

        return

    def setName(self, archiveTo_name):
        """
        This specifies the destination archive filename
        """
        self.archive_name = archiveTo_name
        if not self.archive_name:
            return 0
        return 

    def ToggleCopyChannel(self, channelTuple, delete = "NO"):
        """
        This method allows you to add/delete channels to/from the copy_list.
        Pass a tuple of channel names and a delete = "YES" if you want to
        delete rather than add.
        """

        for channelName in channelTuple:
            if channelName not in self.copy_list:
                self.copy_list.insert(0, channelName)
                self.copy_list.sort()
                self.copy_dict[channelName] = ChannelFrom(channelName)
                #self.num_of_channels = self.num_of_channels + 1
            elif delete == "YES":
                    del self.copy_list[self.copy_list.index(channelName)]
                    del self.copy_dict[channelName]
                    #self.num_of_channels = self.num_of_channels - 1

        return

    def ToggleCopyBatch(self, batchName, channelTuple, delete = "NO"):
        """
        This method allows you to add/delete batchs to/from the batch_list.
        Pass a tuple of batch names and a delete = "YES" if you want to
        delete rather than add.
        """

        if delete == "YES":
            del self.batch_list[self.batch_list.index(batchName)]
            del self.batch_dict[batchName]
            return
        
        elif batchName in self.batch_list:
            theHandler.Raise(9)
            return
        else:            
            self.batch_list.append(batchName)
            self.batch_list.sort()
            self.batch_dict[batchName] = BatchFrom(batchName, channelTuple)
            return

    def document_proc_code(self, the_logger):
        """
        This method outputs all applied code snippets to the log
        file for possible scenario reconstruction down the road
        """

        for batchName in self.batch_list:
            the_logger.AddLine('')
            the_logger.AddLine("Applying following code to batch: " + batchName)
            the_logger.AddLine("This will apply to the following channels: " + repr(self.batch_dict[batchName].channelTuple))
            for snippetName in self.batch_dict[batchName].snippet_list:

                the_logger.AddLine("\n-Start " + snippetName + " code-------------------------------------")
                the_logger.AddLine(self.batch_dict[batchName].code)
                the_logger.AddLine("-End " + snippetName + " code------------------------------------------\n")
                
        return

    def RunInitProcCode(self, theLock, the_logger):
        """
        This method runs the code in all of the snippets contained
        in their respective #$GLBL sections.
        """

        self.__dict__['the_logger'] = locals()['the_logger']

        try:
            for batchName in self.batch_list:
                for snippetName in self.batch_dict[batchName].snippet_list:
                    exec self.batch_dict[batchName].glbl_codeObj_dict[snippetName] in self.__dict__, self.__dict__
        except Exception, e:
            tb = sys.exc_info()[2]
            while tb.tb_next:
                tb = tb.tb_next
            theHandler.Raise(12, tb.tb_frame.f_code.co_filename)
            theLock.release();
            
        return 

    def CreateArchive(self, old_archivePtr, theLock, the_logger):
        """
        The beautiful copy engine...
        A brief flow description
        1) Do some error checks and archive pointer/channelIterator/valueIterator initialization
        2) Do some initial logging
        3) Iterate through copy_list doing
           3a) find channel by name
           3b) getValueAfterTime after time specified by ArchiveTo.start_date
           3c) run appropriate #$INIT snippets after determining batch membership
           3d) run appropriate #$BODY snippets after determining batch membership
           3e) check if the value is to be deleted explicitly
           3f) add/skip value based on whether 3c, 3d, or 3e marked for deletion
           3g) move onto next value
           3h) check if it is valid and is still before ArchiveTo.end_date
           3i) if both checks in 3g pass, go back to 3c, otherwise mark channel_done true and fall through
           3j) run appropriate #$BODY snippets after determining batch membership
        4) Do some post logging
        5) We're done, hopefully... :)
        """


        ### 1) Do some error checks and archive pointer/channelIterator/valueIterator initialization
        if not self.archive_name and self.log_name:
            theHandler.Raise(3)
            theLock.release()
            return

        new_archivePtr = casi.archive()
        chanI = casi.channel()
        valI = casi.value()
        new_chanI = casi.channel()
        self.progress = 0
        
        self.channel_done = "NO"
        self.cancelled = "NO"

        if not new_archivePtr.write(self.archive_name, self.hours_per_file):
            the_logger.AddLine("There was a problem creating the archive")
            theHandler.Raise(4)
            the_logger.AddLine("Cancelling Archive Synthesis")
            the_logger.close()
            theLock.release()
            return
        
        elif len(self.copy_list) == 0:
            the_logger.AddLine("No Channels To Add!")
            theHandler.Raise(5)
            the_logger.AddLine("Cancelling Archive Synthesis")
            the_logger.close()
            theLock.release()
            return
        
        ### 2) Do some initial logging
        the_logger.AddLine("I have created an archive: " + self.archive_name)
        the_logger.AddLine("I will write this log to: " + self.log_name)
        the_logger.AddLine("Values will be added in the TimeRange:")
        the_logger.AddLine("between " + self.start_date + " and " + self.end_date)
        the_logger.AddLine("The archive will write files with " + str(self.hours_per_file) + " hours per file")

        self.document_proc_code(the_logger)

        locals()['chanI'] = chanI
        locals()['valI'] = valI

        ### 3) Iterate through copy_list doing
        for channelName in self.copy_list:

            ### 3a) find channel by name
            old_archivePtr.findChannelByName(channelName, chanI)
            self.channel_done = "NO"

            ### 3b) getValueAfterTime after time specified by ArchiveTo.start_date
            chanI.getValueAfterTime(self.start_date, valI)

            if valI.valid():
                if valI.time() >= self.end_date:
                    self.channel_done = "YES"
                    the_logger.AddLine("Skipped " + chanI.name() + ", no values in time range")
                else:
                    new_archivePtr.addChannel(chanI.name(), new_chanI)
                    the_logger.AddLine("Added " + chanI.name() + " to archive")

                    ### 3c) run appropriate #$INIT  snippets after determining batch membership
                    try:
                        for batchName in self.batch_list:
                            for snippetName in self.batch_dict[batchName].snippet_list:
                                exec self.batch_dict[batchName].init_codeObj_dict[snippetName]
                    except Exception, e:
                        tb = sys.exc_info()[2]
                        while tb.tb_next:
                            tb = tb.tb_next
                        theHandler.Raise(12, tb.tb_frame.f_code.co_filename)
                        theLock.release();
                        return

            else:
                self.channel_done = "YES"
                the_logger.AddLine("Skipped " + chanI.name() + ", no values in time range")
                
            valueCountTotal = valueCountAdded = 0

            for key in self.__dict__.keys():
                globals()[key] = self.__dict__[key]

            while valI.valid() and self.channel_done == "NO":

                if self.cancelled == "YES":
                    theLock.release()
                    the_logger.AddLine("Archive Synthesis has been cancelled")
                    the_logger.close()
                    return

                locals()['skip_value'] = "NO"

                if valI.time() < self.start_date or valI.time() > self.end_date:
                    locals()['skip_value'] = "YES"

                valueCountTotal = valueCountTotal + 1

                ### 3d) run appropriate #$BODY snippets after determining batch membership
                try:
                    for batchName in self.batch_list:
                        if chanI.name() in self.batch_dict[batchName].channelTuple:
                            for snippetName in self.batch_dict[batchName].snippet_list:
                                exec self.batch_dict[batchName].body_codeObj_dict[snippetName]
                except Exception, e:
                    tb = sys.exc_info()[2]
                    while tb.tb_next:
                        tb = tb.tb_next
                    theHandler.Raise(12, tb.tb_frame.f_code.co_filename)
                    theLock.release();
                    return

                ### 3e) check if the value is to be deleted explicitly
                ### AND
                ### 3f) add/skip value based on whether 3c, 3d, or 3e marked for deletion
                if valI.time() not in self.copy_dict[chanI.name()].markedValueList and locals()['skip_value'] == "NO":
                        
                    count_val = "YES"
                    if not new_chanI.addValue (valI):
                        next_file_time = new_archivePtr.nextFileTime(valI.time())
                        count = valI.determineChunk(next_file_time)
                        new_chanI.addBuffer (valI, count)
                        #try again
                        if not new_chanI.addValue (valI):
                            count_val = "NO"
                            raise IOError, "cannot add value"
                else:
                    count_val = "NO"
                            
                if count_val == "YES":
                    valueCountAdded = valueCountAdded + 1

                ### 3g) move onto next value
                valI.next()

                ### 3h) check if it is valid and is still before ArchiveTo.end_date
                ### 3i) if both checks in 3g pass, go back to 3c, otherwise mark channel_done true and fall through    
                if valI.valid() and valI.time() >= self.end_date:
                    self.channel_done = "YES"
                    
            the_logger.AddLine("\tAdded " + str(valueCountAdded) + " values from a possible " + str(valueCountTotal))

            self.progress = self.progress + 1
            new_chanI.releaseBuffer()


        ### 3j) run appropriate #$BODY snippets after determining batch membership
        try:
            for batchName in self.batch_list:
                for snippetName in self.batch_dict[batchName].snippet_list:
                    exec self.batch_dict[batchName].post_codeObj_dict[snippetName]
        except Exception, e:
            tb = sys.exc_info()[2]
            while tb.tb_next:
                tb = tb.tb_next
            theHandler.Raise(12, tb.tb_frame.f_code.co_filename)
            theLock.release();
            return

        ### 4) Do some post logging
        the_logger.AddLine("Archive Synthesis complete")
        the_logger.close()

        ### 5) We're done, hopefully... :)
        theLock.release()

class ChannelFrom:

    def __init__(self, channelNameIn):
        """
        A ChannelFrom is created for each channel in the copy_list.
        Each channels ChannelFrom is accesible from the copy_dict
        referenced by channel name
        """
        self.channelName = channelNameIn
        self.valueList = []
        self.markedValueList = ['Set to copy entire channel']
        self.compiled = "NO"
        return

    def AddMarkedValues(self, valueTuple):
        """
        This method allows you to mark explicit
        values for deletion by passing a tuple
        the string representations (i.e. valI.text())
        of values you wish to remove explicitly
        """
        
        for value in valueTuple:
            self.markedValueList.append(value)

        if len(self.markedValueList) > 1 and 'Set to copy entire channel' in self.markedValueList:
            del self.markedValueList[self.markedValueList.index('Set to copy entire channel')]
        return

    def RemoveMarkedValues(self, valueTuple):
        """
        The complement of AddMarkedValues, this method
        accepts a tuple of value strings and removes
        them from the marked list.  Note: there is no
        error checking, so the value better be on the list
        (i.e. obtained from the list)
        """

        for value in valueTuple:
            del self.markedValueList[self.markedValueList.index(value)]

        if len(self.markedValueList) == 0:
            self.markedValueList.append('Set to copy entire channel')
        return    

class BatchFrom(ChannelFrom):
    def __init__(self, batchName, channelTupleIn):
        """
        A BatchFrom is created for each batch in the batch_list.
        Each batch's BatchFrom is accesible from the bach_dict
        referenced by batch name
        """
        ChannelFrom.__init__(self, batchName)
        self.channelTuple = channelTupleIn
        self.snippet_list = []

        self.glbl_codeObj_dict = {}
        
        self.init_codeObj_dict = {}

        self.body_codeObj_dict = {}

        self.post_codeObj_dict = {}
        return

    def ApplySnippet(self, snippet, full_path):
        """
        This method applies a snippet to a batch:
        That is, it byte-compiles the codes snippet
        using the ParseAndCompile method in this class
        and adds it's name to this BatchFrom's snippet list
        so you can reference the actual code objects in
        various dictionaries.
        """
        if not self.ParseAndCompile(snippet, full_path):
            if snippet not in self.snippet_list:
                self.snippet_list.append(snippet)
        return

    def UnapplySnippet(self, snippet):
        """
        The complement of ApplySnippet, this simply
        removes the compiled code object from this
        BatchFrom's snippet_list.
        """
        if snippet in self.snippet_list:
            del self.snippet_list[self.snippet_list.index(snippet)]
        return

    def ParseAndCompile(self, snippet, file):
        """
        This method performs byte-code compilation
        on each section of the code snippet file and
        puts the snippets in the corresponding code object dictionary,
        referencable by snippet name (which is kept in snippet_list).
        """
        
        my_file = open(file)
        self.code = my_file.read()

        mark = 0
        first = 0
        last = 10
        
        for char in self.code:

            if self.code[first:last] == '#$END_GLBL':
                glbl_code = self.code[0:last] + "\n"
                mark = last
                objects = 0

                try:
                    self.glbl_codeObj_dict[snippet] = compile(glbl_code, "Section #$GLBL in file: " + file, 'exec')
                    objects = objects + 1
                except SyntaxError:
                    theHandler.Raise(6, "Section #$GLBL in file: " + file)
            
            if self.code[first:last] == '#$END_INIT':
                init_code = self.code[mark:last] + "\n"
                mark = last
                try:
                    self.init_codeObj_dict[snippet] = compile(init_code, "Section #$INIT in file: " + file, 'exec')
                    objects = objects + 1
                except SyntaxError:
                    theHandler.Raise(6, "Section #$INIT in file: " + file)

            elif self.code[first:last] == '#$END_BODY':
                body_code = self.code[mark:last] + "\n"
                mark = last
                try:
                    self.body_codeObj_dict[snippet] = compile(body_code, "Section #$BODY in file: " + file, 'exec')
                    objects = objects + 1
                except SyntaxError, e:
                    theHandler.Raise(6, "Section #$BODY in file: " + file)

            elif self.code[first:last] == '#$END_POST':
                post_code = self.code[mark:last] + "\n"
                mark = last
                try:
                    self.post_codeObj_dict[snippet] = compile(post_code, "Section #$POST in file: " + file, 'exec')
                    objects = objects + 1
                except SyntaxError:
                    theHandler.Raise(6, "Section #$POST in file: " + file)
                    
            first = first + 1
            last = last + 1

        if objects < 4:
            return "FAILED"
        else:
            return 0
                                   
class SnippetLibrary:

    def __init__(self, snippets_dir = './CArDMiner/snippets/'):
        """
        This class simply obtains a directory listing of the
        snippets directory and makes it available in a list
        for external use.
        """
        self.snippets_dir = snippets_dir
        self.all_snippets_list = listdir(self.snippets_dir)
        return

    def refresh_path(self, new_path):
        """
        Relists snippets directory
        """
        self.snippets_dir = new_path
        self.all_snippets_list = listdir(self.snippets_dir)

class Logger:
    
    def __init__(self, logTextBox, logFilename):
        """
        The logger is a convenience class that allows writing the
        same log file to both a file and a output stream.
        """
        if logTextBox == "none":
            self.logTextBox = Text()
        else:
            self.logTextBox = logTextBox

        self.logFile = open(logFilename, "w")
        self.logTextBox.delete(0.0, END)
        self.logTextBox.insert(END, "Created logfile " + logFilename + ".......\n")
        return

    def AddLine(self, line):
        """
        Add a line to the logs
        """
        
        self.logFile.write(line + "\n")
        self.logTextBox.insert(END, line + "\n")
        #self.logTextBox.see(self.logTextBox.index(END))
        return

    def close(self):
        """
        Close the logs
        """
        self.logTextBox.insert(END, "Closed logfile........\n")
        self.logFile.close()
        return

    
