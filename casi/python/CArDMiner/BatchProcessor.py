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

import ArchiveStructs
from Tkinter import *
from os.path import join

def MakeBatch(self):
    """
    this method queries the channel list and the batch name list
    on the ValueProcessor page, and sends them to ToggleCopyBatch
    in order to create a batch
    """

    try:
        if not self.BatchName.get():
            self.theHandler.Raise(8)
            return
        elif not self.BChannelList.getcurselection():
            self.theHandler.Raise(7)
            return
    except:
        pass
    else:
        self.archiveTo.ToggleCopyBatch(self.BatchName.get(), self.BChannelList.getcurselection())

    UpdateBatchList(self)
    return

def RemoveBatch(self):
    """
    This method queries the batch list on the ValueProcessor
    page and deletes the batch(s) selected from the batch_list
    by invoking ToggleCopyBatch with a delete = YES
    """

    try:
        if not self.BatchList.getcurselection():
            self.theHandler.Raise(8)
            return
    except:
        pass

    else:
        self.archiveTo.ToggleCopyBatch(self.BatchList.getcurselection()[0], (), delete = "YES")

    UpdateBatchList(self)
    return

def UnApplySnippet(self):
    """
    This method removes the binding between a a batch
    and a code snippet by removing the snippet(s) from
    the snippet list in the corresponding BatchFrom structure
    """
    
    try:
        snippet_tuple = self.AppliedCodeSnippetsList.getcurselection()
    except IndexError:
        self.theHandler.Raise(10)
        return

    try:
        for snippet in snippet_tuple:
            self.archiveTo.batch_dict[self.processing_batch].UnapplySnippet(snippet)
    except AttributeError:
        self.theHandler.Raise(11)
        return

    UpdateBatchList(self)
    ShowBatchProcessing(self)
        
    return

def ApplySnippet(self):
    """
    This method binds a snippet with a batch.  Remember that the
    ApplySnippet function in the BatchFrom class does byte-compilation
    which must be successful for this to result in snippet application to the batch.
    """
    
    try:
        snippet_tuple = self.CodeSnippetsList.getcurselection()
    except IndexError:
        self.theHandler.Raise(10)
        return

    try:
        for snippet in snippet_tuple:
            full_path = join(self.archiveTo.snippetLib.snippets_dir, snippet)
            self.archiveTo.batch_dict[self.processing_batch].ApplySnippet(snippet, full_path)
    except KeyError:
        self.theHandler.Raise(8)
    except AttributeError:
        self.theHandler.Raise(11)
    except SyntaxError:
        self.theHandler.Raise(6)

        return

    UpdateBatchList(self)
    ShowBatchProcessing(self)
        
    return

def ShowBatchProcessing(self):
    """
    This is a convenience method that shows a batch summary
    in the text box on the Batch Processing page.  It also sets
    self.processing_batch, the magic variable that holds the name
    of the batch whenever a batch is clicked on in the batch list
    """

    try:
        self.processing_batch = self.CodeSnipBatchList.getcurselection()[0]
    except IndexError:
        pass
        return
    
    try:
        self.AppliedCodeSnippetsList.setlist(self.archiveTo.batch_dict[self.processing_batch].snippet_list)

        self.CodeSnipUnapplySnippet.configure(text="Unapply Snippet from:\n" + self.processing_batch)
        self.CodeSnipApplySnippet.configure(text="Apply Snippet to:\n" + self.processing_batch)

        self.BatchSummary.delete(0.0, END)
    
        self.BatchSummary.tag_configure('bold', font=('', 12, 'bold'))
        self.BatchSummary.tag_configure('regular', font=('', 12))
    
        self.BatchSummary.insert(END, 'Batch Name: ', 'bold')
        self.BatchSummary.insert(END, self.archiveTo.batch_dict[self.processing_batch].channelName, 'regular')
        self.BatchSummary.insert(END, '\n# of channels: ', 'bold')
        self.BatchSummary.insert(END, repr(len(self.archiveTo.batch_dict[self.processing_batch].channelTuple)), 'regular')
        self.BatchSummary.insert(END, '\nNames of Channels:\n', 'bold')
        self.BatchSummary.insert(END, repr(self.archiveTo.batch_dict[self.processing_batch].channelTuple), 'regular')
        self.BatchSummary.insert(END, '\n# of Applied Code Snippets: ', 'bold')
        self.BatchSummary.insert(END, repr(len(self.archiveTo.batch_dict[self.processing_batch].snippet_list)), 'regular')
        self.BatchSummary.insert(END, '\nApplied Code Snippets:\n', 'bold')
        self.BatchSummary.insert(END, repr(self.archiveTo.batch_dict[self.processing_batch].snippet_list), 'regular')
        self.BatchSummary.insert(END, '\n', 'bold')
    except AttributeError:
        self.theHandler.Raise(11)
        
    return

def ShowSnippet(self, window):
    """
    ShowSnippet is called when someone clicks
    on a snippet in the listbox, it simply loads
    the snippet file into the textbox for viewing, but NO EDITING.
    This could be extended to do directory navigation...
    """

    try:
        if window == "Applied":
            snippet = self.AppliedCodeSnippetsList.getcurselection()[0]
        elif window == "Master":
            snippet = self.CodeSnippetsList.getcurselection()[0]
    except IndexError:
        return

    snippet = join(self.archiveTo.snippetLib.snippets_dir, snippet)

    self.BatchSummary.delete(0.0, END)
    try:
        self.BatchSummary.importfile(snippet)
    except IOError:
        self.theHandler.Raise(13)
    return

def UpdateBatchList(self):
    """
    This method is a convenience function that simply refreshes
    all of the corresponding listboxes with their current lists
    """
    
    self.BatchList.setlist(self.archiveTo.batch_list)
    self.CodeSnipBatchList.setlist(self.archiveTo.batch_list)
    self.CodeSnippetsList.setlist(self.archiveTo.snippetLib.all_snippets_list)
    try:
        self.AppliedCodeSnippetsList.setlist(self.archiveTo.batch_dict[self.processing_batch].snippet_list)
    except KeyError:
        pass
    return

