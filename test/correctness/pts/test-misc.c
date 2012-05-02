/*
 * (C) 2002 Clemson University and The University of Chicago
 *
 * See COPYING in top-level directory.
 */

/* 
 * test-misc: These are tests that really don't fall into a good catagory and are not big enough to have their own individual file
 * Author: Michael Speth
 * Date: 6/26/2003
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <assert.h>

#include "client.h"
#include "mpi.h"
#include "pts.h"
#include "pvfs-helper.h"
#include "pvfs2-util.h"
#include "null_params.h"
#include "pvfs2-request.h"
#include "test-misc.h"

static int test_meta_fields(int testcase){
    int fs_id, ret;
    PVFS_credentials credentials;
    PVFS_object_ref pinode_refn;
    PVFS_sysresp_lookup resp_lookup;
    PVFS_sys_attr attr;
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
    attr.mask = PVFS_ATTR_SYS_ALL_NOSIZE;
    attr.owner = credentials.uid;
    attr.group = credentials.gid;
    attr.perms = 1877;
    attr.atime = attr.mtime = attr.ctime = 0xdeadbeef;
    attr.objtype = PVFS_TYPE_METAFILE;

    switch(testcase){
	case 0:
	    attr.owner = 555;
	    ret = PVFS_sys_setattr(pinode_refn, attr, &credentials);
	    break;
	case 1:
	    attr.group = 555;
	    ret = PVFS_sys_setattr(pinode_refn, attr, &credentials);
	    break;
	case 2:
	    attr.perms = 888;
	    ret = PVFS_sys_setattr(pinode_refn, attr, &credentials);
	    break;
	case 3:
	    attr.atime = 555;
	    ret = PVFS_sys_setattr(pinode_refn, attr, &credentials);
	    break;
	case 4:
	    attr.mtime = 555;
	    ret = PVFS_sys_setattr(pinode_refn, attr, &credentials);
	    break;
	case 5:
	    attr.ctime = 555;
	    ret = PVFS_sys_setattr(pinode_refn, attr, &credentials);
	    break;
	case 6:
	    attr.mask = 2003;
	    ret = PVFS_sys_setattr(pinode_refn, attr, &credentials);
	    break;
    }
/*     finalize_sysint(); */
    return ret;
}

static int test_permissions(int testcase){
    PVFS_credentials credentials;
    PVFS_sysresp_lookup resp_lk;
    PVFS_Request req_io;
    PVFS_Request req_mem;
    PVFS_sysresp_io resp_io;
    PVFS_offset file_req_offset;
    char *filename;
    char io_buffer[100];
    int fs_id, ret;

    file_req_offset = 0;
    filename = (char *) malloc(sizeof(char) * 100);
    filename = strcpy(filename, "name");

    memset(&req_io, 0, sizeof(PVFS_Request));
    memset(&req_mem, 0, sizeof(PVFS_Request));
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

    switch (testcase)
    {
    case 0:
	credentials.uid = 555;
        break;
    case 1:
	credentials.gid = 555;
        break;
    case 2:
	ret = PVFS_sys_lookup(
            fs_id, "invalid_perms", &credentials,
            &resp_lk, PVFS2_LOOKUP_LINK_NO_FOLLOW);
	if (ret < 0)
	{
	    debug_printf("test_pvfs_datatype_hvector: lookup failed "
                     "on %s\n", filename);
	}
	break;
    }
    ret = PVFS_sys_read(resp_lk.ref, req_io, file_req_offset,io_buffer, req_mem,
                          &credentials, &resp_io);
/*     finalize_sysint(); */
    return ret;

}
static int test_size_after_write(void){
    PVFS_credentials credentials;
    PVFS_sysresp_lookup resp_lk;
    PVFS_Request req_io;
    PVFS_Request req_mem;
    PVFS_sysresp_io resp_io;
    PVFS_sysresp_getattr resp;
    PVFS_offset file_req_offset = 0;
    uint32_t attrmask;
    char *filename;
    char io_buffer[100];
    int fs_id, ret, i;
    PVFS_size oldsize;

    attrmask = PVFS_ATTR_SYS_ALL_NOSIZE;

    filename = (char *) malloc(sizeof(char) * 100);
    filename = strcpy(filename, "name");

    memset(&req_io, 0, sizeof(PVFS_Request));
    memset(&req_mem, 0, sizeof(PVFS_Request));
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
    if((ret = PVFS_sys_getattr(resp_lk.ref, attrmask, &credentials, &resp)) < 0)
	return ret;

    oldsize = resp.attr.size;

    for(i = 0; i < 100; i++)
    {
	io_buffer[i] = 'a';
    }

    ret = PVFS_sys_write(resp_lk.ref, req_io, file_req_offset, io_buffer, req_mem,
                           &credentials, &resp_io);
    if(ret < 0){
        debug_printf("write failed on %s\n", filename);
    }

    ret = PVFS_sys_getattr(resp_lk.ref, attrmask, &credentials, &resp);
    if (ret < 0)
    {
        debug_printf("getattr failed on %s\n", filename);
    }
    if(resp.attr.size != 100 + oldsize){
	return -1;
    }
/*     finalize_sysint(); */
    return 0;
}

