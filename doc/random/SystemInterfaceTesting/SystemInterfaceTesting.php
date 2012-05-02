<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.0 Transitional//EN">

<!--Converted with LaTeX2HTML 2002-2-1 (1.71)
original version by:  Nikos Drakos, CBLU, University of Leeds
* revised and updated by:  Marcus Hennecke, Ross Moore, Herb Swan
* with significant contributions from:
  Jens Lippmann, Marek Rouchal, Martin Wilck and others -->
<HTML>
<HEAD>
<TITLE>PVFS2: System Interface
Test Suite</TITLE>
<META NAME="description" CONTENT="PVFS2: System Interface
Test Suite">
<META NAME="keywords" CONTENT="SystemInterfaceTesting">
<META NAME="resource-type" CONTENT="document">
<META NAME="distribution" CONTENT="global">

<META NAME="Generator" CONTENT="LaTeX2HTML v2002-2-1">
<META HTTP-EQUIV="Content-Style-Type" CONTENT="text/css">

<LINK REL="STYLESHEET" HREF="SystemInterfaceTesting.css">

<? include("../../../../header.php"); ?></HEAD>

<?include("../../../..//top.php"); ?> <body id="documentation">

<P>
<H1 ALIGN="LEFT">PVFS2: System Interface
<BR>
Test Suite</H1>
<DIV CLASS="author_info">

<P ALIGN="LEFT"><STRONG>Frank Shorter</STRONG></P>
<P ALIGN="LEFT"><I>Michael Speth</I></P>
<P ALIGN="LEFT"><STRONG>May 16, 2003</STRONG></P>
</DIV>

<P>

<H1><A NAME="SECTION00010000000000000000">
<SPAN CLASS="arabic">1</SPAN> Introduction</A>
</H1>
The Parallel Virtual File System version 2 (PVFS2) is in development.  This document attempts to outline the process of validating the System Interface of the client.  The testing process is divided into two parts: positive tests and negative tests.  Positive tests examine functions operating on normal procedures.  Negative tests examine the functions behavior on abnormal procedures designed to make the functions fail. 

<P>

<H1><A NAME="SECTION00020000000000000000"></A>
<A NAME="setupL"></A>
<BR>
<SPAN CLASS="arabic">2</SPAN> Setup
</H1>
Describe the system (hardware and OS) that the tests are being run on and the version of pvfs2.

<P>

<H1><A NAME="SECTION00030000000000000000">
<SPAN CLASS="arabic">3</SPAN> Positive Tests</A>
</H1>

<P>
The positive tests will verify basic functionality, and ensure that the system interface behaves as expected for a given set of reasonable inputs.  We expect that all of these calls should succeed.  It is the goal of this section to provide coverage for all areas of the system interface that will recieve the most usage.

<P>

<H2><A NAME="SECTION00031000000000000000">
<SPAN CLASS="arabic">3</SPAN>.<SPAN CLASS="arabic">1</SPAN> Startup and Shutdown</A>
</H2>
The most trivial test of the system interface, we initialize and finalize the system interface.

<P>

<H2><A NAME="SECTION00032000000000000000">
<SPAN CLASS="arabic">3</SPAN>.<SPAN CLASS="arabic">2</SPAN> Metadata tests</A>
</H2>

<P>

<DL COMPACT>
<DT><SPAN CLASS="arabic">1</SPAN>.
<DD><SPAN  CLASS="textbf">File creation</SPAN>:  We will test the creation of files with valid attributes inside of directories where we have permission to do so.  The number of data files will be varied from 1 to 2N (where N is the number of I/O servers).  Creation will be verified with a lookup operation.

<P>
<DT><SPAN CLASS="arabic">2</SPAN>.
<DD><SPAN  CLASS="textbf">File removal</SPAN>:  We will test the removal of files that have the appropriate permissions for our user.  Removal will be verified by a failed lookup operation.  After the file is removed, we will re-create a new file with the same name.  Lookup of the new file must return the new handle. as well as the attributes of the new file(including datafile handles).

