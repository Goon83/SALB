DIR := src/common/misc
LIBSRC += $(DIR)/server-config.c \
          $(DIR)/server-config-mgr.c \
          $(DIR)/str-utils.c \
	  $(DIR)/digest.c \
	  $(DIR)/xattr-utils.c \
          $(DIR)/mmap-ra-cache.c \
	  $(DIR)/extent-utils.c \
	  $(DIR)/errno-mapping.c \
	  $(DIR)/pvfs2-util.c \
          $(DIR)/pvfs2-debug.c \
	  $(DIR)/pint-perf-counter.c \
	  $(DIR)/pint-event.c \
	  $(DIR)/pint-cached-config.c \
	  $(DIR)/pint-util.c \
	  $(DIR)/msgpairarray.c \
	  $(DIR)/void.c \
	  $(DIR)/realpath.c \
	  $(DIR)/tcache.c \
	  $(DIR)/state-machine-fns.c \
	  $(DIR)/fsck-utils.c \
	  $(DIR)/pint-eattr.c \
	  $(DIR)/pint-hint.c \
	  $(DIR)/pint-mem.c
SERVERSRC += $(DIR)/server-config.c \
             $(DIR)/server-config-mgr.c \
             $(DIR)/str-utils.c \
	     $(DIR)/extent-utils.c \
	     $(DIR)/errno-mapping.c \
	     $(DIR)/mkspace.c \
             $(DIR)/pvfs2-debug.c \
	     $(DIR)/pint-perf-counter.c \
	     $(DIR)/pint-event.c \
	     $(DIR)/pint-cached-config.c \
	     $(DIR)/pint-util.c \
	     $(DIR)/tcache.c \
	     $(DIR)/state-machine-fns.c \
	     $(DIR)/void.c \
	     $(DIR)/realpath.c \
	     $(DIR)/msgpairarray.c \
	     $(DIR)/pint-eattr.c \
	     $(DIR)/pint-mem.c \
	     $(DIR)/pint-hint.c

LIBBMISRC += $(DIR)/str-utils.c \
	  $(DIR)/pint-event.c \
	  $(DIR)/errno-mapping.c \
	  $(DIR)/pint-mem.c

MODCFLAGS_$(DIR)/server-config.c = \
   -I$(srcdir)/src/server

# autogenerated files
SMCGEN += $(DIR)/msgpairarray.c \
          $(DIR)/void.c
