proc closeDialog {w {wait 1}} {
  if {([info command $w.tf.t] == "$w.tf.t") && ($::var($w,go) == 1)} {
    foreach fh $::var(xec,fh,$w.tf.t) {
      catch {close $fh}
    }
  }
  destroy $w
}
