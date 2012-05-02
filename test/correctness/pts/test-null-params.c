/*
 * (C) 2002 Clemson University and The University of Chicago
 *
 * See COPYING in top-level directory.
 */

/* 
 * test-null-params: tests behavior of all sys-init functions will paramater
 * values set to null.
 * Author: Michael Speth
 * Date: 5/27/2003
 * Last Updated: 6/26/2003
 */

#include <time.h>
#include <sys/time.h>
#include <stdio.h>
#include <stdlib.h>

#include "client.h"
#include "mpi.h"
#include "pts.h"
#include "pvfs-helper.h"
#include "null_params.h"
#include "pvfs2-util.h"
#include "test-null-params.h"

/* 
 * Preconditions: none
 * Postconditions: returs error code of sys initialize; however, I'm not sure what will happen if null params are passed into sys_init so this might seg-fault.
 * Hase 1 test cases
 */
static int test_system_init(void)
{
    int ret = -1;

    memset(&pvfs_helper, 0, sizeof(pvfs_helper));

    ret = PVFS_util_init_defaults();
    if(ret < 0)
    {
	PVFS_perror("PVFS_util_init_defaults", ret);
	return(ret);
    }

    ret = PVFS_util_get_default_fsid(&pvfs_helper.fs_id);
    if(ret < 0)
    {
	PVFS_perror("PVFS_util_get_default_fsid", ret);
	return(ret);
    }

    pvfs_helper.initialized = 1;
    pvfs_helper.num_test_files = NUM_TEST_FILES;

    return 0;
}

/* Preconditions: none
 * Parameters: nullCase - the test case that is checked for this function
 * Postconditions: returns the error code given by lookup - thats if it doesn't segfault or other catostrophic failure
 * Has 2 test cases
 */
static int test_lookup(int nullCase)
{
    int fs_id, ret;
    PVFS_credentials credentials;
    PVFS_sysresp_lookup resp_lookup;
    char *name;

    ret = -2;
    name = (char *) malloc(sizeof(char) * 100);
    name = strcpy(name, "name");

    if (initialize_sysint() < 0)
    {
	debug_printf("UNABLE TO INIT THE SYSTEM INTERFACE\n");
	return -1;
    }
    fs_id = pvfs_helper.fs_id;

    PVFS_util_gen_credentials(&credentials);

    switch (nullCase)
    {
    case 0:
	ret = PVFS_sys_lookup(fs_id, NULL, &credentials,
                              &resp_lookup, PVFS2_LOOKUP_LINK_NO_FOLLOW);
	break;
    case 1:
	ret = PVFS_sys_lookup(fs_id, name, &credentials,
                              NULL, PVFS2_LOOKUP_LINK_NO_FOLLOW);
	break;
    default:
	fprintf(stderr, "Error - not a case\n");
    }
    return ret;
}

/* Preconditions: none
 * Parameters: nullCase - the test case that is checked for this function
 * Postconditions: returns error from getattr
 * Has 1 Test Cases
 */
static int test_getattr(int nullCase)
{
    int fs_id, ret;
    PVFS_credentials credentials;
    PVFS_object_ref pinode_refn;
    uint32_t attrmask;
    PVFS_sysresp_lookup resp_lookup;
    char *name;

    ret = -2;
    name = (char *) malloc(sizeof(char) * 100);
    name = strcpy(name, "name");

    if (initialize_sysint() < 0)
    {
	debug_printf("ERROR UNABLE TO INIT SYSTEM INTERFACE\n");
	return -1;
    }
    fs_id = pvfs_helper.fs_id;

    PVFS_util_gen_credentials(&credentials);

    if ((ret = PVFS_sys_lookup(
             fs_id, name, &credentials,
             &resp_lookup, PVFS2_LOOKUP_LINK_NO_FOLLOW)) < 0)
    {
	fprintf(stderr, "lookup failed %d\n", ret);
	return ret;
    }

    pinode_refn = resp_lookup.ref;
    attrmask = PVFS_ATTR_SYS_ALL_NOSIZE;

    switch (nullCase)
    {
    case 0:
	ret = PVFS_sys_getattr(pinode_refn, attrmask, &credentials, NULL);
	break;
    }
    return ret;
}

