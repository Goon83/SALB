<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.0 Transitional//EN">

<!--Converted with LaTeX2HTML 2002-2-1 (1.71)
original version by:  Nikos Drakos, CBLU, University of Leeds
* revised and updated by:  Marcus Hennecke, Ross Moore, Herb Swan
* with significant contributions from:
  Jens Lippmann, Marek Rouchal, Martin Wilck and others -->
<HTML>
<HEAD>
<TITLE>PVFS Tuning</TITLE>
<META NAME="description" CONTENT="PVFS Tuning">
<META NAME="keywords" CONTENT="pvfs2-tuning">
<META NAME="resource-type" CONTENT="document">
<META NAME="distribution" CONTENT="global">

<META NAME="Generator" CONTENT="LaTeX2HTML v2002-2-1">
<META HTTP-EQUIV="Content-Style-Type" CONTENT="text/css">

<LINK REL="STYLESHEET" HREF="pvfs2-tuning.css">

<? include("../../../../header.php"); ?></HEAD>

<?include("../../../..//top.php"); ?> <body id="documentation">



<table width="95%" class="tabletype1" cellpadding="0" cellspacing="0" align="left">
<tr>
<td>
<table width="100%" cellspacing="0" cellpadding="1">
<tr>                                                          
<td class="nav_white" width="70%" valign="top">               
<br>

<table cellspacing="0" cellpadding="1" align="left" width="70%">
<tr>
<td width="80%" valign="top">
<H1 ALIGN="LEFT">PVFS Tuning</H1>
<DIV CLASS="author_info">

<P ALIGN="LEFT"><STRONG>PVFS Development Team </STRONG></P>
</DIV>

<P>
<BR>

<H2><A NAME="SECTION00010000000000000000">
Contents</A>
</H2>
<!--Table of Contents-->

<UL CLASS="TofC">
<LI><A NAME="tex2html36"
  HREF="pvfs2-tuning.php#SECTION00020000000000000000">1 Introduction</A>
<LI><A NAME="tex2html37"
  HREF="pvfs2-tuning.php#SECTION00030000000000000000">2 Cluster Partitioning</A>
<LI><A NAME="tex2html38"
  HREF="pvfs2-tuning.php#SECTION00040000000000000000">3 Storage</A>
<UL>
<LI><A NAME="tex2html39"
  HREF="pvfs2-tuning.php#SECTION00041000000000000000">3.1 Server Configuration</A>
<LI><A NAME="tex2html40"
  HREF="pvfs2-tuning.php#SECTION00042000000000000000">3.2 Local File System</A>
<LI><A NAME="tex2html41"
  HREF="pvfs2-tuning.php#SECTION00043000000000000000">3.3 Disk Synchronization</A>
<LI><A NAME="tex2html42"
  HREF="pvfs2-tuning.php#SECTION00044000000000000000">3.4 Metadata</A>
<LI><A NAME="tex2html43"
  HREF="pvfs2-tuning.php#SECTION00045000000000000000">3.5 Data</A>
</UL>
<BR>
<LI><A NAME="tex2html44"
  HREF="pvfs2-tuning.php#SECTION00050000000000000000">4 Networks</A>
<UL>
<LI><A NAME="tex2html45"
  HREF="pvfs2-tuning.php#SECTION00051000000000000000">4.1 Network Independent</A>
<LI><A NAME="tex2html46"
  HREF="pvfs2-tuning.php#SECTION00052000000000000000">4.2 TCP</A>
<LI><A NAME="tex2html47"
  HREF="pvfs2-tuning.php#SECTION00053000000000000000">4.3 Infiniband</A>
<LI><A NAME="tex2html48"
  HREF="pvfs2-tuning.php#SECTION00054000000000000000">4.4 Myrinet Express</A>
</UL>
<BR>
<LI><A NAME="tex2html49"
  HREF="pvfs2-tuning.php#SECTION00060000000000000000">5 VFS Layer</A>
<UL>
<LI><A NAME="tex2html50"
  HREF="pvfs2-tuning.php#SECTION00061000000000000000">5.1 Maximum I/O Size</A>
<LI><A NAME="tex2html51"
  HREF="pvfs2-tuning.php#SECTION00062000000000000000">5.2 Workload Specifics</A>
</UL>
<BR>
<LI><A NAME="tex2html52"
  HREF="pvfs2-tuning.php#SECTION00070000000000000000">6 Number of Datafiles</A>
