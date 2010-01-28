Files in this directory are not part of the VPython project, are not maintained by us, 
but are included in CVS for the convenience of developers. Compiling the Boost libraries
from source is a lengthy process, which is why binaries of the static libraries are provided here.

-----
Boost
-----

The files in win_boost.zip or mac_boost.zip are a subset of the Boost library
and should be extracted (into vpython-core2/dependencies/boost_files/) before building.

After extraction, the folder boost_files should contain the following folders:

   boost: header files
   win_libs or mac_libs: static libraries
   WinBoostNotes.txt or MacBoostNotes.txt: tells what version of Boost, for what version of Python
   
The zip files will be updated in CVS to correspond to the current version of Python
used in released versions of Visual.

The Boost libraries can be found at www.boost.org; the BoostPro installer 
containing the Windows binary libraries is at www.boost-consulting.com.

----------
threadpool
----------

From threadpool.sf.net.  A very simple thread pooling library based on Boost.Thread (no 
platform dependent code).  Might become part of boost someday.

The version of threadpool must match the version of Boost.
Go to http://sourceforge.net/projects/threadpool and choose Download,
then click the threadpool link in the Package column.
Click on the "clipboard" icon for each package to see what Boost version is required.
As of March 2009, here is what one finds:
0.2.5 boost::thread 1.37
0.2.4 boost::thread 1.35