/* Preconditions: None
 * Parameters: nullCase - the test case that is checked for this function
 * Postconditions:
 */
static int test_setattr(void)
{
    return -2;
}

/* Preconditions: None
 * Parameters: nullCase - the test case that is checked for this function
 * Postconditions: returns the error returned by mkdir
 * Has 2 test cases
 */
static int test_mkdir(int nullCase)
{
    PVFS_object_ref parent_refn;
    PVFS_sys_attr attr;
    PVFS_sysresp_mkdir resp_mkdir;

    int ret = -2;
    int fs_id;
    PVFS_credentials credentials;
    PVFS_sysresp_lookup resp_lookup;
    char *name;

    name = (char *) malloc(sizeof(char) * 100);
    name = strcpy(name, "name");

    if (initialize_sysint() < 0)
    {
	debug_printf("UNABLE TO INIT THE SYSTEM INTERFACE\n");
	return -1;
    }
    fs_id = pvfs_helper.fs_id;

    PVFS_util_gen_credentials(&credentials);
    if ((ret = PVFS_sys_lookup(
             fs_id, name, &credentials, &resp_lookup,
             PVFS2_LOOKUP_LINK_NO_FOLLOW)) < 0)
    {
	fprintf(stderr, "lookup failed %d\n", ret);
	return -1;
    }

    parent_refn = resp_lookup.ref;
    attr.owner = credentials.uid;
    attr.group = credentials.gid;
    attr.perms = 1877;
    attr.atime = attr.mtime = attr.ctime = 
	time(NULL);

    switch (nullCase)
    {
    case 0:
	ret = PVFS_sys_mkdir(NULL, parent_refn, attr, &credentials, &resp_mkdir);
	break;
    case 1:
	ret = PVFS_sys_mkdir(name, parent_refn, attr, &credentials, NULL);
	break;
    default:
	fprintf(stderr, "Error - no more cases\n");
    }
    return ret;
}

/* Preconditions: none
 * Parameters: nullCase - the test case that is checked for this function
 * Postconditions: returns error code of readdir
 * Has 1 Test cases
 */
static int test_readdir(int nullCase)
{

    int ret;

    PVFS_object_ref pinode_refn;
    PVFS_ds_position token;
    int pvfs_dirent_incount;
    PVFS_credentials credentials;

    int fs_id;
    PVFS_sysresp_lookup resp_lookup;
    char *name;

    ret = -2;
    name = (char *) malloc(sizeof(char) * 100);
    name = strcpy(name, "name");

    if (initialize_sysint() < 0)
    {
	debug_printf("UNABLE TO INIT THE SYSTEM INTERFACE\n");
	return -1;
    }
    fs_id = pvfs_helper.fs_id;

    PVFS_util_gen_credentials(&credentials);
    if ((ret = PVFS_sys_lookup(
             fs_id, name, &credentials,
             &resp_lookup, PVFS2_LOOKUP_LINK_NO_FOLLOW)) < 0)
    {
	fprintf(stderr, "lookup failed %d\n", ret);
	return -1;
    }

    pinode_refn = resp_lookup.ref;
    token = PVFS_READDIR_START;
    pvfs_dirent_incount = 1;

    switch (nullCase)
    {
    case 0:
	ret =
	    PVFS_sys_readdir(pinode_refn, token, pvfs_dirent_incount,
			     &credentials, NULL);
	break;
    }
    return ret;
}

/* Preconditions: none
 * Parameters: nullCase - the test case that is checked for this function
 * Postconditions: returns error code of readdir
 * Has 2 test cases
 */
