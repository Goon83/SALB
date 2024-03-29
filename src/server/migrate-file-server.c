/* WARNING: THIS FILE IS AUTOMATICALLY GENERATED FROM A .SM FILE.
 * Changes made here will certainly be overwritten.
 */

/* 
 * (C) 2001 Clemson University and The University of Chicago 
 *
 * See COPYING in top-level directory.
 */

/*
 *  PVFS2 server state machine for driving I/O operations (read and write).
 */

#include <string.h>
#include <assert.h>

#include "server-config.h"
#include "pvfs2-server.h"
#include "pvfs2-attr.h"
#include "pvfs2-request.h"
#include "pint-distribution.h"
#include "pint-request.h"
#include "pint-util.h"
#include "pvfs2-internal.h"
#include "pint-cached-config.h"

#define IO_SM_PHASE_FINAL_ACK                                1

/* Indicatting, i was selected as migrationg target I/O server */
extern int    mig_state; 
/* The migrating I/O server  */
extern PVFS_handle  mig_datafile_handle;

PVFS_ds_attributes set_df_attr;

/* Global handle, passed between create_new_datafile and send positive ack */
PVFS_handle new_handle;

static struct PINT_state_s ST_prelude;
static struct PINT_pjmp_tbl_s ST_prelude_pjtbl[];
static struct PINT_tran_tbl_s ST_prelude_trtbl[];

static PINT_sm_action create_new_datafile(
	struct PINT_smcb *smcb, job_status_s *js_p);

static struct PINT_state_s ST_create_new_datafile;
static struct PINT_pjmp_tbl_s ST_create_new_datafile_pjtbl[];
static struct PINT_tran_tbl_s ST_create_new_datafile_trtbl[];

static PINT_sm_action get_datafile_attribs_op(
	struct PINT_smcb *smcb, job_status_s *js_p);

static struct PINT_state_s ST_get_datafile_attribs_op;
static struct PINT_pjmp_tbl_s ST_get_datafile_attribs_op_pjtbl[];
static struct PINT_tran_tbl_s ST_get_datafile_attribs_op_trtbl[];

static PINT_sm_action set_new_datafile_attribs_op(
	struct PINT_smcb *smcb, job_status_s *js_p);

static struct PINT_state_s ST_set_new_datafile_attribs_op;
static struct PINT_pjmp_tbl_s ST_set_new_datafile_attribs_op_pjtbl[];
static struct PINT_tran_tbl_s ST_set_new_datafile_attribs_op_trtbl[];

static PINT_sm_action io_send_ack(
	struct PINT_smcb *smcb, job_status_s *js_p);

static struct PINT_state_s ST_send_positive_ack;
static struct PINT_pjmp_tbl_s ST_send_positive_ack_pjtbl[];
static struct PINT_tran_tbl_s ST_send_positive_ack_trtbl[];
static struct PINT_state_s ST_send_negative_ack;
static struct PINT_pjmp_tbl_s ST_send_negative_ack_pjtbl[];
static struct PINT_tran_tbl_s ST_send_negative_ack_trtbl[];

static PINT_sm_action io_start_flow(
	struct PINT_smcb *smcb, job_status_s *js_p);

static struct PINT_state_s ST_start_flow;
static struct PINT_pjmp_tbl_s ST_start_flow_pjtbl[];
static struct PINT_tran_tbl_s ST_start_flow_trtbl[];

static PINT_sm_action io_send_completion_ack(
	struct PINT_smcb *smcb, job_status_s *js_p);

static struct PINT_state_s ST_send_completion_ack;
static struct PINT_pjmp_tbl_s ST_send_completion_ack_pjtbl[];
static struct PINT_tran_tbl_s ST_send_completion_ack_trtbl[];

static PINT_sm_action test_recv_one_msg_f(
	struct PINT_smcb *smcb, job_status_s *js_p);

static struct PINT_state_s ST_test_recv_one_msg;
static struct PINT_pjmp_tbl_s ST_test_recv_one_msg_pjtbl[];
static struct PINT_tran_tbl_s ST_test_recv_one_msg_trtbl[];

