
compare test_data1.dot test_data1.dot.OK "RTree test_data1"
compare update_data.dot update_data.dot.OK "RTree update_data"

O.$EPICS_HOST_ARCH/ReadTest ../DemoData/index fred >test/fred
compare fred.OK fred "ReadTest"

O.$EPICS_HOST_ARCH/ReadTest ../DemoData/index alan >test/alan
compare alan.OK alan "Dump of PV alan"

O.$EPICS_HOST_ARCH/ReadTest ../DemoData/index BoolPV >test/BoolPV
compare BoolPV.OK BoolPV "BoolPV"

# Comparison of last two updates:
#dot -Tpng -o0.png update0.dot
#dot -Tpng -o1.png update1.dot
#pngtopnm 0.png >0.pnm
#pngtopnm 1.png >1.pnm
#pnmcat -tb 0.pnm 1.pnm >u.pnm
#rm 0.png 1.png 0.pnm 1.pnm
#eog u.pnm