static int test_create(int nullCase)
{
    int ret, fs_id;
    PVFS_sys_attr attr;
    PVFS_credentials credentials;
    PVFS_sysresp_lookup resp_look;
    PVFS_sysresp_create resp_create;
    char *filename;

    ret = -2;
    filename = (char *) malloc(sizeof(char) * 100);
    filename = strcpy(filename, "name");

    PVFS_util_gen_credentials(&credentials);
    attr.owner = credentials.uid;
    attr.group = credentials.gid;
    attr.perms = 1877;
    attr.atime = attr.mtime = attr.ctime = time(NULL);

    if (initialize_sysint())
    {
	debug_printf("UNABLE TO INIT THE SYSTEM INTERFACE\n");
	return -1;
    }
    fs_id = pvfs_helper.fs_id;

    ret = PVFS_sys_lookup(fs_id, "/", &credentials,
                          &resp_look, PVFS2_LOOKUP_LINK_NO_FOLLOW);
    if (ret < 0)
    {
	printf("Lookup failed with errcode = %d\n", ret);
	return (-1);
    }

    switch (nullCase)
    {
    case 0:
	ret =
	    PVFS_sys_create(NULL, resp_look.ref, attr, &credentials,
			    NULL, NULL, &resp_create);
	break;
    case 1:
	ret =
	    PVFS_sys_create(filename, resp_look.ref, attr, &credentials,
			    NULL, NULL, NULL);
	break;
    default:
	fprintf(stderr, "Error - incorect case number \n");
	return -3;
    }
    return ret;
}

/* Preconditions: none
 * Parameters: nullCase - the test case that is checked for this function
 * Postconditions: returns error code of readdir
 * Has 1 test case
 */
static int test_remove(int nullCase)
{
    PVFS_credentials credentials;
    PVFS_sysresp_lookup resp_look;
    char *filename;
    int ret;
    int fs_id;

    ret = -2;
    filename = (char *) malloc(sizeof(char) * 100);
    filename = strcpy(filename, "name");

    PVFS_util_gen_credentials(&credentials);

    if (initialize_sysint() < 0)
    {
	debug_printf("UNABLE TO INIT SYSTEM INTERFACE\n");
	return -1;
    }
    fs_id = pvfs_helper.fs_id;

    ret = PVFS_sys_lookup(fs_id, filename, &credentials,
                          &resp_look, PVFS2_LOOKUP_LINK_NO_FOLLOW);
    if (ret < 0)
    {
	printf("Lookup failed with errcode = %d\n", ret);
	return (-1);
    }
    switch (nullCase)
    {
    case 0:
	ret = PVFS_sys_remove(NULL, resp_look.ref, &credentials);
	break;
    default:
	fprintf(stderr, "Error: invalid case number \n");
    }
    return ret;
}

/* Preconditions: none
 * Parameters: nullCase - the test case that is checked for this function
 * Postconditions: returns error code of readdir
 */
static int test_rename(void)
{

/*      return PVFS_sys_rename(old_name, old_parent_refn, new_name, new_parent_refn, credentials); */
    return -2;
}

/* Preconditions: none
 * Parameters: nullCase - the test case that is checked for this function
 * Postconditions: returns error code of readdir
 */
static int test_symlink(void)
{
    return -2;
}

/* Preconditions: none
 * Parameters: nullCase - the test case that is checked for this function
 * Postconditions: returns error code of readdir
 */
static int test_readlink(void)
{
    return -2;
}

/* Preconditions: none
 * Parameters: nullCase - the test case that is checked for this function
 * Postconditions: returns error code of readdir
 * Has 3 test cases
 */
static int test_read(int nullCase)
{
    PVFS_credentials credentials;
    PVFS_sysresp_lookup resp_lk;
    PVFS_Request req_io;
    PVFS_sysresp_io resp_io;
    char *filename;
    char io_buffer[100];
    int fs_id, ret;

    filename = (char *) malloc(sizeof(char) * 100);
    filename = strcpy(filename, "name");

    memset(&req_io, 0, sizeof(PVFS_Request));
    memset(&resp_io, 0, sizeof(PVFS_sysresp_io));

    PVFS_util_gen_credentials(&credentials);
    memset(&resp_lk, 0, sizeof(PVFS_sysresp_lookup));

    if (initialize_sysint() < 0)
    {
	debug_printf("UNABLE TO INIT THE SYSTEM INTERFACE\n");
	return -1;
    }
    fs_id = pvfs_helper.fs_id;

    ret = PVFS_sys_lookup(fs_id, filename, &credentials,
                          &resp_lk, PVFS2_LOOKUP_LINK_NO_FOLLOW);
    if (ret < 0)
    {
	debug_printf("test_pvfs_datatype_hvector: lookup failed "
		     "on %s\n", filename);
    }

    switch (nullCase)
    {
    case 0:
	ret =
	    PVFS_sys_read(resp_lk.ref, NULL, 0, io_buffer, NULL,
			  &credentials, &resp_io);
	break;
    case 1:
	ret =
	    PVFS_sys_read(resp_lk.ref, req_io, 0, NULL, NULL, &credentials,
			  &resp_io);
	break;
    case 2:
	ret =
	    PVFS_sys_read(resp_lk.ref, req_io, 0, io_buffer, NULL,
			  &credentials, NULL);
	break;
    }
    return ret;
}

