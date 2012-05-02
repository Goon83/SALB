<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.0 Transitional//EN">

<!--Converted with LaTeX2HTML 2002-2-1 (1.71)
original version by:  Nikos Drakos, CBLU, University of Leeds
* revised and updated by:  Marcus Hennecke, Ross Moore, Herb Swan
* with significant contributions from:
  Jens Lippmann, Marek Rouchal, Martin Wilck and others -->
<HTML>
<HEAD>
<TITLE>Trove: The PVFS2 Storage Interface</TITLE>
<META NAME="description" CONTENT="Trove: The PVFS2 Storage Interface">
<META NAME="keywords" CONTENT="storage-interface">
<META NAME="resource-type" CONTENT="document">
<META NAME="distribution" CONTENT="global">

<META NAME="Generator" CONTENT="LaTeX2HTML v2002-2-1">
<META HTTP-EQUIV="Content-Style-Type" CONTENT="text/css">

<LINK REL="STYLESHEET" HREF="storage-interface.css">

<? include("../../../../header.php"); ?></HEAD>

<?include("../../../..//top.php"); ?> <body id="documentation">

<P>
<H1 ALIGN="LEFT">Trove: The PVFS2 Storage Interface</H1>
<DIV CLASS="author_info">

<P ALIGN="LEFT"><STRONG>PVFS Development Team</STRONG></P>
</DIV>

<P>

<H1><A NAME="SECTION00010000000000000000">
<SPAN CLASS="arabic">1</SPAN> Motivation and Goals</A>
</H1>

<P>
The Trove storage interface will be the lowest level interface used by the
PVFS server for storing both file data and metadata.  It will be used by
individual servers (and servers only) to keep track of locally stored
information.  There are several goals and ideas that we should keep in
mind when discussing this interface:

<P>

<UL>
<LI><SPAN  CLASS="textbf">Multiple storage instances</SPAN>: This interface is intended to
hide the use of multiple storage instances for storage of data.  This
data can be roughly categorized into two types, bytestream and keyval
spaces, which are described in further detail below.

<P>
</LI>
<LI><SPAN  CLASS="textbf">Contiguous and noncontiguous data access</SPAN>:  The first cut
of this interface will probably only handle contiguous data access.
However, we would like to also support some form of noncontiguous access.  We
think that this will be done through list I/O type operations, as we don't
necessarily want anything more complicated at this level.

<P>
</LI>
<LI><SPAN  CLASS="textbf">Metadata storage</SPAN>:  This
interface will be used as a building block for storing metadata in 
addition to file data.  This includes extended metadata.

<P>
</LI>
<LI><SPAN  CLASS="textbf">Nonblocking semantics</SPAN>:  This interface will be completely
nonblocking for both file data and metadata operations.  The usual
argument for scalability and flexible interaction with other I/O
devices applies here.  We should try to provide this
functionality without sacrificing latency if possible.
<SPAN  CLASS="textit">The interface will not require interface calls to be made in order
for progress to occur.</SPAN>  This implies that threads will be used
underneath where necessary.

<P>
</LI>
<LI><SPAN  CLASS="textbf">Compatibility with flows</SPAN>:  Flows will almost certainly be
built on top of this interface.  Both the default BMI flow
implementation and custom implementations should be able to use this
interface.

<P>
</LI>
<LI><SPAN  CLASS="textbf">Consistency semantics</SPAN>:  If we are going to support
consistency, locking, etc, then we need to be able to enforce
consistency semantics at the storage interface level.  The interface
will provide the option for serializing access to a dataspace and a vtag
interface.

<P>
</LI>
<LI><SPAN  CLASS="textbf">Error recovery</SPAN>:  The system must detect and report errors
occurring while accessing data storage.  The system may or may not
implement redundancy, journaling, etc. for recovering from errors
resulting in data loss.
</LI>
</UL>

<P>
Our first cut implementation of this interface will have the following
restrictions:

<UL>
<LI>only one type of storage for bytestreams and one type for keyvals
      will be supported
