package require Tclx
catch {eval exec $argv >@stdout 2>@stderr}
puts stderr TeRmInAtEd
exit
