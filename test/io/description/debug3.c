/*
 * (C) 2002 Clemson University.
 *
 * See COPYING in top-level directory.
 */       

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <pvfs2-types.h>
#include <gossip.h>
#include <pvfs2-debug.h>

#include <pint-distribution.h>
#include <pint-dist-utils.h>
#include <pvfs2-request.h>
#include <pint-request.h>

#include <debug.h>

#define SEGMAX 16
#define BYTEMAX (4*1024*1024)

PVFS_offset exp1_offset [] = {
	65536,
	196608,
	327680,
	458752,
	589824,
	720896,
	851968,
	983040,
	1114112,
	1245184,
	1376256,
	1507328,
	1638400,
	1769472,
	1900544,
	2031616,
	2162688,
	2293760,
	2424832,
	2555904,
	2686976,
	2818048,
	2949120,
	3080192,
	3211264,
	3342336,
	3473408,
	3604480,
	3735552,
	3866624,
	3997696,
	4128768,
	4259840
};

PVFS_size exp1_size [] = {
	65536,
	65536,
	65536,
	65536,
	65536,
	65536,
	65536,
	65536,
	65536,
	65536,
	65536,
	65536,
	65536,
	65536,
	65536,
	65536,
	65536,
	65536,
	65536,
	65536,
	65536,
	65536,
	65536,
	65536,
	65536,
	65536,
	65536,
	65536,
	65536,
	65536,
	65536,
	65536,
	65536
};

PINT_Request_result expected[] =
{{
	  offset_array : &exp1_offset[0],
	  size_array : &exp1_size[0],
	  segmax : SEGMAX,
	  segs : 16,
	  bytes : 16*65536
}, {
	  offset_array : &exp1_offset[16],
	  size_array : &exp1_size[16],
	  segmax : SEGMAX,
	  segs : 16,
	  bytes : 16*65536
}, {
	  offset_array : &exp1_offset[32],
	  size_array : &exp1_size[32],
	  segmax : SEGMAX,
	  segs : 1,
	  bytes : 65536
}};


int request_debug(void)
{
	int i;
	PINT_Request *r;
	PINT_Request *r_enc;
	PINT_Request *r_dec;
	PINT_Request_state *rs1;
	PINT_request_file_data rf1;
	PINT_Request_result seg1;
	int ret = -1;
	int pack_size = 0;

	/* DESCRIPTION: 
	 * in this case, we are doing a single write, of size 4390228,
	 * at offset 20M.  There are two servers.  We are looking at
	 * the output of the request processing code on the client side
	 */

	/* PVFS_Process_request arguments */
	int retval;
	int32_t blocklength = 4390228;
	PVFS_size displacement = 20*1024*1024;

	PVFS_Request_indexed(1, &blocklength, &displacement, PVFS_BYTE, &r);

	/* allocate a new request and pack the original one into it */
	pack_size = PINT_REQUEST_PACK_SIZE(r);
	r_enc = (PINT_Request*)malloc(pack_size);
	ret = PINT_request_commit(r_enc, r);
	if(ret < 0)
	{
		fprintf(stderr, "PINT_Request_commit() failure.\n");
		return(-1);
	}
	ret = PINT_request_encode(r_enc);
	if(ret < 0)
	{
		fprintf(stderr, "PINT_Request_encode() failure.\n");
		return(-1);
	}

	/* decode the encoded request (hopefully ending up with something
	 * equivalent to the original request)
	 */
	r_dec = (PINT_Request*)malloc(pack_size);
	memcpy(r_dec, r_enc, pack_size);
	free(r_enc);
	free(r);
	ret = PINT_request_decode(r_dec);
	if(ret < 0)
	{
		fprintf(stderr, "PINT_request_decode() failure.\n");
		return(-1);
	}

	rs1 = PINT_new_request_state(r_dec);

	/* set up file data for each server */
	PINT_dist_initialize(NULL);
	rf1.server_nr = 1;
	rf1.server_ct = 2;
	rf1.fsize = 0;
	rf1.dist = PINT_dist_create("simple_stripe");
	rf1.extend_flag = 1;
	PINT_dist_lookup(rf1.dist);

	/* set up response struct */
	seg1.offset_array = (int64_t *)malloc(SEGMAX * sizeof(int64_t));
	seg1.size_array = (int64_t *)malloc(SEGMAX * sizeof(int64_t));
	seg1.segmax = SEGMAX;
	seg1.bytemax = BYTEMAX;
	seg1.bytes = 0;
	seg1.segs = 0;

   /* Turn on debugging */
	if (gossipflag)
	{
		gossip_enable_stderr();
		gossip_set_debug_mask(1,GOSSIP_REQUEST_DEBUG);
	}

	i = 0;

	printf("\n************************************\n");
	printf("One request in CLIENT mode size 4390228 contiguous server 0 of 2\n");
	printf("Simple stripe, default stripe size (64K)\n");
	printf("Offset 20M, file size 0, extend flag\n");
	printf("\n************************************\n");
	do
	{
		seg1.bytes = 0;
		seg1.segs = 0;

		/* process request */
		/* note that bytemax is exactly large enough to hold all of the
		 * data that I should find here
		 */
		retval = PINT_process_request(rs1, NULL, &rf1, &seg1, PINT_CLIENT);

		if(retval >= 0)
		{
			 prtseg(&seg1,"Results obtained");
	       prtseg(&expected[i],"Results expected");
	       cmpseg(&seg1,&expected[i]);
	   }

      i++;

		if(PINT_REQUEST_DONE(rs1) && i < 3)
		{
			fprintf(stderr, "  AAIIEEE! Why am I done?\n");
		}
	} while(!PINT_REQUEST_DONE(rs1) && retval >= 0);
	
	if(retval < 0)
	{
		fprintf(stderr, "Error: PINT_process_request() failure.\n");
		return(-1);
	}
	if(PINT_REQUEST_DONE(rs1))
	{
		printf("**** first request done.\n");
	}

	return 0;
}