</LI>
<LI>consistency semantics will not be implemented
</LI>
<LI>errors will be reported, but no measures will be taken to recover
</LI>
<LI>noncontiguous access will not be enabled
</LI>
<LI>only one process/thread will be accessing a given storage instance
      through this interface at a time
</LI>
</UL>

<P>
<SPAN  CLASS="textit">PARTIAL COMPLETION SEMANTICS NEED MUCH WORK!!!</SPAN>

<P>

<H1><A NAME="SECTION00020000000000000000">
<SPAN CLASS="arabic">2</SPAN> Storage space concepts</A>
</H1>

<P>
A server controls one storage space.

<P>
Within this storage space are some number of <SPAN  CLASS="textit">collections</SPAN>, which
are akin to file systems.  Collections serve as a mechanism for
supporting multiple traditional file systems on a single server and for
separating the use of various physical resources.  (Collections can span
multiple underlying storage devices, and hints would be used in that case to
specify the device on which to place files.  This concept might be used in
systems that can migrate data from slow storage to faster storage as well).

<P>
Two collections will be created for each file system: one collection will
support the dataspaces needed for the file system's data and metadata objects.
A second collection will be created for administrative purposes.  If the
underlying implementation needs to perform disk i/o, for example, it can use
bstream and keyval objects from the administration collection. 

<P>
A collection id will be used in conjunction with other parameters in
order to specify a unique entity on a server to access or modify, just as a
file system ID might be used.

<P>

<H1><A NAME="SECTION00030000000000000000">
<SPAN CLASS="arabic">3</SPAN> Dataspace concepts</A>
</H1>

<P>
This storage interface stores and accesses what we will call
<SPAN  CLASS="textit">dataspaces</SPAN>.  These are logical collections of data organized in one
of two possible ways.  The first organization for a dataspace is the
traditional ``byte stream''.  This term refers to arbitrary binary data
that can be referenced using offsets and sizes.  The second organization
is ``keyword/value'' data.  This term refers to information that is
accessed in a simplified database-like manner.  The data is indexed by way of a
variable length key rather than an offset and size.  Both keyword and
value are arbitrary byte arrays with a length parameter (i.e.&nbsp; need not
be readable strings).  We will refer to a
dataspace organized as a byte stream as a bytestream dataspace or simply
a <SPAN  CLASS="textit">bytestream space</SPAN>, and a dataspace organized by keyword/value
pairs as a keyval dataspace or <SPAN  CLASS="textit">keyval space</SPAN>.
Each dataspace will have an identifier that is unique to its server,
which we will simply call a <SPAN  CLASS="textit">handle</SPAN>.  Physically these dataspaces
may be stored in any number of ways on underlying storage.

<P>
Here are some potential uses of each type:

<UL>
<LI>Byte stream

<UL>
<LI>traditional file data
</LI>
<LI>binary metadata storage (as is currently done in PVFS 1)
</LI>
</UL>
</LI>
<LI>Key/value

<UL>
<LI>extended metadata attributes 
</LI>
<LI>directory entries
</LI>
</UL>
</LI>
</UL>

<P>
In our design thus far (reference the system interface documents) we have
defined four types of <SPAN  CLASS="textit">system level objects</SPAN>.  These are data files,
metadata files, directories, and symlinks.  All four of these will be
implemented using a combination of bytestream and/or keyval dataspaces.
At the storage interface level there is no real distinction between
different types of system level objects.

<P>

<H1><A NAME="SECTION00040000000000000000">
<SPAN CLASS="arabic">4</SPAN> Vtag concepts</A>
</H1>

<P>
Vtags are a capability that can be used to implement atomic updates in
shared storage systems.  In this case they can be used to implement
atomic access to a set of shared storage devices through the storage
interface.  To clarify, these would be of particular use when multiple
threads are using the storage interface to access local storage or when
multiple servers are accessing shared storage devices such as a MySQL
database or SAN storage.

<P>
This section can be skipped if you are not interested in consistency
semantics.  Vtags will probably not be implemented in the first cut
anyway.

