proc httpd::Busy {ind msg} {
  camMisc::arcSet $ind busy $msg
  after 5000 "camMisc::arcSet $ind busy \"\""
}