<LI><A NAME="tex2html53"
  HREF="pvfs2-tuning.php#SECTION00080000000000000000">7 Distributions</A>
<UL>
<LI><A NAME="tex2html54"
  HREF="pvfs2-tuning.php#SECTION00081000000000000000">7.1 Simple Stripe</A>
<LI><A NAME="tex2html55"
  HREF="pvfs2-tuning.php#SECTION00082000000000000000">7.2 Basic</A>
<LI><A NAME="tex2html56"
  HREF="pvfs2-tuning.php#SECTION00083000000000000000">7.3 Two Dimensional Stripe</A>
<LI><A NAME="tex2html57"
  HREF="pvfs2-tuning.php#SECTION00084000000000000000">7.4 Variable Strip</A>
</UL>
<BR>
<LI><A NAME="tex2html58"
  HREF="pvfs2-tuning.php#SECTION00090000000000000000">8 Workloads</A>
<UL>
<LI><A NAME="tex2html59"
  HREF="pvfs2-tuning.php#SECTION00091000000000000000">8.1 Small files</A>
<LI><A NAME="tex2html60"
  HREF="pvfs2-tuning.php#SECTION00092000000000000000">8.2 Large Files</A>
<LI><A NAME="tex2html61"
  HREF="pvfs2-tuning.php#SECTION00093000000000000000">8.3 Concurrent IO</A>
</UL>
<BR>
<LI><A NAME="tex2html62"
  HREF="pvfs2-tuning.php#SECTION000100000000000000000">9 Benchmarking</A>
<LI><A NAME="tex2html63"
  HREF="pvfs2-tuning.php#SECTION000110000000000000000">10 References</A>
</UL>
<!--End of Table of Contents-->

<P>

<H1><A NAME="SECTION00020000000000000000">
<SPAN CLASS="arabic">1</SPAN> Introduction</A>
</H1>

<P>
The default settings for PVFS (those provided and in the source code
and added to the config files by <TT>pvfs2-genconfig</TT>)
provide good performance on most systems and for a wide variety of workloads.
This document describes system level and PVFS specific parameters 
that can be tuned to improve
performance on specific hardware, or for specific workloads and usage
scenarios.  

<P>
In general performance tuning should begin with the available hardware
and system software, to maximize the bandwidth of the network and
transfer rates of the storage hardware.  From there, PVFS server
parameters can be tuned to improve performance of the
entire system, especially
if specific usage scenarios are targetted.  Finally, file system 
extended attributes and hints can be tweaked by different users to
improve individual performance within a system with varying workloads.

<P>
Some (especially system level) parameters can be tuned to provide better
performance without sacrificing another property of the system.  
Tuning some parameters though, may have a direct effect on the
performance of other usage scenarios, or some other property of the
system (such as durability).  
Our discussion of performance tuning will include the tradeoffs
that must be made during the tuning process, but the final decisions are best
made by the administrators to determine the optimal 
setup that meets the needs of their users.

<P>

<H1><A NAME="SECTION00030000000000000000">
<SPAN CLASS="arabic">2</SPAN> Cluster Partitioning</A>
</H1>

<P>
For users that have one use case, and a generic cluster, what's the best
partition of compute/IO nodes?  Is this section needed?

<P>

<H1><A NAME="SECTION00040000000000000000">
<SPAN CLASS="arabic">3</SPAN> Storage</A>
</H1>

<P>

<H2><A NAME="SECTION00041000000000000000">
<SPAN CLASS="arabic">3</SPAN>.<SPAN CLASS="arabic">1</SPAN> Server Configuration</A>
</H2>

<P>
How many IO servers? <A NAME="tex2html1"
  HREF="#foot14"><SUP><SPAN CLASS="arabic">1</SPAN></SUP></A>How many MD servers?
Should IO and MD servers be shared?

<P>

<H2><A NAME="SECTION00042000000000000000">
<SPAN CLASS="arabic">3</SPAN>.<SPAN CLASS="arabic">2</SPAN> Local File System</A>
</H2>

<P>

<UL>
<LI>ext3
</LI>
<LI>xfs
</LI>
</UL>

<P>

<H2><A NAME="SECTION00043000000000000000">
<SPAN CLASS="arabic">3</SPAN>.<SPAN CLASS="arabic">3</SPAN> Disk Synchronization</A>
</H2>

