#! /bin/env python
#
# ListSelectedFilteredChannels.py
#
# Adapted from ListChannels.py (written by Kay-Uwe Kasemir).
# Author: Bob Hall, 2/14/01 (rdh@slac.stanford.edu)
#
from Tkinter import *
import Pmw
import casi
import sys
import os
import time
import string
import Tkinter
from ErrorDlg import ErrorDlg

class ChannelDialog:
	"""Interactively lists selected channel names from archive.  The
           selected channels may be filtered by selecting items from scrolled
           lists to specify associated files of channel names for subsystems,
           sensor types, and stations. 
	   Input:
	     1. root - Tk root widget, Pmw initialized.
	     2. archive - open casi.archive.
             3. include_or_exclude - flag containing the + or - character to
                   indicate whether only the specified channels in the
                   selected_channel list that are archived should be included
                   in the displayed channel list or excluded from this list.
             4. selected_channels - list of channels names.  Those specified
                   channels that are archived will either be included in the
                   displayed channel list or be excluded from this list,
                   depending on the value of the include_or_exclude flag.
             5. archive_dir - directory in which the archive channel list and
                   filter files reside.
	     6. pattern - optional regular expression pattern.

	   Returns tuple of selected channels or ()
	"""
	def __init__ (self, root, archive, include_or_exclude, selected_channels, archive_dir, pattern=''):
		"List all channel names that match the pattern"

                """ Initialize global variables from input variables.
                """
		self._archive = archive
                self._include_or_exclude = include_or_exclude
                self._selected_channels = selected_channels

                self._cur_valid_channels = []
                self._activated = 0
                self._root = root
                self._archive_file_dir = archive_dir 
                self._dirfiles_file_name = "dirfiles.txt"

                nlcta_match_offset = string.find(archive_dir, 'nlcta')
                pepii_match_offset = string.find(archive_dir, 'pepii')

                if nlcta_match_offset != -1:
                    filtering_lists = 1
                    self._stations_file_name = "nlcta_stations.txt"
                    self._subsystems_file_name = "nlcta_subsystems.txt"
                    self._quantities_file_name = "nlcta_quantities.txt" 
                elif pepii_match_offset != -1:
                    filtering_lists = 1
                    self._stations_file_name = "pepii_stations.txt"
                    self._subsystems_file_name = "pepii_subsystems.txt"
                    self._quantities_file_name = "pepii_quantities.txt"
                else:
                    filtering_lists = 0

                """ Initialize two global identifiers for each filter scrolled list
                    (subsystem, sensor type, and station):
                      (1) a list of allowed channels for the filter, and
                      (2) a flag indicating whether this filter is active (a filter
                          is active if the selected scrolled list filter item is
                          not set to "All."
                    In addition, initialize similar global identifiers for the composite
                    filter, representing the "intersection" of each of the individual
                    filters.
                """
                self._allowed_subsystem_channels = []
                self._filtering_subsystem_active = 0

                self._allowed_sensor_type_channels = []
                self._filtering_sensor_type_active = 0

                self._allowed_station_channels = []
                self._filtering_station_active = 0

                self._allowed_channels = []
                self._filtering_active = 0

                self._initCurValidChannels()

                """ Setup a Dialog widget containing an interior for added widgets and
                    two buttons at the bottom: 'Ok" and 'Cancel.'
                """
                if include_or_exclude == '+':
                    self._dlg = Pmw.Dialog (root, buttons=('Ok', 'Cancel'),
                        title='Remove Channels')
                else:
                    self._dlg = Pmw.Dialog (root, buttons=('Ok', 'Cancel'),
                        title='Add Channels')

		root = self._dlg.interior()

                if filtering_lists:
                    """ For the subsystem filter, initialize a list containing all of the
                        subsystem items to be displayed in this filter's scrolled list.  Also
                        initialize another list containing the associated file for each subsystem
                        item (each file contains a list of allowed channels for the associated
                        subsystem).  Then setup a combo box for this filter with the initialized
                        values and a validation routine, which ensures that any values typed
                        into the entry field associated with the combo box matches one of the
                        items in the scrolled list.
                    """

                    if nlcta_match_offset != -1:
                        self._create_nlcta_filter_lists()
                    elif pepii_match_offset != -1:
                        self._create_pepii_filter_lists()

#                    self._create_filter_lists()

                    """ Call _create_station_lists to initialize a list containing all of the
                        station items to be displayed in the station filter scrolled list and
                        to initialize a list containing the associated file for each station
                        item (each file contains a list of allowed channels for the associated
                        subsystem).  These lists will be constructed from all files in the
                        directory containing filter files with the file extension "station_arch."
                    """
