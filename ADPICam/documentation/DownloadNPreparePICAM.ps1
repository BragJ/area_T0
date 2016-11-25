"Create Directories"
mkdir c:\PICAM
mkdir C:\PICAM\Downloads
#Download modules
"----------------"
"Download tar files"
"downloading base ..."
powershell -command "& { (New-Object Net.Webclient).DownloadFile('http://www.aps.anl.gov/epics/download/base/baseR3.14.12.4.tar.gz', 'C:\PICAM\Downloads\baseR3.14.12.4.tar.gz' ) }"
"downloading synApps"
powershell -command "& { (New-Object Net.Webclient).DownloadFile('http://www.aps.anl.gov/bcda/synApps/tar/synApps_5_7.tar.gz', 'C:\PICAM\Downloads\synApps_5_7.tar.gz' ) }"
"downloading asyn update"
powershell -command "& { (New-Object Net.Webclient).DownloadFile('http://www.aps.anl.gov/epics/download/modules/asyn4-25.tar.gz', 'C:\PICAM\Downloads\asyn4-25.tar.gz' ) }"
"downloading autosave update"
powershell -command "& { (New-Object Net.Webclient).DownloadFile('http://www.aps.anl.gov/bcda/synApps/tar/autosave_R5-5.tar.gz', 'C:\PICAM\Downloads\autosave_R5-5.tar.gz' ) }"
"downloading calc update"
powershell -command "& { (New-Object Net.Webclient).DownloadFile('http://www.aps.anl.gov/bcda/synApps/tar/calc_R3-4-2.tar.gz', 'C:\PICAM\Downloads\calc_R3-4-2.tar.gz' ) }"
"downloading busy update"
powershell -command "& { (New-Object Net.Webclient).DownloadFile('http://www.aps.anl.gov/bcda/synApps/tar/busy_R1-6-1.tar.gz', 'C:\PICAM\Downloads\busy_R1-6-1.tar.gz' ) }"
"downloading sscan update"
powershell -command "& { (New-Object Net.Webclient).DownloadFile('http://www.aps.anl.gov/bcda/synApps/tar/sscan_R2-10.tar.gz', 'C:\PICAM\Downloads\sscan_R2-10.tar.gz' ) }"
"downloading seq update"
powershell -command "& { (New-Object Net.Webclient).DownloadFile('http://www-csr.bessy.de/control/SoftDist/sequencer/releases/seq-2.1.18.tar.gz', 'C:\PICAM\Downloads\seq-2-1-18.tar.gz' ) }"
"downloading mkmf patch"
powershell -command "& { (New-Object Net.Webclient).DownloadFile('http://www.aps.anl.gov/epics/base/R3-14/12-docs/mkmf-includes.patch', 'C:\PICAM\Downloads\mkmf-includes.patch' ) }"
"downloading inc-defs patch"
powershell -command "& { (New-Object Net.Webclient).DownloadFile('http://www.aps.anl.gov/epics/base/R3-14/12-docs/inc-deps.patch', 'C:\PICAM\Downloads\inc-deps.patch' ) }"

$env:Path = $env:Path + ";C:\GnuWin32\bin"

cd \PICAM
"----------------"
"uncompress modules"
"uncompressing base ..."
gzip -d C:\PICAM\Downloads\baseR3.14.12.4.tar.gz
"uncompressing synApps ..."
gzip -d C:\PICAM\Downloads\synApps_5_7.tar.gz
"uncompressing asyn ..."
gzip -d C:\PICAM\Downloads\asyn4-25.tar.gz
"uncompressing autosave ..."
gzip -d C:\PICAM\Downloads\autosave_R5-5.tar.gz
"uncompressing calc ..."
gzip -d C:\PICAM\Downloads\calc_R3-4-2.tar.gz
"uncompressing busy ..."
gzip -d C:\PICAM\Downloads\busy_R1-6-1.tar.gz
"uncompressing sscan ..."
gzip -d C:\PICAM\Downloads\sscan_R2-10.tar.gz
"uncompressing sequencer ..."
gzip -d C:\PICAM\Downloads\seq-2-1-18.tar.gz