static int test_sparse_files(void){
    PVFS_credentials credentials;
    PVFS_sysresp_lookup resp_lk;
    PVFS_Request req_io;
    PVFS_Request req_mem;
    PVFS_sysresp_io resp_io;
    PVFS_sysresp_getattr resp;
    PVFS_offset file_req_offset = 0;
    uint32_t attrmask;
    char *filename;
    char io_buffer[100];
    int fs_id, ret, i;
    PVFS_size oldsize;

    attrmask = PVFS_ATTR_SYS_ALL_NOSIZE;

    filename = (char *) malloc(sizeof(char) * 100);
    filename = strcpy(filename, "sparse");

    memset(&req_io, 0, sizeof(PVFS_Request));
    memset(&req_mem, 0, sizeof(PVFS_Request));
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
    if((ret = PVFS_sys_getattr(resp_lk.ref, attrmask, &credentials, &resp)) < 0)
	return ret;

    oldsize = resp.attr.size;

    assert(0);
    /* TODO: what is this?  We shouldn't edit these fields directly -Phil */
#if 0
    switch(testcase)
    {
    case 0:
	req_io->offset = 78000;
	break;
    case 1:
	req_io->offset = 150000;
	break;
    case 2:
	req_io->offset = 330000;
	break;
    }
#endif

    for(i = 0; i < 100; i++)
    {
	io_buffer[i] = 'a';
    }

    ret = PVFS_sys_write(resp_lk.ref, req_io, file_req_offset, io_buffer, req_mem,
                           &credentials, &resp_io);
    if(ret < 0){
	debug_printf("write failed on %s\n", filename);
    }

    ret = PVFS_sys_getattr(resp_lk.ref, attrmask, &credentials, &resp);
    if (ret < 0)
    {
        debug_printf("getattr failed on %s\n", filename);
    }
    if(resp.attr.size != 100 + oldsize){
	return -1;
    }
/*     finalize_sysint(); */
    return 0;
}