<P>

<H2><A NAME="SECTION00041000000000000000">
<SPAN CLASS="arabic">4</SPAN>.<SPAN CLASS="arabic">1</SPAN> Phil's poor explanation</A>
</H2>

<P>
Vtags are an approach to ensuring consistency for multiple readers
and writers that avoids the use of locks and their associated problems
within a distributed environment.  These problems include complexity,
poor performance in the general case, and awkward error recovery.

<P>
A vtag fundamentally provides a version number for any region of a byte
stream or any individual key/value pair.  This allows the implementation
of an optimistic approach to consistency.  Take the example of a
read-modify-write operation.  The caller first reads a data region,
obtaining a version tag in the process.  It then modifies it's own copy
of the data.  When it writes the data back, it gives the vtag back to
the storage interface.  The storage interface compares the given vtag against
the current vtag for the region.  If the vtags match, it indicates that
the data has not been modified since it was read by the caller, and the
operation succeeds.  If the vtags do not match, then the operation fails
and the caller must retry the operation.

<P>
This is an optimistic approach in that the caller always assumes that
the region has not been modified.  

<P>
Many different locking primitives can be built upon the vtag concept...

<P>

<H2><A NAME="SECTION00042000000000000000">
<SPAN CLASS="arabic">4</SPAN>.<SPAN CLASS="arabic">2</SPAN> Use of vtags</A>
</H2>

<P>
Layers above trove can take advantage of vtags as a way to simplify the
enforcement of consistency semantics (rather than keeping complicated lists of
concurrent operations, simply use the vtag facility to ensure that operations
occur atomically).  Alternatively they could be used to handle the case of
trove resources shared by multiple upper layers.  Finally they might be used
in conjunction with higher level consistency control in some complimentary
fashion (dunno yet...).

<P>

<H1><A NAME="SECTION00050000000000000000">
<SPAN CLASS="arabic">5</SPAN> The storage interface</A>
</H1>

<P>
In this section we describe all the functions that make up the storage
interface.  The storage interface functions can be divided into four
categories: dataspace management functions, bytestream access functions,
keyval access functions, and completion test functions.  The access
functions can be further subdivided into contiguous and noncontiguous
access capabilities.

<P>
First we describe the return values and error values for the interface.
Then we describe special vtag values and the implementation of keys.
Next we describe the dataspace management functions.  Next we
describe the contiguous and noncontiguous dataspace access functions.
Finally we cover the completion test functions.

<P>

<H2><A NAME="SECTION00051000000000000000">
<SPAN CLASS="arabic">5</SPAN>.<SPAN CLASS="arabic">1</SPAN> Return values</A>
</H2>

<P>
Unless otherwise noted, all functions return an integer with three
possible values:

<UL>
<LI>0: Success.  If the operation was nonblocking, then this return
  value indicates the caller must test for completion later.
</LI>
<LI>1: Success with immediate completion.  No later testing is required, and
  no handle is returned for use in testing.
</LI>
<LI>-errno: Failure.  The error code is encoded in the negative return
value.
</LI>
</UL>

<P>

<H2><A NAME="SECTION00052000000000000000">
<SPAN CLASS="arabic">5</SPAN>.<SPAN CLASS="arabic">2</SPAN> Error values</A>
</H2>

<P>
Table&nbsp;<A HREF="#table:storage_errors">1</A> shows values.  All values will be returned as
integers in the native format (size and byte order).

<P>
<SPAN  CLASS="textit">Needs to be fleshed out.  Need to pick a reasonable prefix.</SPAN>

<P>
<SPAN  CLASS="textit">Phil:  Once this is fleshed out, can we apply the same sort of
scheme to BMI?  BMI doesn't have a particularly informative error
reporting mechanism.</SPAN> 

<P>
<SPAN  CLASS="textit">Rob: Definitely.  I would really like to make sure that in
addition to getting error values back, the error values actually make
sense :).  This was (and still is in some cases) a real problem for
PVFS1.</SPAN>

<P>
<BR><P></P>
<DIV ALIGN="LEFT">