<P>
<DT><SPAN CLASS="arabic">3</SPAN>.
<DD><SPAN  CLASS="textbf">Setting/retrieving attributes on a file</SPAN>:  Setting/retrieving attributes will be tested by setting all the attributes on a file to some known values, then calling getattr to ensure that they have been set.  Important things to pay attention to here are the filesize, as well as permissions.

<P>
<DT><SPAN CLASS="arabic">4</SPAN>.
<DD><SPAN  CLASS="textbf">Lookup of a file</SPAN>:  Lookup will be tested by creating a file, and then looking up that file and comparing the handles.  Create and Lookup should return the same handles and file system id numbers.

<P>
<DT><SPAN CLASS="arabic">5</SPAN>.
<DD><SPAN  CLASS="textbf">Renaming files</SPAN>:  We will create a file, lookup the file, then call rename.  We will verify rename by calling lookup on the old filename, ensuring that it fails, and then calling lookup on the new filename and ensuring that it returns the handle we were given at create time.  Renaming will need to be tested within the same directory, as well as renaming(essentially moving) files into different directories.

<P>
<DT><SPAN CLASS="arabic">6</SPAN>.
<DD><SPAN  CLASS="textbf">Directory creation</SPAN>:  We will test the creation of directories with valid attributes inside of directories where we have permission to do so.  We're only looking to create a moderate number of directories with this test case.  Please refer to the stress testing section for info on the directory tests where a very large number of directories are added.  directory creation will be verified both by having it appear when readdir is called, as well as being able to look it up with the lookup function.

<P>
<DT><SPAN CLASS="arabic">7</SPAN>.
<DD><SPAN  CLASS="textbf">Directory removal</SPAN>:  We will create a directory, verify that it exists with readdir and lookup, then call rmdir.  To ensure that it has been deleted, we will attempt to lookup and call readdir on the directory name that was just removed.  Both of these calls must not return any trace of the directory.  Additionally, we will create a directory of the same name, and compare its attributes to the previous directory.

<P>
<DT><SPAN CLASS="arabic">8</SPAN>.
<DD><SPAN  CLASS="textbf">Lookup of a directory</SPAN>:  Lookup will be tested by creating a directory, and then looking up that directory and comparing the handles.  Create and Lookup should return the same handles and file system id numbers.

<P>
<DT><SPAN CLASS="arabic">9</SPAN>.
<DD><SPAN  CLASS="textbf">Setting/retrieving attributes on a directory</SPAN>:  Setting/retrieving attributes will be tested by setting all the attributes on a directory to some known values, then calling getattr to ensure that they have been set.
</DD>
</DL>

<P>

<H2><A NAME="SECTION00033000000000000000">
<SPAN CLASS="arabic">3</SPAN>.<SPAN CLASS="arabic">3</SPAN> I/O tests</A>
</H2>

<P>

<H3><A NAME="SECTION00033100000000000000">
<SPAN CLASS="arabic">3</SPAN>.<SPAN CLASS="arabic">3</SPAN>.<SPAN CLASS="arabic">1</SPAN> Reading</A>
</H3>
The read tests will be performed on files where we have read permission, and data exists within the file.  The request will be committed prior to the IO call.  We will need to test every combination of requests (from pvfs-request.h):

<P>

<DL COMPACT>
<DT><SPAN CLASS="arabic">10</SPAN>.
<DD><SPAN  CLASS="textbf">Contiguous</SPAN>:  count should be varied to ensure that we're hitting multiple servers as well as only getting data from each server at time.
<DT><SPAN CLASS="arabic">11</SPAN>.
<DD><SPAN  CLASS="textbf">Vector</SPAN>: with "stride" lengths that span multiple servers.
<DT><SPAN CLASS="arabic">12</SPAN>.
<DD><SPAN  CLASS="textbf">Hvector</SPAN>:  see vector.
<DT><SPAN CLASS="arabic">13</SPAN>.
<DD><SPAN  CLASS="textbf">Indexed</SPAN>:  indexed will be tested with varying block lengths and displacements.  displacements that cause multiple servers to be spanned as well as large block lengths will also be used.
<DT><SPAN CLASS="arabic">14</SPAN>.
<DD><SPAN  CLASS="textbf">Hindexed</SPAN>:  see indexed.
<DT><SPAN CLASS="arabic">15</SPAN>.
<DD><SPAN  CLASS="textbf">Manual ub/lb/extents</SPAN>:  Calling read with varying the displacements on ub, lb, and extents will be performed.
</DD>
</DL>

