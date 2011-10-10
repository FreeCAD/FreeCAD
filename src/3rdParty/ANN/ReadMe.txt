ANN: Approximate Nearest Neighbors
Version: 1.1.1
Release date: Aug 4, 2006
----------------------------------------------------------------------------
Copyright (c) 1997-2005 University of Maryland and Sunil Arya and David
Mount. All Rights Reserved.  See Copyright.txt and License.txt for
complete information on terms and conditions of use and distribution of
this software.
----------------------------------------------------------------------------

Authors
-------
David Mount
Dept of Computer Science
University of Maryland,
College Park, MD 20742 USA
mount@cs.umd.edu
http://www.cs.umd.edu/~mount/

Sunil Arya
Dept of Computer Science
Hong University of Science and Technology
Clearwater Bay, HONG KONG
arya@cs.ust.hk
http://www.cs.ust.hk/faculty/arya/

Introduction
------------
ANN is a library written in the C++ programming language to support both
exact and approximate nearest neighbor searching in spaces of various
dimensions.  It was implemented by David M. Mount of the University of
Maryland, and Sunil Arya of the Hong Kong University of Science and
Technology.  ANN (pronounced like the name ``Ann'') stands for
Approximate Nearest Neighbors.  ANN is also a testbed containing
programs and procedures for generating data sets, collecting and
analyzing statistics on the performance of nearest neighbor algorithms
and data structures, and visualizing the geometric structure of these
data structures.

The ANN source code and documentation is available from the following
web page:

    http://www.cs.umd.edu/~mount/ANN

For more information on ANN and its use, see the ``ANN Programming
Manual,'' which is provided with the software distribution.

----------------------------------------------------------------------------
History
  Version 0.1  03/04/98
    Preliminary release
  Version 0.2  06/24/98
    Changes for SGI compiler.
  Version 1.0  04/01/05
    Fixed a number of small bugs
    Added dump/load operations
    Added annClose to eliminate minor memory leak
    Improved compatibility with current C++ compilers
    Added compilation for Microsoft Visual Studio.NET
    Added compilation for Linux 2.x
  Version 1.1  05/03/05
    Added make target for Mac OS X
    Added fixed-radius range searching and counting
    Added instructions on compiling/using ANN on Windows platforms
    Fixed minor output bug in ann2fig
  Version 1.1.1  08/04/06
    Added "planted" distribution
    Updated old source comments for GNU LPL.
