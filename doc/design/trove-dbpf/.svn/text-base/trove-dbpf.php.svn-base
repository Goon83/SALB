<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.0 Transitional//EN">

<!--Converted with LaTeX2HTML 2002-2-1 (1.71)
original version by:  Nikos Drakos, CBLU, University of Leeds
* revised and updated by:  Marcus Hennecke, Ross Moore, Herb Swan
* with significant contributions from:
  Jens Lippmann, Marek Rouchal, Martin Wilck and others -->
<HTML>
<HEAD>
<TITLE>Trove Database + Files (DBPF) Implementation</TITLE>
<META NAME="description" CONTENT="Trove Database + Files (DBPF) Implementation">
<META NAME="keywords" CONTENT="trove-dbpf">
<META NAME="resource-type" CONTENT="document">
<META NAME="distribution" CONTENT="global">

<META NAME="Generator" CONTENT="LaTeX2HTML v2002-2-1">
<META HTTP-EQUIV="Content-Style-Type" CONTENT="text/css">

<LINK REL="STYLESHEET" HREF="trove-dbpf.css">

<? include("../../../../header.php"); ?></HEAD>

<?include("../../../..//top.php"); ?> <body id="documentation">

<P>
<H1 ALIGN="LEFT">Trove Database + Files (DBPF) Implementation</H1>
<DIV CLASS="author_info">

<P ALIGN="LEFT"><STRONG>PVFS Development Team</STRONG></P>
</DIV>

<P>

<H1><A NAME="SECTION00010000000000000000">
<SPAN CLASS="arabic">1</SPAN> Introduction</A>
</H1>

<P>
The purpose of this document is to sketch out the Database + Files (DBPF)
implementation of the Trove storage interface.  Included will be discussion of
how one might use the implementation in practice.

<P>
DBPF uses UNIX files and Berkeley DB (3 or 4 I think) databases to store file
and directory data and metadata.

<P>

<H1><A NAME="SECTION00020000000000000000">
<SPAN CLASS="arabic">2</SPAN> Entities on Physical Storage</A>
</H1>

<P>
The locations of these entities are defined in dbpf.h, although in future
versions they should be relative to the storage name passed in at initialize time.

<P>
For a given server there are single instances of each of the following:

<UL>
<LI>storage attributes DB - holds attributes on the storage space as a whole
</LI>
<LI>collections DB - holds information on collections and is used to map
      from a collection name to a collection ID
</LI>
</UL>

<P>
For each collection there are one of each of the following:

<UL>
<LI>collection attributes DB - holds attributes for the collection as a
      whole, including handle usage information and root handle
</LI>
<LI>dataspace attributes DB - holds dbpf_dspace_attr structure for each
      dataspace, referenced by handle
</LI>
</UL>

<P>
In addition, dataspaces have a one to one mapping to the following, although
they are created lazily (so they will not exist if not yet used):

<UL>
<LI>bstream file - UNIX files that holds bstream data, just as in the PVFS1 iod
</LI>
<LI>keyval DB - holds keyvals for a specific dataspace
</LI>
</UL>

<P>
At this time both bstream files and keyval DBs are stored in a flat directory
structure.  This may change to be a hashed directory system as in the PVFS1
iod if/when we see that we are incurring substantial overhead from lookups in
a directory with many entries.

<P>

<H1><A NAME="SECTION00030000000000000000">
<SPAN CLASS="arabic">3</SPAN> Hooking to Upper Trove Layer</A>
</H1>

<P>
The DBPF implementation hooks to the upper trove ``wrapper'' layer through a
set of structures that contain pointers to the functions that make up the
Trove API.  These structures are defined in the upper-layer trove.h and are
TROVE_bstream_ops, TROVE_keyval_ops, TROVE_dspace_ops, TROVE_mgmt_ops,
and TROVE_fs_ops.

<P>
The TROVE_mgmt_ops structure includes a pointer to the function initialize().
This is used to initialize the underlying Trove implementation (e.g. DBPF).
The initialize function takes the storage name as a parameter; this should be
used to locate the various entities that are used for storing the DBPF
collections.

<P>
<SPAN  CLASS="textit">Additionally a method ID is passed in...why???</SPAN>

<P>
The method name field is filled in by the DBPF implementation to describe what
type of underlying storage is available.

<P>

<H1><A NAME="SECTION00040000000000000000">
<SPAN CLASS="arabic">4</SPAN> Operation Queue</A>
</H1>

<P>
The DBPF implementation has its own queue that it uses to hold operations in
progress.  Operations may have one of a number of states (defined in dbpf.h):