static int test_read_sparse_files(void){
    PVFS_credentials credentials;
    PVFS_sysresp_lookup resp_lk;
    PVFS_Request req_io;
    PVFS_Request req_mem;
    PVFS_sysresp_io resp_io;
    PVFS_sysresp_getattr resp;
    PVFS_offset file_req_offset = 0;
    uint32_t attrmask;
    char *filename;
    char io_buffer[100];
    int fs_id, ret, i;

    attrmask = PVFS_ATTR_SYS_ALL_NOSIZE;

    filename = (char *) malloc(sizeof(char) * 100);
    filename = strcpy(filename, "sparse2");

    memset(&req_io, 0, sizeof(PVFS_Request));
    memset(&req_mem, 0, sizeof(PVFS_Request));
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
    if((ret = PVFS_sys_getattr(resp_lk.ref, attrmask, &credentials, &resp)) < 0)
	return ret;

    assert(0);
    /* TODO: what's this?  we shouldn't edit these fields directly -Phil */
#if 0
    switch(testcase)
    {
    case 0:
	req_io->offset = 78000;
	break;
    case 1:
	req_io->offset = 150000;
	break;
    case 2:
	req_io->offset = 330000;
	break;
    }
#endif

    for(i = 0; i < 100; i++)
    {
	io_buffer[i] = 'a';
    }
    ret = PVFS_sys_write(resp_lk.ref, req_io, file_req_offset, io_buffer, req_mem,
                           &credentials, &resp_io);
    if(ret < 0){
	debug_printf("write failed on %s\n", filename);
    }
    ret = PVFS_sys_read(resp_lk.ref, req_io, file_req_offset, io_buffer, req_mem,
                           &credentials, &resp_io);
    if(ret < 0){
	debug_printf("write failed on %s\n", filename);
    }

/*     finalize_sysint(); */
    for(i = 0; i < 100; i++)
    {
	if(io_buffer[i] != 0){
	    return -1;
	}
    }

    /* check for whats in the buffer */
    return 0;
}

static int test_allcat(int testcase)
{
    int ret, fs_id;
    PVFS_credentials credentials;
    PVFS_sysresp_lookup resp_look;
    PVFS_size size = 0, oldsize = 0;
    PVFS_sysresp_getattr resp;
    uint32_t attrmask;
    char *filename;

    attrmask = PVFS_ATTR_SYS_ALL_NOSIZE;

    filename = (char *) malloc(sizeof(char) * 100);
    filename = strcpy(filename, "altrun");

    PVFS_util_gen_credentials(&credentials);

    if (initialize_sysint() < 0)
    {
        debug_printf("UNABLE TO INIT THE SYSTEM INTERFACE\n");
        return -1;
    }
    fs_id = pvfs_helper.fs_id;

    /* get file */
    ret = PVFS_sys_lookup(fs_id, filename, &credentials,
                          &resp_look, PVFS2_LOOKUP_LINK_NO_FOLLOW);
    if (ret < 0)
    {
        printf("Lookup failed with errcode = %d\n", ret);
        return (-1);
    }
    if((ret = PVFS_sys_getattr(resp_look.ref, attrmask, &credentials, &resp)) < 0)
	return ret;

    oldsize = resp.attr.size;
    switch(testcase)
    {
    case 0:
	size = 5;
	/* ret =  PVFS_sys_allocate(resp_look.ref, size ); */
	break;
    case 1:
	size = 100000;
	/* ret =  PVFS_sys_allocate(resp_look.ref, size ); */
	break;
    case 2:
	size = 1000000;
	/* ret =  PVFS_sys_allocate(resp_look.ref, size ); */
	break;
    }
    /* get file */
    ret = PVFS_sys_lookup(fs_id, filename, &credentials,
                          &resp_look, PVFS2_LOOKUP_LINK_NO_FOLLOW);
    if (ret < 0)
    {
        printf("Lookup failed with errcode = %d\n", ret);
        return (-1);
    }
    ret = PVFS_sys_getattr(resp_look.ref, attrmask, &credentials, &resp);
    if (ret < 0)
    {
        debug_printf("getattr failed on %s\n", filename);
    }
    if(resp.attr.size != (size + oldsize)){
	return -1;
    }
    return 0;
}

