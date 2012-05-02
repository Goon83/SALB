/*
 * (C) 2002 Clemson University and The University of Chicago
 *
 * See COPYING in top-level directory.
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <getopt.h>

#include <trove.h>

#include "trove-test.h"

char storage_space[SSPACE_SIZE] = "/tmp/trove-test-space";
char file_system[FS_SIZE] = "fs-foo";

TROVE_handle requested_file_handle = 9999;
struct teststruct {
    int a;
    long b;
    int size;
    char *string;
};

int main(int argc, char ** argv )
{

    int ret, count;
    TROVE_size buffsz;
    TROVE_op_id op_id;
    TROVE_coll_id coll_id;
    TROVE_ds_state state;
    TROVE_handle file_handle, parent_handle;
    TROVE_extent cur_extent;
    TROVE_handle_extent_array extent_array;
    TROVE_context_id trove_context = -1;

    struct teststruct foo = { 8, 8, 0, NULL };
    struct teststruct bar;

    foo.string = strdup("monkey");
    foo.size = strlen(foo.string);

    ret = trove_initialize(
        TROVE_METHOD_DBPF, NULL, storage_space, 0);
    if (ret < 0 ) {
	fprintf(stderr, "initialize failed\n");
	return -1;
    }

    ret = trove_collection_lookup(
        TROVE_METHOD_DBPF, file_system, &coll_id, NULL, &op_id);
    if (ret < 0 ) {
	fprintf(stderr, "collection lookup failed");
	return -1;
    }

    ret = trove_open_context(coll_id, &trove_context);
    if (ret < 0)
    {
        fprintf(stderr, "trove_open_context failed\n");
        return -1;
    }

    ret = path_lookup(coll_id, "/", &parent_handle);
    if (ret < 0 ) {
	return -1;
    }

    file_handle = 0;
    cur_extent.first = cur_extent.last = requested_file_handle;
    extent_array.extent_count = 1;
    extent_array.extent_array = &cur_extent;
    ret = trove_dspace_create(coll_id,
			      &extent_array,
                              &file_handle,
			      TROVE_TEST_BSTREAM,
			      NULL,
			      TROVE_FORCE_REQUESTED_HANDLE | TROVE_SYNC,
			      NULL,
                              trove_context,
			      &op_id,
                              NULL);
    while (ret == 0) ret = trove_dspace_test(
        coll_id, op_id, trove_context, &count, NULL, NULL, &state,
        TROVE_DEFAULT_TEST_TIMEOUT);
    if (ret < 0) {
	fprintf(stderr, "dspace create failed.\n");
	return -1;
    }

    /* not sure where to find the handle for bstream for the handle
     * generator.  store some keys into the collection? */

    buffsz = sizeof(foo);
    ret = trove_bstream_write_at(coll_id, file_handle, 
				 &foo, &buffsz,
				 0, 0, NULL, NULL,
                                 trove_context, &op_id,
                                 NULL);
    while ( ret == 0) ret = trove_dspace_test(
        coll_id, op_id, trove_context, &count, NULL, NULL, &state,
        TROVE_DEFAULT_TEST_TIMEOUT);
    if (ret < 0 ) {
	fprintf(stderr, "bstream write failed.\n");
	return -1;
    }
    ret = trove_bstream_write_at(coll_id, file_handle,
				 foo.string, &buffsz,
				 buffsz, 0, NULL, NULL,
                                 trove_context, &op_id,
                                 NULL);
    while ( ret == 0) ret = trove_dspace_test(
        coll_id, op_id, trove_context, &count, NULL, NULL, &state,
        TROVE_DEFAULT_TEST_TIMEOUT);
    if (ret < 0 ) {
	fprintf(stderr, "bstream write failed.\n");
	return -1;
    }

    buffsz = sizeof(bar);
    ret = trove_bstream_read_at(coll_id, file_handle,
				&bar, &buffsz,
				0, 0, NULL, NULL,
                                trove_context, &op_id,
                                NULL);
    while ( ret == 0) ret = trove_dspace_test(
        coll_id, op_id, trove_context, &count, NULL, NULL, &state,
        TROVE_DEFAULT_TEST_TIMEOUT);
    if (ret < 0 ) {
	fprintf(stderr, "bstream read failed.\n");
	return -1;
    }
    bar.string = malloc(bar.size + 1);
    ret = trove_bstream_read_at(coll_id, file_handle, 
				bar.string, &buffsz,
				buffsz, 0, NULL, NULL,
                                trove_context, &op_id,
                                NULL);
    while ( ret == 0) ret = trove_dspace_test(
        coll_id, op_id, trove_context, &count, NULL, NULL, &state,
        TROVE_DEFAULT_TEST_TIMEOUT);
    if (ret < 0 ) {
	fprintf(stderr, "bstream write failed.\n");
	return -1;
    }

    trove_close_context(coll_id, trove_context);
    trove_finalize(TROVE_METHOD_DBPF); 
    return 0;
}		
	
	
/*
 * Local variables:
 *  c-indent-level: 4
 *  c-basic-offset: 4
 * End:
 *
 * vim: ts=8 sts=4 sw=4 expandtab
 */
