function compare()
{
    name=$1
    info=$2
    diff -q test/${name}.OK test/${name}
    if [ $? -eq 0 ]
    then
        echo "OK : $info"
    else
        echo "FAILED : $info. Check test/${name}.OK against test/${name}"
    fi
}

ArchiveExport index -l >test/names
compare "names" "Name List"

ArchiveExport index -i >test/name_info
compare "names" "Name List with info"

ArchiveExport index BoolPV >test/bool
compare "bool" "Bool PV Dump"

ArchiveExport index janet -t >test/janet
compare "janet" "Double PV Dump"

ArchiveExport index janet -t -s "03/23/2004 10:49:20" >test/janet_start
compare "janet_start" "Double PV Dump with 'start'"

ArchiveExport index janet -t -e "03/23/2004 10:48:37" >test/janet_end
compare "janet_end" "Double PV Dump with 'end'"

ArchiveExport index alan -t >test/alan
compare "alan" "Empty Array PV Dump"

ArchiveExport index ExampleArray -s "03/05/2004 19:50:35" -text >test/array
compare "array" "Array PV Dump"
