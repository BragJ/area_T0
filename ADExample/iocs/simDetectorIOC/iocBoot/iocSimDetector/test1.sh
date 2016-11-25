caput 13SIM1:cam1:SizeX 10
caput 13SIM1:cam1:SizeY 10
caput 13SIM1:cam1:AcquireTime .001
caput 13SIM1:cam1:AcquirePeriod 0.1
caput 13SIM1:cam1:ImageMode "Continuous"
caput 13SIM1:cam1:ArrayCallbacks "Enable"
caput -S 13SIM1:HDF1:NDAttributesFile "/opt/egcs/epics/modules/areaDetector/ADExample/iocs/simDetectorIOC/iocBoot/iocSimDetector/simDetectorAttributes.xml"
caput 13SIM1:cam1:Acquire 0
caput 13SIM1:cam1:EnableCallbacks Enable
caput -S 13SIM1:HDF1:FilePath "/opt/egcs/epics/modules/areaDetector/ADExample/iocs/simDetectorIOC/iocBoot/iocSimDetector/7_19_data"
caput -S 13SIM1:HDF1:FileName "GDDP_monitor_date"
caput -S 13SIM1:HDF1:FileTemplate "%s%s_%3.3d.h5"
caput -S 13SIM1:HDF1:XMLFileName "hdf5_layout_demo.xml"
caput 13SIM1:HDF1:AutoIncrement 1
caput 13SIM1:HDF1:FileNumber 1
caput 13SIM1:HDF1:FileWriteMode "Stream"
caput 13SIM1:HDF1:NumCapture "10"
caput 13SIM1:HDF1:Capture 0
sleep 5
h5dump k.h5 > test1_001.txt