"----------------"
"untar source files"
"untar base ..."
bsdtar -xf C:\PICAM\Downloads\baseR3.14.12.4.tar
"untar synApps ..."
bsdtar -xf C:\PICAM\Downloads\synApps_5_7.tar
cd synApps_5_7\support
"untar asyn ..."
bsdtar -xf C:\PICAM\Downloads\asyn4-25.tar
move C:/PICAM/synApps_5_7/support/asyn4-25 C:/PICAM/synApps_5_7/support/asyn-4-25
"untar autosave ..."
bsdtar -xf C:\PICAM\Downloads\autosave_R5-5.tar
"untar calc ..."
bsdtar -xf C:\PICAM\Downloads\calc_R3-4-2.tar
"untar busy ..."
bsdtar -xf C:\PICAM\Downloads\busy_R1-6-1.tar
"untar sscan ..."
bsdtar -xf C:\PICAM\Downloads\sscan_R2-10.tar
"untar sequencer ..."
bsdtar -xf C:\PICAM\Downloads\seq-2-1-18.tar
move C:\PICAM\synApps_5_7\support\seq-2.1.18 C:\PICAM\synApps_5_7\support\seq-2-1-18
cd ../..
"----------------"
"Patch Base"
cd base-3.14.12.4
Get-Content C:\PICAM\Downloads\mkmf-includes.patch | patch -p0 
Get-Content C:\PICAM\Downloads\inc-deps.patch | patch -p0
cd ..
cd synApps_5_7/support
"----------------"
" Append Visual Studio 2013"
sed.exe -e '/ Visual / s/^/REM /' -i \PICAM\base-3.14.12.4\startup\win32.bat
sed.exe -e '79 a\REM ------ Visual Studio 2013------/' -i \PICAM\base-3.14.12.4\startup\win32.bat
sed.exe -e '80 a\call \"C:\\Program files \(x86\)\\Microsoft Visual Studio 12\.0\\VC\\vcvarsall\.bat\" x64' -i \PICAM\base-3.14.12.4\startup\win32.bat
sed.exe -e 's/win32-x86/windows-x64/' -i C:\PICAM\base-3.14.12.4\startup\win32.bat
sed.exe -e 's/Perl/Perl64/' -i C:\PICAM\base-3.14.12.4\startup\win32.bat
"Add in debug cross-compile"
sed.exe -e '/^CROSS_COMPILER_TARGET_ARCHS/ s/$/windows-x64-debug/' -i C:/PICAM/base-3.14.12.4/configure/CONFIG_SITE
sed.exe -e 's/SHARED_LIBRARIES=YES/SHARED_LIBRARIES=NO/' -i C:/PICAM/base-3.14.12.4/configure/CONFIG_SITE
sed.exe -e 's/STATIC_BUILD=NO/STATIC_BUILD=YES/' -i C:/PICAM/base-3.14.12.4/configure/CONFIG_SITE
copy C:/PICAM/base-3.14.12.4/configure/os/CONFIG.windows-x64-debug.windows-x64-debug C:/PICAM/base-3.14.12.4/configure/os/CONFIG.windows-x64.windows-x64-debug
sed.exe -e '/HOST_OPT/ a\OPT_LDFLAGS += \/debug' -i  C:/PICAM/base-3.14.12.4/configure/os/CONFIG.windows-x64.windows-x64-debug
sed.exe -e '/HOST_OPT/ a\OPT_CXXFLAGS += \/Zi' -i  C:/PICAM/base-3.14.12.4/configure/os/CONFIG.windows-x64.windows-x64-debug
sed.exe -e '/HOST_OPT/ a\USR_LDFLAGS += \/debug' -i  C:/PICAM/base-3.14.12.4/configure/os/CONFIG.windows-x64.windows-x64-debug
sed.exe -e '/HOST_OPT/ a\USR_CXXFLAGS += \/Zi' -i  C:/PICAM/base-3.14.12.4/configure/os/CONFIG.windows-x64.windows-x64-debug

