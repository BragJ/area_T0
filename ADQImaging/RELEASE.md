Qimaging Releases
=================

The latest untagged master branch can be obtained at
https://github.com/areaDetector/ADQImaging.

Tagged source code and pre-built binary releases prior to R2-0 are included
in the areaDetector releases available via links at
http://cars.uchicago.edu/software/epics/areaDetector.html.

The versions of EPICS base, asyn, and other synApps modules used for each release can be obtained from 
the EXAMPLE_RELEASE_PATHS.local, EXAMPLE_RELEASE_LIBS.local, and EXAMPLE_RELEASE_PRODS.local
files respectively, in the configure/ directory of the appropriate release of the 
[top-level areaDetector](https://github.com/areaDetector/areaDetector) repository.


Compile Note
=============
The QImaging SDK (http://www.qimaging.com/support/downloads/sdk.php) is required to compile.
-From C:\Program Files\QImaging\SDK\Headers copy QCamApi.h to ADQImaging/Support
-From C:\Program Files\QImaging\SDK\libs\i386 copy QCamDriver.lib to ADQImaging\Support\os\win32-x86
-From C:\Program Files\QImaging\SDK\libs\AMD64 copy QCamDriver64.lib to ADQImaging\Support\os\windows-x64

DLLs are located at
-C:\Windows\Systems32\QCamDriver.dll
-C:\Windows\SysWOW64\QCamDriver64.dll, and QCamChildDriverx64.dll

Release Notes
=============
