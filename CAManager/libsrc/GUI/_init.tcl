option add *textBackground white widgetDefault
option add *borderWidth 1 widgetDefault
option add *buttonForeground black widgetDefault
option add *outline black widgetDefault
option add *weekdayBackground white widgetDefault
option add *weekendBackground mistyrose widgetDefault
option add *selectColor red widgetDefault

if {1} {
  option add *startDay sunday widgetDefault
  option add *days {Su Mo Tu We Th Fr Sa} widgetDefault
} else {
  option add *startDay monday widgetDefault
  option add *days {Mo Tu We Th Fr Sa Su} widgetDefault
}

set ::tl_cnt 0
set ::_port 4610
set ::dontCheckAtAll 0
set ::checkBgMan 1
set ::checkInt 10

namespace eval camGUI {
  variable aEngines
  variable row
  variable datetime
}

proc camGUI::init {} {
  variable aEngines
  variable row

  foreach pack {BWidget Tktable Iwidgets} {
    namespace inscope :: package require $pack
  } 
 
  option add *TitleFrame.l.font {helvetica 11 bold italic}
  
  wm withdraw .

  set ::selected(archive) {}
  set ::selected(misc) {}
  set ::selected(regex) {}
  if {"$::tcl_platform(platform)" == "unix"} {
    catch {source $camMisc::rcdir/settings}
  } else {
    registry set "$camMisc::reg_stem\\Settings"
    foreach v [registry values "$camMisc::reg_stem\\Settings"] {
      set ::$v [registry get "$camMisc::reg_stem\\Settings" $v]
    }
    foreach k [registry keys "$camMisc::reg_stem\\Settings"] {
      foreach v [registry values "$camMisc::reg_stem\\Settings\\$k"] {
	puts "  setting ::${k}($v) to [registry get $camMisc::reg_stem\\Settings\\$k $v]"
	set ::${k}($v) [registry get "$camMisc::reg_stem\\Settings\\$k" $v]
      }
    }
  }
}

source About.tcl
source Block.tcl
source Check.tcl
source Config.tcl
source Delete.tcl
source Edit.tcl
source Exit.tcl
source Export.tcl
source Info.tcl
source ModArchive.tcl
source New.tcl
source Open.tcl
source Prefs.tcl
source Save.tcl
source SaveSettings.tcl
source Start.tcl
source Stop.tcl
source Test.tcl
source actionDialog.tcl
source checkJob.tcl
source clearSel.tcl
source closeDialog.tcl
source getAInfo.tcl
source getCfgFile.tcl
source getPorts.tcl
source handleDialog.tcl
source initTable.tcl
source isArchive.tcl
source mainWindow.tcl
source mouseSelect.tcl
source pXec.tcl
source packTree.tcl
source scrollSel.tcl
source selFile.tcl
source setButtons.tcl
source setDT.tcl
source spinDay.tcl
source spinPorts.tcl
source switchOptions.tcl
source toggleBlock.tcl
source boxes.tcl