#                    self._create_station_lists()

                    """ Setup a combo box for the station filter with the initialized values 
                        and a validation routine, which ensures that any values typed into
                        the entry field associated with the combo box matches one of the
                        items in the scrolled list.
                    """
                    self.stations_box = Pmw.ComboBox(root,
                            label_text = 'Stations:',
                            labelpos = 'nw',
                            scrolledlist_items = tuple(self._stations),
                            dropdown = 0)
                    self.stations_box.component('entryfield').configure(validate = self._validate_stations)

                    self.stations_box.pack(side=LEFT, expand=YES, fill=BOTH)

#                    self._subsystems = ['All',
#                                        'Klystron and Circulator',
#                                        'HVPS',
#                                        'Waveguide',
#                                        'Cavities',
#                                        'System',
#                                        'Tuners',
#                                        'Feedback']

#                    self._subsystem_files = ['none',
#                                             'klys_circ.subsys_arch',
#                                             'hvps.subsys_arch',
#                                             'waveguide.subsys_arch',
#                                             'cavities.subsys_arch',
#                                             'system.subsys_arch',
#                                             'tuners.subsys_arch',
#                                             'feedback.subsys_arch']
 
                    self.subsystems_box = Pmw.ComboBox(root,
                            label_text = 'Subsystems:',
                            labelpos = 'nw',
                            scrolledlist_items = tuple(self._subsystems),
                            dropdown = 0)
                    self.subsystems_box.component('entryfield').configure(validate = self._validate_subsystem)

                    self.subsystems_box.pack(side=LEFT, expand=YES, fill=BOTH)

                    """ For the sensor type filter, do the same type of initialization for this
                        filter as was just done for the subsystem filter.
                    """
#                    self._sensor_types = ['All',
#                                          'Vacuum',
#                                          'Temperature',
#                                          'Voltage',
#                                          'Current',
#                                          'Position',
#                                          'Power/Energy',
#                                          'Phase',
#                                          'Miscellaneous']

#                    self._sensor_type_files = ['none',
#                                               'vacuum.sensor_arch',
#                                               'temperature.sensor_arch',
#                                               'voltage.sensor_arch',
#                                               'current.sensor_arch',
#                                               'position.sensor_arch',
#                                               'power_energy.sensor_arch',
#                                               'phase.sensor_arch',
#                                               'miscellaneous.sensor_arch']

                    self.sensor_types_box = Pmw.ComboBox(root,
                            label_text = 'Quantities:',
                            labelpos = 'nw',
                            scrolledlist_items = tuple(self._sensor_types),
                            dropdown = 0)
                    self.sensor_types_box.component('entryfield').configure(validate = self._validate_sensor_type)

                    self.sensor_types_box.pack(side=LEFT, expand=YES, fill=BOTH)

                """ Setup an entry field for the regular expression pattern and a scrolled list
                    box for the selection list of channels.
                """
		self._pattern = Pmw.EntryField (root, labelpos=W,
						label_text="Pattern:",
						entry_width=40,
						value = pattern,
						command=self._listChannels)
		
		self._list = Pmw.ScrolledListBox (root,
						  listbox_selectmode=EXTENDED,
						  labelpos=NW,
						  label_text="Channels:")
		self._pattern.pack (expand=YES, fill=X)
		self._list.pack (expand=YES, fill=BOTH)

                if filtering_lists:
                    """ Initially set each filter combo box to the selection item 'All.'
                    """ 
                    self.subsystems_box.selectitem('All')
                    self.sensor_types_box.selectitem('All')
                    self.stations_box.selectitem('All')

        def _create_filter_lists (self):
                """ This routine forms the following types of lists for the subsystems,
                    sensor types, and stations filtering groups:
                      1. A list of all filter names to be displayed in the filter
                         scrolled list.
                      2. A list of file names of files containing channel names (one
                         file name per filter name).

                    These lists are formed from file names having extensions associated
                    with filtering groups ("subsystem_arch" for the subsystems
                    filtering group, "quantity_arch" for the sensor types filtering
                    group, and "station_arch" for the stations filtering group).  These
                    files containing channel names for each filter name were generated
                    from the channel list file used as input to the Channel Archiver.
                """
                self._subsystems = ['All']
                self._subsystem_files = ['none']
                self._sensor_types = ['All']
                self._sensor_type_files = ['none']
                self._stations = ['All']
                self._station_files = ['none']

                """ Spawn a shell command to list all files in the directory containing
                    archive filter files in one column and redirect the output in this
                    list to a file.  Then open this file for reading.
                """ 
                list_command = "ls -1 " + self._archive_file_dir + " >> " + self._dirfiles_file_name 
                os.system(list_command)

                try:
                    f = open(self._dirfiles_file_name, "r")
                except:
                    ErrorDlg (self._root, "Cannot open %s" % self._dirfiles_file_name)
                    return 0

                """ Read each line from the file containing file names until the end of file
                    (the readline routine returns the null string).  Only process file names 
                    containing a file extension.
                """   
                while 1:
                    afilename = f.readline()
                    if afilename == '':
                        break

                    period_offset = string.find(afilename, ".")
                    if period_offset == -1:
                        continue

                    filename_len = len(afilename)
                    file_extension = afilename[period_offset + 1: filename_len - 1]

                    if file_extension == "subsystem_arch":
                        self._subsystems.append(string.replace(afilename[:period_offset], '_', ' '))
                        self._subsystem_files.append(afilename[:filename_len - 1])
                    elif file_extension == "quantity_arch":
                        self._sensor_types.append(string.replace(afilename[:period_offset], '_', ' '))
                        self._sensor_type_files.append(afilename[:filename_len - 1])
                    elif file_extension == "station_arch":
                        self._stations.append(string.replace(afilename[:period_offset], '_', ' '))
                        self._station_files.append(afilename[:filename_len - 1])

                """ Close the file and delete it.  The file must be deleted so the
                    next invocation of this routine will not append file names to
                    the old file containing file names!
                """
                f.close()
                os.unlink(self._dirfiles_file_name)

        def _create_nlcta_filter_lists (self):
                (self._subsystem_codes, self._subsystems) = self._build_filter_translation_table(self._subsystems_file_name) 
