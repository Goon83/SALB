<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.0 Transitional//EN">

<!--Converted with LaTeX2HTML 2002-2-1 (1.71)
original version by:  Nikos Drakos, CBLU, University of Leeds
* revised and updated by:  Marcus Hennecke, Ross Moore, Herb Swan
* with significant contributions from:
  Jens Lippmann, Marek Rouchal, Martin Wilck and others -->
<HTML>
<HEAD>
<TITLE>Trove DBPF Handle Allocator </TITLE>
<META NAME="description" CONTENT="Trove DBPF Handle Allocator ">
<META NAME="keywords" CONTENT="handle-allocator">
<META NAME="resource-type" CONTENT="document">
<META NAME="distribution" CONTENT="global">

<META NAME="Generator" CONTENT="LaTeX2HTML v2002-2-1">
<META HTTP-EQUIV="Content-Style-Type" CONTENT="text/css">

<LINK REL="STYLESHEET" HREF="handle-allocator.css">

<? include("../../../../header.php"); ?></HEAD>

<?include("../../../..//top.php"); ?> <body id="documentation">
<H1 ALIGN="LEFT">Trove DBPF Handle Allocator </H1>
<DIV CLASS="author_info">

<P ALIGN="LEFT"><STRONG>PVFS Development Team</STRONG></P>
</DIV>
<PRE>$Id: handle-allocator.tex,v 1.1 2003/01/24 23:29:18 pcarns Exp $
</PRE>

<P>

<H1><A NAME="SECTION00010000000000000000">
<SPAN CLASS="arabic">1</SPAN> Introduction</A>
</H1>

<P>
The Trove interface gives out handles - unique identifiers to trove
objects.  In addition to being unique, handles will not be reused within
a configurable amount of time.  These two constraints make for a handle
allocator that ends up being a bit more complicated than one might
expect.  Add to that the fact that we want to serialize on disk all or
part of the handle allocator's state, and here we are with a document to
explain it all.

<P>

<H2><A NAME="SECTION00011000000000000000">
<SPAN CLASS="arabic">1</SPAN>.<SPAN CLASS="arabic">1</SPAN> Data Structures</A>
</H2>

<H3><A NAME="SECTION00011100000000000000">
<SPAN CLASS="arabic">1</SPAN>.<SPAN CLASS="arabic">1</SPAN>.<SPAN CLASS="arabic">1</SPAN> Extents</A>
</H3>
We have a large handle space we need to represent efficiently.  This
approach uses extents:
<PRE>
struct extent {
	int64_t first;
	int64_t last;
};
</PRE>

<P>

<H3><A NAME="SECTION00011200000000000000">
<SPAN CLASS="arabic">1</SPAN>.<SPAN CLASS="arabic">1</SPAN>.<SPAN CLASS="arabic">2</SPAN> Extent List</A>
</H3>
We keep the extents (not nescessarily sorted) in the <TT>extents</TT>
array.  For faster searches, <TT>index</TT> keeps an index into
<TT>extents</TT> in an AVL tree. 
In addition
to the extents themselves, some bookkeeping members are added.  The most
important is the <TT>timestamp</TT> member, used to make sure no handle in
its list gets reused before it should.  <TT>__size</TT> is only used
internally, keeping track of how big <TT>extents</TT> is.  

<P>
<PRE>
struct extentlist {
	int64_t __size;
	int64_t num_extents;
	int64_t num_handles;
	struct timeval timestamp;
	struct extent * extents;
};
</PRE>

<P>

<H3><A NAME="SECTION00011300000000000000">
<SPAN CLASS="arabic">1</SPAN>.<SPAN CLASS="arabic">1</SPAN>.<SPAN CLASS="arabic">3</SPAN> Handle Ledger</A>
</H3>
We manage several lists.  The <TT>free_list</TT> contains all the valid
handles. The <TT>recently_freed_list</TT> contains handles which have been
freed, but possibly before some expire time has passed.  The
<TT>overflow_list</TT> holds freed handles while items on the
<TT>recently_freed_list</TT> wait for the expire time to pass.

<P>
We save our state by writing out and reading from the three
<TT>TROVE_handle</TT> members, making use of the higher level trove
interface. 
<PRE>
struct handle_ledger {
        struct extentlist free_list;
	struct extentlist recently_freed_list;
	struct extentlist overflow_list;
	FILE *backing_store;
	TROVE_handle free_list_handle;
	TROVE_handle recently_freed_list_handle;
	TROVE_handle overflow_list_handle;
}
</PRE>