<P>
The easiest way to see an improvement in performance is to set the
<TT>TroveSyncMeta</TT> and <TT>TroveSyncData</TT> attributes to ``no''
in the <TT>&lt;StorageHints&gt;</TT> section.  If those attributes are set to
``no'' then Trove will read and write data from a cache and not the
underlying file.  Performance will increase greatly, but if the server
dies at some point, you could lose data.  At this point in PVFS2
development, server crashes are rare outside of hardware failures.
PVFS2 developers should probably leave these settings to ``yes''.  If
PVFS2 hosts the only copy of your data, leave these settings to ``yes''.
Otherwise, give ``no'' a shot.

<P>
Sync or not, metadata, data
  coalescing

<P>
distributed metadata

<P>

<H2><A NAME="SECTION00044000000000000000">
<SPAN CLASS="arabic">3</SPAN>.<SPAN CLASS="arabic">4</SPAN> Metadata</A>
</H2>

<H3><A NAME="SECTION00044100000000000000">
<SPAN CLASS="arabic">3</SPAN>.<SPAN CLASS="arabic">4</SPAN>.<SPAN CLASS="arabic">1</SPAN> Coalescing</A>
</H3>

<P>

<H2><A NAME="SECTION00045000000000000000">
<SPAN CLASS="arabic">3</SPAN>.<SPAN CLASS="arabic">5</SPAN> Data</A>
</H2>

<P>

<H1><A NAME="SECTION00050000000000000000">
<SPAN CLASS="arabic">4</SPAN> Networks</A>
</H1>

<P>

<H2><A NAME="SECTION00051000000000000000">
<SPAN CLASS="arabic">4</SPAN>.<SPAN CLASS="arabic">1</SPAN> Network Independent</A>
</H2>

<P>

<OL>
<LI>Unexpected message size
</LI>
<LI>Flow Parameters
</LI>
</OL>

<P>

<UL>
<LI>buffer size
</LI>
<LI>count
</LI>
</UL>

<P>

<H2><A NAME="SECTION00052000000000000000">
<SPAN CLASS="arabic">4</SPAN>.<SPAN CLASS="arabic">2</SPAN> TCP</A>
</H2>

<P>

<H3><A NAME="SECTION00052100000000000000">
<SPAN CLASS="arabic">4</SPAN>.<SPAN CLASS="arabic">2</SPAN>.<SPAN CLASS="arabic">1</SPAN> Kernel Parameters</A>
</H3>

<H3><A NAME="SECTION00052200000000000000">
<SPAN CLASS="arabic">4</SPAN>.<SPAN CLASS="arabic">2</SPAN>.<SPAN CLASS="arabic">2</SPAN> Socket Buffer Sizes</A>
</H3>

<H3><A NAME="SECTION00052300000000000000">
<SPAN CLASS="arabic">4</SPAN>.<SPAN CLASS="arabic">2</SPAN>.<SPAN CLASS="arabic">3</SPAN> Listening Backlog (?)</A>
</H3>

<P>

<H2><A NAME="SECTION00053000000000000000">
<SPAN CLASS="arabic">4</SPAN>.<SPAN CLASS="arabic">3</SPAN> Infiniband</A>
</H2>

<H2><A NAME="SECTION00054000000000000000">
<SPAN CLASS="arabic">4</SPAN>.<SPAN CLASS="arabic">4</SPAN> Myrinet Express</A>
</H2>

<P>

<H1><A NAME="SECTION00060000000000000000">
<SPAN CLASS="arabic">5</SPAN> VFS Layer</A>
</H1>

<P>

<H2><A NAME="SECTION00061000000000000000">
<SPAN CLASS="arabic">5</SPAN>.<SPAN CLASS="arabic">1</SPAN> Maximum I/O Size</A>
</H2>

<P>

<H2><A NAME="SECTION00062000000000000000">
<SPAN CLASS="arabic">5</SPAN>.<SPAN CLASS="arabic">2</SPAN> Workload Specifics</A>
</H2>

<P>

<H1><A NAME="SECTION00070000000000000000">
<SPAN CLASS="arabic">6</SPAN> Number of Datafiles</A>
</H1>

<P>
Each file stored on PVFS is broken into smaller parts to be
distributed across servers.  The metadata (which includes
information such as the owner and permissions of the file) is stored in
a metadata file on one server.  The actual file contents are stored in
<SPAN  CLASS="textit">datafiles</SPAN> distributed among multiple servers.

<P>
By default, each file in PVFS is made up of N datafiles, where N is
the number of servers.  In most situations, this is the most efficient
number of datafiles to use because it leverages all available resources
evenly and allows the load to be distributed.  This is especially
beneficial for large files accessed in parallel.

