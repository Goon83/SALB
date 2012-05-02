/*
 * (C) 2002 Clemson University.
 *
 * See COPYING in top-level directory.
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

#include "pvfs2-types.h"
#include "gossip.h"
#include "pvfs2-debug.h"
#include "pint-request.h"
#include "pint-distribution.h"
#include "pint-dist-utils.h"

void PINT_Dist_dump(PINT_dist *dist);

int main(int argc, char **argv)
{
    PINT_dist *d;
    int ret = -1;
    PVFS_offset tmp_off = 0;
    PINT_request_file_data file_data;

    /* grab a distribution */
    PINT_dist_initialize(NULL);
    d = PINT_dist_create("simple_stripe");
    assert(d);

    ret = PINT_dist_lookup(d);
    assert(ret == 0);

    PINT_dist_dump(d);

    /* Initialize the file data */
    memset(&file_data, 0, sizeof(file_data));
    file_data.server_ct = 4;
    file_data.server_nr = 0;
    
    /* easy case */
    tmp_off = d->methods->logical_to_physical_offset(d->params,&file_data,100);
    printf("offset: %lld\n", (long long)tmp_off);


    /* just before strip */
    tmp_off = d->methods->logical_to_physical_offset(d->params,
                                                     &file_data,
                                                     (64*1024-1));
    printf("offset: %lld\n", (long long)tmp_off);

    /* at strip */
    tmp_off = d->methods->logical_to_physical_offset(d->params,
                                                     &file_data,
                                                     (64*1024));
    printf("offset: %lld\n", (long long)tmp_off);

    /* just after strip */
    tmp_off = d->methods->logical_to_physical_offset(d->params,
                                                     &file_data,
                                                     (64*1024+1));
    printf("offset: %lld\n", (long long)tmp_off);

    /* wrap around tests */
    tmp_off = d->methods->logical_to_physical_offset(d->params,
                                                     &file_data,
                                                     (64*1024*4+1));
    printf("offset: %lld\n", (long long)tmp_off);

    /* try a different io server */
    file_data.server_nr = 3;
    tmp_off = d->methods->logical_to_physical_offset(d->params,
                                                     &file_data,
                                                     (64*1024-1));
    printf("offset: %lld\n", (long long)tmp_off);

    /* same as above, but in his region w/ wrap around */
    tmp_off = d->methods->logical_to_physical_offset(d->params,
                                                     &file_data,
                                                     (64*1024*7+15));
    printf("offset: %lld\n", (long long)tmp_off);


    /* free dist */
    PINT_dist_free(d);

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