<P>

<H3><A NAME="SECTION00033200000000000000">
<SPAN CLASS="arabic">3</SPAN>.<SPAN CLASS="arabic">3</SPAN>.<SPAN CLASS="arabic">2</SPAN> Writing</A>
</H3>
The write tests will be performed on files where we have write permission.  Data may or may not exist within file prior to calling write.  The request will be committed prior to the IO call.  We will need to test every combination of requests (from pvfs-request.h):

<P>

<DL COMPACT>
<DT><SPAN CLASS="arabic">16</SPAN>.
<DD><SPAN  CLASS="textbf">Contiguous</SPAN>:  count should be varied to ensure that we're hitting multiple servers as well as only getting data from each server at time.
<DT><SPAN CLASS="arabic">17</SPAN>.
<DD><SPAN  CLASS="textbf">Vector</SPAN>: with "stride" lengths that span multiple servers.
<DT><SPAN CLASS="arabic">18</SPAN>.
<DD><SPAN  CLASS="textbf">Hvector</SPAN>:  see vector.
<DT><SPAN CLASS="arabic">19</SPAN>.
<DD><SPAN  CLASS="textbf">Indexed</SPAN>:  indexed will be tested with varying block lengths and displacements.  displacements that cause multiple servers to be spanned as well as large block lengths will also be used.
<DT><SPAN CLASS="arabic">20</SPAN>.
<DD><SPAN  CLASS="textbf">Hindexed</SPAN>:  see indexed.
<DT><SPAN CLASS="arabic">21</SPAN>.
<DD><SPAN  CLASS="textbf">Manual ub/lbextent</SPAN>:  Calling write with varying the displacements on ub, lb, and extents will be performed.
</DD>
</DL>

<P>

<H3><A NAME="SECTION00033300000000000000">
<SPAN CLASS="arabic">3</SPAN>.<SPAN CLASS="arabic">3</SPAN>.<SPAN CLASS="arabic">3</SPAN> Truncate</A>
</H3>
The truncate tests will be performed on files where we have write permission.  This test will need to be performed on files of size 0, as well as files with data on every combination of servers.

<P>

<H1><A NAME="SECTION00040000000000000000">
<SPAN CLASS="arabic">4</SPAN> Negative Tests</A>
</H1>
The negative tests are broken up into two sections: invalid parameters and functional ordering.  The invalid parameters tests examines the functions' behaviors when invalid parameters are supplied.  The tests for functional ordering examines functions' behaviors when the ordering of functions are incorrect.

<P>

<H2><A NAME="SECTION00041000000000000000">
<SPAN CLASS="arabic">4</SPAN>.<SPAN CLASS="arabic">1</SPAN> Invalid Parameters</A>
</H2>
Tests functions' behavior when invalid parameters are supplied

<P>

<H3><A NAME="SECTION00041100000000000000"></A>
<A NAME="null1L"></A>
<BR>
<SPAN CLASS="arabic">4</SPAN>.<SPAN CLASS="arabic">1</SPAN>.<SPAN CLASS="arabic">1</SPAN> Null parameters
</H3>
All parameters of each function are null. Note, before any of the functions can be called, initialize must be called with valid parameters expect for tests regarding
initialize.

<P>

