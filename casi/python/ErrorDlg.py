# $Id$

import Pmw

class ErrorDlg:
    "One-line Error dialog, takes root and message"
    def __init__ (self, root, message):
        Pmw.MessageDialog (root,
                           buttons=('Oh, well...',), title="Error",
                           message_text=message).activate()
        
