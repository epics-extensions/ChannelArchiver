proc camGUI::mainWindow {} {
  variable aEngines
  variable row
  wm protocol . WM_DELETE_WINDOW "after 1 camGUI::Exit"
  set descmenu {
    "&File" all file 0 {
      {command "&New..." {} "New Configuration" {} -command {camGUI::aOpen Save}}
      {command "&Open..." {} "Open Configuration" {} -command camGUI::aOpen}
      {command "Open D&efault" {} "Open default Configuration" {} -command camGUI::aOpenDefault}
      {separator}
      {command "&Revert" {} "Revert to last saved status" {} -command camGUI::reOpen}
      {separator}
      {command "&Save" {} "Save Configuration" {} -command camGUI::aSave}
      {command "Save &as..." {} "Save Configuration as..." {} -command camGUI::aSaveAs}
      {command "Save as &Default" {} "Save Configuration as Default" {} -command camGUI::aSaveAsDefault}
      {separator}
      {command "E&xit" {} "Exit ArchiveManager" {} -command camGUI::Exit}
    }
    "&Edit" all edit 0 {
      {command "&Preferences" {} "Edit global preferences" {Ctrl p} -command {camGUI::aPrefs}}
    }
    "&Tools" all tools 0 {
      {command "&Info" {} "Info on Archive" {Ctrl i} -command {camGUI::aInfo .tf.t}}
      {command "&Test" {} "Test an Archive" {Ctrl t} -command {camGUI::aTest .tf.t}}
      {command "E&xport" {} "Export an Archive" {Ctrl x} -command {camGUI::aExport .tf.t}}
      {command "&Modify" {} "Copy/Delete Channels in an Archive" {Ctrl m} -command {camGUI::aModArchive .tf.t}}
    }
    "&Help" all help 0 {
      {command "&About" {} "Version Info" {Ctrl a} -command {camGUI::aAbout}}
    }
  }
  set ::busyIndicator ""
  set mainframe [MainFrame .mainframe -menu $descmenu -textvariable ::status ]
  $mainframe addindicator -textvariable camGUI::datetime
  $mainframe addindicator -textvariable camMisc::cfg_file_tail
  $mainframe addindicator -textvariable ::busyIndicator -width 2 -foreground blue
  pack $mainframe -side bottom -fill both -expand no

  set f [frame .tf -bd 0]

  scrollbar $f.sy -command "$f.t yview"
  pack $f.sy -side right -fill y

  set row [expr max([llength [camMisc::arcIdx]]+2, 8)]

  set table [table $f.t -rows $row -cols 5 -titlerows 1 \
		 -rowheight -22 \
		 -colstretchmode all -rowstretchmode none -roworigin -1 \
		 -yscrollcommand "$f.sy set" -flashmode 1 \
		 -borderwidth 1 -state disabled -selecttype row]
  pack $table -side right -expand t -fill both
  frame $table.f -bd 0
  frame $table.f.b -bd 1 -relief sunken
  pack $table.f.b -side bottom -expand t -fill both
  $table spans 0,4 $row,1
  $table window config 0,4 -sticky news -window $table.f


  initTable $table
  $table config -variable camGUI::aEngines 

  set brel raised
  set f [frame .bf -bd 0]
  Button $f.start -text Start -bd 1 -relief $brel \
      -command "camGUI::aStart $table" -state disabled \
      -helptype variable -helpvar ::status \
      -helptext "start the selected Archiver"
  Button $f.stop -text Stop -bd 1 -relief $brel \
      -command "camGUI::aStop $table" -state disabled \
      -helptype variable -helpvar ::status \
      -helptext "stop the selected Archiver (may restart if not blocked)"
  Button $f.new -text New -bd 1 -relief $brel \
      -command "camGUI::aNew $table" \
      -helptype variable -helpvar ::status \
      -helptext "create a new Archiver"
  Button $f.delete -text Delete -bd 1 -relief $brel \
      -command "camGUI::aDelete $table" -state disabled \
      -helptype variable -helpvar ::status \
      -helptext "delete the selected Archiver (without stopping it))"
  Button $f.edit -text Edit -bd 1 -relief $brel \
      -command "camGUI::aEdit $table" -state disabled \
      -helptype variable -helpvar ::status \
      -helptext "edit properties of selected Archiver (restricted if Archiver is runnung)"
  Button $f.check -text Check -bd 1 -relief $brel \
      -command "camGUI::aCheck $table" \
      -helptype variable -helpvar ::status \
      -helptext "manually check which Archivers are running"
  ArrowButton $f.up -type button -dir top -bd 1 \
      -command "camGUI::aSwap up $table" \
      -helptype variable -helpvar ::status \
      -helptext "move selected Archiver up in list" \
      -width 25 -height 25 -ipadx 4 -ipady 4
  ArrowButton $f.down -type button -dir bottom -bd 1 \
      -command "camGUI::aSwap down $table" \
      -helptype variable -helpvar ::status \
      -helptext "move selected Archiver down in list" \
      -width 25 -height 25 -ipadx 4 -ipady 4

  pack $f.delete $f.new $f.edit $f.stop $f.start $f.check $f.down $f.up -padx 8 -pady 8 -side right

#  event add <<B1>> <1>
#  event add <<B1>> <Button1-Motion>
  bind $table <<B1>> {
    mouseSelect %W %x %y
    break
  }

  bind $table <3> {
    if {[mouseSelect %W %x %y]} {
      catch {destroy %W.m}
      menu %W.m -tearoff 0
      %W.m add command -label Edit -command {camGUI::aEdit %W}
      %W.m add separator
      %W.m add command -label "Check if running" -command {camGUI::aCheck %W}
      %W.m add command -label "View/Edit configuration" -command {camGUI::aConfig %W}
      %W.m add separator
      %W.m add command -label Delete -command {camGUI::aDelete %W}
      after 1 {tk_popup %W.m %X %Y}
    }
    break
  }
  bind $table <Button3-Motion> {break}

  bind $table <Control-i> { camGUI::aInfo %W; break }
  bind $table <Control-t> { camGUI::aTest %W; break }
  bind $table <Control-x> { camGUI::aExport %W; break }
  bind $table <ButtonRelease-1> { camGUI::setButtons %W }
  bind $table <Up> { scrollSel %W -1 ; break }
  bind $table <Down> { scrollSel %W 1 ; break }
  bind $table <Right> { break }
  bind $table <Left> { break }

  pack .bf -side bottom -fill x
  pack .tf -side top -expand t -fill both

  bind $table <Double-Button-1> "$f.edit invoke"
  bind $table <Delete> "$f.delete invoke"

  BWidget::place . 0 0 center
  wm deiconify .
  raise .
  focus -force .
  wm geom . [wm geom .]
  setButtons $table
}
