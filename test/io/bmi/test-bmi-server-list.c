/*
 * (C) 2001 Clemson University and The University of Chicago
 *
 * See COPYING in top-level directory.
 */




/*
 * This is an example of a server program that uses the BMI 
 * library for communications
 */

#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#include "pvfs2.h"
#include "bmi.h"
#include "gossip.h"
#include "test-bmi.h"

/**************************************************************
 * Data structures 
 */

/* A little structure to hold program options, either defaults or
 * specified on the command line 
 */
struct options
{
    char *hostid;		/* host identifier */
};

/**************************************************************
 * Internal utility functions
 */

static struct options *parse_args(
    int argc,
    char *argv[]);


/**************************************************************/

int main(
    int argc,
    char **argv)
{

    struct options *user_opts = NULL;
    struct server_request *my_req = NULL;
    struct server_ack *my_ack = NULL;
    int ret = -1;
    PVFS_BMI_addr_t client_addr;
    void *recv_buffer1 = NULL;
    void *recv_buffer2 = NULL;
    bmi_op_id_t server_ops[2];
    bmi_error_code_t error_code;
    int outcount = 0;
    struct BMI_unexpected_info request_info;
    bmi_size_t actual_size;
    bmi_size_t size_list[2];
    void *buffer_list[2];
    int last = 0;
    int i = 0;
    bmi_context_id context;
    char method[24], *cp;
    int len;

    /* grab any command line options */
    user_opts = parse_args(argc, argv);
    if (!user_opts)
    {
	return (-1);
    }

    /* set debugging stuff */
    gossip_enable_stderr();
    gossip_set_debug_mask(0, GOSSIP_BMI_DEBUG_ALL);

    /* convert address to bmi method type by prefixing bmi_ */
    cp = strchr(user_opts->hostid, ':');
    if (!cp)
        return 1;
    len = cp - user_opts->hostid;
    strcpy(method, "bmi_");
    strncpy(method + 4, user_opts->hostid, len);
    method[4+len] = '\0';

    /* initialize local interface (default options) */
    ret = BMI_initialize(method, user_opts->hostid, BMI_INIT_SERVER);
    if (ret < 0)
    {
	errno = -ret;
	perror("BMI_initialize");
	return (-1);
    }

    ret = BMI_open_context(&context);
    if (ret < 0)
    {
	errno = -ret;
	perror("BMI_open_context()");
	return (-1);
    }

    /* wait for an initial request  */
    do
    {
	ret = BMI_testunexpected(1, &outcount, &request_info, 10);
    } while (ret == 0 && outcount == 0);
    if (ret < 0)
    {
	fprintf(stderr, "Request recv failure (bad state).\n");
	errno = -ret;
	perror("BMI_testunexpected");
	return (-1);
    }
    if (request_info.error_code != 0)
    {
	fprintf(stderr, "Request recv failure (bad state).\n");
	return (-1);
    }

    printf("Received a new request.\n");

    if (request_info.size != sizeof(struct server_request))
    {
	fprintf(stderr, "Bad Request!\n");
	exit(-1);
    }

    my_req = (struct server_request *) request_info.buffer;
    client_addr = request_info.addr;

    /* create an ack */
    my_ack = (struct server_ack *) BMI_memalloc(client_addr,
						sizeof(struct server_ack),
						BMI_SEND);
    if (!my_ack)
    {
	fprintf(stderr, "BMI_memalloc failed.\n");
	return (-1);
    }
    memset(my_ack, 0, sizeof(struct server_ack));

    /* create 2 buffers to recv into */
    recv_buffer1 = BMI_memalloc(client_addr, (my_req->size / 2), BMI_RECV);
    recv_buffer2 = BMI_memalloc(client_addr,
				(my_req->size - (my_req->size / 2)), BMI_RECV);
    if (!recv_buffer1 || !recv_buffer2)
    {
	fprintf(stderr, "BMI_memalloc failed.\n");
	return (-1);
    }
    buffer_list[0] = recv_buffer1;
    buffer_list[1] = recv_buffer2;
    size_list[0] = my_req->size / 2;
    size_list[1] = my_req->size - (my_req->size / 2);

    /* post the ack */
    ret = BMI_post_send(&(server_ops[1]), client_addr, my_ack,
			sizeof(struct server_ack), BMI_PRE_ALLOC, 0, NULL,
			context, NULL);
    if (ret < 0)
    {
	fprintf(stderr, "BMI_post_send_failure.\n");
	return (-1);
    }
    if (ret == 0)
    {
	/* turning this into a blocking call for testing :) */
	/* check for completion of ack send */
	do
	{
	    ret = BMI_test(server_ops[1], &outcount, &error_code,
			   &actual_size, NULL, 10, context);
	} while (ret == 0 && outcount == 0);

	if (ret < 0 || error_code != 0)
	{
	    fprintf(stderr, "ack send failed.\n");
	    return (-1);
	}
    }

    /* post the recv */
    ret = BMI_post_recv_list(&(server_ops[0]), client_addr, buffer_list,
			     size_list, 2, my_req->size, &actual_size,
			     BMI_PRE_ALLOC, 0, NULL, context, NULL);
    if (ret < 0)
    {
	fprintf(stderr, "BMI_post_recv_failure.\n");
	return (-1);
    }
    if (ret == 0)
    {
	/* turning this into a blocking call for testing :) */
	/* check for completion of data payload recv */
	do
	{
	    ret = BMI_test(server_ops[0], &outcount, &error_code,
			   &actual_size, NULL, 10, context);
	} while (ret == 0 && outcount == 0);

	if (ret < 0 || error_code != 0)
	{
	    fprintf(stderr, "data recv failed.\n");
	    return (-1);
	}
    }

    if (actual_size != my_req->size)
    {
	printf("Short recv.\n");
	printf("diff: %d\n", (int) (my_req->size - actual_size));
    }

    /* check validity of received message */
    for (i = 0; i < ((my_req->size / 2) / sizeof(int)); i++)
    {
	if (((int *) recv_buffer1)[i] != i)
	{
	    fprintf(stderr, "Validation failure, offset %d.\n", i);
	}
    }
    last = i;
    for (i = last;
	 i < (last + ((my_req->size - (my_req->size / 2)) / sizeof(int))); i++)
    {
	if (((int *) recv_buffer2)[i - last] != i)
	{
	    fprintf(stderr, "Validation failure, offset %d.\n", i);
	}
    }

    /* free up the message buffers */
    BMI_memfree(client_addr, recv_buffer1, (my_req->size / 2), BMI_RECV);
    BMI_memfree(client_addr, recv_buffer2, (my_req->size -
					    (my_req->size / 2)), BMI_RECV);
    BMI_memfree(client_addr, my_ack, sizeof(struct server_ack), BMI_SEND);
    BMI_unexpected_free(client_addr, my_req);

    /* shutdown the local interface */
    BMI_close_context(context);
    ret = BMI_finalize();
    if (ret < 0)
    {
	errno = -ret;
	perror("BMI_finalize");
	return (-1);
    }

    /* turn off debugging stuff */
    gossip_disable();

    free(user_opts->hostid);
    free(user_opts);

    return (0);
}