<P>

<H1><A NAME="SECTION00020000000000000000">
<SPAN CLASS="arabic">2</SPAN> Algorithm</A>
</H1>

<H2><A NAME="SECTION00021000000000000000">
<SPAN CLASS="arabic">2</SPAN>.<SPAN CLASS="arabic">1</SPAN> Assigning handles</A>
</H2>
Start off with a <TT>free_list</TT> of one big extent encompassing the
entire handle space.

<UL>
<LI>Get the last extent from the <TT>free_list</TT> (We hope getting
the last extent improves the effiency of the extent representation)
</LI>
<LI>Save <TT>last</TT> for later return to the caller
</LI>
<LI>Decrement <TT>last</TT>
</LI>
<LI>if <SPAN CLASS="MATH"><IMG
 WIDTH="88" HEIGHT="30" ALIGN="MIDDLE" BORDER="0"
 SRC="img1.png"
 ALT="$ first &gt; last $"></SPAN>, mark the extent as empty. 
</LI>
</UL>

<P>

<H2><A NAME="SECTION00022000000000000000">
<SPAN CLASS="arabic">2</SPAN>.<SPAN CLASS="arabic">2</SPAN> returning handles</A>
</H2>

<UL>
<LI>when the first handle is returned, it gets added to the
    <TT>recently_freed</TT> list. Because this is the first item on that
    list, we check the time. 
</LI>
<LI>now we add more handles to the list.  we check the time after <SPAN CLASS="MATH"><IMG
 WIDTH="19" HEIGHT="14" ALIGN="BOTTOM" BORDER="0"
 SRC="img2.png"
 ALT="$N$"></SPAN> handles are returned and update the timestamp.
</LI>
<LI>Once we have added <SPAN CLASS="MATH"><IMG
 WIDTH="19" HEIGHT="14" ALIGN="BOTTOM" BORDER="0"
 SRC="img3.png"
 ALT="$H$"></SPAN> handles, we decide the <TT>recently_freed</TT>
    list has enough handles.  We then start using the
    <TT>overflow_list</TT> to hold returned handles.
</LI>
<LI>as with the <TT>recently_freed</TT> list, we record the time that
    this handle was added, updating the timestamp after every <SPAN CLASS="MATH"><IMG
 WIDTH="19" HEIGHT="14" ALIGN="BOTTOM" BORDER="0"
 SRC="img2.png"
 ALT="$N$"></SPAN>
    additions.  We also check how old the <TT>recently_freed</TT> list is. 
</LI>
<LI>at some point in time, the whole <TT>recently_freed</TT> list is ready
    to be returned to the <TT>free_list</TT>.  The <TT>recently_freed</TT>
    list is merged into the <TT>free_list</TT>, the <TT>overflow_list</TT>
    becomes the <TT>recently_freed</TT> list and the  <TT>overflow_list</TT>
    is empty.
</LI>
</UL>

<P>

<H2><A NAME="SECTION00023000000000000000">
<SPAN CLASS="arabic">2</SPAN>.<SPAN CLASS="arabic">3</SPAN> I don't know what to call this section</A>
</H2>

<P>
Let <SPAN CLASS="MATH"><IMG
 WIDTH="21" HEIGHT="30" ALIGN="MIDDLE" BORDER="0"
 SRC="img4.png"
 ALT="$T_{r}$"></SPAN> be the minimum response time for an operation of any sort,
<SPAN CLASS="MATH"><IMG
 WIDTH="22" HEIGHT="30" ALIGN="MIDDLE" BORDER="0"
 SRC="img5.png"
 ALT="$T_{f}$"></SPAN> be the time a handle must sit before being moved back to the free list, and <SPAN CLASS="MATH"><IMG
 WIDTH="34" HEIGHT="30" ALIGN="MIDDLE" BORDER="0"
 SRC="img6.png"
 ALT="$N_{tot}$"></SPAN> be the total number of handles available on a server.

<P>
The pathological case would be one where a caller

<UL>
<LI>fills up the <TT>recently_freed</TT> list
</LI>
<LI>immediately starts consuming handles as quickly as possible to make for
 the largest possible <TT>recently_freed</TT> list in the next pass