static PINT_sm_action decode_recv_msg_f(
	struct PINT_smcb *smcb, job_status_s *js_p);

static struct PINT_state_s ST_decode_recv_msg;
static struct PINT_pjmp_tbl_s ST_decode_recv_msg_pjtbl[];
static struct PINT_tran_tbl_s ST_decode_recv_msg_trtbl[];

static PINT_sm_action io_release(
	struct PINT_smcb *smcb, job_status_s *js_p);

static struct PINT_state_s ST_release;
static struct PINT_pjmp_tbl_s ST_release_pjtbl[];
static struct PINT_tran_tbl_s ST_release_trtbl[];

static PINT_sm_action io_cleanup(
	struct PINT_smcb *smcb, job_status_s *js_p);

static struct PINT_state_s ST_cleanup;
static struct PINT_pjmp_tbl_s ST_cleanup_pjtbl[];
static struct PINT_tran_tbl_s ST_cleanup_trtbl[];

struct PINT_state_machine_s pvfs2_migrate_file_server_sm = {
	.name = "pvfs2_migrate_file_server_sm",
	.first_state = &ST_prelude
};

static struct PINT_state_s ST_prelude = {
	 .state_name = "prelude" ,
	 .parent_machine = &pvfs2_migrate_file_server_sm ,
	 .flag = SM_JUMP ,
	 .action.nested = &pvfs2_prelude_sm ,
	 .pjtbl = NULL ,
	 .trtbl = ST_prelude_trtbl 
};

static struct PINT_tran_tbl_s ST_prelude_trtbl[] = {
	{ .return_value = 0 ,
	 .next_state = &ST_create_new_datafile },
	{ .return_value = -1 ,
	 .next_state = &ST_send_negative_ack }
};

static struct PINT_state_s ST_create_new_datafile = {
	 .state_name = "create_new_datafile" ,
	 .parent_machine = &pvfs2_migrate_file_server_sm ,
	 .flag = SM_RUN ,
	 .action.func = create_new_datafile ,
	 .pjtbl = NULL ,
	 .trtbl = ST_create_new_datafile_trtbl 
};

static struct PINT_tran_tbl_s ST_create_new_datafile_trtbl[] = {
	{ .return_value = 0 ,
	 .next_state = &ST_get_datafile_attribs_op },
	{ .return_value = -1 ,
	 .next_state = &ST_send_negative_ack }
};

static struct PINT_state_s ST_get_datafile_attribs_op = {
	 .state_name = "get_datafile_attribs_op" ,
	 .parent_machine = &pvfs2_migrate_file_server_sm ,
	 .flag = SM_RUN ,
	 .action.func = get_datafile_attribs_op ,
	 .pjtbl = NULL ,
	 .trtbl = ST_get_datafile_attribs_op_trtbl 
};

static struct PINT_tran_tbl_s ST_get_datafile_attribs_op_trtbl[] = {
	{ .return_value = 0 ,
	 .next_state = &ST_set_new_datafile_attribs_op },
	{ .return_value = -1 ,
	 .next_state = &ST_send_negative_ack }
};

static struct PINT_state_s ST_set_new_datafile_attribs_op = {
	 .state_name = "set_new_datafile_attribs_op" ,
	 .parent_machine = &pvfs2_migrate_file_server_sm ,
	 .flag = SM_RUN ,
	 .action.func = set_new_datafile_attribs_op ,
	 .pjtbl = NULL ,
	 .trtbl = ST_set_new_datafile_attribs_op_trtbl 
};

static struct PINT_tran_tbl_s ST_set_new_datafile_attribs_op_trtbl[] = {
	{ .return_value = 0 ,
	 .next_state = &ST_send_positive_ack },
	{ .return_value = -1 ,
	 .next_state = &ST_send_negative_ack }
};

static struct PINT_state_s ST_send_positive_ack = {
	 .state_name = "send_positive_ack" ,
	 .parent_machine = &pvfs2_migrate_file_server_sm ,
	 .flag = SM_RUN ,
	 .action.func = io_send_ack ,
	 .pjtbl = NULL ,
	 .trtbl = ST_send_positive_ack_trtbl 
};