<P>
However, there are also cases where it may be helpful to use a different
number of datafiles.  For example, if you have a set of many small
files (a few KB each) that are accessed in serial, then distributing
them across all servers increases overhead without gaining any benefit
from parallelism.  In this case it will most likely perform better if
the number of datafiles is set to 1 for each file.

<P>
The most straightforward way to change the number of datafiles is by
setting extended attributes on a directory.  Any new files created in
that directory will utilize the specified number of datafiles
regardless of how many servers are available.  New subdirectories will
inherit the same settings.  It is also possible to set a default number
of datafiles in the configuration file for the entire file system, but
this is rarely advisable.

<P>
PVFS does not allow the number of datafiles to be changed dynamically
for existing files.  If you wish to convert an existing file, then you must
copy it to a new file with the appropriate datafile setting and then delete
the old file.

<P>
Use this command to specify the number of datafiles to use within a given
directory:

<P>
<PRE>
$ setfattr -n user.pvfs2.num_dfiles -v 1 /mnt/pvfs2/dir
</PRE>

<P>
Use these commands to create a new file and confirm the number of
datafiles that it is using:

<P>
<PRE>
$ touch /mnt/pvfs2/dir/foo
$ pvfs2-viewdist -f /mnt/pvfs2/dir/foo
dist_name = simple_stripe
dist_params:
strip_size:65536

Number of datafiles/servers = 1
Server 0 - tcp://localhost:3334, handle: 5223372036854744173
(487d2531626f846d.bstream)
</PRE>

<P>

<H1><A NAME="SECTION00080000000000000000">
<SPAN CLASS="arabic">7</SPAN> Distributions</A>
</H1>

<P>
A <SPAN  CLASS="textit">distribution</SPAN> is an algorithm that defines how a file's data
will be distributed among available servers.  PVFS provides an API
for implementing arbitrary distributions, but four specific ones are
available by default.  Each distribution has different performance
characteristics that may be helpful depending on your workload.

<P>
The most straightforward way to use an alternative distribution is by
setting an extended attribute on a specific directory.  Any new files
created in that directory will utilize the specified distribution and
parameters.  New subdirectories will inherit the same settings.  You may
also use the server configuration files to define default distribution
settings for the entire file system, but we suggest experimenting at a
directory level first.

<P>
PVFS does not allow the distribution to be changed dynamically for
existing files.  If you wish to convert an existing file, then you must
copy it to a new file with the appropriate distribution and then delete
the old file.

<P>
This section describes the four available distributions and gives
command line examples of how to use each one.  

<P>

<H2><A NAME="SECTION00081000000000000000">
<SPAN CLASS="arabic">7</SPAN>.<SPAN CLASS="arabic">1</SPAN> Simple Stripe</A>
</H2>

<P>
The simple stripe distribution is the default distribution used by PVFS.
It dictates that file data will be striped evenly across all available
servers in a round robin fashion.  This is very similar to RAID 0,
except the data is distributed across servers rather than local block
devices. 

<P>
The only tuning parameter within simple stripe is the <SPAN  CLASS="textit">strip size</SPAN>.
The strip size determines how much data is stored on one server before
switching to the next server.  The default value in PVFS is 64 KB.  You
may want to experiment with this value in order to find a tradeoff
between the amount of concurrency achieved with small accesses vs. the
amount of data streamed to each server.

<P>
<PRE>
# to enable simple stripe distribution for a directory:
$ setfattr -n user.pvfs2.dist_name -v simple_stripe /mnt/pvfs2/dir

# to change the strip size to 128 KB:
$ setfattr -n user.pvfs2.dist_params -v strip_size:131072 /mnt/pvfs2/dir

# to create a new file and confirm the distribution:
$ touch /mnt/pvfs2/dir/file
$ pvfs2-viewdist -f /mnt/pvfs2/dir/file
dist_name = simple_stripe
dist_params:
strip_size:131072

Number of datafiles/servers = 4
Server 0 - tcp://localhost:3337, handle: 8223372036854744180 (721f494c589b8474.bstream)
Server 1 - tcp://localhost:3334, handle: 5223372036854744174 (487d2531626f846e.bstream)
Server 2 - tcp://localhost:3335, handle: 6223372036854744178 (565ddbe509d38472.bstream)
Server 3 - tcp://localhost:3336, handle: 7223372036854744180 (643e9298b1378474.bstream)
</PRE>

