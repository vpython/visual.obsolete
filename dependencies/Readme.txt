Files in this directory are not part of the VPython project, are not maintained by us, 
but are included in CVS for the convenience of developers. Compiling the Boost libraries
from source is a very lengthy process, which is why binaries of the static libraries 
are provided here.

-----
Boost
-----

The files in boost_files2.5.zip are a subset of the Boost library for Python 2.5 
and should be extracted (into vpython-core2/dependencies/boost_files/) before building.

After extraction, the folder boost_files should contain the following folders:

   boost: header files (valid for all platforms -- Windows, Mac, Linux)
   win_libs: static libraries for Windows
   mac_libs: static libraries for Intel Macs running OS X 10.5
      and the following (currently missing):
   mac_ppc_104_lib: static libraries for PowerPC Macs running OS X 10.4
   mac_ppc_105_lib: static libraries for PowerPC Macs running OS X 10.5

The Boost libraries can be found at www.boost.org; the BoostPro installer 
containing the Windows binary libraries is at www.boost-consulting.com.

The files in boost_files2.6.zip are for Python 2.6. If you use these, you need
to adjust the build project to refer to 2.6 instead of 2.5.

----------
threadpool
----------

From threadpool.sf.net.  A very simple thread pooling library based on Boost.Thread (no 
platform dependent code).  Might become part of boost someday.