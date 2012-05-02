/*
 * (C) 2001 Clemson University and The University of Chicago
 *
 * See COPYING in top-level directory.
 */

#include <time.h>
#include "client.h"
#include <sys/time.h>
#include <unistd.h>
#include <sys/types.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include "pvfs2-util.h"
#include "pvfs2-mgmt.h"
#include "pvfs2-internal.h"

#define DEFAULT_IO_SIZE 8*1024*1024

int main(int argc, char **argv)
{
    PVFS_sysresp_lookup resp_lk;
    PVFS_sysresp_create resp_cr;
    PVFS_sysresp_io resp_io;
    char *filename = NULL;
    int ret = -1, io_size = DEFAULT_IO_SIZE;
    int *io_buffer = NULL;
    int i, errors, buffer_size;
    PVFS_fs_id fs_id;
    char name[512] = {0};
    char *entry_name = NULL;
    PVFS_credentials credentials;
    PVFS_object_ref parent_refn;
    PVFS_sys_attr attr;
    PVFS_object_ref pinode_refn;
    PVFS_Request file_req;
    PVFS_Request mem_req;
    void *buffer = NULL;
    PVFS_sysresp_getattr resp_getattr;
    PVFS_handle *dfile_array = NULL;

    if (argc != 2)
    {
	fprintf(stderr, "Usage: %s <file name>\n", argv[0]);
	return (-1);
    }

    /* create a buffer for running I/O on */
    io_buffer = (int *) malloc(io_size * sizeof(int));
    if (!io_buffer)
    {
	return (-1);
    }

    /* put some data in the buffer so we can verify */
    for (i = 0; i < io_size; i++)
    {
	io_buffer[i] = i;
    }

    ret = PVFS_util_init_defaults();
    if (ret < 0)
    {
	PVFS_perror("PVFS_util_init_defaults", ret);
	return (-1);
    }
    ret = PVFS_util_get_default_fsid(&fs_id);
    if (ret < 0)
    {
	PVFS_perror("PVFS_util_get_default_fsid", ret);
	return (-1);
    }

    if (argv[1][0] == '/')
    {
        snprintf(name, 512, "%s", argv[1]);
    }
    else
    {
        snprintf(name, 512, "/%s", argv[1]);
    }

    PVFS_util_gen_credentials(&credentials);
    ret = PVFS_sys_lookup(fs_id, name, &credentials,
			  &resp_lk, PVFS2_LOOKUP_LINK_FOLLOW, NULL);
    if (ret == -PVFS_ENOENT)
    {
        PVFS_sysresp_getparent gp_resp;

	printf("IO-TEST: lookup failed; creating new file.\n");

        memset(&gp_resp, 0, sizeof(PVFS_sysresp_getparent));
	ret = PVFS_sys_getparent(fs_id, name, &credentials, &gp_resp, NULL);
	if (ret < 0)
	{
            PVFS_perror("PVFS_sys_getparent failed", ret);
	    return ret;
	}

	attr.owner = credentials.uid;
	attr.group = credentials.gid;
	attr.perms = PVFS_U_WRITE | PVFS_U_READ;
	attr.atime = attr.ctime = attr.mtime = time(NULL);
	attr.mask = PVFS_ATTR_SYS_ALL_SETABLE;
	parent_refn = gp_resp.parent_ref;

        entry_name = rindex(name, (int)'/');
        assert(entry_name);
        entry_name++;
        assert(entry_name);

	ret = PVFS_sys_create(entry_name, parent_refn, attr,
			      &credentials, NULL, &resp_cr, NULL, NULL);
	if (ret < 0)
	{
	    PVFS_perror("PVFS_sys_create() failure", ret);
	    return (-1);
	}

	pinode_refn.fs_id = fs_id;
	pinode_refn.handle = resp_cr.ref.handle;
    }
    else
    {
	printf("IO-TEST: lookup succeeded; performing I/O on "
               "existing file.\n");

	pinode_refn.fs_id = fs_id;
	pinode_refn.handle = resp_lk.ref.handle;
    }

	/**************************************************************
	 * carry out I/O operation
	 */

    printf("IO-TEST: performing write on handle: %ld, fs: %d\n",
	   (long) pinode_refn.handle, (int) pinode_refn.fs_id);

    buffer = io_buffer;
    buffer_size = io_size * sizeof(int);

    /*
      file datatype is tiled, so we can get away with a trivial type
      here
    */
    file_req = PVFS_BYTE;

    ret = PVFS_Request_contiguous(io_size * sizeof(int),
                                  PVFS_BYTE, &(mem_req));
    if (ret < 0)
    {
        PVFS_perror("PVFS_request_contiguous failure", ret);
	return (-1);
    }

    ret = PVFS_sys_write(pinode_refn, file_req, 0, buffer, mem_req,
			 &credentials, &resp_io, NULL);
    if (ret < 0)
    {
        PVFS_perror("PVFS_sys_write failure", ret);
	return (-1);
    }

    printf("IO-TEST: wrote %d bytes.\n", (int) resp_io.total_completed);

    /* uncomment and try out the readback-and-verify stuff that follows
     * once reading back actually works */
    memset(io_buffer, 0, io_size * sizeof(int));

    /* verify */
    printf("IO-TEST: performing read on handle: %ld, fs: %d\n",
	   (long) pinode_refn.handle, (int) pinode_refn.fs_id);

    ret = PVFS_sys_read(pinode_refn, file_req, 0, buffer, mem_req,
			&credentials, &resp_io, NULL);
    if (ret < 0)
    {
        PVFS_perror("PVFS_sys_read failure", ret);
	return (-1);
    }
    printf("IO-TEST: read %d bytes.\n", (int) resp_io.total_completed);
    if ((io_size * sizeof(int)) != resp_io.total_completed)
    {
	fprintf(stderr, "Error: SHORT READ! skipping verification...\n");
    }
    else
    {
	errors = 0;
	for (i = 0; i < io_size; i++)
	{
	    if (i != io_buffer[i])
	    {
		fprintf(stderr,
			"error: element %d differs: should be %d, is %d\n", i,
			i, io_buffer[i]);
		errors++;
	    }
	}
	if (errors != 0)
	{
	    fprintf(stderr, "ERROR: found %d errors\n", errors);
	}
	else
	{
	    printf("IO-TEST: no errors found.\n");
	}
    }

    /* test out some of the mgmt functionality */
    ret = PVFS_sys_getattr(pinode_refn, PVFS_ATTR_SYS_ALL_NOSIZE,
			   &credentials, &resp_getattr, NULL);
    if (ret < 0)
    {
	PVFS_perror("PVFS_sys_getattr", ret);
	return (-1);
    }

    printf("Target file had %d datafiles:\n", resp_getattr.attr.dfile_count);

    dfile_array = (PVFS_handle *) malloc(resp_getattr.attr.dfile_count
					 * sizeof(PVFS_handle));
    if (!dfile_array)
    {
	perror("malloc");
	return (-1);
    }

    ret = PVFS_mgmt_get_dfile_array(pinode_refn, &credentials,
				    dfile_array, resp_getattr.attr.dfile_count, NULL);
    if (ret < 0)
    {
	PVFS_perror("PVFS_mgmt_get_dfile_array", ret);
	return (-1);
    }
    for (i = 0; i < resp_getattr.attr.dfile_count; i++)
    {
	printf("%llu\n", llu(dfile_array[i]));
    }

	/**************************************************************
	 * shut down pending interfaces
	 */

    ret = PVFS_sys_finalize();
    if (ret < 0)
    {
	fprintf(stderr, "Error: PVFS_sys_finalize() failed with errcode = %d\n",
		ret);
	return (-1);
    }

    free(filename);
    free(io_buffer);
    return (0);
}

/*
 * Local variables:
 *  c-indent-level: 4
 *  c-basic-offset: 4
 * End:
 *
 * vim: ts=8 sts=4 sw=4 expandtab
 */