static int test_truncat(int testcase)
{
    int ret, fs_id;
    PVFS_credentials credentials;
    PVFS_sysresp_lookup resp_look;
    PVFS_size size = 0, oldsize = 0;
    PVFS_sysresp_getattr resp;
    uint32_t attrmask;
    char *filename;

    attrmask = PVFS_ATTR_SYS_ALL_NOSIZE;

    filename = (char *) malloc(sizeof(char) * 100);
    filename = strcpy(filename, "altrun");

    PVFS_util_gen_credentials(&credentials);

    if (initialize_sysint() < 0)
    {
        debug_printf("UNABLE TO INIT THE SYSTEM INTERFACE\n");
        return -1;
    }
    fs_id = pvfs_helper.fs_id;

    /* get file */
    ret = PVFS_sys_lookup(fs_id, filename, &credentials,
                          &resp_look, PVFS2_LOOKUP_LINK_NO_FOLLOW);
    if (ret < 0)
    {
        printf("Lookup failed with errcode = %d\n", ret);
        return (-1);
    }
    if((ret = PVFS_sys_getattr(resp_look.ref, attrmask, &credentials, &resp)) < 0)
	return ret;

    oldsize = resp.attr.size;

    switch(testcase)
    {
    case 0:
	size = 1000000;
	ret = PVFS_sys_truncate(resp_look.ref, size, &credentials);
	break;
    case 1:
	size = 100000;
	ret = PVFS_sys_truncate(resp_look.ref, size, &credentials);
	break;
    case 2:
	size = 5;
	ret = PVFS_sys_truncate(resp_look.ref, size, &credentials);
	break;
    }

    /* get file */
    ret = PVFS_sys_lookup(fs_id, filename, &credentials,
                          &resp_look, PVFS2_LOOKUP_LINK_NO_FOLLOW);
    if (ret < 0)
    {
        printf("Lookup failed with errcode = %d\n", ret);
        return (-1);
    }
    if((ret = PVFS_sys_getattr(resp_look.ref, attrmask, &credentials, &resp)) < 0)
	return ret;

    if(resp.attr.size != (oldsize - size))
    {
	return -1;
    }
    return 0;
}

static int test_read_beyond(void){
    PVFS_sysresp_lookup resp_lk;
    PVFS_Request req_io;
    PVFS_Request req_mem;
    PVFS_sysresp_io resp_io;
    PVFS_credentials credentials;
    PVFS_offset file_req_offset = 0;
    char *filename;
    char *io_buffer;
    int fs_id, ret;
    PVFS_sysresp_getattr resp;
    uint32_t attrmask;

    attrmask = PVFS_ATTR_SYS_ALL_NOSIZE;

    filename = (char *) malloc(sizeof(char) * 100);
    filename = strcpy(filename, "name");

    memset(&req_io, 0, sizeof(PVFS_Request));
    memset(&req_mem, 0, sizeof(PVFS_Request));
    memset(&resp_io, 0, sizeof(PVFS_sysresp_io));

    memset(&resp_lk, 0, sizeof(PVFS_sysresp_lookup));

    PVFS_util_gen_credentials(&credentials);

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
    if((ret = PVFS_sys_getattr(resp_lk.ref, attrmask, &credentials, &resp)) < 0)
	return ret;
    io_buffer = malloc(sizeof(char)*(size_t)resp.attr.size+100);

    ret = PVFS_sys_read(resp_lk.ref, req_io, file_req_offset, io_buffer, req_mem, &credentials, &resp_io);
    if(ret < 0){
	debug_printf("write failed on %s\n", filename);
    }

/*     finalize_sysint(); */
    return ret;
}

static int test_write_beyond(void){
    PVFS_sysresp_lookup resp_lk;
    PVFS_Request req_io;
    PVFS_Request req_mem;
    PVFS_sysresp_io resp_io;
    PVFS_credentials credentials;
    char *filename;
    char *io_buffer;
    int fs_id, ret, i;
    PVFS_size oldsize;
    PVFS_sysresp_getattr resp;
    PVFS_offset file_req_offset = 0;
    uint32_t attrmask;

    attrmask = PVFS_ATTR_SYS_ALL_NOSIZE;

    filename = (char *) malloc(sizeof(char) * 100);
    filename = strcpy(filename, "name");

    memset(&req_io, 0, sizeof(PVFS_Request));
    memset(&req_mem, 0, sizeof(PVFS_Request));
    memset(&resp_io, 0, sizeof(PVFS_sysresp_io));

    memset(&resp_lk, 0, sizeof(PVFS_sysresp_lookup));

    PVFS_util_gen_credentials(&credentials);

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
    if((ret = PVFS_sys_getattr(resp_lk.ref, attrmask, &credentials, &resp)) < 0)
	return ret;
    io_buffer = malloc(sizeof(char)*(size_t)resp.attr.size+100);

    /* req_io.size = resp_io.size + 100; */
    oldsize = resp.attr.size +100;
    for(i = 0; i < oldsize; i++)
    {
	io_buffer[i] = 'a';
    }
    ret = PVFS_sys_write(resp_lk.ref, req_io, file_req_offset, io_buffer, req_mem,
                           &credentials, &resp_io);
    if(ret < 0){
	debug_printf("write failed on %s\n", filename);
    }

/*     finalize_sysint(); */
    return ret;
}

