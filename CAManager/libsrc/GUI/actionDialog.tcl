proc actionDialog {title} {
  incr ::tl_cnt
  set f [toplevel .t$::tl_cnt]
  wm protocol $f WM_DELETE_WINDOW "after 1 $f.bb.cancel invoke"
  wm title $f $title

  camGUI::packTree $f {
    {frame bb {-bd 0} {-side bottom -fill x} {
      {button go {-text Go -command {set ::var(%P,go) 1}} {-side right -padx 4 -pady 4}}
      {button cancel {-text Cancel -command {set ::var(%P,go) 0}} {-side right -padx 4 -pady 4}}
    }}
    {frame tf {-bd 0} {-side bottom -expand t -fill both -padx 4 -pady 4} {
      {scrollbar h {-orient horiz -command {%p.t xview}} {-side bottom -fill x}}
      {scrollbar v {-orient vert -command {%p.t yview}} {-side right -fill y}}
      {text t {-bg white -state disabled -wrap none -width 80 -height 15 -xscrollcommand {%p.h set}  -yscrollcommand {%p.v set}} {-fill both -expand t} {} ::w(%P,txt)}
    }}
  }

#  frame $f.bb -bd 0
#  button $f.bb.go -text Go -command "set ::var($f,go) 1"
#  button $f.bb.cancel -text Cancel -command "set ::var($f,go) 0"
#  pack $f.bb.go $f.bb.cancel -side right -padx 4 -pady 4
#  pack $f.bb -side bottom -fill x

#  frame $f.tf -bd 0
#  text $f.tf.t -bg white -state disabled -wrap none -width 80 -height 15 -xscrollcommand "$f.tf.h set" -yscrollcommand "$f.tf.v set"
  $f.tf.t tag add error end; $f.tf.t tag configure error -foreground red
  $f.tf.t tag add command end; $f.tf.t tag configure command -foreground blue
  $f.tf.t tag add normal end;
#  scrollbar $f.tf.h -orient horiz -command "$f.tf.t xview"
#  scrollbar $f.tf.v -orient vert -command "$f.tf.t yview"
  #     grid $f.tf.t $f.tf.v -sticky wens
  #     grid $f.tf.h -sticky wens
  #     grid columnconfigure $f 1 -weight 10
  #     grid rowconfigure $f 1 -weight 10
#  pack $f.tf.h -side bottom -fill x
#  pack $f.tf.v -side right -fill y
#  pack $f.tf.t -fill both -expand t
#  pack $f.tf -side bottom -expand t -fill both -padx 4 -pady 4
  return $f
}