static struct PINT_tran_tbl_s ST_send_positive_ack_trtbl[] = {
	{ .return_value = 0 ,
	 .next_state = &ST_start_flow },
	{ .return_value = -1 ,
	 .next_state = &ST_release }
};

static struct PINT_state_s ST_send_negative_ack = {
	 .state_name = "send_negative_ack" ,
	 .parent_machine = &pvfs2_migrate_file_server_sm ,
	 .flag = SM_RUN ,
	 .action.func = io_send_ack ,
	 .pjtbl = NULL ,
	 .trtbl = ST_send_negative_ack_trtbl 
};

static struct PINT_tran_tbl_s ST_send_negative_ack_trtbl[] = {
	{ .return_value = -1 ,
	 .next_state = &ST_release }
};

static struct PINT_state_s ST_start_flow = {
	 .state_name = "start_flow" ,
	 .parent_machine = &pvfs2_migrate_file_server_sm ,
	 .flag = SM_RUN ,
	 .action.func = io_start_flow ,
	 .pjtbl = NULL ,
	 .trtbl = ST_start_flow_trtbl 
};

static struct PINT_tran_tbl_s ST_start_flow_trtbl[] = {
	{ .return_value = 0 ,
	 .next_state = &ST_send_completion_ack },
	{ .return_value = -1 ,
	 .next_state = &ST_release }
};

static struct PINT_state_s ST_send_completion_ack = {
	 .state_name = "send_completion_ack" ,
	 .parent_machine = &pvfs2_migrate_file_server_sm ,
	 .flag = SM_RUN ,
	 .action.func = io_send_completion_ack ,
	 .pjtbl = NULL ,
	 .trtbl = ST_send_completion_ack_trtbl 
};

static struct PINT_tran_tbl_s ST_send_completion_ack_trtbl[] = {
	{ .return_value = -1 ,
	 .next_state = &ST_test_recv_one_msg }
};

static struct PINT_state_s ST_test_recv_one_msg = {
	 .state_name = "test_recv_one_msg" ,
	 .parent_machine = &pvfs2_migrate_file_server_sm ,
	 .flag = SM_RUN ,
	 .action.func = test_recv_one_msg_f ,
	 .pjtbl = NULL ,
	 .trtbl = ST_test_recv_one_msg_trtbl 
};

static struct PINT_tran_tbl_s ST_test_recv_one_msg_trtbl[] = {
	{ .return_value = -1 ,
	 .next_state = &ST_decode_recv_msg }
};

static struct PINT_state_s ST_decode_recv_msg = {
	 .state_name = "decode_recv_msg" ,
	 .parent_machine = &pvfs2_migrate_file_server_sm ,
	 .flag = SM_RUN ,
	 .action.func = decode_recv_msg_f ,
	 .pjtbl = NULL ,
	 .trtbl = ST_decode_recv_msg_trtbl 
};

static struct PINT_tran_tbl_s ST_decode_recv_msg_trtbl[] = {
	{ .return_value = -1 ,
	 .next_state = &ST_release }
};

static struct PINT_state_s ST_release = {
	 .state_name = "release" ,
	 .parent_machine = &pvfs2_migrate_file_server_sm ,
	 .flag = SM_RUN ,
	 .action.func = io_release ,
	 .pjtbl = NULL ,
	 .trtbl = ST_release_trtbl 
};

static struct PINT_tran_tbl_s ST_release_trtbl[] = {
	{ .return_value = -1 ,
	 .next_state = &ST_cleanup }
};

static struct PINT_state_s ST_cleanup = {
	 .state_name = "cleanup" ,
	 .parent_machine = &pvfs2_migrate_file_server_sm ,
	 .flag = SM_RUN ,
	 .action.func = io_cleanup ,
	 .pjtbl = NULL ,
	 .trtbl = ST_cleanup_trtbl 
};

static struct PINT_tran_tbl_s ST_cleanup_trtbl[] = {
	{ .return_value = -1 ,

	 .flag = SM_TERM }
};

# 115 "src/server/migrate-file-server.sm"