#                print "self._subsystem_codes = ", self._subsystem_codes
#                print "self._subsystems = ", self._subsystems
#                print "len(self._subsystems) = ", len(self._subsystems)

                self._subsystem_channels = []
                num_subsystems = len(self._subsystems)
                for i in range(num_subsystems):
                    self._subsystem_channels.append([])

                (self._sensor_type_codes, self._sensor_types) = self._build_filter_translation_table(self._quantities_file_name)
                self._sensor_type_channels = []
                num_sensor_types = len(self._sensor_types)
                for i in range(num_sensor_types):
                    self._sensor_type_channels.append([])

                (self._station_codes, self._stations) = self._build_filter_translation_table(self._stations_file_name)
                self._station_channels = []
                num_stations = len(self._stations)
#                print "num_stations = ", num_stations
                for i in range(num_stations):
                    self._station_channels.append([])

                for channel_name in self._cur_valid_channels:
                    parse_ok = 1
                    found_station = 0
                    found_subsystem = 0
                    found_quantity = 0

                    i_station_start = 0
                    i_station_end = string.find(channel_name, ":")
                    if i_station_end == -1:
                        parse_ok = 0
                    else:
                        found_station = 1

                    if parse_ok:
                        i_subsystem_start = i_station_end + 1
                        i_end = string.find(channel_name[i_subsystem_start:], ":")
                        if i_end == -1:
                            parse_ok = 0
                        else:
                            found_subsystem = 1
                            i_subsystem_end = i_subsystem_start + i_end

                    if parse_ok: 
                        i_detail_start = i_subsystem_end + 1
                        i_end = string.find(channel_name[i_detail_start:], ":")
                        if i_end == -1:
                            parse_ok = 0
                        else:
                            i_detail_end = i_detail_start + i_end

                    if parse_ok: 
                        i_quantity_start = i_detail_end + 1
                        i_end = string.find(channel_name[i_quantity_start:], ":")
                        if i_end != -1:
                            parse_ok = 0
                        else:
                            found_quantity = 1
                            i_quantity_end = len(channel_name) - 1

                    if found_subsystem:
#                        print "subsystem = ", channel_name[i_subsystem_start:i_subsystem_end]

                        found = 0
                        i = 0
                        while i < len(self._subsystem_codes) and not found:
                            i_subsystem_last = i_subsystem_start + len(self._subsystem_codes[i])
                            if self._subsystem_codes[i] == channel_name[i_subsystem_start:i_subsystem_last]:
                                found = 1
                            else:
                                i = i + 1

                        if found:
#                            print "subsystem index = ", i
                            self._subsystem_channels[i].append(channel_name)
                        else:
#                            print "subystem " + channel_name[i_subsystem_start:i_subsystem_end] + " not found"
                            self._subsystem_channels[num_subsystems - 1].append(channel_name)

                    else:
                        self._subsystem_channels[num_subsystems - 1].append(channel_name)


                    if found_quantity:
#                        print "quantity = ", channel_name[i_quantity_start:i_quantity_end]

                        found = 0
                        i = 0
                        while i < len(self._sensor_type_codes) and not found:
                            i_quantity_last = i_quantity_start + len(self._sensor_type_codes[i])
                            if self._sensor_type_codes[i] == channel_name[i_quantity_start:i_quantity_last]:
                                found = 1
                            else:
                                i = i + 1

                        if found:
#                            print "quantity index = ", i
                            self._sensor_type_channels[i].append(channel_name)
                        else:
#                            print "quantity " + channel_name[i_quantity_start:i_quantity_end] + " not found"
                            self._sensor_type_channels[num_sensor_types - 1].append(channel_name)

                    else:
                        self._sensor_type_channels[num_sensor_types - 1].append(channel_name)


                    if found_station:
#                        print "station = ", channel_name[i_station_start:i_station_end]

                        found = 0
                        i = 0
                        while i < len(self._station_codes) and not found:
                            i_station_last = i_station_start + len(self._station_codes[i]) 
                            if self._station_codes[i] == channel_name[i_station_start:i_station_last]:
                                found = 1
                            else:
                                i = i + 1

                        if found:
#                            print "station index = ", i
                            self._station_channels[i].append(channel_name)
                        else:
#                            print "station " + channel_name[i_station_start:i_station_end] + " not found"
                            self._station_channels[num_stations - 1].append(channel_name)

                    else:
                        self._station_channels[num_stations - 1].append(channel_name)

#                print "Miscellaneous subsystem channels = ", self._subsystem_channels[num_subsystems - 1]

        def _build_filter_translation_table(self, translation_file_name):
                codes_list = ['none']
                filter_names_list = ['All']

                full_translation_file_name = self._archive_file_dir + translation_file_name 
                try:
                    f = open(full_translation_file_name, "r")
                except:
                    ErrorDlg (self._root, "Cannot open %s" % full_translation_file_name)
                    return 0

                while 1:
                    line_buf = f.readline()
                    if line_buf == '':
                        break

                    if line_buf[0] == '#':
                        continue

                    i_code_end = string.find(line_buf, ' ')
                    if i_code_end == -1:
                        continue

                    i = i_code_end
                    found = 0
                    while i < len(line_buf) and not found:
                        if line_buf[i] != ' ':
                            found = 1
                        else:
                            i = i + 1

                    if not found:
                        continue

                    i_trans_start = i

                    i_end = string.find(line_buf[i_trans_start:], '\n')
                    if i_end == -1:
#                        print "end-of-line not found"
                        continue

                    i_trans_end = i_trans_start + i_end

                    codes_list.append(string.replace(line_buf[:i_code_end], '_', ' '))
                    filter_names_list.append(string.replace(line_buf[i_trans_start:i_trans_end], '_', ' '))

                f.close()

                codes_list.append('N/A')
                filter_names_list.append('Miscellaneous')

                return (codes_list, filter_names_list)  

        def _create_pepii_filter_lists (self):
                (self._subsystem_codes, self._subsystems) = self._build_filter_translation_table(self._subsystems_file_name) 
#                print "self._subsystem_codes = ", self._subsystem_codes
#                print "self._subsystems = ", self._subsystems
#                print "len(self._subsystems) = ", len(self._subsystems)

                self._subsystem_channels = []
                num_subsystems = len(self._subsystems)
                for i in range(num_subsystems):
                    self._subsystem_channels.append([])

                (self._sensor_type_codes, self._sensor_types) = self._build_filter_translation_table(self._quantities_file_name)
                self._sensor_type_channels = []
                num_sensor_types = len(self._sensor_types)
                for i in range(num_sensor_types):
                    self._sensor_type_channels.append([])

                (self._station_codes, self._stations) = self._build_filter_translation_table(self._stations_file_name)
                self._station_channels = []
                num_stations = len(self._stations)
#                print "num_stations = ", num_stations
                for i in range(num_stations):
                    self._station_channels.append([])

                for channel_name in self._cur_valid_channels:
                    parse_ok = 1
                    found_station = 0
                    found_subsystem = 0
                    found_quantity = 0

                    i_station_start = 0
                    i_station_end = string.find(channel_name, ":")
                    if i_station_end == -1:
                        parse_ok = 0
                    else:
                        found_station = 1

                    if parse_ok:
                        i_subsystem_start = i_station_end + 1
                        i_end = string.find(channel_name[i_subsystem_start:], ":")
                        if i_end == -1:
                            parse_ok = 0
                        else:
                            found_subsystem = 1
                            i_subsystem_end = i_subsystem_start + i_end

                    if parse_ok: 
                        i_quantity_start = i_subsystem_end + 1
                        i_end = string.find(channel_name[i_quantity_start:], ":")
                        if i_end != -1:
                            parse_ok = 0
                        else:
                            found_quantity = 1
                            i_quantity_end = len(channel_name) - 1

                    if found_subsystem:
#                        print "subsystem = ", channel_name[i_subsystem_start:i_subsystem_end]

                        found = 0
                        i = 0
                        while i < len(self._subsystem_codes) and not found:
                            i_subsystem_last = i_subsystem_start + len(self._subsystem_codes[i])
                            if self._subsystem_codes[i] == channel_name[i_subsystem_start:i_subsystem_last]:
                                found = 1
                            else:
                                i = i + 1

                        if found:
#                            print "subsystem index = ", i
                            self._subsystem_channels[i].append(channel_name)
                        else:
#                            print "subystem " + channel_name[i_subsystem_start:i_subsystem_end] + " not found"
                            self._subsystem_channels[num_subsystems - 1].append(channel_name)

                    else:
                        self._subsystem_channels[num_subsystems - 1].append(channel_name)


                    if found_quantity:
#                        print "quantity = ", channel_name[i_quantity_start:i_quantity_end]

                        found = 0
                        i = 0
                        while i < len(self._sensor_type_codes) and not found:
                            i_quantity_last = i_quantity_start + len(self._sensor_type_codes[i])
                            if self._sensor_type_codes[i] == channel_name[i_quantity_start:i_quantity_last]:
                                found = 1
                            else:
                                i = i + 1

                        if found:
#                            print "quantity index = ", i
                            self._sensor_type_channels[i].append(channel_name)
                        else:
#                            print "quantity " + channel_name[i_quantity_start:i_quantity_end] + " not found"
                            self._sensor_type_channels[num_sensor_types - 1].append(channel_name)

                    else:
                        self._sensor_type_channels[num_sensor_types - 1].append(channel_name)


                    if found_station:
#                        print "station = ", channel_name[i_station_start:i_station_end]

                        found = 0
                        i = 0
                        while i < len(self._station_codes) and not found:
                            i_station_last = i_station_start + len(self._station_codes[i]) 
                            if self._station_codes[i] == channel_name[i_station_start:i_station_last]:
                                found = 1
                            else:
                                i = i + 1

                        if found:
#                            print "station index = ", i
                            self._station_channels[i].append(channel_name)
                        else:
#                            print "station " + channel_name[i_station_start:i_station_end] + " not found"
                            self._station_channels[num_stations - 1].append(channel_name)

                    else:
                        self._station_channels[num_stations - 1].append(channel_name)

#                print "Miscellaneous subsystem channels = ", self._subsystem_channels[num_subsystems - 1]

        def _create_station_lists (self):
                """ This routine forms the list of all station names to be displayed
                    in the station filter scrolled list (self._stations) and the list
                    of each associated file containing channels names for the station
                    (self._station_files).
                """  
                self._stations = ['All']
                self._station_files = ['none']

                """ Spawn a shell command to list all files in the directory containing
                    archive filter files in one column and redirect the output in this
                    list to a file.  Then open this file for reading.
                """ 
                list_command = "ls -1 " + self._archive_file_dir + " >> " + self._dirfiles_file_name 
                os.system(list_command)

                try:
                    f = open(self._dirfiles_file_name, "r")
                except:
                    ErrorDlg (self._root, "Cannot open %s" % self._dirfiles_file_name)
                    return 0

                """ Read each line from the file containing file names until the end of file
                    (the readline routine returns the null string).  Only process file names 
                    containing a file extension.  If the file extension is "station_arch",
                    append the name of the file before the file extension to the list of
                    station filter items and append the full file name to the list of associated
                    files containing channel names.
                """   
                while 1:
                    afilename = f.readline()
                    if afilename == '':
                        break

                    period_offset = string.find(afilename, ".")
                    if period_offset == -1:
                        continue

                    filename_len = len(afilename)
                    file_extension = afilename[period_offset + 1: filename_len - 1]

                    if file_extension == "station_arch":
                        self._stations.append(afilename[:period_offset])
                        self._station_files.append(afilename[:filename_len - 1])

#                print "self._stations = ", self._stations
#                print "self._station_files = ", self._station_files

                """ Close the file and delete it.  The file must be deleted so the
                    next invocation of this routine will not append file names to
                    the old file containing file names!
                """
                f.close()
                os.unlink(self._dirfiles_file_name)

        def _validate_subsystem(self, text):
                """ This routine validates any entry entered into the entry field
                    of the subsystem filter combo box.  It makes sure that any entry
                    matches one of the items in the subsystem combo box scrolled
                    list.  This routine is also called when the user selects an
                    item in the subsystem filter scrolled list.  After validation,
                    this routine applys the selected item to form a new list
                    containing the allowed channels for the subsystem filter (if
                    the item selected was not 'All') and regenerates the list of
                    displayed channels based on the new subsystem filter selection.
                """
                found = 0
                i = 0
                while i < len(self._subsystems) and not found:
                    if text == self._subsystems[i]:
                        found = 1
                    else:
                        i = i + 1

                if found:
#                    self._change_subsystem(text)
                    self._switch_subsystem(text)
                    self._listFilteredChannels()
                    return 1
                else:
                    return -1 

        def _change_subsystem(self, text):
                """ This routine applys the selected item in the subsystem combo box
                    to form a new list containing the allowed channels for the subsystem
                    filter (if the item selected was not 'All').
                """

                if self._subsystems.count(text) == 0:
                    return 0
                list_index = self._subsystems.index(text)