<DIV ALIGN="LEFT">
<A NAME="63"></A>
<TABLE CELLPADDING=3 BORDER="1">
<CAPTION><STRONG>Table 1:</STRONG>
Error values for storage interface</CAPTION>
<TR><TD ALIGN="LEFT">Value</TD>
<TD ALIGN="LEFT">Meaning</TD>
</TR>
<TR><TD ALIGN="LEFT">TROVE_ENOENT</TD>
<TD ALIGN="LEFT">no such dataspace</TD>
</TR>
<TR><TD ALIGN="LEFT">TROVE_EIO</TD>
<TD ALIGN="LEFT">I/O error</TD>
</TR>
<TR><TD ALIGN="LEFT">TROVE_ENOSPC</TD>
<TD ALIGN="LEFT">no space on storage device</TD>
</TR>
<TR><TD ALIGN="LEFT">TROVE_EVTAG</TD>
<TD ALIGN="LEFT">vtag didn't match</TD>
</TR>
<TR><TD ALIGN="LEFT">TROVE_ENOMEM</TD>
<TD ALIGN="LEFT">unable to allocate memory for operation</TD>
</TR>
<TR><TD ALIGN="LEFT">TROVE_EINVAL</TD>
<TD ALIGN="LEFT">invalid input parameter</TD>
</TR>
</TABLE>
</DIV>
<A NAME="table:storage_errors"></A>
</DIV>
<BR>

<P>

<H2><A NAME="SECTION00053000000000000000">
<SPAN CLASS="arabic">5</SPAN>.<SPAN CLASS="arabic">3</SPAN> Flags related to vtags</A>
</H2>

<P>
As mentioned earlier, the usage of vtags is not manditory.  Therefore we
define two flags values that can be used to control the behavior of the calls
with respect to vtags:

<P>
<SPAN  CLASS="textit">TODO: pick a reasonable prefix for our flags.</SPAN>

<P>

<UL>
<LI><SPAN  CLASS="textbf">FLAG_VTAG</SPAN>:  Indicates that the vtag is valid.
The caller does not have a valid vtag for input, nor does he desire a
valid vtag in response.
</LI>
<LI><SPAN  CLASS="textbf">FLAG_VTAG_RETURN</SPAN>:  Indicates that the caller wishes to obtain
a vtag from the operation.  However, the caller does not wish to use a
vtag for input.  
</LI>
</UL>

<P>
By default calls ignore vtag values on input and do not create vtag values for output.

<P>

<H2><A NAME="SECTION00054000000000000000">
<SPAN CLASS="arabic">5</SPAN>.<SPAN CLASS="arabic">4</SPAN> Implementation of keys, values, and hints</A>
</H2>

<P>
<SPAN  CLASS="textit">TODO: sync. with code on data_sz element.</SPAN>

<P>
<PRE>
struct TROVE_keyval {
    void *  buffer;
    int32_t buffer_sz;
    int32_t data_sz;
};
typedef struct TROVE_keyval TROVE_keyval_s;
</PRE>

<P>
Keys, values, and hints are all implemented with the same TROVE_keyval
structure (do we want a different name?), shown above.  Keys and values used
in keyval spaces are arbitrary binary data values with an associated length.

<P>
Hint keys and values have the additional constraint of being null-terminated,
readable strings.  This makes them very similar to MPI_Info key/value pairs.

<P>
<SPAN  CLASS="textit">TODO: we should build hints out of a pair of the TROVE_keyvals.  We'll
call them a TROVE_hint_s in here for now.</SPAN>

<P>

<H2><A NAME="SECTION00055000000000000000">
<SPAN CLASS="arabic">5</SPAN>.<SPAN CLASS="arabic">5</SPAN> Functions</A>
</H2>

<P>
<SPAN  CLASS="textit">Note: need to add valid error values for each function.</SPAN>

<P>
<SPAN  CLASS="textit">TODO: find a better format for function descriptions.</SPAN>

<P>