static struct options *parse_args(
    int argc,
    char *argv[])
{

    /* getopt stuff */
    char flags[] = "h:";
    int one_opt = 0;

    struct options *tmp_opts = NULL;
    int len = -1;

    /* create storage for the command line options */
    tmp_opts = (struct options *) malloc(sizeof(struct options));
    if (!tmp_opts)
    {
	goto parse_args_error;
    }
    tmp_opts->hostid = NULL;

    /* look at command line arguments */
    while ((one_opt = getopt(argc, argv, flags)) != EOF)
    {
	switch (one_opt)
	{
	case ('h'):
	    len = (strlen(optarg)) + 1;
	    if ((tmp_opts->hostid = (char *) malloc(len)) == NULL)
	    {
		goto parse_args_error;
	    }
	    memcpy(tmp_opts->hostid, optarg, len);
	    break;
	default:
	    break;
	}
    }

    /* if we didn't get a host argument, fill in a default: */
    if (tmp_opts->hostid == NULL) {
        len = (strlen(DEFAULT_SERVERID)) + 1;
        if ((tmp_opts->hostid = (char *) malloc(len)) == NULL)
        {
            goto parse_args_error;
        }
        memcpy(tmp_opts->hostid, DEFAULT_SERVERID, len);
    }

    return (tmp_opts);

  parse_args_error:

    /* if an error occurs, just free everything and return NULL */
    if (tmp_opts)
    {
	if (tmp_opts->hostid)
	{
	    free(tmp_opts->hostid);
	}
	free(tmp_opts);
    }
    return (NULL);
}

/*
 * Local variables:
 *  c-indent-level: 4
 *  c-basic-offset: 4
 * End:
 *
 * vim: ts=8 sts=4 sw=4 expandtab
 */