#                print "list_index = ", list_index

                self._allowed_subsystem_channels = []
                if text == 'All':
                    self._filtering_subsystem_active = 0
                else:
                    """ The subsystem selected filter item was not 'All.'  Open the file
                        associated with the selected filter item that contains the names
                        of channels associated with the subsystem.  Then read each line
                        from the file, appending the read channel name line to this list
                        of allowed channels for the subsystem filter.
                    """
                    self._filtering_subsystem_active = 1 

                    try:
                        filename = self._archive_file_dir + self._subsystem_files[list_index]
                        f = open(filename, "r")
                    except:
                        self._form_allowed_channels_list()
                        return 0

                    while 1:
                        achannel = f.readline()
                        if achannel == '':
                            break

                        str_len = len(achannel)
                        self._allowed_subsystem_channels.append(achannel[:str_len - 1])

#                    print "allowed_subsystem_channels = ", self._allowed_subsystem_channels
                    f.close()

                """ Call routine _form_allowed_channels_list to form a list of allowed
                    channels from the subsystem, sensor type, and station filters by
                    determining the "intersection" of allowed channels for each filter.
                """
                self._form_allowed_channels_list()
                return 1

        def _switch_subsystem(self, text):
                """ This routine applys the selected item in the subsystem combo box
                    to form a new list containing the allowed channels for the subsystem
                    filter (if the item selected was not 'All').
                """

                if self._subsystems.count(text) == 0:
                    return 0
                list_index = self._subsystems.index(text)
#                print "subsystem  = ", text 
#                print "list_index = ", list_index

                self._allowed_subsystem_channels = []
                if text == 'All':
                    self._filtering_subsystem_active = 0
                else:
                    self._filtering_subsystem_active = 1 

                    self._allowed_subsystem_channels = self._subsystem_channels[list_index]

                """ Call routine _form_allowed_channels_list to form a list of allowed
                    channels from the subsystem, sensor type, and station filters by
                    determining the "intersection" of allowed channels for each filter.
                """
                self._form_allowed_channels_list()
                return 1

        def _validate_sensor_type(self, text):
                """ This routine validates any entry entered into the entry field
                    of the sensor type filter combo box.  It makes sure that any entry
                    matches one of the items in the sensor type combo box scrolled
                    list.  This routine is also called when the user selects an
                    item in the sensor type filter scrolled list.  After validation,
                    this routine applys the selected item to form a new list
                    containing the allowed channels for the sensor type filter (if
                    the item selected was not 'All') and regenerates the list of
                    displayed channels based on the new sensor type filter selection.
                """
                found = 0
                i = 0
                while i < len(self._sensor_types) and not found:
                    if text == self._sensor_types[i]:
                        found = 1
                    else:
                        i = i + 1

                if found:
#                    self._change_sensor_type(text)
                    self._switch_sensor_type(text)
                    self._listFilteredChannels()
                    return 1
                else:
                    return -1

        def _change_sensor_type(self, text):
                """ This routine applys the selected item in the sensor type combo box
                    to form a new list containing the allowed channels for the sensor type 
                    filter (if the item selected was not 'All').
                """

                if self._sensor_types.count(text) == 0:
                    return 0
                list_index = self._sensor_types.index(text)
#                print "list_index = ", list_index

                self._allowed_sensor_type_channels = []
                if text == 'All':
                    self._filtering_sensor_type_active = 0
                else:
                    """ The sensor type selected filter item was not 'All.'  Open the file
                        associated with the selected filter item that contains the names
                        of channels associated with the sensor type.  Then read each line
                        from the file, appending the read channel name line to this list
                        of allowed channels for the sensor type filter.
                    """
                    self._filtering_sensor_type_active = 1 

                    try:
                        filename = self._archive_file_dir + self._sensor_type_files[list_index]
                        f = open(filename, "r")
                    except:
                        self._form_allowed_channels_list()
                        return 0

                    while 1:
                        achannel = f.readline()
                        if achannel == '':
                            break

                        str_len = len(achannel)
                        self._allowed_sensor_type_channels.append(achannel[:str_len - 1])

#                    print "allowed_sensor_type_channels = ", self._allowed_sensor_type_channels
                    f.close()

                """ Call routine _form_allowed_channels_list to form a list of allowed
                    channels from the subsystem, sensor type, and station filters by
                    determining the "intersection" of allowed channels for each filter.
                """
                self._form_allowed_channels_list()
                return 1

        def _switch_sensor_type(self, text):
                """ This routine applys the selected item in the sensor type combo box
                    to form a new list containing the allowed channels for the sensor type 
                    filter (if the item selected was not 'All').
                """

                if self._sensor_types.count(text) == 0:
                    return 0
                list_index = self._sensor_types.index(text)
