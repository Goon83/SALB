#
# Special Makefile for building kernel module.  Invoked both by user and
# by kernel build process.
#
here = src/kernel/linux-2.6
top = ../../..

absolute_src_dir = /home/goon/library/SALB
absolute_build_dir= /home/goon/library/SALB
src_relative_top = ./
ifeq "/" "$(shell echo ${src_relative_top} | cut -c 1)"
# not actually relative (as if someone called /tmp/foo/configure
#    instead of ../foo/configure)
    relative_src_dir = ${src_relative_top}
else
    relative_src_dir = $(top)/${src_relative_top}
endif

QUIET_COMPILE = 1
ifdef V
    QUIET_COMPILE = 0
endif

ifeq ($(QUIET_COMPILE),1)
  # say a one-line description of the action, do not echo the command
  Q=@echo
  E=@
else
  # do not say the short Q lines, but do echo the entire command
  Q=@echo >/dev/null
  E=
endif

csrc = \
    pvfs2-utils.c \
    devpvfs2-req.c \
    pvfs2-cache.c \
    dcache.c \
    file.c \
    inode.c \
    dir.c \
    namei.c \
    super.c \
    pvfs2-mod.c \
    pvfs2-bufmap.c \
    symlink.c \
    xattr.c \
    acl.c \
    xattr-trusted.c \
    xattr-default.c \
    waitqueue.c \
    pvfs2-proc.c
hsrc = \
    pvfs2-kernel.h \
    pvfs2-dev-proto.h \
    pvfs2-bufmap.h \
    upcall.h \
    downcall.h \
    pvfs2-proc.h

objs = $(csrc:.c=.o)
othergen = pvfs2.o pvfs2.ko pvfs2.mod.c pvfs2.mod.o
othergendir = .tmp_versions  # around 2.6.6 this is generated locally
cmds = $(patsubst %,.%.cmd,$(objs) $(othergen))

ifneq ($(KERNELRELEASE),)

EXTRA_CFLAGS = \
    -I$(absolute_src_dir)/ \
    -I$(absolute_build_dir)/ \
    -I$(absolute_src_dir)/include \
    -I$(absolute_build_dir)/include \
    -I$(absolute_src_dir)/src/io/dev \
    -I$(absolute_src_dir)/src/io/bmi \
    -I$(absolute_src_dir)/src/common/quickhash \
	 -I$(absolute_src_dir)/src/proto \
    -I$(absolute_src_dir)/src/common/gossip \
    -I$(absolute_src_dir)/src/common/misc

EXTRA_CFLAGS += 
EXTRA_CFLAGS += -DPVFS2_VERSION="\"2.8.2\""

# uncomment the following line for kernel specific
# debugging output or features
#EXTRA_CFLAGS += -DPVFS2_KERNEL_DEBUG

obj-m += pvfs2.o
pvfs2-objs := $(objs)

else

#KDIR	:= /lib/modules/$(shell uname -r)/build
#KDIR	:= /usr/src/linux-$(shell uname -r)
KDIR    := 
PWD	:= $(shell pwd)

default: links
	$(E)$(MAKE) -C $(KDIR) SUBDIRS=$(PWD) modules Q=@

# link to real source directory if out-of-tree build
links:
	$(E)for i in $(csrc) $(hsrc); do \
	    if [ ! -f $$i  -a  ! -L $$i ] ; then \
		ln -s $(relative_src_dir)/$(here)/$$i ;\
	    fi ;\
	done

clean:
	$(E)for i in $(csrc) $(hsrc); do \
	    if [ -L $$i ] ; then \
		rm -f $$i ;\
	    fi ;\
	done
	$(E)rm -f $(objs) $(othergen) $(cmds)
	$(E)rm -rf $(othergendir)

endif
