proc toggleBlock {row} {
  if {$camGUI::aEngines($row,$::iBlocked)} {
    camMisc::Block $row camGUI::aEngines($row,$::iBlocked)
  } else {
    camMisc::Release $row camGUI::aEngines($row,$::iBlocked)
  }
}