/* Add metafile to datafile's attributes */
static PINT_sm_action set_new_datafile_attribs_op(struct PINT_smcb *smcb, job_status_s *js_p)
{

  int ret;
  struct PINT_server_op *s_op = PINT_sm_frame(smcb, PINT_FRAME_CURRENT);
  job_id_t j_id;

 
  set_df_attr.metafile = s_op->req->u.migposter.metafile;

  ret = job_trove_dspace_setattr(
				 s_op->req->u.migposter.fs_id,
				 new_handle,
				 &set_df_attr, 
				 TROVE_SYNC,
				 smcb, 
				 0, 
				 js_p, 
				 &j_id, 
				 server_job_context, 
				 NULL);
  
  return ret;  
  
}


/* Get the origianl datafile's attributes */
static PINT_sm_action get_datafile_attribs_op(struct PINT_smcb *smcb, job_status_s *js_p)
{

  
  job_id_t j_id;
  struct PINT_server_op *s_op = PINT_sm_frame(smcb, PINT_FRAME_CURRENT);
  
  //  gossip_debug(GOSSIP_LB_DEBUG, "\n\n Get the request \n\n");
  return (job_trove_dspace_getattr(
				   s_op->req->u.migposter.fs_id,
				   new_handle,
				   smcb,
				   &set_df_attr,
				   0,
				   js_p,
				   &j_id,
				   server_job_context,
				   NULL)
	  );
}


/*
 * Synopsis: Create the new dataspace with the values provided in the response.
 *           
 */
static int create_new_datafile(
        struct PINT_smcb *smcb, job_status_s *js_p)
{
	gossip_debug(GOSSIP_SERVER_DEBUG,"enter create new datafile \n ");

	struct PINT_server_op *s_op = PINT_sm_frame(smcb, PINT_FRAME_CURRENT);
	int ret = -1;
	job_id_t tmp_id;

	PVFS_handle_extent_array data_handle_ext_array;
	server_configuration_s *config = get_server_config_struct();
	//int i;
	//int tmp_index = 0;
	PVFS_ds_keyval metafile;
	
	/*
	 *When server receive mig request
	 *Set mig_state to 1
	 */
	mig_state = 1;

	//gossip_debug(GOSSIP_LB_DEBUG, "\n Create handle status: metahandle = %llu\n", llu(s_op->req->u.migposter.metafile)); 
	
	
#define PINT_SERVER_TYPE_IO      PVFS_MGMT_IO_SERVER
	
	/* find local extent array */
	ret = PINT_cached_config_get_server(
		s_op->req->u.migposter.fs_id,
		config->host_id,
		PINT_SERVER_TYPE_IO,
		&data_handle_ext_array);
	if(ret < 0)
	{
		js_p->error_code = ret;
		return(SM_ACTION_COMPLETE);
	}

	
	
	metafile.buffer = &s_op->req->u.migposter.metafile;
	metafile.buffer_sz = 8;
	
	
    
	/* deliberately not setting SYNC flag, because both the attrs and
	 * keyvals will be synced in later states
	 */
	ret = job_trove_dspace_create_list(
		s_op->req->u.migposter.fs_id,
		&data_handle_ext_array,
		&new_handle,
		1,
		PVFS_TYPE_DATAFILE,
		(void*)&metafile,
		TROVE_SYNC,
		smcb,
		0,
		js_p,
		&tmp_id,
		server_job_context,
		NULL);

	
	return ret;
	
}



/*
 * Function: io_send_ack()
 *
 * Params:   server_op *s_op, 
 *           job_status_s* js_p
 *
 * Pre:      error code has been set in job status for us to
 *           report to client
 *
 * Post:     response has been sent to client
 *            
 * Returns:  int
 *
 * Synopsis: fills in a response to the I/O request, encodes it,
 *           and sends it to the client via BMI.  Note that it may
 *           send either positive or negative acknowledgements.
 *           
 */
static int io_send_ack(
        struct PINT_smcb *smcb, job_status_s *js_p)
{
	struct PINT_server_op *s_op = PINT_sm_frame(smcb, PINT_FRAME_CURRENT);
	int err = -PVFS_EIO;
	job_id_t tmp_id;
	struct server_configuration_s *user_opts = get_server_config_struct();
        