<H3><A NAME="SECTION00055100000000000000">
<SPAN CLASS="arabic">5</SPAN>.<SPAN CLASS="arabic">5</SPAN>.<SPAN CLASS="arabic">1</SPAN> IDs</A>
</H3>

<P>
In this context, IDs are unique identifiers assigned to each storage interface
operation.  They are used as handles to test for completion of operations once
they have been submitted.  If an operation completes immediately, then the ID
field should be ignored.

<P>
These IDs are only unique in the context of the storage interface, so upper
layers may have to handle management of multiple ID spaces (if working with
both a storage interface and a network interface, for instance).

<P>
The type for these IDs is TROVE_op_id.

<P>

<H3><A NAME="SECTION00055200000000000000">
<SPAN CLASS="arabic">5</SPAN>.<SPAN CLASS="arabic">5</SPAN>.<SPAN CLASS="arabic">2</SPAN> User pointers</A>
</H3>

<P>
Each function allows the user to pass in a pointer value (void *).  This value
is returned by the test functions, and it allows for quick reference to user
data structures associated with the completed operation.

<P>
To motivate, normally there is some data at the caller's level that
corresponds with the trove operation.  Without some help, the caller would
have to map IDs for completed operations back to the caller data structures
manually.  By providing a parameter that the caller can pass in, they can
directly reference these structures on trove operation completion.

<P>

<H3><A NAME="SECTION00055300000000000000">
<SPAN CLASS="arabic">5</SPAN>.<SPAN CLASS="arabic">5</SPAN>.<SPAN CLASS="arabic">3</SPAN> Dataspace management</A>
</H3>

<UL>
<LI><SPAN  CLASS="textbf">ds_create(
[in]coll_id,
[in/out]handle,
[in]bitmask, 
[in]type,
[in/out]hint,
[in]user_ptr,
[out]id
)</SPAN>:
Creates a new storage interface object.  The interface will
fill any any portion of the handle that is not already filled in and
ensure that it is unique.  For example, if the caller wants to
specify the first 16 bits of the handle, it may do so by setting the
appropriate bits and then specifying with the bitmask that the storage
interface should not modify those bits.  

<P>
The type field can be used by the caller to assign an arbitrary integer
type to the object.  This may, for example, be used to distinguish
between directories, symlinks, datafiles, and metadata files.  The storage
interface does not assign any meaning to the type value. <SPAN  CLASS="textit">Do we even need
this type field?</SPAN>

<P>
The hint field may be used to specify what type of underlying storage
should be used for this dataspace in the case where multiple potential
underlying storage methods are available.

<P>
</LI>
<LI><SPAN  CLASS="textbf">ds_remove(
[in]handle, 
[in]user_ptr,
[out]id
)</SPAN>:
Removes an existing object from the system.

<P>
</LI>
<LI><SPAN  CLASS="textbf">ds_verify(
[in]coll_id,
[in]handle,
[out]type,
[in]user_ptr,
[out]id
)</SPAN>:
Verifies that an object exists with the specified handle.  If the object
does exist, then the type of the object is also returned.  Useful for
verifying sanity of handles provided by client.

<P>
</LI>
<LI><SPAN  CLASS="textbf">ds_getattr(
[in]coll_id,
[in]handle,
[out]ds_attr,
[in]user_ptr,
[out]id
)</SPAN>:
Obtains statistics about the given dataspace that aren't actually stored
within the dataspace.  This may include information such as number of
key/value pairs, size of byte stream, access statistics, on what medium
it is stored, etc.

<P>
</LI>
<LI><SPAN  CLASS="textbf">ds_setattr() ???</SPAN>

<P>
</LI>
<LI><SPAN  CLASS="textbf">ds_hint(
[in]coll_id,
[in]handle,
[in/out]hint
);</SPAN>:
Passes a hint to the underlying trove implementation.  Used to indicate
caching needs, access patterns, begin/end of use, etc.

