proc httpd::connect {fd addr port} {
  variable _query
  fconfigure $fd -blocking 0
  set _query($fd) {}
  fileevent $fd readable "httpd::getInput $fd"
}