<UL>
<LI>OP_UNITIALIZED
</LI>
<LI>OP_NOT_QUEUED
</LI>
<LI>OP_QUEUED
</LI>
<LI>OP_IN_SERVICE
</LI>
<LI>OP_COMPLETED
</LI>
<LI>OP_DEQUEUED
</LI>
</UL>

<P>

<H1><A NAME="SECTION00050000000000000000">
<SPAN CLASS="arabic">5</SPAN> Dataspaces Implementation</A>
</H1>

<P>
Dataspaces are stored as:

<UL>
<LI>an entry in the dataspace DB
</LI>
<LI>zero or one keyval DBs (zero if no keyvals have been set)
</LI>
<LI>zero or one bstream UNIX files (zero if no data has been written to the
      bstream)
</LI>
</UL>

<P>
Keyval and bstream files are named by the handle (because they don't really
have names...).

<P>

<H1><A NAME="SECTION00060000000000000000">
<SPAN CLASS="arabic">6</SPAN> Keyval Spaces Implementation</A>
</H1>

<P>
Currently all keyval space operations use blocking DB calls.  They queue the
operation when the I/O call is made, and they service the operation when the
service function is called (at test time).

<P>
On each call the database is opened and closed, rather than caching the FD.
This will obviously need to change in the near future; probably we'll do
something similar to the bstream fdcache.

<P>
The dbpf_keyval_iterate_op_svc function walks through all the keyval
pairs.  The current implementation uses the btree format with record
numbers to make it easy to pick up where the last call to the iterator
function left off. The iterator function will process <SPAN  CLASS="textit">count</SPAN> items per
call: if fewer than <SPAN  CLASS="textit">count</SPAN> items are left in the database, we set
<SPAN  CLASS="textit">count</SPAN> to how many items were processed.  The caller should check that
<SPAN  CLASS="textit">count</SPAN> has the same value as before the function was called. 

<P>

<H1><A NAME="SECTION00070000000000000000">
<SPAN CLASS="arabic">7</SPAN> Bytestreams Implementation</A>
</H1>

<P>
The read_at and write_at functions are implemented using blocking UNIX file
I/O calls.  They queue the operation when the I/O call is made, and they
service the operation when the service function is called (at test time).

<P>
The read_list and write_list functions are implemented using lio_listio.
Both of these functions actually call a function dbpf_bstream_rw_list,
which implements the necessary functionality for both reads and writes.  This
function sets up the queue entry and immediately calls the service function.

<P>
The necessary aiocb array is allocated within the service function if one
isn't already allocated for the operation.  The maximum number of aiocb
entries is controlled by a compile-time constant AIOCB_ARRAY_SZ; if there
are more list elements than this, the list will be split up and handled by
multiple lio_listio calls.

<P>
If the service function returns to dbpf_bstream_rw_list without completing
the entire operation, the operation is queued.

<P>
Both the _at and _list functions use a FD cache for getting open FDs.

<P>

<H1><A NAME="SECTION00080000000000000000">
<SPAN CLASS="arabic">8</SPAN> Creating a ``File System''</A>
</H1>

<P>
Theoretically Trove in general doesn't know anything about ``file systems''
per se.  However, it's helpful for us to provide functions intended to ease
the creation of FSs in Trove.  These functions are subject to change or
removal as we get a better understanding of how we're going to use Trove.

<P>
There's more than one way to build a file system given the infrastructure
provided by Trove; we're just going through a single example here.

<P>
At this time, a single file system is associated with one and only one
collection, and a single collection is associated with one and only one file
system.

<P>

<H2><A NAME="SECTION00081000000000000000">
<SPAN CLASS="arabic">8</SPAN>.<SPAN CLASS="arabic">1</SPAN> Creating a Storage Space</A>
</H2>
First a storage space must be created.  Storage spaces are created with the
storage_create mgmt operation.  <SPAN  CLASS="textit">This function can be used without calling the
initialize function?</SPAN>  This function creates the storage attributes database
and the collections database.

<P>

<H2><A NAME="SECTION00082000000000000000">
<SPAN CLASS="arabic">8</SPAN>.<SPAN CLASS="arabic">2</SPAN> Creating a Collection</A>
</H2>
Given that a storage space has been created, the next thing to do is create a
collection.  This is done with the collection_create mgmt operation.  This
function takes a collection name and an associated collection ID.  First it
looks up the storage space (which currently is done by mapping to the single
possible storage space).  Next it checks to see that a collection with that
name does not already exist (and errors out if it does).  Next it will
create the collection by writing a new entry into the collections DB.
Following this it will create a dataspace attributes database and the
directories for keyval and bstream spaces for the collection.  Then it creates
the collection attributes database and puts a last handle value into it, which
is used in subsequent dataspace creates.