<P>
</LI>
<LI><SPAN  CLASS="textbf">ds_migrate(
[in]coll_id,
[in]handle,
[in/out]hint,
[in]user_ptr,
[out]id
);</SPAN>:
Used to indicate that a dataspace should be migrated to another medium.
<SPAN  CLASS="textit">could this be done with just the hint call?  having an id in this case
is particularly useful ... so we know the operation is completed...</SPAN>
</LI>
</UL>

<P>

<H3><A NAME="SECTION00055400000000000000">
<SPAN CLASS="arabic">5</SPAN>.<SPAN CLASS="arabic">5</SPAN>.<SPAN CLASS="arabic">4</SPAN> Byte stream access</A>
</H3>

<P>
Parameters in read and write at calls are ordered similarly to pread and pwrite.

<P>

<UL>
<LI><SPAN  CLASS="textbf">bstream_read_at(
[in]coll_id,
[in]handle, 
[in]buffer,
[in]size,
[in]offset,
[in]flags,
[out]vtag,
[in]user_ptr,
[out]id
)</SPAN>:
Reads a contiguous region from bytestream.  Most of the arguments are self
explanatory.  The flags are not yet defined, but may include such
possibilities as specifying atomic operations.  The vtag returned from this
function applies to the region of the byte stream defined by the requested
offset and size.  <SPAN ID="txt102">A flag can be passed in if the caller does not
want a vtag returned.</SPAN>  This allows the underlying implementation to avoid the
overhead of calculating the value.

<P>
<SPAN  CLASS="textit">The size is [in/out] in code?  Figure out semantics!!!</SPAN>

<P>
</LI>
<LI><SPAN  CLASS="textbf">bstream_write_at(
[in]coll_id,
[in]handle,
[in]buffer,
[in]size,
[in]offset,
[in]flags,
[in/out]vtag,
[in]user_ptr,
[out]id
)</SPAN>:

<P>
Writes a contiguous region to the bytestream.  Same arguments as
read_bytestream, except that the vtag is an in/out parameter.

<P>
<SPAN  CLASS="textit">The size is [in/out] in code?  Figure out semantics!!!</SPAN>

<P>
</LI>
<LI><SPAN  CLASS="textbf">bstream_resize(
[in]coll_id,
[in]handle,
[in]size,
[in]flags,
[in/out]vtag,
[in]user_ptr,
[out]id
)</SPAN>:
Used to truncate or allocate storage for a bytestream.  Flags are used
to specify if preallocation is desired.

<P>
</LI>
<LI><SPAN  CLASS="textbf">bstream_validate(
[in]handle,
[in/out]vtag,
[in]user_ptr,
[out]id
)</SPAN>:
This function may be used to check for modification of a particular
bytestream.  

<P>
<SPAN  CLASS="textit">Flags?</SPAN>

<P>
</LI>
</UL>

<P>

<H3><A NAME="SECTION00055500000000000000">
<SPAN CLASS="arabic">5</SPAN>.<SPAN CLASS="arabic">5</SPAN>.<SPAN CLASS="arabic">5</SPAN> Key/value access</A>
</H3>

<P>
An important call for keyval spaces is the iterator function.  The
iterator function is used to obtain all keyword/value pairs from the
keyval space with a sequence of calls from the client.  The iterator
function returns a logical, opaque ``position'' value that allows a
client to continue reading pairs from the keyval space where it last
left off.

<P>

<UL>
<LI><SPAN  CLASS="textbf">keyval_read(
[in]coll_id,
[in]handle,
[in]key,
[out]val,
[in]flags,
[out]vtag,
[in]user_ptr,
[out]id
)</SPAN>:
Reads the value corresponding to a given key.  Fails if the key does
not exist.  A buffer is provided for the value to be placed in (the
value may be an arbitrary type).  

<P>
The amount of data actually placed in the value buffer should be indicated by
the data_sz element of the structure.

