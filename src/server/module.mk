BUILD_SERVER = 1
PVFS2_SEGV_BACKTRACE = 1

ifdef BUILD_SERVER
        DIR := src/server

        # automatically generated c files
        SERVER_SMCGEN := \
		$(DIR)/setparam.c \
		$(DIR)/lookup.c \
		$(DIR)/create.c \
		$(DIR)/batch-create.c \
		$(DIR)/batch-remove.c \
		$(DIR)/crdirent.c \
		$(DIR)/set-attr.c \
		$(DIR)/mkdir.c \
		$(DIR)/get-attr.c \
		$(DIR)/list-attr.c \
		$(DIR)/readdir.c \
		$(DIR)/get-config.c \
		$(DIR)/remove.c \
		$(DIR)/rmdirent.c \
		$(DIR)/chdirent.c \
		$(DIR)/io.c \
		$(DIR)/small-io.c \
		$(DIR)/flush.c \
		$(DIR)/truncate.c\
		$(DIR)/noop.c \
		$(DIR)/statfs.c \
		$(DIR)/prelude.c \
		$(DIR)/final-response.c \
		$(DIR)/perf-update.c \
		$(DIR)/perf-mon.c \
		$(DIR)/iterate-handles.c \
		$(DIR)/job-timer.c \
		$(DIR)/proto-error.c \
		$(DIR)/mgmt-remove-object.c \
		$(DIR)/mgmt-remove-dirent.c \
		$(DIR)/mgmt-get-dirdata-handle.c \
		$(DIR)/get-eattr.c \
		$(DIR)/set-eattr.c \
		$(DIR)/del-eattr.c \
		$(DIR)/list-eattr.c \
		$(DIR)/unexpected.c \
		$(DIR)/precreate-pool-refiller.c \
		$(DIR)/unstuff.c \
		$(DIR)/migrate-file-server.c\
		$(DIR)/migrate-file-client.c\
		$(DIR)/per-mon-client.c \
                $(DIR)/set-datafile-sttr-server.c \
                $(DIR)/set-datafile-sttr-client.c

	# c files that should be added to server library
	SERVERSRC += \
		$(SERVER_SMCGEN)

	# track generate .c files to remove during dist clean, etc. 
		SMCGEN += $(SERVER_SMCGEN)

	# server code that will be linked manually, not included in library
	SERVERBINSRC += \
		$(DIR)/pvfs2-server.c $(DIR)/pvfs2-server-req.c

	# to stat the fs, need to know about handle statistics
	MODCFLAGS_$(DIR)/statfs.c = \
		-I$(srcdir)/src/io/trove/trove-handle-mgmt

ifdef PVFS2_SEGV_BACKTRACE
	MODCFLAGS_$(DIR)/pvfs2-server.c := -D__PVFS2_SEGV_BACKTRACE__
endif

endif