#                print "list_index = ", list_index

                self._allowed_sensor_type_channels = []
                if text == 'All':
                    self._filtering_sensor_type_active = 0
                else:
                    self._filtering_sensor_type_active = 1 

                    self._allowed_sensor_type_channels = self._sensor_type_channels[list_index]

                """ Call routine _form_allowed_channels_list to form a list of allowed
                    channels from the subsystem, sensor type, and station filters by
                    determining the "intersection" of allowed channels for each filter.
                """
                self._form_allowed_channels_list()
                return 1

        def _validate_stations(self, text):
                """ This routine validates any entry entered into the entry field
                    of the station filter combo box.  It makes sure that any entry
                    matches one of the items in the station combo box scrolled
                    list.  This routine is also called when the user selects an
                    item in the station filter scrolled list.  After validation,
                    this routine applys the selected item to form a new list
                    containing the allowed channels for the station filter (if
                    the item selected was not 'All') and regenerates the list of
                    displayed channels based on the new station filter selection.
                """
                found = 0
                i = 0
                while i < len(self._stations) and not found:
                    if text == self._stations[i]:
                        found = 1
                    else:
                        i = i + 1

                if found:
#                    self._change_stations(text)
                    self._switch_stations(text)
                    self._listFilteredChannels()
                    return 1
                else:
                    return -1

        def _change_stations(self, text):
                """ This routine applys the selected item in the station combo box
                    to form a new list containing the allowed channels for the station
                    filter (if the item selected was not 'All').
                """

                if self._stations.count(text) == 0:
                    return 0
                list_index = self._stations.index(text)
#                print "list_index = ", list_index

                self._allowed_station_channels = []
                if text == 'All':
                    self._filtering_station_active = 0
                else:
                    """ The station selected filter item was not 'All.'  Open the file
                        associated with the station filter item that contains the names
                        of channels associated with the station.  Then read each line
                        from the file, appending the read channel name line to this list
                        of allowed channels for the station filter.
                    """
                    self._filtering_station_active = 1 

                    try:
                        filename = self._archive_file_dir + self._station_files[list_index]
                        f = open(filename, "r")
                    except:
                        self._form_allowed_channels_list()
                        return 0

                    while 1:
                        achannel = f.readline()
                        if achannel == '':
                            break

                        str_len = len(achannel)
                        self._allowed_station_channels.append(achannel[:str_len - 1])

#                    print "allowed_station_channels = ", self._allowed_station_channels
                    f.close()

                """ Call routine _form_allowed_channels_list to form a list of allowed
                    channels from the subsystem, sensor type, and station filters by
                    determining the "intersection" of allowed channels for each filter.
                """
                self._form_allowed_channels_list()
                return 1

        def _switch_stations(self, text):
                """ This routine applys the selected item in the station combo box
                    to form a new list containing the allowed channels for the station
                    filter (if the item selected was not 'All').
                """

                if self._stations.count(text) == 0:
                    return 0
                list_index = self._stations.index(text)
#                print "list_index = ", list_index

                self._allowed_station_channels = []
                if text == 'All':
                    self._filtering_station_active = 0
                else:
                    self._filtering_station_active = 1 

                    self._allowed_station_channels = self._station_channels[list_index]

                """ Call routine _form_allowed_channels_list to form a list of allowed
                    channels from the subsystem, sensor type, and station filters by
                    determining the "intersection" of allowed channels for each filter.
                """
                self._form_allowed_channels_list()
                return 1

        def _form_allowed_channels_list (self):
                """ This routine forms a list of allowed channels to be displayed from the
                    archive channels by determining the "intersection" of allowed channels
                    from the subsystem, sensor type, and station filters.
                """

                """ If neither the subsystem, sensor type, or station filters are active (i.e.,
                    each filter selection is set to 'All'), indicate that the composite
                    filter is inactive.  In this case, there is no filtering.
                """ 
                if not self._filtering_subsystem_active and not self._filtering_sensor_type_active and not self._filtering_station_active:
                    self._filtering_active = 0
                    self._allowed_channels = []
                    return

                """ At least one of the subsystem, sensor type, or station filters are active
                    (i.e., at least one has an associated list of allowed channels for the filter).
                    Calculate the number of active filters and append each the name of each
                    allowed channel from those active filters to a composite list.  If the same
                    channel is allowed for more than one active filter setting, the composite
                    list will have more than one element having the same channel name.  Those
                    channel names that appear the same number of times in the composite list
                    as the total number of active filters are channels that are allowed by all
                    the filters and are appended to the list of allowed channels.
                """
                self._filtering_active = 1
                self._allowed_channels = []

                num_filters = self._filtering_subsystem_active + self._filtering_sensor_type_active + self._filtering_station_active
                all_filter_channels = []

                if self._filtering_subsystem_active:
                    for channel in self._allowed_subsystem_channels:
                        all_filter_channels.append(channel)

                if self._filtering_sensor_type_active:
                    for channel in self._allowed_sensor_type_channels:
                        all_filter_channels.append(channel)

                if self._filtering_station_active:
                    for channel in self._allowed_station_channels:
                        all_filter_channels.append(channel)

                for channel in all_filter_channels:
                    if all_filter_channels.count(channel) == num_filters:
                        if self._allowed_channels.count(channel) == 0:
                            self._allowed_channels.append(channel)

