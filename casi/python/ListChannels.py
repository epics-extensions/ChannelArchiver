# --------------------------------------------------------
# $Id$
#
# Please refer to NOTICE.txt,
# included as part of this distribution,
# for legal information.
#
# Kay-Uwe Kasemir, kasemir@lanl.gov
# --------------------------------------------------------

from Tkinter import *
import Pmw
import casi
import sys

class ChannelDialog:
	"""Interactively list channel names from archive.
	   Has to be called with
	   * Tk root widget, Pmw initialized,
	   * open casi.archive
	   * optional regular expression pattern

	   Returns tuple of selected channels or ()
	"""
	def __init__ (self, root, archive, pattern=''):
		"List all channel names that match the pattern"

		self._dlg = Pmw.Dialog (root, buttons=('Ok', 'Cancel'),
					title='Channel List')
		root = self._dlg.interior()

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
		self._archive = archive

	def _listChannels (self):
		channel = casi.channel()
		pattern = self._pattern.get()
		self._archive.findChannelByPattern (pattern, channel)
		# Quicker and w/ "feedback": fill list name by name
		box = self._list.component('listbox')
		box.delete (0, END)
		while channel.valid():
			box.insert (END, channel.name())
			channel.next()
		# then sort
		names = list(self._list.get ())
		names.sort ()
		self._list.setlist (names)

	def activate (self):
		self._listChannels ()
		if self._dlg.activate () == 'Ok':
			return self._list.getcurselection ()
		return ()

def usage():
	print "USAGE: " + sys.argv[0] + " archive [ channelPattern ]"
	sys.exit (1)

if __name__=="__main__":
	argc=len(sys.argv)
	if argc < 2: usage ()
	archiveName = sys.argv[1]
	if argc==3:
		pattern=sys.argv[2]
	else:
		pattern=''
		
	archive = casi.archive ()
	if not archive.open(archiveName):
		print "Cannot open", archiveName
		sys.exit()
	root = Tk()
	Pmw.initialise()
	dlg = ChannelDialog (root, archive)
	channels = dlg.activate()
	for name in channels:
		print name





