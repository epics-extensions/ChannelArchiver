# $Id$

import Pmw

class ErrorDlg:
    "One-line Error dialog, takes root and message"
    def __init__ (self, root, message):
        Pmw.MessageDialog (root,
                           buttons=('Oh, well...',), title="Error",
                           message_text=message).activate()
        



if __name__ == "__main__":
    import Tkinter
    root = Tkinter.Tk()
    Pmw.initialise()
    ErrorDlg (root, "Test....")