<P>

<H2><A NAME="SECTION00082000000000000000">
<SPAN CLASS="arabic">7</SPAN>.<SPAN CLASS="arabic">2</SPAN> Basic</A>
</H2>

<P>
The basic distribution is mainly included as an example for distribution
developers.  It performs no striping at all, and instead places all
data on one server.  The basic distribution overrides the number of
datafiles (as shown in the previous section) and only uses one datafile
in all cases.  There are no tunable parameters.

<P>
<PRE>
# to enable basic distribution for a directory:
$ setfattr -n user.pvfs2.dist_name -v basic_dist /mnt/pvfs2/dir

# to create a new file and confirm the distribution:
$ touch /mnt/pvfs2/dir/file
$ pvfs2-viewdist -f /mnt/pvfs2/dir/file
dist_name = basic_dist
dist_params:
none
Number of datafiles/servers = 1
Server 0 - tcp://localhost:3334, handle: 5223372036854744172 (487d2531626f846c.bstream)
</PRE>

<P>

<H2><A NAME="SECTION00083000000000000000">
<SPAN CLASS="arabic">7</SPAN>.<SPAN CLASS="arabic">3</SPAN> Two Dimensional Stripe</A>
</H2>

<P>
The two dimensional stripe distribution is a variation of the
simple stripe distribution that is intended to combat the affects of
<SPAN  CLASS="textit">incast</SPAN>.  Incast occurs when a client requests a range of data that
is striped across several servers, therefore causing the transmission of
data from many sources to one destination simultaneously.  Some networks
or network protocols may perform poorly in this scenario due to switch
buffering or congestion avoidance.  This problem becomes more
significant as more servers are used.

<P>
The two dimensional stripe distribution operates by grouping servers
into smaller subsets and striping data within each group multiple times
before switching.  Three parameters control the grouping and
striping:

<P>

<UL>
<LI>strip size: same as in the simple stripe distribution
</LI>
<LI>number of groups: how many groups to divide the servers into
</LI>
<LI>factor: how many times to stripe within each group
</LI>
</UL>

<P>
The common access pattern that benefits from this distribution is the
case of N clients operating on one file of size B, where each client is
responsible for a contiguous region of size B/N.  With simple striping,
each client may have to access all servers in order to read its specific
B/N byte range.  With two dimensional striping (using appropriate
parameters), each client only accesses a subset of servers.  All servers
are still active over the file as a whole so that full bandwidth is
still preserved.  However, the network traffic patterns limit the amount
of incast produced by any one client.

<P>
The default incast parameters use a strip size of 64 KB, 2 groups, and a
factor of 256.

<P>
<PRE>
# to enable basic distribution for a directory:
$ setfattr -n user.pvfs2.dist_name -v twod_stripe /mnt/pvfs2/dir

# to change the strip size to 128 KB, the number of groups to 4, and a
# factor of 228:
$ setfattr -n user.pvfs2.dist_params -v strip_size:131072,num_groups:4,group_strip_factor:128 /mnt/pvfs2/dir

# to create a new file and confirm the distribution:
$ touch /mnt/pvfs2/dir/file
$ pvfs2-viewdist -f /mnt/pvfs2/dir/file
dist_name = twod_stripe
dist_params:
num_groups:4,strip_size:131072,factor:128

Number of datafiles/servers = 4
Server 0 - tcp://localhost:3336, handle: 7223372036854744175
(643e9298b137846f.bstream)
Server 1 - tcp://localhost:3337, handle: 8223372036854744175
(721f494c589b846f.bstream)
Server 2 - tcp://localhost:3334, handle: 5223372036854744167
(487d2531626f8467.bstream)
Server 3 - tcp://localhost:3335, handle: 6223372036854744173
(565ddbe509d3846d.bstream)
</PRE>

<P>

<H2><A NAME="SECTION00084000000000000000">
<SPAN CLASS="arabic">7</SPAN>.<SPAN CLASS="arabic">4</SPAN> Variable Strip</A>
</H2>

<P>
Variable strip is similar to simple stripe, except that it allows you to
specify a different strip size for each server.  For example, you could
place the first 64 KB on one server, the next 32 KB on the next server,
and then 128 KB on the final server.  The striping still round robins
once all servers have been filled.  This distribution may be useful for
applications that have a very specific data format and can take
advantage of a correspondingly specific placement of file data to match it.

