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

def ShowArchiveSummary(self):
    """
    FillArchiveSummary fills text buffer to be displayed
    in the ArchiveSummary page of CArDMiner
    """

    self.SummaryText.delete(0.0, END)
    
    self.SummaryText.tag_configure('bold', font=('', 12, 'bold'))
    self.SummaryText.tag_configure('regular', font=('', 12))
    
    self.SummaryText.insert(END, 'Archive Name: ', 'bold')
    self.SummaryText.insert(END, self.archiveRecord.archive_name, 'regular')
    self.SummaryText.insert(END, '\n# of channels: ', 'bold')
    self.SummaryText.insert(END, self.archiveRecord.num_of_channels, 'regular')
    self.SummaryText.insert(END, '\nFirst Time_Stamp in Archive:\n', 'bold')
    self.SummaryText.insert(END, self.archiveRecord.first_stamp, 'regular')
    self.SummaryText.insert(END, ' (', 'bold')
    self.SummaryText.insert(END, self.archiveRecord.first_channel, 'regular')
    self.SummaryText.insert(END, ')\nLast Time_Stamp in Archive:\n', 'bold')
    self.SummaryText.insert(END, self.archiveRecord.last_stamp, 'regular')
    self.SummaryText.insert(END, ' (', 'bold')
    self.SummaryText.insert(END, self.archiveRecord.last_channel, 'regular')
    self.SummaryText.insert(END, ')', 'bold')
    return