static int test_files_as_dirs(int testcase)
{
    int ret = 1, fs_id;
    PVFS_sys_attr attr;
    PVFS_credentials credentials;
    PVFS_sysresp_lookup resp_look;
    PVFS_sysresp_create resp_create;
    char *filename;

    filename = (char *) malloc(sizeof(char) * 100);
    filename = strcpy(filename, "name");

    PVFS_util_gen_credentials(&credentials);

    attr.mask = PVFS_ATTR_SYS_ALL_NOSIZE;
    attr.owner = credentials.uid;
    attr.group = credentials.gid;
    attr.perms = 1877;
    attr.atime = attr.mtime = attr.ctime = 0xdeadbeef;

    if (initialize_sysint() < 0)
    {
        debug_printf("UNABLE TO INIT THE SYSTEM INTERFACE\n");
        return -1;
    }
    fs_id = pvfs_helper.fs_id;

    switch(testcase)
    {
    case 0:
	/* get root */
	ret = PVFS_sys_lookup(fs_id, "/", &credentials,
                              &resp_look, PVFS2_LOOKUP_LINK_NO_FOLLOW);
	if (ret < 0)
	{
	    printf("Lookup failed with errcode = %d\n", ret);
	    return (-1);
	}

	ret = PVFS_sys_create("foo", resp_look.ref, attr, &credentials,
                           NULL, NULL, &resp_create);
	/* get root */
	ret = PVFS_sys_lookup(fs_id, "/foo", &credentials,
                              &resp_look, PVFS2_LOOKUP_LINK_NO_FOLLOW);
	if (ret < 0)
	{
	    printf("Lookup failed with errcode = %d\n", ret);
	    return (-1);
	}

	ret = PVFS_sys_create("bar", resp_look.ref, attr, &credentials,
                           NULL, NULL, &resp_create);
	break;
    case 1:
	/* Need to add some more interesting cases */
	break; 
    }

    return ret;
}

static int test_get_set_attr_empty(int testcase)
{
    int fs_id, ret;
    PVFS_credentials credentials;
    PVFS_object_ref pinode_refn;
    PVFS_sysresp_lookup resp_lookup;
    PVFS_sys_attr attr;
    PVFS_sysresp_getattr resp;
    uint32_t attrmask;
    char *name;

    ret = -2;
    name = (char *) malloc(sizeof(char) * 100);
    name = strcpy(name, "nofileyea");

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
        fprintf(stderr, "lookup failed which it should but keep going\n");
	
       /* return ret; */
    }

    pinode_refn = resp_lookup.ref;
    attr.mask = PVFS_ATTR_SYS_ALL_NOSIZE;
    attr.owner = credentials.uid;
    attr.group = credentials.gid;
    attr.perms = 1877;
    attr.atime = attr.mtime = attr.ctime = 0xdeadbeef;
    attr.objtype = PVFS_TYPE_METAFILE;

    attrmask = PVFS_ATTR_SYS_ALL_NOSIZE;

    switch(testcase){
	case 0:
	    ret = PVFS_sys_setattr(pinode_refn, attr, &credentials);
	    break;
	case 1:
	    ret = PVFS_sys_getattr(pinode_refn, attrmask, &credentials, &resp);
	    break;
    }

    return ret;
}

