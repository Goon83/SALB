/*
 * (C) 2002 Clemson University and The University of Chicago
 *
 * See COPYING in top-level directory.
 */
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>

#include "client.h"
#include "mpi.h"
#include "pts.h"
#include "pvfs2-util.h"
#include "pvfs-helper.h"
#include "test-dir-operations.h"
#include "pvfs2-internal.h"

/*
 * parent_refn:  pinode_refn of parent directory
 * depth:   how many directories to make at this level
 * rank:    rank in the mpi process group 
 */

static int remove_dirs(PVFS_object_ref parent_refn,
                       int ndirs,
                       int rank)
{
    int i, ret = -1;
    char name[PVFS_SEGMENT_MAX];

    for (i = 0; i < ndirs; i++)
    {
	snprintf(name, PVFS_SEGMENT_MAX, "rank%d-iter%d", rank, i);
	ret = remove_dir(parent_refn, name);
	if (ret < 0)
	{
	    return -1;
	}
    }
    return 0;
}

static int read_dirs(PVFS_object_ref refn,
                     int ndirs,
                     int rank)
{
    int i, iter, ret;
    PVFS_credentials credentials;
    PVFS_sysresp_readdir resp_readdir;

    memset(&resp_readdir,0,sizeof(PVFS_sysresp_readdir));

    PVFS_util_gen_credentials(&credentials);

    printf("Calling readdir with handle %lld and fsid %d\n",
           lld(refn.handle), refn.fs_id);
    printf("ndirs is %d\n",ndirs);
    ret = PVFS_sys_readdir(refn, PVFS_READDIR_START, ndirs,
                           &credentials, &resp_readdir);
    if (ret < 0)
    {
	printf("readdir failed with errcode = %d\n", ret);
	return(-1);
    }

    /* examine the results */

    if (resp_readdir.pvfs_dirent_outcount != ndirs)
    {
	debug_printf("we were expecting %d directories, and recieved %d\n",
                     ndirs, resp_readdir.pvfs_dirent_outcount);

	free(resp_readdir.dirent_array);
	return -1;
    }

    /* check each of our directories to ensure that they have sane names */

    for (i = 0; i < ndirs; i++)
    {
	if (0 > sscanf(resp_readdir.dirent_array[i].d_name,
                       "rank%d-iter%d", &rank, &iter))
	{
	    debug_printf("unable to read directory name iter: %d\n",i);
	    free(resp_readdir.dirent_array);
	    return -1;
	}

	if ((iter > ndirs) || (iter < 0))
	{
	    debug_printf("invalid directory name %s\n",
                         resp_readdir.dirent_array[i].d_name);
	    free(resp_readdir.dirent_array);
	    return -1;
	}
    }

    free(resp_readdir.dirent_array);
    return 0;
}

static int create_dirs(PVFS_object_ref refn,
                       int ndirs,
                       int rank)
{
    int i;
    char name[PVFS_SEGMENT_MAX];
    PVFS_object_ref out_refn;

    for (i = 0; i < ndirs; i++)
    {
	snprintf(name, PVFS_SEGMENT_MAX, "rank%d-iter%d", rank, i);
	if (create_dir(refn, name, &out_refn) < 0)
	{
            printf("failed to mkdir %s ... skipping.\n",name);
	}
    }
    return 0;
}

/*
 * driver for the test
 * comm:	special pts communicator
 * rank:	rank among processes
 * buf:		stuff data in here ( not used )
 * rawparams:	our configuration information
 *
 * returns: 
 * 	0:  	all went well
 * 	nonzero: errors encountered making reading, or removing directories
 */
int test_dir_operations(MPI_Comm * comm,
                        int rank,
                        char *buf __unused,
                        void *rawparams)
{
    int ret = -1;
    int nerrs = 0;
    char name[PVFS_SEGMENT_MAX];
    PVFS_fs_id fs_id = 0;
    PVFS_object_ref root_refn, out_refn;
    generic_params *myparams = (generic_params *) rawparams;

    if (rank == 0)
    {
	if (!pvfs_helper.initialized && initialize_sysint())
	{
	    printf("System initialization error\n");
	    return (-1);
	}
	fs_id = pvfs_helper.fs_id;
	printf("fs_id: %d\n", fs_id);
    }

    MPI_Bcast(&fs_id, 1, MPI_LONG_INT, 0, *comm );
    printf("rank: %d  fs_id: %d\n", rank, fs_id );

    get_root(fs_id, &root_refn);
    printf("got root handle %lld in fsid %d\n",
           lld(root_refn.handle),root_refn.fs_id);

    /* setup a dir in the root directory to do tests in (so the root dir is
     * less cluttered)
     *
     */
    memset(name,0,PVFS_SEGMENT_MAX);
    snprintf(name, PVFS_SEGMENT_MAX, "dir_op_test");
    if (rank == 0)
    {
	if (create_dir(root_refn, name, &out_refn) < 0)
	{
	    return -1;
	}
        else
        {
            printf("created directory %s (handle is %lld)\n",name, lld(out_refn.handle));
        }
    }
    MPI_Barrier(*comm);
    if (rank != 0)
    {
	/* for everyone that didn't create the dir entry, we should get the 
	 * handle via lookup */
	if (lookup_name(root_refn, name, &out_refn) < 0)
	{
	    return -1;
	}
        else
        {
            printf("directory %s has handle %lld\n",name, lld(out_refn.handle));
        }
    }

    printf("test dir handle is %lld\n", lld(out_refn.handle));
    ret = create_dirs(out_refn, myparams->mode, rank);
    if (ret < 0)
    {
	printf("creating directories failed with errcode = %d\n", ret);
	return (-1);
    }

    ret = read_dirs(out_refn, myparams->mode, rank);
    if (ret < 0)
    {
	printf("reading directories failed with errcode = %d\n", ret);
	return (-1);
    }

    ret = remove_dirs(out_refn, myparams->mode, rank);
    if (ret < 0)
    {
	printf("removing directories failed with errcode = %d\n", ret);
	return (-1);
    }

    if (rank == 0)
    {
	/* remove the test directory */
	ret = remove_dir(root_refn, name);
	if (ret < 0)
	{
	    return -1;
	}
    }

/*     ret = finalize_sysint(); */
/*     if (ret < 0) */
/*     { */
/* 	printf("finalizing sysint failed with errcode = %d\n", ret); */
/* 	return (-1); */
/*     } */

    return -nerrs;
}

/*
 * Local variables:
 *  c-indent-level: 4
 *  c-basic-offset: 4
 * End:
 *
 * vim: ts=8 sts=4 sw=4 expandtab
 */
