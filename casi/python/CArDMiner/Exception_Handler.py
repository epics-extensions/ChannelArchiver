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

from tkMessageBox import showerror, askyesno

class theHandler:
    def __init__(self):
        """
        This isn't really an exception handler, just a error box spawner,
        can be replaced in ArchiveStructs.py with another mechanism,
        however be aware that all of the errors are reported in this
        format
        """
        pass
    
    def Raise(self, errno, error_string = ''):
        if errno == 0:
            theMsg = "There must be a problem. ReOpen the Archive"
        elif errno == 1:
            theMsg = "There was a problem opening the Archive"
        elif errno == 2:
            theMsg = "Invalid Request.\nYou must select a valid channel"
        elif errno == 3:
            theMsg = "You need to specify target archive and/or log file"
        elif errno == 4:
            theMsg = "There was a problem creating the archive"
        elif errno == 5:
            theMsg = "No Channels To Add!"
        elif errno == 6:
            theMsg = "Syntax Error in Code Snippet\n" + error_string 
        elif errno == 7:
            theMsg = "You need to specify Channels to include in Batch"
        elif errno == 8:
            theMsg = "You need to specify Batch Name"
        elif errno == 9:
            theMsg = "The Batch Name you are using already exists"
        elif errno == 10:
            theMsg = "No snippets selected!"
        elif errno == 11:
            theMsg = "Must select batch"
        elif errno == 12:
            theMsg = "Runtime Error in Snippet\n" + error_string
        elif errno == 13:
            theMsg = "That isn't a file that you can open"
        elif errno == 14:
            theMsg = "Invalid Operation"

        showerror("ArchiveError:", theMsg)
        return