static int test_lookup_empty(void)
{
    int fs_id, ret;
    PVFS_credentials credentials;
    PVFS_sysresp_lookup resp_lookup;
    char *name;

    ret = -2;
    name = (char *) malloc(sizeof(char) * 100);
    name = strcpy(name, "nofileyea");

    if (initialize_sysint() < 0)
    {
        debug_printf("ERROR UNABLE TO INIT SYSTEM INTERFACE\n");
        return -1;
    }
    fs_id = pvfs_helper.fs_id;

    PVFS_util_gen_credentials(&credentials);
    ret = PVFS_sys_lookup(fs_id, name, &credentials,
                          &resp_lookup, PVFS2_LOOKUP_LINK_NO_FOLLOW);

    return ret;
}

static int test_io_on_dir(int testcase)
{
    int fs_id, ret,i;
    PVFS_credentials credentials;
    PVFS_sysresp_lookup resp_lookup;
    PVFS_Request req_io;
    PVFS_Request req_mem;
    PVFS_sysresp_io resp_io;
    PVFS_offset file_req_offset = 0;
    char *name;
    char io_buffer[100];

    ret = -2;
    name = (char *) malloc(sizeof(char) * 100);
    name = strcpy(name, "/");

    if (initialize_sysint() < 0)
    {
        debug_printf("ERROR UNABLE TO INIT SYSTEM INTERFACE\n");
        return -1;
    }
    fs_id = pvfs_helper.fs_id;

    PVFS_util_gen_credentials(&credentials);
    if((ret = PVFS_sys_lookup(
            fs_id, name, &credentials,
            &resp_lookup, PVFS2_LOOKUP_LINK_NO_FOLLOW)) < 0)
    {
	fprintf(stderr,"lookup failed\n");
	return ret;
    }

    memset(&req_io, 0, sizeof(req_io));
    memset(&req_mem, 0, sizeof(req_mem));

    switch(testcase)
    {
	case 0:
	    ret = PVFS_sys_read(resp_lookup.ref, req_io, file_req_offset, io_buffer, req_mem, &credentials, &resp_io);
	    break;
	case 1:
	    for(i = 0; i < 100; i++)
	    {
		io_buffer[i] = 'a';
	    }
	    ret = PVFS_sys_write(resp_lookup.ref, req_io, file_req_offset, io_buffer, req_mem, &credentials, &resp_io);
	    break;

    }

/*     finalize_sysint(); */
    return ret;
}

static int test_remove_nonempty_dir(int testcase)
{
    PVFS_credentials credentials;
    PVFS_sysresp_lookup resp_look;
    char *filename;
    int ret; 
    int fs_id;
    
    ret = -2;
    filename = (char *) malloc(sizeof(char) * 100);
    filename = strcpy(filename, "/");
    
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
    switch (testcase)
    {
    case 0:
        ret = PVFS_sys_remove(NULL, resp_look.ref, &credentials);
        break;
    default:
        fprintf(stderr, "Error: invalid case number \n");
    }
    return ret;

}