"----------------"
"Get Areadetector from gitHub"
& 'C:\Program Files (x86)\Git\bin\git.exe' clone https://github.com/areaDetector/areaDetector.git areaDetector-2-x-git
cd areaDetector-2-x-git
"----------------"
"Get areaDetector submodules from github"
"Get ADCore"
& 'C:\Program Files (x86)\Git\bin\git.exe' submodule update --init --recursive ADCore
"Get ADBinaries"
& 'C:\Program Files (x86)\Git\bin\git.exe' submodule update --init  --recursive ADBinaries
"Setting 2-1 for ADCore"
cd ADCore
& 'C:\Program Files (x86)\Git\bin\git.exe' checkout -b R2-1
cd ..
"Setting 2-1 for ADBinaries"
cd ADBinaries
& 'C:\Program Files (x86)\Git\bin\git.exe' checkout -b R2-1
cd ..
"Setting 2-0 for areaDetector"
& 'C:\Program Files (x86)\Git\bin\git.exe' checkout -b R2-0
"Get ADPICAM"
& 'C:\Program Files (x86)\Git\bin\git.exe' clone -b "code_review_2015_01_28" --recursive https://github.com/JPHammonds/ADPICAM
cd ..

#Comment out unused synApps modules
"----------------"
"Comment unused synAppsModules"
sed.exe -e '/ALLEN_BRADLEY/ s/^/#/' -i \PICAM\synApps_5_7\support\configure\RELEASE
sed.exe -e '/CAMAC/ s/^/#/' -i \PICAM\synApps_5_7\support\configure\RELEASE
sed.exe -e '/DELAYGEN/ s/^/#/' -i \PICAM\synApps_5_7\support\configure\RELEASE
sed.exe -e '/DEVIOCSTATS/ s/^/#/' -i \PICAM\synApps_5_7\support\configure\RELEASE
sed.exe -e '/DXP/ s/^/#/' -i \PICAM\synApps_5_7\support\configure\RELEASE
sed.exe -e '/IP330/ s/^/#/' -i \PICAM\synApps_5_7\support\configure\RELEASE
# sed.exe -e '/IPAC/ s/^/#/' -i \PICAM\synApps_5_7\support\configure\RELEASE
sed.exe -e '/IPUNIDIG/ s/^/#/' -i \PICAM\synApps_5_7\support\configure\RELEASE
sed.exe -e '/LOVE/ s/^/#/' -i \PICAM\synApps_5_7\support\configure\RELEASE
sed.exe -e '/MEASCOMP/ s/^/#/' -i \PICAM\synApps_5_7\support\configure\RELEASE
sed.exe -e '/MCA/ s/^/#/' -i \PICAM\synApps_5_7\support\configure\RELEASE
sed.exe -e '/MODBUS/ s/^/#/' -i \PICAM\synApps_5_7\support\configure\RELEASE
sed.exe -e '/MOTOR/ s/^/#/' -i \PICAM\synApps_5_7\support\configure\RELEASE
sed.exe -e '/OPTICS/ s/^/#/' -i \PICAM\synApps_5_7\support\configure\RELEASE
sed.exe -e '/QUADEM/ s/^/#/' -i \PICAM\synApps_5_7\support\configure\RELEASE
sed.exe -e '/SOFTGLUE/ s/^/#/' -i \PICAM\synApps_5_7\support\configure\RELEASE
sed.exe -e '/STREAM/ s/^/#/' -i \PICAM\synApps_5_7\support\configure\RELEASE
sed.exe -e '/VAC/ s/^/#/' -i \PICAM\synApps_5_7\support\configure\RELEASE
sed.exe -e '/VME/ s/^/#/' -i \PICAM\synApps_5_7\support\configure\RELEASE
sed.exe -e '/XXX/ s/^/#/' -i \PICAM\synApps_5_7\support\configure\RELEASE
sed.exe -e '/STD/ s/^/#/' -i \PICAM\synApps_5_7\support\configure\RELEASE
sed.exe -e 's/asyn-4-21/asyn-4-25/' -i \PICAM\synApps_5_7\support\configure\RELEASE
sed.exe -e 's/calc-3-2/calc-3-4-2/' -i \PICAM\synApps_5_7\support\configure\RELEASE
sed.exe -e 's/EPICS_BASE=\/APSshare\/epics\/base-3.14.12.3/EPICS_BASE=C:\/PICAM\/base-3.14.12.4/' -i \PICAM\synApps_5_7\support\configure\RELEASE
sed.exe -e 's/SUPPORT=\/APSshare\/epics\/synApps_5_7\/support/SUPPORT=C:\/PICAM\/synApps_5_7\/support/' -i \PICAM\synApps_5_7\support\configure\RELEASE
sed.exe -e 's/areaDetector-1-9-1/areaDetector-2-x-git/' -i \PICAM\synApps_5_7\support\configure\RELEASE
sed.exe -e 's/autosave-5-1/autosave-5-5/' -i \PICAM\synApps_5_7\support\configure\RELEASE
sed.exe -e 's/busy-1-6/busy-1-6-1/' -i \PICAM\synApps_5_7\support\configure\RELEASE
sed.exe -e 's/sscan-2-9/sscan-2-10/' -i \PICAM\synApps_5_7\support\configure\RELEASE
sed.exe -e 's/seq-2-1-13/seq-2-1-18/' -i \PICAM\synApps_5_7\support\configure\RELEASE
sed.exe -e '/ipac-2-12/ s/#//' -i \PICAM\synApps_5_7\support\configure\RELEASE
sed.exe -e '/SNCSEQ/ s/^/#/' -i \PICAM\synApps_5_7\support\configure\RELEASE
sed.exe -e '/SNCSEQ/ s/^/#/' -i C:\PICAM\synApps_5_7\support\sscan-2-10\configure\RELEASE
sed.exe -e '/SNCSEQ/ s/^/#/' -i C:\PICAM\synApps_5_7\support\asyn-4-25\configure\RELEASE
sed.exe -e '/SNCSEQ/ s/^/#/' -i C:\PICAM\synApps_5_7\support\calc-3-4-2\configure\RELEASE
sed.exe -e '/DAC128V/ s/^/#/' -i \PICAM\synApps_5_7\support\configure\RELEASE
sed.exe -e '/IP=/ s/^/#/' -i \PICAM\synApps_5_7\support\configure\RELEASE