/* Preconditions: none
 * Parameters: nullCase - the test case that is checked for this function
 * Postconditions: returns error code of readdir
 * Has 3 test cases
 */
static int test_write(int nullCase)
{
    PVFS_credentials credentials;
    PVFS_sysresp_lookup resp_lk;
    PVFS_Request req_io;
    PVFS_sysresp_io resp_io;
    char *filename;
    char io_buffer[100];
    int fs_id, ret;

    filename = (char *) malloc(sizeof(char) * 100);
    filename = strcpy(filename, "name");

    memset(&req_io, 0, sizeof(PVFS_Request));
    memset(&resp_io, 0, sizeof(PVFS_sysresp_io));

    PVFS_util_gen_credentials(&credentials);
    memset(&resp_lk, 0, sizeof(PVFS_sysresp_lookup));

    if (initialize_sysint() < 0)
    {
	debug_printf("UNABLE TO INIT THE SYSTEM INTERFACE\n");
	return -1;
    }
    fs_id = pvfs_helper.fs_id;

    ret = PVFS_sys_lookup(fs_id, filename, &credentials,
                          &resp_lk, PVFS2_LOOKUP_LINK_NO_FOLLOW);
    if (ret < 0)
    {
	debug_printf("test_pvfs_datatype_hvector: lookup failed "
		     "on %s\n", filename);
    }

    switch (nullCase)
    {
    case 0:
	ret =
	    PVFS_sys_write(resp_lk.ref, NULL, 0, io_buffer, NULL,
			   &credentials, &resp_io);
	break;
    case 1:
	ret =
	    PVFS_sys_write(resp_lk.ref, req_io, 0, NULL, NULL, &credentials,
			   &resp_io);
	break;
    case 2:
	ret =
	    PVFS_sys_write(resp_lk.ref, req_io, 0, io_buffer, NULL,
			   &credentials, NULL);
	break;
    }
    return ret;
}

static int init_file(void)
{
    int ret, fs_id;
    PVFS_sys_attr attr;
    PVFS_credentials credentials;
    PVFS_sysresp_lookup resp_look;
    PVFS_sysresp_create resp_create;
    char *filename;

    filename = (char *) malloc(sizeof(char) * 100);
    filename = strcpy(filename, "name");

    PVFS_util_gen_credentials(&credentials);
    attr.owner = credentials.uid;
    attr.group = credentials.gid;
    attr.perms = 1877;
    attr.atime = attr.mtime = attr.ctime = time(NULL);

    if (initialize_sysint() < 0)
    {
	debug_printf("UNABLE TO INIT THE SYSTEM INTERFACE\n");
	return -1;
    }
    fs_id = pvfs_helper.fs_id;

    /* get root */
    ret = PVFS_sys_lookup(fs_id, "/", &credentials,
                          &resp_look, PVFS2_LOOKUP_LINK_NO_FOLLOW);
    if (ret < 0)
    {
	printf("Lookup failed with errcode = %d\n", ret);
	return (-1);
    }

    return PVFS_sys_create(filename, resp_look.ref, attr, &credentials,
			   NULL, NULL, &resp_create);

}

/* Preconditions: Parameters must be valid
 * Parameters: comm - special pts communicator, rank - the rank of the process, buf -  * (not used), rawparams - configuration information to specify which function to test
 * Postconditions: 0 if no errors and nonzero otherwise
 */