<DL COMPACT>
<DT><SPAN CLASS="arabic">22</SPAN>.
<DD>Call initialize and set its parameters to null.  Record the return value and error code returned.
<DT><SPAN CLASS="arabic">23</SPAN>.
<DD>Call finalize and set its parameters to null.  Record the return value and error code returned.
<DT><SPAN CLASS="arabic">24</SPAN>.
<DD>Call lookup and set its parameters to null.  Record the return value and error code returned.
<DT><SPAN CLASS="arabic">25</SPAN>.
<DD>Call getattr and set its parameters to null.  Record the return value and error code returned.
<DT><SPAN CLASS="arabic">26</SPAN>.
<DD>Call setattr and set its parameters to null.  Record the return value and error code returned.
<DT><SPAN CLASS="arabic">27</SPAN>.
<DD>Call mkdir and set its parameters to null.  Record the return value and error code returned.
<DT><SPAN CLASS="arabic">28</SPAN>.
<DD>Call readdir and set its parameters to null.  Record the return value and error code returned.
<DT><SPAN CLASS="arabic">29</SPAN>.
<DD>Call create and set its parameters to null.  Record the return value and error code returned.
<DT><SPAN CLASS="arabic">30</SPAN>.
<DD>Call remove and set its parameters to null.  Record the return value and error code returned.
<DT><SPAN CLASS="arabic">31</SPAN>.
<DD>Call rename and set its parameters to null.  Record the return value and error code returned.
<DT><SPAN CLASS="arabic">32</SPAN>.
<DD>Call symlink and set its parameters to null.  Record the return value and error code returned.
<DT><SPAN CLASS="arabic">33</SPAN>.
<DD>Call readlink and set its parameters to null.  Record the return value and error code returned.
<DT><SPAN CLASS="arabic">34</SPAN>.
<DD>Call read and set its parameters to null.  Record the return value and error code returned.
<DT><SPAN CLASS="arabic">35</SPAN>.
<DD>Call write and set its parameters to null.  Record the return value and error code returned.
</DD>
</DL>

<P>

<H3><A NAME="SECTION00041200000000000000">
<SPAN CLASS="arabic">4</SPAN>.<SPAN CLASS="arabic">1</SPAN>.<SPAN CLASS="arabic">2</SPAN> Varied Null Parameters</A>
</H3>
Some of the parameters of each function are null.  Note, before any of the functions can be called, initialize must be called with valid parameters expect for tests regarding
initialize.

<P>

<DL COMPACT>
<DT><SPAN CLASS="arabic">36</SPAN>.
<DD>Iterate through the list found in section <A HREF="#null1L">4.1.1</A> with the first parameter set to null.  The remaining parameters (if there are any) are set to a valid value.
Record the return value and error code returned.
<DT><SPAN CLASS="arabic">37</SPAN>.
<DD>Iterate through the list found in section <A HREF="#null1L">4.1.1</A> with the second parameter (if there is one)  set to null.  The remaining parameters (if there are any) are set to a valid value.  Record the return value and error code returned.
<DT><SPAN CLASS="arabic">38</SPAN>.
<DD>Iterate through the list found in section <A HREF="#null1L">4.1.1</A> with the third parameter (if there is one) set to null.  The remaining parameters (if there are any) are set to a valid value. Record the return value and error code returned.
<DT><SPAN CLASS="arabic">39</SPAN>.
<DD>Iterate through the list found in section <A HREF="#null1L">4.1.1</A> with the first and second parameter (if there is one) set to null.  The remaining parameters (if there are any) are set to a valid value.  Record the return value and error code returned.
</DD>
</DL>

<P>

<H3><A NAME="SECTION00041300000000000000">
<SPAN CLASS="arabic">4</SPAN>.<SPAN CLASS="arabic">1</SPAN>.<SPAN CLASS="arabic">3</SPAN> Invalid File</A>
</H3>
All test cases use an invalid file

<P>