<P>
The only parameter used by the variable strip distribution is the
<SPAN  CLASS="textit">strips</SPAN> parameter, which specifies the strip size for each server.
The number of datafiles used will not exceed the number of servers
listed.  For example, if the strips parameter specifies a strip size for
three servers, then files using this distribution will have at most
three datafiles.

<P>
The format of the strips parameter is a list of semicolon separated
<!-- MATH
 $<server>:<strip size>$
 -->
<SPAN CLASS="MATH"><IMG
 WIDTH="202" HEIGHT="35" ALIGN="MIDDLE" BORDER="0"
 SRC="img1.png"
 ALT="$&lt;server&gt;:&lt;strip size&gt;$"></SPAN> pairs.  The strip size can be specified with short hand
notation, such as ``K'' for kilobytes or ``M'' for megabytes.

<P>
<PRE>
# to enable basic distribution for a directory:
$ setfattr -n user.pvfs2.dist_name -v varstrip_dist /mnt/pvfs2/dir

# to change the strip sizes to match the example above:
$ setfattr -n user.pvfs2.dist_params -v "strips:0:32K;1:64K;2:128K" /mnt/pvfs2/dir

# to create a new file and confirm the distribution:
$ touch /mnt/pvfs2/dir/file
$ pvfs2-viewdist -f /mnt/pvfs2/dir/file
dist_name = varstrip_dist
dist_params:
0:32K;1:64K;2:128K
Number of datafiles/servers = 3
Server 0 - tcp://localhost:3336, handle: 7223372036854744173
(643e9298b137846d.bstream)
Server 1 - tcp://localhost:3334, handle: 5223372036854744165
(487d2531626f8465.bstream)
Server 2 - tcp://localhost:3335, handle: 6223372036854744171
(565ddbe509d3846b.bstream)
</PRE>

<P>

<H1><A NAME="SECTION00090000000000000000">
<SPAN CLASS="arabic">8</SPAN> Workloads</A>
</H1>

<P>

<H2><A NAME="SECTION00091000000000000000">
<SPAN CLASS="arabic">8</SPAN>.<SPAN CLASS="arabic">1</SPAN> Small files</A>
</H2>

<H2><A NAME="SECTION00092000000000000000">
<SPAN CLASS="arabic">8</SPAN>.<SPAN CLASS="arabic">2</SPAN> Large Files</A>
</H2>

<H2><A NAME="SECTION00093000000000000000">
<SPAN CLASS="arabic">8</SPAN>.<SPAN CLASS="arabic">3</SPAN> Concurrent IO</A>
</H2>

<P>

<H1><A NAME="SECTION000100000000000000000">
<SPAN CLASS="arabic">9</SPAN> Benchmarking</A>
</H1>

<P>

<UL>
<LI>mpi-io-test
</LI>
<LI>mpi-md-test
</LI>
</UL>

<P>

<H1><A NAME="SECTION000110000000000000000">
<SPAN CLASS="arabic">10</SPAN> References</A>
</H1>

<P>

<H1><A NAME="SECTION000120000000000000000">
About this document ...</A>
</H1>
 <STRONG>PVFS Tuning</STRONG><P>
This document was generated using the
<A HREF="http://www.latex2html.org/"><STRONG>LaTeX</STRONG>2<tt>HTML</tt></A> translator Version 2002-2-1 (1.71)
<P>
Copyright &#169; 1993, 1994, 1995, 1996,
Nikos Drakos, 
Computer Based Learning Unit, University of Leeds.
<BR>
Copyright &#169; 1997, 1998, 1999,
<A HREF="http://www.maths.mq.edu.au/~ross/">Ross Moore</A>, 
Mathematics Department, Macquarie University, Sydney.
<P>
The command line arguments were: <BR>
 <STRONG>latex2html</STRONG> <TT>-split 0 -show_section_numbers -nonavigation -init_file /tmp/pvfs-2.8.2/doc/latex2html-init pvfs2-tuning.tex</TT>
<P>
The translation was initiated by Samuel Lang on 2010-02-04
<BR><HR><H4>Footnotes</H4>
<DL>
<DT><A NAME="foot14">... servers?</A><A
 HREF="pvfs2-tuning.php#tex2html1"><SUP><SPAN CLASS="arabic">1</SPAN></SUP></A></DT>
<DD>The FAQ already answers this to some
degree

</DD>
</DL>
<BR><HR>
<ADDRESS>
Samuel Lang
2010-02-04
</ADDRESS></table></table></table><?include("../../../../bottom.php"); ?>
</BODY>
</HTML>