<P>
</LI>
<LI><SPAN  CLASS="textbf">keyval_write(
[in]coll_id,
[in]handle,
[in]key,
[in]val,
[in]flags,
[in/out]vtag,
[in]user_ptr,
[out]id
)</SPAN>:
Writes out a value for a given key.  If the key does not exist, it is
added.  
</LI>
<LI><SPAN  CLASS="textbf">keyval_remove(
[in]coll_id,
[in]handle,
[in]key,
[in]flags,
[in/out]vtag,
[in]user_ptr,
[out]id
)</SPAN>:
Removes a key/value pair from the keyval data space.

<P>
</LI>
<LI><SPAN  CLASS="textbf">keyval_validate(
[in]coll_id,
[in]handle,
[in/out]vtag,
[in]user_ptr,
[out]id
)</SPAN>:
Used to check for modification of a particular key/value pair.
</LI>
<LI><SPAN  CLASS="textbf">keyval_iterate(
[in]coll_id,
[in]handle,
[in/out]position, 
[out]key_array,
[out]val_array,
[in/out]count,
[in]flags,
[in/out]vtag,
[in]user_ptr,
[out]id
)</SPAN>:
Reads count keyword/value pairs from the provided logical position in
the keyval space.  Fails if the vtag doesn't match.  The position
SI_START_POSITION is used to start at the beginning, and a new
position is returned allowing the caller to continue where they left
off.

<P>
keyval_iterate will always read <SPAN  CLASS="textit">count</SPAN> items, unless it hits the end
of the keyval space (EOK).  After hitting EOK, <SPAN  CLASS="textit">count</SPAN> will be set to
the number of pairs processed.  Thus, callers must compare <SPAN  CLASS="textit">count</SPAN>
after calling and compare with the value it had before the function call:
if they are different, EOK has been reached.  If there are N items left in
the keyspace, and keyval_iterate requests N items, there will be no
indication that EOK has been reached and only after making another call
will the caller know he is at EOK.  The value of <SPAN  CLASS="textit">position</SPAN> is not
meaningful after reaching EOK.

<P>
</LI>
<LI><SPAN  CLASS="textbf">keyval_iterate_keys(
[in]coll_id,
[in]handle,
[in/out]position, 
[out]key_array,
[in]count,
[in]flags,
[in/out]vtag,
[in]user_ptr,
[out]id
)</SPAN>:
Similar to above, but only returns keys, not corresponding values. <SPAN  CLASS="textit">need
to fix parameters</SPAN>
</LI>
</UL>

<P>

<H3><A NAME="SECTION00055600000000000000">
<SPAN CLASS="arabic">5</SPAN>.<SPAN CLASS="arabic">5</SPAN>.<SPAN CLASS="arabic">6</SPAN> Noncontiguous (list) access</A>
</H3>
These functions are used to read noncontiguous byte stream regions or
multiple key/value pairs.

<P>
<SPAN  CLASS="textit">How do vtags work with noncontiguous calls?</SPAN>

<P>
The byte stream functions will implement simple listio style
noncontiguous access.  Any more advanced data types should be unrolled
into flat regions before reaching this interface.  The process for unrolling
is outside the scope of this document, but examples are available in the ROMIO code.

<P>
<SPAN  CLASS="textit">TODO: SEMANTICS!!!!!</SPAN>

<P>
<SPAN  CLASS="textit">TODO: how to we report partial success for listio calls?</SPAN>

<P>

<UL>
<LI><SPAN  CLASS="textbf">bstream_read_list(
[in]coll_id,
[in]handle,
[in]mem_offset_array,
[in]mem_size_array,
[in]mem_count,
[in]stream_offset_array,
[in]stream_size_array,
[in]stream_count,
[in]flags,
[out]vtag(?),
[in]user_ptr,
[out]id
)</SPAN>:

<P>
</LI>
<LI><SPAN  CLASS="textbf">bstream_write_list(
[in]coll_id,
[in]handle,
[in]mem_offset_array,
[in]mem_size_array,
[in]mem_count,
[in]stream_offset_array,
[in]stream_size_array,
[in]stream_count,
[in]flags,
[in/out]vtag(?),
[in]user_ptr,
[out]id
)</SPAN>:

<P>
</LI>
<LI><SPAN  CLASS="textbf">keyval_read_list(
[in]coll_id,
[in]handle,
[in]key_array,
[in]value_array,
[in]count,
[in]flags,
[out]vtag,
[in]user_ptr,
[out]id
)</SPAN>:

<P>
</LI>
<LI><SPAN  CLASS="textbf">keyval_write_list(
[in]coll_id,
[in]handle,
[in]key_array,
[in]value_array,
[in]count,
[in]flags,
[in/out]vtag,
[in]user_ptr,
[out]id
)</SPAN>:
</LI>
</UL>

<P>

<H3><A NAME="SECTION00055700000000000000">
<SPAN CLASS="arabic">5</SPAN>.<SPAN CLASS="arabic">5</SPAN>.<SPAN CLASS="arabic">7</SPAN> Testing for completion</A>
</H3>

<P>
<SPAN  CLASS="textit">Do we need coll_ids here?</SPAN>

<P>

<UL>
<LI><SPAN  CLASS="textbf">test(
[in]coll_id,
[in]id,
[out]count,
[out]vtag,
[out]user_ptr,
[out]state
)</SPAN>:
Tests for completion of a storage interface operation.  The count field
indicates how many operations completed (in this case either 1 or 0).
If an operation completes, then the final status of the operation should
be checked using the state parameter.  Note the vtag output argument
here; it is used to provide vtags for operations that did not complete
immediately.

<P>
</LI>
<LI><SPAN  CLASS="textbf">testsome(
[in]coll_id,
[in/out]id_array,
[in/out]count,
[out]vtag_array,
[out]user_ptr_array,
[out]state_array
)</SPAN>:
Tests for completion of one or more trove operations.  The id_array lists
operations to test on.  A value of TROVE_OP_ID_NULL will be ignored.  Count is
set to the number of completed items on return.

<P>
<SPAN  CLASS="textit">TODO: fix up semantics for testsome; look at MPI functions for ideas.</SPAN>

<P>
<SPAN  CLASS="textit">wait function for testing purposes if nothing else?</SPAN>

<P>
</LI>
</UL>

<P>
<SPAN  CLASS="textit">Note: need to discuss completion queue, internal or external?</SPAN>

<P>
<SPAN  CLASS="textit">Phil: See pvfs2-internal email at 
<BR>
http://beowulf-underground.org/pipermail/pvfs2-internal/2001-October/000010.html
for my thoughts on this topic.</SPAN>

<P>

<H3><A NAME="SECTION00055800000000000000">
<SPAN CLASS="arabic">5</SPAN>.<SPAN CLASS="arabic">5</SPAN>.<SPAN CLASS="arabic">8</SPAN> Batch operations</A>
</H3>

<P>
Batch operations are used to perform a sequence of operations possibly as an
atomic whole.  These will be handled at a higher level.

<P>

<H1><A NAME="SECTION00060000000000000000">
<SPAN CLASS="arabic">6</SPAN> Optimizations</A>
</H1>

<P>
This section lists some potential optimizations that might be applied at this
layer or that are related to this layer.

<P>

<H2><A NAME="SECTION00061000000000000000">
<SPAN CLASS="arabic">6</SPAN>.<SPAN CLASS="arabic">1</SPAN> Metadata Stuffing</A>
</H2>

<P>
In many file systems ``inode stuffing'' is used to store the data for small
files in the space used to store pointers to indirect blocks.  The analogous
approach for PVFS2 would be to store the data for small files in the
bytestream space associated with the metafile.

<P>

<H1><A NAME="SECTION00070000000000000000">
About this document ...</A>
</H1>
 <STRONG>Trove: The PVFS2 Storage Interface</STRONG><P>
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
 <STRONG>latex2html</STRONG> <TT>-split 0 -show_section_numbers -nonavigation storage-interface.tex</TT>
<P>
The translation was initiated by Samuel Lang on 2010-02-04
<BR><HR>
<ADDRESS>
Samuel Lang
2010-02-04
</ADDRESS></table></table></table><?include("../../../../bottom.php"); ?>
</BODY>
</HTML>