<DL COMPACT>
<DT><SPAN CLASS="arabic">40</SPAN>.
<DD>Call lookup and set the pinode_refernce.handle to -1.  Record the return value and error code returned.
<DT><SPAN CLASS="arabic">41</SPAN>.
<DD>Call getattr and set its parameters to null.  Record the return value and error code returned.
<DT><SPAN CLASS="arabic">42</SPAN>.
<DD>Call setattr and set its parameters to null.  Record the return value and error code returned.
<DT><SPAN CLASS="arabic">43</SPAN>.
<DD>Call mkdir and set its parameters to null.  Record the return value and error code returned.
<DT><SPAN CLASS="arabic">44</SPAN>.
<DD>Call readdir and set its parameters to null.  Record the return value and error code returned.
<DT><SPAN CLASS="arabic">45</SPAN>.
<DD>Call create and set its parameters to null.  Record the return value and error code returned.
<DT><SPAN CLASS="arabic">46</SPAN>.
<DD>Call remove and set its parameters to null.  Record the return value and error code returned.
<DT><SPAN CLASS="arabic">47</SPAN>.
<DD>Call rename and set its parameters to null.  Record the return value and error code returned.
<DT><SPAN CLASS="arabic">48</SPAN>.
<DD>Call symlink and set its parameters to null.  Record the return value and error code returned.
<DT><SPAN CLASS="arabic">49</SPAN>.
<DD>Call readlink and set its parameters to null.  Record the return value and error code returned.
<DT><SPAN CLASS="arabic">50</SPAN>.
<DD>Call read and set its parameters to null.  Record the return value and error code returned.
<DT><SPAN CLASS="arabic">51</SPAN>.
<DD>Call write and set its parameters to null.  Record the return value and error code returned.
</DD>
</DL>

<P>

<H2><A NAME="SECTION00042000000000000000">
<SPAN CLASS="arabic">4</SPAN>.<SPAN CLASS="arabic">2</SPAN> Functional Ordering</A>
</H2>
All test cases use the pre-built file found in section <A HREF="#setupL">2</A>.

<P>

<H3><A NAME="SECTION00042100000000000000">
<SPAN CLASS="arabic">4</SPAN>.<SPAN CLASS="arabic">2</SPAN>.<SPAN CLASS="arabic">1</SPAN> Client uninitialized</A>
</H3>
Test the behavior of all functions when the initialize function has not been called.

<P>

<DL COMPACT>
<DT><SPAN CLASS="arabic">52</SPAN>.
<DD>Call lookup and record the return value and error codes.
<DT><SPAN CLASS="arabic">53</SPAN>.
<DD>Call getattr and record the return value and error codes.
<DT><SPAN CLASS="arabic">54</SPAN>.
<DD>Call setattr and record the return value and error codes.
<DT><SPAN CLASS="arabic">55</SPAN>.
<DD>Call mkdir and record the return value and error codes.
<DT><SPAN CLASS="arabic">56</SPAN>.
<DD>Call readdir and record the return value and error codes.
<DT><SPAN CLASS="arabic">57</SPAN>.
<DD>Call create and record the return value and error codes.
<DT><SPAN CLASS="arabic">58</SPAN>.
<DD>Call remove and record the return value and error codes.
<DT><SPAN CLASS="arabic">59</SPAN>.
<DD>Call rename and record the return value and error codes.
<DT><SPAN CLASS="arabic">60</SPAN>.
<DD>Call symlink and record the return value and error codes.
<DT><SPAN CLASS="arabic">61</SPAN>.
<DD>Call readlink and record the return value and error codes.
<DT><SPAN CLASS="arabic">62</SPAN>.
<DD>Call read and record the return value and error codes.
<DT><SPAN CLASS="arabic">63</SPAN>.
<DD>Call write and record the return value and error codes.
</DD>
</DL>

<P>

<H3><A NAME="SECTION00042200000000000000">
<SPAN CLASS="arabic">4</SPAN>.<SPAN CLASS="arabic">2</SPAN>.<SPAN CLASS="arabic">2</SPAN> Client unfinalized</A>
</H3>
Test the behavior of the system when the system is initialized but the program exits without calling finalize.  Another program is run after the previous program exited and all functions are tested including initialize and finalize.

<P>

