proc camMisc::recCopyCfg {file dir} {
  set sdir [file dirname $file]
  file copy -force $file $dir/
  for_file line $file {
    if [regexp "^!group\[ 	\]*(.*)\[ 	\]*$" $line all nfile] {
      recCopyCfg $sdir/$nfile $dir
    }
  }
}