</LI>
</UL>

<P>
This results in the largest number of handles being unavailable due to sitting
on the <TT>overflow_list</TT>.  Call <SPAN CLASS="MATH"><IMG
 WIDTH="45" HEIGHT="30" ALIGN="MIDDLE" BORDER="0"
 SRC="img7.png"
 ALT="$N_{purg}$"></SPAN> the number of handles waiting
in ``purgatory'' ( waiting for <SPAN CLASS="MATH"><IMG
 WIDTH="22" HEIGHT="30" ALIGN="MIDDLE" BORDER="0"
 SRC="img5.png"
 ALT="$T_{f}$"></SPAN> to pass) 
<BR>
<DIV ALIGN="RIGHT" CLASS="mathdisplay">

<!-- MATH
 \begin{equation}
N_{purg} = T_{f} / T_{r}
\end{equation}
 -->
<TABLE WIDTH="100%" ALIGN="LEFT">
<TR VALIGN="MIDDLE"><TD ALIGN="LEFT" NOWRAP><IMG
 WIDTH="102" HEIGHT="29" BORDER="0"
 SRC="img8.png"
 ALT="\begin{displaymath}
N_{purg} = T_{f} / T_{r}
\end{displaymath}"></TD>
<TD CLASS="eqno" WIDTH=10 ALIGN="RIGHT">
(<SPAN CLASS="arabic">1</SPAN>)</TD></TR>
</TABLE>
<BR CLEAR="ALL"></DIV><P></P>

<P>
<BR>
<DIV ALIGN="RIGHT" CLASS="mathdisplay">

<!-- MATH
 \begin{equation}
F_{purg} = N_{purg} / N_{tot}
\end{equation}
 -->
<TABLE WIDTH="100%" ALIGN="LEFT">
<TR VALIGN="MIDDLE"><TD ALIGN="LEFT" NOWRAP><IMG
 WIDTH="135" HEIGHT="29" BORDER="0"
 SRC="img9.png"
 ALT="\begin{displaymath}
F_{purg} = N_{purg} / N_{tot}
\end{displaymath}"></TD>
<TD CLASS="eqno" WIDTH=10 ALIGN="RIGHT">
(<SPAN CLASS="arabic">2</SPAN>)</TD></TR>
</TABLE>
<BR CLEAR="ALL"></DIV><P></P>
<BR>
<DIV ALIGN="RIGHT" CLASS="mathdisplay">

<!-- MATH
 \begin{equation}
F_{purg} = T_{f} / (T_{r} * N_{tot})
\end{equation}
 -->
<TABLE WIDTH="100%" ALIGN="LEFT">
<TR VALIGN="MIDDLE"><TD ALIGN="LEFT" NOWRAP><IMG
 WIDTH="156" HEIGHT="29" BORDER="0"
 SRC="img10.png"
 ALT="\begin{displaymath}
F_{purg} = T_{f} / (T_{r} * N_{tot})
\end{displaymath}"></TD>
<TD CLASS="eqno" WIDTH=10 ALIGN="RIGHT">
(<SPAN CLASS="arabic">3</SPAN>)</TD></TR>
</TABLE>
<BR CLEAR="ALL"></DIV><P></P>

<P>
We should try to collect statistics and see what <SPAN CLASS="MATH"><IMG
 WIDTH="21" HEIGHT="30" ALIGN="MIDDLE" BORDER="0"
 SRC="img4.png"
 ALT="$T_{r}$"></SPAN> and <SPAN CLASS="MATH"><IMG
 WIDTH="45" HEIGHT="30" ALIGN="MIDDLE" BORDER="0"
 SRC="img7.png"
 ALT="$N_{purg}$"></SPAN> end up being for real and pathological workloads.

<P>

<H1><A NAME="SECTION00030000000000000000">
About this document ...</A>
</H1>
 <STRONG>Trove DBPF Handle Allocator </STRONG><P>
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
 <STRONG>latex2html</STRONG> <TT>-split 0 -show_section_numbers -nonavigation handle-allocator.tex</TT>
<P>
The translation was initiated by Samuel Lang on 2010-02-04
<BR><HR>
<ADDRESS>
Samuel Lang
2010-02-04
</ADDRESS></table></table></table><?include("../../../../bottom.php"); ?>
</BODY>
</HTML>