# Copy template config files
"----------------"
"Copy areaDetector Template config files"
copy \PICAM\synApps_5_7\support\areaDetector-2-x-git\configure\EXAMPLE_RELEASE_LIBS.local \PICAM\synApps_5_7\support\areaDetector-2-x-git\configure\RELEASE_LIBS.local
copy \PICAM\synApps_5_7\support\areaDetector-2-x-git\configure\EXAMPLE_RELEASE_PATHS.local \PICAM\synApps_5_7\support\areaDetector-2-x-git\configure\RELEASE_PATHS.local
copy \PICAM\synApps_5_7\support\areaDetector-2-x-git\configure\EXAMPLE_RELEASE_PRODS.local \PICAM\synApps_5_7\support\areaDetector-2-x-git\configure\RELEASE_PRODS.local

#Modify copied Template config files
"----------------"
"Modify AreaDetector Template ConfigFile RELEASE_LIBS.local"
sed.exe -e 's/asyn-4-24/asyn-4-25/' -i \PICAM\synApps_5_7\support\areaDetector-2-x-git\configure\RELEASE_LIBS.local
"Modify AreaDetector Template ConfigFile RELEASE_PATHS.local"
sed.exe -e 's/\/corvette\/home\/epics\/devel/C:\/PICAM\/synApps_5_7\/support/' -i \PICAM\synApps_5_7\support\areaDetector-2-x-git\configure\RELEASE_PATHS.local
sed.exe -e 's/\/corvette\/usr\/local\/epics/C:\/PICAM/' -i \PICAM\synApps_5_7\support\areaDetector-2-x-git\configure\RELEASE_PATHS.local
sed.exe -e 's/areaDetector-2-1/areaDetector-2-x-git/' -i \PICAM\synApps_5_7\support\areaDetector-2-x-git\configure\RELEASE_PATHS.local
sed.exe -e 's/base-3.14.12.3/base-3.14.12.4/' -i \PICAM\synApps_5_7\support\areaDetector-2-x-git\configure\RELEASE_PATHS.local
"Modify AreaDetector Template ConfigFile RELEASE_PRODS.local"
sed.exe -e 's/autosave-5-4-2/autosave-5-5/' -i \PICAM\synApps_5_7\support\areaDetector-2-x-git\configure\RELEASE_PRODS.local
sed.exe -e 's/busy-1-6/busy-1-6-1/' -i \PICAM\synApps_5_7\support\areaDetector-2-x-git\configure\RELEASE_PRODS.local
sed.exe -e 's/sscan-2-9/sscan-2-10/' -i \PICAM\synApps_5_7\support\areaDetector-2-x-git\configure\RELEASE_PRODS.local
sed.exe -e 's/calc-3-4-1/calc-3-4-2/' -i \PICAM\synApps_5_7\support\areaDetector-2-x-git\configure\RELEASE_PRODS.local
sed.exe -e 's/areaDetector_2_git/areaDetector-2-x-git/' -i C:\PICAM\synApps_5_7\support\areaDetector-2-x-git\ADPICAM\iocs\PICamIOC\iocBoot\iocPICam\start_epics-displays.bat
"----------------"
"Modify areadetector Make file to remove non used detectors and add PICAM"
sed.exe -e '/ADPerkinElmer_DEPEND/ a\\nDIRS := \$\(DIRS\) ADPICAM\nADPICAM_DEPEND_DIRS += ADCore' -i \PICAM\synApps_5_7\support\areaDetector-2-x-git\Makefile
sed.exe -e '/ADADSC/ s/^/#/' -i \PICAM\synApps_5_7\support\areaDetector-2-x-git\Makefile
sed.exe -e '/ADAndor/ s/^/#/' -i \PICAM\synApps_5_7\support\areaDetector-2-x-git\Makefile
sed.exe -e '/ADAndor3/ s/^/#/' -i \PICAM\synApps_5_7\support\areaDetector-2-x-git\Makefile
sed.exe -e '/ADBruker/ s/^/#/' -i \PICAM\synApps_5_7\support\areaDetector-2-x-git\Makefile
sed.exe -e '/ADFireWireWin/ s/^/#/' -i \PICAM\synApps_5_7\support\areaDetector-2-x-git\Makefile
sed.exe -e '/ADLightField/ s/^/#/' -i \PICAM\synApps_5_7\support\areaDetector-2-x-git\Makefile
sed.exe -e '/ADPSL/ s/^/#/' -i \PICAM\synApps_5_7\support\areaDetector-2-x-git\Makefile
sed.exe -e '/ADPerkinElmer/ s/^/#/' -i \PICAM\synApps_5_7\support\areaDetector-2-x-git\Makefile
sed.exe -e '/ADPilatus/ s/^/#/' -i \PICAM\synApps_5_7\support\areaDetector-2-x-git\Makefile
sed.exe -e '/ADPixirad/ s/^/#/' -i \PICAM\synApps_5_7\support\areaDetector-2-x-git\Makefile
sed.exe -e '/ADPointGrey/ s/^/#/' -i \PICAM\synApps_5_7\support\areaDetector-2-x-git\Makefile
sed.exe -e '/ADProsilica/ s/^/#/' -i \PICAM\synApps_5_7\support\areaDetector-2-x-git\Makefile
sed.exe -e '/ADPvCam/ s/^/#/' -i \PICAM\synApps_5_7\support\areaDetector-2-x-git\Makefile
sed.exe -e '/ADURL/ s/^/#/' -i \PICAM\synApps_5_7\support\areaDetector-2-x-git\Makefile
sed.exe -e '/ADmarCCD/ s/^/#/' -i \PICAM\synApps_5_7\support\areaDetector-2-x-git\Makefile
sed.exe -e '/ADmar345/ s/^/#/' -i \PICAM\synApps_5_7\support\areaDetector-2-x-git\Makefile

mkdir C:\PICAM\synApps_5_7\support\areaDetector-2-x-git\ADPICAM\iocs\PICamIOC\iocBoot\iocPICam\autosave