<P>
Once a collection is created, it can be looked up with collection_lookup.
Collections are looked up by name.  This will return collection ID that can be
used to direct subsequent operations to the collection.  Currently the
implementation only supports creation of one collection.

<P>
<SPAN  CLASS="textit"> The collection lookup routine opens the collection attribute database and
finds the root handle.  This really isn't right...the collection routines
don't need to know about this...the fs routines should instead.</SPAN>

<P>
It also opens the dataspace database and returns the collection ID.

<P>
In addition to creating one collection for the file system, a second
``administrative'' collection will be created.  Currently, the handle allocator
uses the admin collection to store handle state in a bstream. 

<P>

<H2><A NAME="SECTION00083000000000000000">
<SPAN CLASS="arabic">8</SPAN>.<SPAN CLASS="arabic">3</SPAN> Creating a Root Directory</A>
</H2>
There is a collection of file system helper functions too...

<P>

<H2><A NAME="SECTION00084000000000000000">
<SPAN CLASS="arabic">8</SPAN>.<SPAN CLASS="arabic">4</SPAN> Creating Dataspaces</A>
</H2>
At this point dataspaces can be created with dspace_create.

<P>
The dataspace create code checks to see if the handle is in use.
<SPAN  CLASS="textit">Currently this code will not try to come up with a new handle if the
proposed one is used...fix?</SPAN>

<P>
An entry is placed in the dataspace attribute DB for the collection.

<P>
<SPAN  CLASS="textit">What is the ``type'' field used for?</SPAN>

<P>
I think that the basic metadata for a file will be stored in the dataspace
attribute DB by adding members to the dbpf_dspace_attr structure (defined in dbpf.h).

<P>

<H3><A NAME="SECTION00084100000000000000">
<SPAN CLASS="arabic">8</SPAN>.<SPAN CLASS="arabic">4</SPAN>.<SPAN CLASS="arabic">1</SPAN> Creating a Directory</A>
</H3>

<P>
First we create a dataspace.  Then we add a key/value to map the name of the
directory to the handle into the parent directory keyval space.

<P>

<H3><A NAME="SECTION00084200000000000000">
<SPAN CLASS="arabic">8</SPAN>.<SPAN CLASS="arabic">4</SPAN>.<SPAN CLASS="arabic">2</SPAN> Creating a File</A>
</H3>

<P>
First we create a dataspace.  Then we add a key/value to map the name to the
handle into the parent directory keyval space.

<P>

<H3><A NAME="SECTION00084300000000000000">
<SPAN CLASS="arabic">8</SPAN>.<SPAN CLASS="arabic">4</SPAN>.<SPAN CLASS="arabic">3</SPAN> Deleting a Directory</A>
</H3>
We check to make sure the ``directory'' has no elements in it.  Then we remove
the key/value mapping the name of the directory to the handle from the parent
directory keyval space.  After removing the key/value, we mark the dataspace as
ready for deletion. 

<P>

<H3><A NAME="SECTION00084400000000000000">
<SPAN CLASS="arabic">8</SPAN>.<SPAN CLASS="arabic">4</SPAN>.<SPAN CLASS="arabic">4</SPAN> Deleting a File</A>
</H3>
Given a file name, we retrieve the corresponding handle from the parent
directory keyval space.  Entries in the parent directory keyval space map the
name of the file to a handle - the handle of the dataspace for that keyval
space. We remove the key/value for the given name.  We then use the handle we
retrieved to find the dataspace and mark it as ready for deletion. 

<P>

<H1><A NAME="SECTION00090000000000000000">
<SPAN CLASS="arabic">9</SPAN> Current Deficiencies</A>
</H1>

<P>
Only one collection can be used with the current implementation.

<P>
Error checking is weak at best.  Assert(0)s are used in many places in error conditions.

<P>

<H1><A NAME="SECTION000100000000000000000">
About this document ...</A>
</H1>
 <STRONG>Trove Database + Files (DBPF) Implementation</STRONG><P>
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
 <STRONG>latex2html</STRONG> <TT>-split 0 -show_section_numbers -nonavigation trove-dbpf.tex</TT>
<P>
The translation was initiated by Samuel Lang on 2010-02-04
<BR><HR>
<ADDRESS>
Samuel Lang
2010-02-04
</ADDRESS></table></table></table><?include("../../../../bottom.php"); ?>
</BODY>
</HTML>
