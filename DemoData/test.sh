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
