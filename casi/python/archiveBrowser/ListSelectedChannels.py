#! /bin/env python
#
# ListSelectedChannels.py
#
# Adapted from ListChannels.py (written by Kay-Uwe Kasemir).
# Author: Bob Hall, 2/14/01 (rdh@slac.stanford.edu)
#
from Tkinter import *
import Pmw
import casi
import sys

class ChannelDialog:
	"""Interactively lists channel names from archive.
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
             5. pattern - optional regular expression pattern.

	   Returns tuple of selected channels or ()
	"""
	def __init__ (self, root, archive, include_or_exclude, selected_channels, pattern=''):
		"List all channel names that match the pattern"

                """ Initialize global variables from input variables.
                """
		self._archive = archive
                self._include_or_exclude = include_or_exclude
                self._selected_channels = selected_channels

                self._cur_valid_channels = []

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

	def _initCurValidChannels (self):
                Pmw.showbusycursor()

		channel = casi.channel()
		pattern = '' 
		self._archive.findChannelByPattern (pattern, channel)
		while channel.valid():
                    channel_name = channel.name()
                    self._cur_valid_channels.append(channel_name)
                    channel.next()

                Pmw.hidebusycursor()

	def _showChannels (self):
		# Quicker and w/ "feedback": fill list name by name
		box = self._list.component('listbox')
		box.delete (0, END)

                for channel_name in self._cur_valid_channels:
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
                        3.  If a regular expression is specified, the channel names must match
                            the regular expression.
                """
		channel = casi.channel()
		pattern = self._pattern.get()
		self._archive.findChannelByPattern (pattern, channel)
		# Quicker and w/ "feedback": fill list name by name
		box = self._list.component('listbox')
		box.delete (0, END)
		while channel.valid():
                        if self._include_or_exclude == "+" and channel.name() in self._selected_channels:
			    box.insert (END, channel.name())

                        if self._include_or_exclude == "-" and channel.name() not in self._selected_channels:
			    box.insert (END, channel.name())

			channel.next()
		# then sort
		names = list(self._list.get ())
		names.sort ()
		self._list.setlist (names)

	def activate (self):
                """ This routine causes channel names to be inserted into the channel
                    scrolled list box and, if the 'Ok' button was pressed in the Dialog
                    box, returns any selected channel name.
                """
		self._showChannels ()
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
	dlg = ChannelDialog (root, archive)
	channels = dlg.activate()
#       for name in channels:
#               print name





