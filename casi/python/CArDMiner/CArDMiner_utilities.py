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
from tkFileDialog import asksaveasfilename
from Exception_Handler import theHandler
from re import split
from string import atoi
class Command:
    def __init__(self, func, *args, **kw):
        self.func = func
        self.args = args
        self.kw = kw
        
    def __call__(self, *args, **kw):
        args = self.args + args
        kw.update(self.kw)
        apply(self.func, args, kw)


class CArDMinerProperties:
    """
    this class is the popup properties dialog
    """
    def __init__ (self, root, start_date, end_date, hours_per_file, archiveTo_name = '', archiveTo_log = ''):

        self.archiveTo_name = archiveTo_name
        self.archiveTo_log = archiveTo_log
        self.month_list = ['',
                           'January',
                           'February',
                           'March',
                           'April',
                           'May',
                           'June',
                           'July',
                           'August',
                           'September',
                           'October',
                           'November',
                           'December']
        self.hours_per_file = hours_per_file
        self.start_date = start_date
        self.end_date = end_date
        
        self.__GUI (root)

    def activate (self):
        "Call activate to display & run dialog"

        if self.__dlg.activate () == 'Ok':
            if self.month_list.index(self.__SetStartMonth.get()) < 10:
                self.start_date = self.__SetStartYear._entryFieldEntry.get() + '/0' + str(self.month_list.index(self.__SetStartMonth.get())) + '/' + self.__SetStartDay._entryFieldEntry.get() + ' ' + self.__SetStartTime.getstring()
                self.end_date = self.__SetEndYear._entryFieldEntry.get() + '/0' + str(self.month_list.index(self.__SetEndMonth.get())) + '/' + self.__SetEndDay._entryFieldEntry.get() + ' ' + self.__SetEndTime.getstring()
            else:
                self.start_date = self.__SetStartYear._entryFieldEntry.get() + '/' + str(self.month_list.index(self.__SetStartMonth.get())) + '/' + self.__SetStartDay._entryFieldEntry.get() + ' ' + self.__SetStartTime.getstring()
                self.end_date = self.__SetEndYear._entryFieldEntry.get() + '/' + str(self.month_list.index(self.__SetEndMonth.get())) + '/' + self.__SetEndDay._entryFieldEntry.get() + ' ' + self.__SetEndTime.getstring()
            return self.archiveTo_name
        return ''

    def __GUI (self, root):
        # GUI
        root.option_readfile ('optionDB')

        self.__dlg = Pmw.Dialog (root, buttons=('Ok', 'Cancel'),
                                title='CArDMiner Properties')

        root = self.__dlg.interior()

        self.__SetDestGrp = Pmw.Group(root)
        self.__SetDestLbl = Label(self.__SetDestGrp.interior(), text = "Destination Archive: " + self.archiveTo_name)
        self.__SetDestBtn = Button(self.__SetDestGrp.interior(), text = "Pick Destination Filename",
                                   command = Command(self.__setDestName))

        self.__SetLogGrp = Pmw.Group(root)
        self.__SetLogLbl = Label(self.__SetLogGrp.interior(), text = "Log File: " + self.archiveTo_log)
        self.__SetLogBtn = Button(self.__SetLogGrp.interior(), text = "Pick Log Filename",
                                   command = Command(self.__setLogName))

        self.__SetHPFGrp = Pmw.Group(root)
        self.__SetHPF = Pmw.EntryField(self.__SetHPFGrp.interior(),
                                            label_text = "Hours Per File: " + str(self.hours_per_file), labelpos = N)
        self.__SetHPFBtn = Button(self.__SetHPFGrp.interior(), text = "Set Hours per file",
                                   command = Command(self.__setHPFName))


        start_month = ''
        start_list = split('/', self.start_date)
        for char in start_list[1]:
            if char != '0':
                start_month = start_month + char
        if start_month == '':
            start_month = '1'
        another_list = split(' ', start_list[2])
        last_list = split('\.', another_list[1])

        self.__SetStartGrp = Pmw.Group(root, tag_text = "New Archive Start Time")
        self.__SetStartMonth = Pmw.ComboBox(self.__SetStartGrp.interior(),
                                            label_text = "Month", labelpos = N)
        self.__SetStartMonth._list.setlist(self.month_list)
        self.__SetStartMonth.selectitem(atoi(start_month))
        self.__SetStartDay = Pmw.EntryField(self.__SetStartGrp.interior(),
                                            label_text = "Day", labelpos = N)
        self.__SetStartDay._entryFieldEntry.configure(width = 2)
        self.__SetStartDay.setentry(another_list[0])        
        self.__SetStartYear = Pmw.EntryField(self.__SetStartGrp.interior(),
                                             label_text = "Year", labelpos = N)
        self.__SetStartYear._entryFieldEntry.configure(width = 4)
        self.__SetStartYear.setentry(start_list[0])        
        self.__SetStartTime = Pmw.TimeCounter(self.__SetStartGrp.interior(),
                                              label_text = "Time (24 Hour)", labelpos = N, value=last_list[0])

        end_month = ''
        end_list = split('/', self.end_date)
        for char in end_list[1]:
            if char != '0':
                end_month = end_month + char 
        if end_month == '':
            end_month = '1'
        another_list = split(' ', end_list[2])
        last_list = split('\.', another_list[1])
        
        self.__SetEndGrp = Pmw.Group(root, tag_text = "New Archive End Time")
        self.__SetEndMonth = Pmw.ComboBox(self.__SetEndGrp.interior(),
                                            label_text = "Month", labelpos = N)
        self.__SetEndMonth._list.setlist(self.month_list)
        self.__SetEndMonth.selectitem(atoi(end_month))
        self.__SetEndDay = Pmw.EntryField(self.__SetEndGrp.interior(),
                                            label_text = "Day", labelpos = N)
        self.__SetEndDay._entryFieldEntry.configure(width = 2)
        self.__SetEndDay.setentry(another_list[0])        
        self.__SetEndYear = Pmw.EntryField(self.__SetEndGrp.interior(),
                                             label_text = "Year", labelpos = N)
        self.__SetEndYear._entryFieldEntry.configure(width = 4)
        self.__SetEndYear.setentry(end_list[0])        
        self.__SetEndTime = Pmw.TimeCounter(self.__SetEndGrp.interior(),
                                              label_text = "Time (24 Hour)", labelpos = N, value=last_list[0])


        self.__SetDestLbl.pack()
        self.__SetDestBtn.pack()
        self.__SetDestGrp.pack()

        self.__SetLogLbl.pack()
        self.__SetLogBtn.pack()
        self.__SetLogGrp.pack()

        self.__SetHPF.pack()
        self.__SetHPFBtn.pack()
        self.__SetHPFGrp.pack()

        self.__SetStartMonth.pack(side = LEFT)
        self.__SetStartDay.pack(side = LEFT)
        self.__SetStartYear.pack(side = LEFT)
        self.__SetStartTime.pack(side = LEFT)
        self.__SetStartGrp.pack()

        self.__SetEndMonth.pack(side = LEFT)
        self.__SetEndDay.pack(side = LEFT)
        self.__SetEndYear.pack(side = LEFT)
        self.__SetEndTime.pack(side = LEFT)
        self.__SetEndGrp.pack()

    
        return

    def __setDestName(self):
        """
        called from GUI to set this classes archiven_name field,
        will actually be set in setDestName in CArDMiner.py
        """
        temp = asksaveasfilename()
        if temp != '':
            self.archiveTo_name = temp            
            
        self.__SetDestLbl.configure(text = "Destination Archive: " + self.archiveTo_name)
        return

    def __setLogName(self):
        """
        called from GUI to set this classes log_name field,
        will actually be set in setDestName in CArDMiner.py 
        """
        
        temp = asksaveasfilename()
        if temp != '':
            self.archiveTo_log = temp            
        self.__SetLogLbl.configure(text = "Log File: " + self.archiveTo_log)
        return

    def __setHPFName(self):
        """
        called from GUI to set this classes hours_per_file field,
        will actually be set in setDestName in CArDMiner.py 
        """
        self.hours_per_file = atoi(self.__SetHPF._entryFieldEntry.get())
        if self.hours_per_file == 0:
            self.hours_per_file = 1
        self.__SetHPF.configure(label_text = "Hours Per File: " + str(self.hours_per_file))
        pass
        
theHandler = theHandler()