	gossip_debug(GOSSIP_LB_DEBUG, "new df handle %lld with md handle %lld", new_handle, set_df_attr.metafile);

	/* this is where we report the file size to the client before
	 * starting the I/O transfer, or else report an error if we
	 * failed to get the size, or failed for permission reasons
	 */
	s_op->resp.status = js_p->error_code;
	s_op->resp.u.migposter.handle = new_handle;
	/*
	 * record the handle for new data file
	 * to escape the load counter for this handle
	 */
	mig_datafile_handle = new_handle;

 	gossip_debug(GOSSIP_SERVER_DEBUG, 
 		     "nio_send_ack error_code = %d\n new handle = %llu\n", 
 		     js_p->error_code, 
 		     llu(new_handle)); 


 	err = PINT_encode(&s_op->resp,  
 			  PINT_ENCODE_RESP, 
 			  &(s_op->encoded), 
 			  s_op->addr, 
 			  s_op->decoded.enc_type); 

	//gossip_debug(GOSSIP_LB_DEBUG, "\n\nThe enc type is :%d \n\n",s_op->decoded.enc_type);	
 	if (err < 0) 
 	{ 
 		gossip_lerr("Server: migrate server SM: PINT_encode() failure.\n"); 
 		js_p->error_code = err; 
 		return SM_ACTION_COMPLETE; 
 	} 

 	err = job_bmi_send_list( 
 		s_op->addr,  
 		s_op->encoded.buffer_list,  
 		s_op->encoded.size_list, 
 		s_op->encoded.list_count,  
 		s_op->encoded.total_size, 
 		s_op->tag,  
 		s_op->encoded.buffer_type,  
 		0,  
 		smcb, 
 		0,  
 		js_p, 
 		&tmp_id,  
 		server_job_context,  
 		user_opts->server_job_bmi_timeout, 
 		NULL); 

/*  	gossip_debug(GOSSIP_LB_DEBUG,  */
/*  		     "send status:%d, %d, %llu\n",  */
/*  		     err, js_p->error_code,  */
/*  		     llu(s_op->resp.u.migposter.handle));  */

//	js_p->error_code = 0;

	return err;
}

/*
 * Function: io_start_flow()
 *
 * Params:   server_op *s_op, 
 *           job_status_s* js_p
 *
 * Pre:      all of the previous steps have succeeded, so that we
 *           are ready to actually perform the I/O
 *
 * Post:     I/O has been carried out
 *            
 * Returns:  int
 *
 * Synopsis: this is the most important part of the state machine.
 *           we setup the flow descriptor and post it in order to 
 *           carry out the data transfer
 *           
 */
static PINT_sm_action io_start_flow(
        struct PINT_smcb *smcb, job_status_s *js_p)
{
	struct PINT_server_op *s_op = PINT_sm_frame(smcb, PINT_FRAME_CURRENT);
	int err = -PVFS_EIO;
	job_id_t tmp_id;
	struct server_configuration_s *user_opts = get_server_config_struct();
	struct filesystem_configuration_s *fs_conf;
	

	//	gossip_debug(GOSSIP_LB_DEBUG, "\n\n Receive mig request: set mig_state = %d\n\n", mig_state);
	
	gossip_debug(GOSSIP_SERVER_DEBUG, "IO start flow\n");    

	s_op->u.migposter.flow_desc = PINT_flow_alloc();
	if (!s_op->u.migposter.flow_desc)
	{
		js_p->error_code = -PVFS_ENOMEM;
		return SM_ACTION_COMPLETE;
	}

	/* we still have the file size stored in the response structure 
	 * that we sent in the previous state, other details come from
	 * request
	 */
	s_op->u.migposter.flow_desc->file_data.fsize = 0;
	s_op->u.migposter.flow_desc->file_data.dist = PINT_dist_create("simple_stripe");
	s_op->u.migposter.flow_desc->file_data.server_nr = 0;
	s_op->u.migposter.flow_desc->file_data.server_ct = 1;

	/* on writes, we allow the bstream to be extended at EOF */
	gossip_debug(GOSSIP_SERVER_DEBUG, "io_start_flow() issuing flow to "
		     "write data.\n");
	s_op->u.migposter.flow_desc->file_data.extend_flag = 1;