static int init_files(void)
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
    attr.mask = PVFS_ATTR_SYS_ALL_NOSIZE;
    attr.owner = credentials.uid;
    attr.group = credentials.gid;
    attr.perms = 1877;
    attr.atime = attr.mtime = attr.ctime = 0xdeadbeef;

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

    ret = PVFS_sys_create(filename, resp_look.ref, attr, &credentials,
                           NULL, NULL, &resp_create);

    /* create sparse file */
    filename = strcpy(filename, "sparse");

    ret = PVFS_sys_create(filename, resp_look.ref, attr, &credentials,
                           NULL, NULL, &resp_create);

    /* create a file for testing alocate and truncate*/
    filename = strcpy(filename, "altrun");

    ret = PVFS_sys_create(filename, resp_look.ref, attr, &credentials,
                           NULL, NULL, &resp_create);


    filename = strcpy(filename, "invalid_perms");

    credentials.uid = 444;
    credentials.gid = 444;

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
int test_misc(MPI_Comm * comm __unused,
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
		fprintf(stderr, "[test_misc] test_meta_fields %d\n",
			params->p2);
		ret = test_meta_fields(params->p2);
		if(ret >= 0){
		    PVFS_perror("test_meta_fields",ret);
		    return ret;
		}
		return 0;
	    case 1:
		fprintf(stderr, "[test_misc] test_permissions %d\n",
			params->p2);
		ret = test_permissions(params->p2);
		if(ret >= 0){
		    PVFS_perror("test_permissions",ret);
		    return ret;
		}
		return 0;
	    case 2:
		fprintf(stderr, "[test_misc] test_size_after_write %d\n",
			params->p2);
		ret = test_size_after_write();
		if(ret >= 0){
		    PVFS_perror("test_size_after_write",ret);
		    return ret;
		}
		return 0;
	    case 3:
		fprintf(stderr, "[test_misc] test_sparse_files %d\n", params->p2);
		ret = test_sparse_files();
		if(ret >= 0){
		    PVFS_perror("test_mkdir",ret);
		    return ret;
		}
		return 0;
	    case 4:
		fprintf(stderr, "[test_misc] test_read_sparse_files %d\n",
			params->p2);
		ret = test_read_sparse_files();
		if(ret >= 0){
		    PVFS_perror("test_read_sparse_files",ret);
		    return ret;
		}
		return 0;
	    case 5:
		fprintf(stderr, "[test_misc] test_allcat %d\n",
			params->p2);
		ret = test_allcat(params->p2);
		if(ret >= 0){
		    PVFS_perror("test_allcat",ret);
		    return ret;
		}
		return 0;
	    case 6:
		fprintf(stderr, "[test_misc] test_truncat %d\n",
			params->p2);
		ret = test_truncat(params->p2);
		if(ret >= 0){
		    PVFS_perror("test_truncat",ret);
		    return ret;
		}
		return 0;
	    case 7:
		fprintf(stderr, "[test_misc] test_read_beyond %d\n",
			params->p2);
		ret = test_read_beyond();
		if(ret >= 0){
		    PVFS_perror("test_read_beyond",ret);
		    return ret;
		}
		return 0;
	    case 8:
		fprintf(stderr, "[test_misc] test_write_beyond %d\n",
			params->p2);
		ret = test_write_beyond();
		if(ret >= 0){
		    PVFS_perror("test_symlink",ret);
		    return ret;
		}
		return 0;
	    case 9:
		fprintf(stderr, "[test_misc] test_files_as_dirs %d\n",
			params->p2);
		ret = test_files_as_dirs(params->p2);
		if(ret >= 0){
		    PVFS_perror("test_files_as_dirs",ret);
		    return ret;
		}
		return 0;
	    case 10:
		fprintf(stderr, "[test_misc] test_get_set_attr_emtpy %d\n", params->p2);
		ret = test_get_set_attr_empty(params->p2);
		if(ret >= 0){
		    PVFS_perror("test_get_set_attr_empty",ret);
		    return ret;
		}
		return 0;
	    case 11:
		fprintf(stderr, "[test_misc] test_lookup_empty %d\n", params->p2);
		ret = test_lookup_empty();
		if(ret >= 0){
		    PVFS_perror("test_lookup_empty",ret);
		    return ret;
		}
		return 0;
	    case 12:
		fprintf(stderr, "[test_misc] test_io_on_dir %d\n",
			params->p2);
		ret = test_io_on_dir(params->p2);
		if(ret >= 0){
		    PVFS_perror("test_io_on_dir",ret);
		    return ret;
		}
		return 0;
	    case 13:
		fprintf(stderr, "[test_misc] test_remove_nonempty_dir %d\n",
			params->p2);
		ret = test_remove_nonempty_dir(params->p2);
		if(ret >= 0){
		    PVFS_perror("test_remove_nonempty_dir",ret);
		    return ret;
		}
		return 0;
	    case 99:
		fprintf(stderr, "[test_misc] init_files %d\n",
			params->p2);
		ret = init_files();
		if(ret >= 0){
		    PVFS_perror("init_files",ret);
		    return ret;
		}
		return 0;
	    default:
		fprintf(stderr, "Error: invalid param %d\n", params->p1);
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