#                print "_allowed_channels = ", self._allowed_channels

	def _initCurValidChannels (self):
                Pmw.showbusycursor()

                self._cur_valid_channels = []
		channel = casi.channel()
                pattern = '' 
		self._archive.findChannelByPattern (pattern, channel)

		while channel.valid():
                    channel_name = channel.name()
                    self._cur_valid_channels.append(channel_name)
                    channel.next()

                Pmw.hidebusycursor()

	def _listFilteredChannels (self):
                if not self._activated:
                    return

		# Quicker and w/ "feedback": fill list name by name
		box = self._list.component('listbox')
		box.delete (0, END)

                for channel_name in self._cur_valid_channels:
                    if not self._filtering_active or channel_name in self._allowed_channels:
                        if self._include_or_exclude == "+" and channel_name in self._selected_channels:
			    box.insert (END, channel_name)

                        if self._include_or_exclude == "-" and channel_name not in self._selected_channels:
			    box.insert (END, channel_name)

		# then sort
		names = list(self._list.get ())
		names.sort ()
		self._list.setlist (names)

	def _listChannels (self):
                """ This routine inserts channel names into the channel scrolled list box.
                    The channel names that appear in this list:
                        1.  Must be archived.
                        2.  Must appear in the list of selected channels if the include_or_exclude
                            flag is "+" or must NOT be in the list of selected channels if the
                            include_or_exclude flag is "-".
                        3.  If filtering is active, the channel names must appear in the list of
                            allowed filter channels.
                        4.  If a regular expression is specified, the channel names must match
                            the regular expression.
                """
                Pmw.showbusycursor()

                self._cur_valid_channels = []
		channel = casi.channel()
		pattern = self._pattern.get()
		self._archive.findChannelByPattern (pattern, channel)
		# Quicker and w/ "feedback": fill list name by name
		box = self._list.component('listbox')
		box.delete (0, END)

#                t0 = time.clock()
#                i = 0
#                num_inserts = 0

		while channel.valid():
                    channel_name = channel.name()
                    self._cur_valid_channels.append(channel_name)

                    if not self._filtering_active or channel_name in self._allowed_channels:
                        if self._include_or_exclude == "+" and channel_name in self._selected_channels:
			    box.insert (END, channel_name)
#                            num_inserts =  num_inserts + 1

                        if self._include_or_exclude == "-" and channel_name not in self._selected_channels:
			    box.insert (END, channel_name)
#                            num_inserts = num_inserts + 1

                    channel.next()
#                    i = i + 1

#                dt = time.clock() - t0
#                print "channel name loop seconds = ", dt
#                print "number of channels = ", i
#                print "num_inserts = ", num_inserts 

		# then sort
		names = list(self._list.get ())
		names.sort ()
		self._list.setlist (names)

                Pmw.hidebusycursor()

	def activate (self):
                """ This routine causes channel names to be inserted into the channel
                    scrolled list box and, if the 'Ok' button was pressed in the Dialog
                    box, returns any selected channel name.
                """
                self._activated = 1
                self._listFilteredChannels ()
		if self._dlg.activate () == 'Ok':
			return self._list.getcurselection ()
		return ()

def usage():
	print "USAGE: " + sys.argv[0] + " archive {+,-} selected_channels [ channelPattern ]"
	sys.exit (1)

if __name__=="__main__":
        """ First, validate the number of arguments and save the arguments into
            variables.
        """ 
	argc=len(sys.argv)
	if argc < 3: usage ()
	archiveName = sys.argv[1]
        includeExclude = sys.argv[2] 
        selectedChannels = sys.argv[3]
	if argc==5:
		pattern=sys.argv[4]
	else:
		pattern=''

        """ Open the specified CASI archive and perform initialization prior to
            bringing up the dialog box.
        """ 
	archive = casi.archive ()
	if not archive.open(archiveName):
		print "Cannot open", archiveName
		sys.exit()
	root = Tk()
	Pmw.initialise()
	dlg = ChannelDialog (root, archive, includeExclude, selectedChannels, pattern)
	channels = dlg.activate()
#        for name in channels:
#            print name