	s_op->u.migposter.flow_desc->file_req = PVFS_BYTE;
	s_op->u.migposter.flow_desc->file_req_offset = 0;
	s_op->u.migposter.flow_desc->mem_req = NULL;
	s_op->u.migposter.flow_desc->aggregate_size = s_op->req->u.migposter.dfsize;
	s_op->u.migposter.flow_desc->tag = 0;
	s_op->u.migposter.flow_desc->user_ptr = NULL;
	s_op->u.migposter.flow_desc->type = FLOWPROTO_MULTIQUEUE;

	fs_conf = PINT_config_find_fs_id(user_opts, 
					 s_op->req->u.io.fs_id);
	if(fs_conf)
	{
		/* pick up any buffer settings overrides from fs conf */
		s_op->u.migposter.flow_desc->buffer_size = fs_conf->fp_buffer_size;
		s_op->u.migposter.flow_desc->buffers_per_flow = fs_conf->fp_buffers_per_flow;
	}

	gossip_debug(GOSSIP_SERVER_DEBUG, "flow: fsize: %lld, " 
		     "server_nr: %d, server_ct: %d\n",
		     lld(s_op->u.migposter.flow_desc->file_data.fsize),
		     (int)s_op->u.migposter.flow_desc->file_data.server_nr,
		     (int)s_op->u.migposter.flow_desc->file_data.server_ct);

	gossip_debug(GOSSIP_SERVER_DEBUG, "file_req_offset: %lld,"
		     "aggregate_size: %lld, handle: %llu\n", 
		     lld(s_op->u.migposter.flow_desc->file_req_offset),
		     lld(s_op->u.migposter.flow_desc->aggregate_size),
		     llu(s_op->resp.u.migposter.handle));

	/* set endpoints depending on type of io requested */
	s_op->u.migposter.flow_desc->src.endpoint_id = BMI_ENDPOINT;
	s_op->u.migposter.flow_desc->src.u.bmi.address = s_op->addr;
	s_op->u.migposter.flow_desc->dest.endpoint_id = TROVE_ENDPOINT;
	s_op->u.migposter.flow_desc->dest.u.trove.handle = s_op->resp.u.migposter.handle;
	s_op->u.migposter.flow_desc->dest.u.trove.coll_id = s_op->req->u.migposter.fs_id;

	err = job_flow(s_op->u.migposter.flow_desc, smcb, 0, js_p, &tmp_id,
		       server_job_context, user_opts->server_job_flow_timeout, NULL);

	return err;
}

/*
 * Function: io_release()
 *
 * Params:   server_op *b, 
 *           job_status_s* js_p
 *
 * Pre:      we are done with all steps necessary to service
 *           request
 *
 * Post:     operation has been released from the scheduler
 *
 * Returns:  int
 *
 * Synopsis: releases the operation from the scheduler
 */
static PINT_sm_action io_release(
        struct PINT_smcb *smcb, job_status_s *js_p)
{
	struct PINT_server_op *s_op = PINT_sm_frame(smcb, PINT_FRAME_CURRENT);
	gossip_debug(GOSSIP_SERVER_DEBUG, "\n\nMigration over: io release\n\n"); 
	int ret = 0;
	job_id_t i;

	/*
	 *When  mig finish
	 *clean mig_state to 0
	 */
	mig_state = 0;
	//	gossip_debug(GOSSIP_LB_DEBUG, "\n\n Finish mig: set mig_state = %d\n\n", mig_state);

	/*
	  tell the scheduler that we are done with this operation (if it
	  was scheduled in the first place)
	*/
	ret = job_req_sched_release(
		s_op->scheduled_id, smcb, 0, js_p, &i, server_job_context);
	return ret;

}

/*
 * Function: io_cleanup()
 *
 * Params:   server_op *b, 
 *           job_status_s* js_p
 *
 * Pre:      all jobs done, simply need to clean up
 *
 * Post:     everything is free
 *
 * Returns:  int
 *
 * Synopsis: free up any buffers associated with the operation,
 *           including any encoded or decoded protocol structures
 */