int test_null_params(MPI_Comm * comm __unused,
		     int rank,
		     char *buf __unused,
		     void *rawparams)
{
    int ret = -1;
    null_params *params;

    params = (null_params *) rawparams;
    /* right now, the system interface isn't threadsafe, so we just want to run with one process. */
    if (rank == 0)
    {
	if (params->p1 >= 0 && params->p2 >= 0)
	{
	    switch (params->p1)
	    {
	    case 0:
		fprintf(stderr, "[test_null_params] test_system_init %d\n",
			params->p2);
		ret = test_system_init();
		if(ret >= 0){
		    PVFS_perror("test_system_init",ret);
		    return ret;
		}
		return 0;
	    case 1:
		fprintf(stderr, "[test_null_params] test_lookup %d\n",
			params->p2);
		ret = test_lookup(params->p2);
		if(ret >= 0){
		    PVFS_perror("test_lookup",ret);
		    return ret;
		}
		return 0;
	    case 2:
		fprintf(stderr, "[test_null_params] test_getattr %d\n",
			params->p2);
		ret = test_getattr(params->p2);
		if(ret >= 0){
		    PVFS_perror("test_getattr",ret);
		    return ret;
		}
		return 0;
	    case 3:
		fprintf(stderr, "[test_null_params] test_setattr %d\n",
			params->p2);
		ret = test_setattr();
		if(ret >= 0){
		    PVFS_perror("test_setattr",ret);
		    return ret;
		}
		return 0;
	    case 4:
		fprintf(stderr, "[test_null_params] test_mkdir %d\n",
			params->p2);
		ret = test_mkdir(params->p2);
		if(ret >= 0){
		    PVFS_perror("test_mkdir",ret);
		    return ret;
		}
		return 0;
	    case 5:
		fprintf(stderr, "[test_null_params] test_readdir %d\n",
			params->p2);
		ret = test_readdir(params->p2);
		if(ret >= 0){
		    PVFS_perror("test_readdir",ret);
		    return ret;
		}
		return 0;
	    case 6:
		fprintf(stderr, "[test_null_params] test_create %d\n",
			params->p2);
		ret = test_create(params->p2);
		if(ret >= 0){
		    PVFS_perror("test_create",ret);
		    return ret;
		}
		return 0;
	    case 7:
		fprintf(stderr, "[test_null_params] test_remove %d\n",
			params->p2);
		ret = test_remove(params->p2);
		if(ret >= 0){
		    PVFS_perror("test_remove",ret);
		    return ret;
		}
		return 0;
	    case 8:
		fprintf(stderr, "[test_null_params] test_rename %d\n",
			params->p2);
		ret = test_rename();
		if(ret >= 0){
		    PVFS_perror("test_rename",ret);
		    return ret;
		}
		return 0;
	    case 9:
		fprintf(stderr, "[test_null_params] test_symlink %d\n",
			params->p2);
		ret = test_symlink();
		if(ret >= 0){
		    PVFS_perror("test_symlink",ret);
		    return ret;
		}
		return 0;
	    case 10:
		fprintf(stderr, "[test_null_params] test_readlink %d\n",
			params->p2);
		ret = test_readlink();
		if(ret >= 0){
		    PVFS_perror("test_readlink",ret);
		    return ret;
		}
		return 0;
	    case 11:
		fprintf(stderr, "[test_null_params] test_read %d\n",
			params->p2);
		ret = test_read(params->p2);
		if(ret >= 0){
		    PVFS_perror("test_read",ret);
		    return ret;
		}
		return 0;
	    case 12:
		fprintf(stderr, "[test_null_params] test_write %d\n",
			params->p2);
		ret = test_write(params->p2);
		if(ret >= 0){
		    PVFS_perror("test_write",ret);
		    return ret;
		}
		return 0;
	    case 99:
		fprintf(stderr, "[test_null_params] init_file %d\n",
			params->p2);
		return init_file();
	    default:
		fprintf(stderr, "Error: invalid param\n");
		return -2;
	    }
	}
    }
    return ret;
}

/*
 * Local variables:
 *  c-indent-level: 4
 *  c-basic-offset: 4
 * End:
 *
 * vim: ts=8 sts=4 sw=4 expandtab
 */