<DL COMPACT>
<DT><SPAN CLASS="arabic">64</SPAN>.
<DD>Call initialize and exit the program.  Call initialize and record the return value and error codes.
<DT><SPAN CLASS="arabic">65</SPAN>.
<DD>Call initialize and exit the program.  Call initialize then lookup and record the return value and error codes.
<DT><SPAN CLASS="arabic">66</SPAN>.
<DD>Call initialize and exit the program.  Call initialize then getattr and record the return value and error codes.
<DT><SPAN CLASS="arabic">67</SPAN>.
<DD>Call initialize and exit the program.  Call initialize then setattr and record the return value and error codes.
<DT><SPAN CLASS="arabic">68</SPAN>.
<DD>Call initialize and exit the program.  Call initialize then mkdir and record the return value and error codes.
<DT><SPAN CLASS="arabic">69</SPAN>.
<DD>Call initialize and exit the program.  Call initialize then readdir and record the return value and error codes.
<DT><SPAN CLASS="arabic">70</SPAN>.
<DD>Call initialize and exit the program.  Call initialize then create and record the return value and error codes.
<DT><SPAN CLASS="arabic">71</SPAN>.
<DD>Call initialize and exit the program.  Call initialize then remove and record the return value and error codes.
<DT><SPAN CLASS="arabic">72</SPAN>.
<DD>Call initialize and exit the program.  Call initialize then rename and record the return value and error codes.
<DT><SPAN CLASS="arabic">73</SPAN>.
<DD>Call initialize and exit the program.  Call initialize then symlink and record the return value and error codes.
<DT><SPAN CLASS="arabic">74</SPAN>.
<DD>Call initialize and exit the program.  Call initialize then readlink and record the return value and error codes.
<DT><SPAN CLASS="arabic">75</SPAN>.
<DD>Call initialize and exit the program.  Call initialize then read and record the return value and error codes.
<DT><SPAN CLASS="arabic">76</SPAN>.
<DD>Call initialize and exit the program.  Call initialize then write and record the return value and error codes.
</DD>
</DL>

<P>

<H3><A NAME="SECTION00042300000000000000">
<SPAN CLASS="arabic">4</SPAN>.<SPAN CLASS="arabic">2</SPAN>.<SPAN CLASS="arabic">3</SPAN> Client finalized</A>
</H3>
The initialize function is called and immediately after the finalize function is called.  Test behavior of system functions under this scenario.

<P>

<DL COMPACT>
<DT><SPAN CLASS="arabic">77</SPAN>.
<DD>Call initialize then finalize.  Immediately after finalize, call lookup and record the return and error codes.
<DT><SPAN CLASS="arabic">78</SPAN>.
<DD>Call initialize then finalize.  Immediately after finalize, call getattr and record the return and error codes.
<DT><SPAN CLASS="arabic">79</SPAN>.
<DD>Call initialize then finalize.  Immediately after finalize, call setattr and record the return and error codes.
<DT><SPAN CLASS="arabic">80</SPAN>.
<DD>Call initialize then finalize.  Immediately after finalize, call mkdir and record the return and error codes.
<DT><SPAN CLASS="arabic">81</SPAN>.
<DD>Call initialize then finalize.  Immediately after finalize, call readdir and record the return and error codes.
<DT><SPAN CLASS="arabic">82</SPAN>.
<DD>Call initialize then finalize.  Immediately after finalize, call create and record the return and error codes.
<DT><SPAN CLASS="arabic">83</SPAN>.
<DD>Call initialize then finalize.  Immediately after finalize, call remove and record the return and error codes.
<DT><SPAN CLASS="arabic">84</SPAN>.
<DD>Call initialize then finalize.  Immediately after finalize, call rename and record the return and error codes.
<DT><SPAN CLASS="arabic">85</SPAN>.
<DD>Call initialize then finalize.  Immediately after finalize, call symlink and record the return and error codes.
<DT><SPAN CLASS="arabic">86</SPAN>.
<DD>Call initialize then finalize.  Immediately after finalize, call readlink and record the return and error codes.
<DT><SPAN CLASS="arabic">87</SPAN>.
<DD>Call initialize then finalize.  Immediately after finalize, call read and record the return and error codes.
<DT><SPAN CLASS="arabic">88</SPAN>.
<DD>Call initialize then finalize.  Immediately after finalize, call write and record the return and error codes.
</DD>
</DL>

<P>

<H3><A NAME="SECTION00042400000000000000">
<SPAN CLASS="arabic">4</SPAN>.<SPAN CLASS="arabic">2</SPAN>.<SPAN CLASS="arabic">4</SPAN> Operations on non-existent Files</A>
</H3>
Tests for functions that operate on existing files on a file that has not been created.

<P>