static PINT_sm_action io_cleanup(
        struct PINT_smcb *smcb, job_status_s *js_p)
{
	struct PINT_server_op *s_op = PINT_sm_frame(smcb, PINT_FRAME_CURRENT);
	char status_string[64] = {0};

	PVFS_strerror_r(-1073741973, status_string,64);
	PINT_ACCESS_DEBUG(s_op, GOSSIP_ACCESS_DEBUG, "finish (%s)\n", status_string);

	if (s_op->u.migposter.flow_desc)
	{
		PINT_flow_free(s_op->u.migposter.flow_desc);
	}

	/* let go of our encoded response buffer, if we appear to have
	 * made one
	 */
	if (s_op->encoded.total_size)
	{
		PINT_encode_release(&s_op->encoded, PINT_ENCODE_RESP);
	}

    	
	return(server_state_machine_complete(smcb));
}

/*
 * Function: io_send_completion_ack()
 *
 * Params:   server_op *s_op, 
 *           job_status_s* js_p
 *
 * Pre:      flow is completed so that we can report its status
 *
 * Post:     if this is a write, response has been sent to client
 *           if this is a read, do nothing
 *            
 * Returns:  int
 *
 * Synopsis: fills in a response to the I/O request, encodes it,
 *           and sends it to the client via BMI.  Note that it may
 *           send either positive or negative acknowledgements.
 *           
 */
static PINT_sm_action io_send_completion_ack(
        struct PINT_smcb *smcb, job_status_s *js_p)
{
	struct PINT_server_op *s_op = PINT_sm_frame(smcb, PINT_FRAME_CURRENT);
	int err = -PVFS_EIO;
	job_id_t tmp_id;
	struct server_configuration_s *user_opts = get_server_config_struct();
    
	gossip_debug(GOSSIP_SERVER_DEBUG,
		     "send completion ack 1 :%lld\n",
		     lld(s_op->u.migposter.flow_desc->total_transferred));
	/* release encoding of the first ack that we sent */
	PINT_encode_release(&s_op->encoded, PINT_ENCODE_RESP);


	/* zero size for safety */
	s_op->encoded.total_size = 0;

	/*
	  fill in response -- status field is the only generic one we
	  should have to set
	*/
	s_op->resp.op = PVFS_SERV_WRITE_COMPLETION;  /* not IO */
	s_op->resp.status = js_p->error_code;
	s_op->resp.u.write_completion.total_completed = 1111;
	//		s_op->u.migposter.flow_desc->total_transferred;

	gossip_debug(GOSSIP_LB_DEBUG,
		     "Server->send flow completion ack  :%lld\n",
		     s_op->resp.u.write_completion.total_completed);

 	err = PINT_encode( 
 		&s_op->resp, 
 		PINT_ENCODE_RESP, 
 		&(s_op->encoded), 
 		s_op->addr, 
 		s_op->decoded.enc_type); 

 	if (err < 0) 
 	{ 
 		gossip_lerr("Server: IO SM: PINT_encode() failure.\n"); 
 		js_p->error_code = err; 
 		return SM_ACTION_COMPLETE; 
 	} 

	gossip_debug(GOSSIP_SERVER_DEBUG,
		     "send completion ack 3 :%lld\n",
		     lld(s_op->u.migposter.flow_desc->total_transferred));

 	err = job_bmi_send_list( 
 		s_op->addr, s_op->encoded.buffer_list, s_op->encoded.size_list, 
 		s_op->encoded.list_count, s_op->encoded.total_size, 5, 
 		s_op->encoded.buffer_type, 0, smcb, 0, js_p, &tmp_id, 
 		server_job_context, user_opts->client_job_bmi_timeout,NULL); 
        

	gossip_debug(GOSSIP_SERVER_DEBUG,
		     "job_bmi_send_list: err=%d\n",
		     err);
	return err;
}

static PINT_sm_action  test_recv_one_msg_f(
        struct PINT_smcb *smcb, job_status_s *js_p)
{
        struct PINT_server_op *s_op = PINT_sm_frame(smcb, PINT_FRAME_CURRENT);
       	
	int ret;

	gossip_debug(GOSSIP_SERVER_DEBUG, "\n\n%s: entry\n\n", __func__);    
	

	s_op->msgarray_op.msgpair.max_resp_sz = PINT_encode_calc_max_size(
		PINT_ENCODE_RESP, 
		PVFS_SERV_WRITE_COMPLETION,
		ENCODING_LE_BFIELD);
	s_op->msgarray_op.msgpair.encoded_resp_p = BMI_memalloc(
		s_op->addr, //s_op->msgarray_op.msgpair.svr_addr,
		s_op->msgarray_op.msgpair.max_resp_sz,
		BMI_RECV);

	if (!s_op->msgarray_op.msgpair.encoded_resp_p)
	{
		gossip_err("BMI_memalloc (for write ack) failed\n");
		return -PVFS_ENOMEM;
	}
	//gossip_debug(GOSSIP_LB_DEBUG, "bmi memalloc success\n");
	
	/*
	  pre-post this recv with an infinite timeout and adjust it
	  after the flow completes since we don't know how long a flow
	  can take at this point
	*/ 
	ret = job_bmi_recv(
		s_op->addr, //s_op->msgarray_op.msgpair.svr_addr,
		s_op->msgarray_op.msgpair.encoded_resp_p,
		s_op->msgarray_op.msgpair.max_resp_sz,
		5,
		BMI_PRE_ALLOC,
		smcb, 
		IO_SM_PHASE_FINAL_ACK,
		js_p,
		&s_op->msgarray_op.msgpair.recv_id,
		server_job_context,
		JOB_TIMEOUT_INF, NULL);
	if (ret < 0)
	{
		gossip_err("job_bmi_recv (write ack) failed\n");
	}else{
		gossip_debug(GOSSIP_LB_DEBUG, "job_bmi_recv correct:ret = %d\n", ret);	
	} 

	return ret;
//	assert(ret == 0);
    
}

static PINT_sm_action  decode_recv_msg_f(
        struct PINT_smcb *smcb, job_status_s *js_p)
{
    	/* Do some decode test */
	struct PINT_decoded_msg decoded_resp;
	int ret;
	struct PINT_server_op *s_op = PINT_sm_frame(smcb, PINT_FRAME_CURRENT);

	ret = PINT_decode(s_op->msgarray_op.msgpair.encoded_resp_p, 
			  PINT_DECODE_RESP,
			  &decoded_resp, /* holds data on decoded resp */
			  s_op->addr,       //s_op->msgarray_op.msgpair.svr_addr, 
			  js_p->actual_size);

	gossip_debug(GOSSIP_LB_DEBUG, " \nServer->receiv again : actual size = %d, totle size: %d \n", js_p->actual_size, decoded_resp.stub_dec.resp.u.write_completion.total_completed);
	PINT_decode_release(&decoded_resp, PINT_ENCODE_RESP);

	return ret;
}

/* static enum PINT_server_req_access_type PINT_server_req_access_mig( */
/* 	struct PVFS_server_req *req) */
/* { */
/* 	return PINT_SERVER_REQ_MODIFY; */
/* } */

//PINT_GET_OBJECT_REF_DEFINE(migposter);

static inline int PINT_get_object_ref_migposter(
    struct PVFS_server_req *req, PVFS_fs_id *fs_id, PVFS_handle *handle)
{
    *fs_id = req->u.migposter.fs_id;
    *handle = PVFS_HANDLE_NULL;
    return 0;
};



static enum PINT_server_req_access_type PINT_server_req_migration(
    struct PVFS_server_req *req)
{
    return PINT_SERVER_REQ_MODIFY;
}


struct PINT_server_req_params pvfs2_migrate_server_params =
{
	.string_name = "migposter",
	.perm = PINT_SERVER_CHECK_NONE,
	.access_type = PINT_server_req_migration,
	.sched_policy = PINT_SERVER_REQ_SCHEDULE,
	.get_object_ref = PINT_get_object_ref_migposter,
	.state_machine = &pvfs2_migrate_file_server_sm
};


/*
 * Local variables:
 *  mode: c
 *  c-indent-level: 4
 *  c-basic-offset: 4
 * End:
 *
 * vim: ft=c ts=8 sts=4 sw=4 expandtab
 */