<DL COMPACT>
<DT><SPAN CLASS="arabic">89</SPAN>.
<DD>Call initialize then lookup on a file that has not been created.  Record the return value and error codes.
<DT><SPAN CLASS="arabic">90</SPAN>.
<DD>Call initialize then getattr on a file that has not been created.  Record the return value and error codes.
<DT><SPAN CLASS="arabic">91</SPAN>.
<DD>Call initialize then setattr on a file that has not been created.  Record the return value and error codes.
<DT><SPAN CLASS="arabic">92</SPAN>.
<DD>Call initialize then readdir on a file that has not been created.  Record the return value and error codes.
<DT><SPAN CLASS="arabic">93</SPAN>.
<DD>Call initialize then remove on a file that has not been created.  Record the return value and error codes.
<DT><SPAN CLASS="arabic">94</SPAN>.
<DD>Call initialize then rename on a file that has not been created.  Record the return value and error codes.
<DT><SPAN CLASS="arabic">95</SPAN>.
<DD>Call initialize then symlink on a file that has not been created.  Record the return value and error codes.
<DT><SPAN CLASS="arabic">96</SPAN>.
<DD>Call initialize then readlink on a file that has not been created.  Record the return value and error codes.
<DT><SPAN CLASS="arabic">97</SPAN>.
<DD>Call initialize then read on a file that has not been created.  Record the return value and error codes.
<DT><SPAN CLASS="arabic">98</SPAN>.
<DD>Call initialize then write on a file that has not been created.  Record the return value and error codes.
</DD>
</DL>

<P>

<H3><A NAME="SECTION00042500000000000000">
<SPAN CLASS="arabic">4</SPAN>.<SPAN CLASS="arabic">2</SPAN>.<SPAN CLASS="arabic">5</SPAN> Repeated Operations: meta data</A>
</H3>
Continually call functions that change the meta data of a file.

<P>

<DL COMPACT>
<DT><SPAN CLASS="arabic">99</SPAN>.
<DD>Call initialize and then call setattr on the same test file 100 times.  Record return values and error codes.
<DT><SPAN CLASS="arabic">100</SPAN>.
<DD>Call initialize and then call rename 100 times.  Record return values and error codes.
<DT><SPAN CLASS="arabic">101</SPAN>.
<DD>Call initialize and then call symlink 100 times.  Record return values and error codes.
</DD>
</DL>

<P>

<H3><A NAME="SECTION00042600000000000000">
<SPAN CLASS="arabic">4</SPAN>.<SPAN CLASS="arabic">2</SPAN>.<SPAN CLASS="arabic">6</SPAN> Repeated Operations: create</A>
</H3>
Continually call functions on one file that create new files such as mkdir and create.

<P>

<DL COMPACT>
<DT><SPAN CLASS="arabic">102</SPAN>.
<DD>Call initialize and then call mkdir on the same test file 100 times.  Record return values and error codes.
<DT><SPAN CLASS="arabic">103</SPAN>.
<DD>Call initialize and then call create on the same test file 100 times.  Record return values and error codes.
<DT><SPAN CLASS="arabic">104</SPAN>.
<DD>Call initialize and then call mkdir on differnt test files 100 times.  Record return values and error codes.
<DT><SPAN CLASS="arabic">105</SPAN>.
<DD>Call initialize and then call create on differnt test file 100 times.  Record return values and error codes.
</DD>
</DL>

<P>

<H1><A NAME="SECTION00050000000000000000">
<SPAN CLASS="arabic">5</SPAN> Results</A>
</H1>

<P>

<H1><A NAME="SECTION00060000000000000000">
About this document ...</A>
</H1>
 <STRONG>PVFS2: System Interface
<BR>
Test Suite</STRONG><P>
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
 <STRONG>latex2html</STRONG> <TT>-split 0 -show_section_numbers -nonavigation SystemInterfaceTesting.tex</TT>
<P>
The translation was initiated by Samuel Lang on 2010-02-04
<BR><HR>
<ADDRESS>
Samuel Lang
2010-02-04
</ADDRESS></table></table></table><?include("../../../../bottom.php"); ?>
</BODY>
</HTML>
