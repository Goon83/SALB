/*
 * (C) 2001 Clemson University and The University of Chicago
 *
 * See COPYING in top-level directory.
 */

#ifndef __TROVE_INTERNAL_H
#define __TROVE_INTERNAL_H

#include "trove-types.h"

PVFS_error trove_errno_to_trove_error(int errno_value);

int trove_get_version (TROVE_coll_id coll_id, int* major, int* minor, int* incremental);
int trove_put_version (TROVE_coll_id coll_id, int major, int minor, int incremental);

/* These structures contains the function pointers that should be provided
 * by valid trove "method" implementations
 */

struct TROVE_bstream_ops
{
    int (*bstream_read_at)(
			   TROVE_coll_id coll_id,
			   TROVE_handle handle,
			   void *buffer,
			   TROVE_size *inout_size_p,
			   TROVE_offset offset,
			   TROVE_ds_flags flags,
			   TROVE_vtag_s *out_vtag, 
			   void *user_ptr,
			   TROVE_context_id context_id,
			   TROVE_op_id *out_op_id_p,
               PVFS_hint hints);
    
    int (*bstream_write_at)(
			    TROVE_coll_id coll_id,
			    TROVE_handle handle,
			    void *buffer,
			    TROVE_size *inout_size_p,
			    TROVE_offset offset,
			    TROVE_ds_flags flags,
			    TROVE_vtag_s *inout_vtag,
			    void *user_ptr,
			    TROVE_context_id context_id,
			    TROVE_op_id *out_op_id_p,
                PVFS_hint hints);
    
    int (*bstream_resize)(
			  TROVE_coll_id coll_id,
			  TROVE_handle handle,
			  TROVE_size *inout_size_p,
			  TROVE_ds_flags flags,
			  TROVE_vtag_s *vtag,
			  void *user_ptr,
			  TROVE_context_id context_id,
			  TROVE_op_id *out_op_id_p,
              PVFS_hint hints);
    
    int (*bstream_validate)(
			    TROVE_coll_id coll_id,
			    TROVE_handle handle,
			    TROVE_ds_flags flags,
			    TROVE_vtag_s *vtag,
			    void *user_ptr,
			    TROVE_context_id context_id,
			    TROVE_op_id *out_op_id_p,
                PVFS_hint hints);
    
    int (*bstream_read_list)(
			     TROVE_coll_id coll_id,
			     TROVE_handle handle,
			     char **mem_offset_array, 
			     TROVE_size *mem_size_array,
			     int mem_count,
			     TROVE_offset *stream_offset_array, 
			     TROVE_size *stream_size_array,
			     int stream_count,
			     TROVE_size *out_size_p, /* status indicates partial */
			     TROVE_ds_flags flags, 
			     TROVE_vtag_s *out_vtag,
			     void *user_ptr,
			     TROVE_context_id context_id,
			     TROVE_op_id *out_op_id_p,
                 PVFS_hint hints);
    
    int (*bstream_write_list)(
			      TROVE_coll_id coll_id,
			      TROVE_handle handle,
			      char **mem_offset_array, 
			      TROVE_size *mem_size_array,
			      int mem_count,
			      TROVE_offset *stream_offset_array, 
			      TROVE_size *stream_size_array,
			      int stream_count,
			      TROVE_size *out_size_p, /* status indicates partial */
			      TROVE_ds_flags flags, 
			      TROVE_vtag_s *inout_vtag,
			      void *user_ptr,
			      TROVE_context_id context_id,
			      TROVE_op_id *out_op_id_p,
                  PVFS_hint hints);

     int (*bstream_flush)(
			TROVE_coll_id coll_id,
			TROVE_handle handle,
			TROVE_ds_flags flags,
			void *user_ptr,
			TROVE_context_id context_id,
			TROVE_op_id *out_op_id_p,
            PVFS_hint hints);

     int (*bstream_cancel)(
         TROVE_coll_id coll_id,
         TROVE_op_id cancel_id,
         TROVE_context_id context_id);
};

struct TROVE_keyval_ops
{
    int (*keyval_read)(
		       TROVE_coll_id coll_id,
		       TROVE_handle handle,
		       TROVE_keyval_s *key_p,
		       TROVE_keyval_s *val_p,
		       TROVE_ds_flags flags,
		       TROVE_vtag_s *out_vtag, 
		       void *user_ptr,
		       TROVE_context_id context_id,
		       TROVE_op_id *out_op_id_p,
               PVFS_hint hints);
    
    int (*keyval_write)(
			TROVE_coll_id coll_id,
			TROVE_handle handle,
			TROVE_keyval_s *key_p,
			TROVE_keyval_s *val_p,
			TROVE_ds_flags flags,
			TROVE_vtag_s *inout_vtag,
			void *user_ptr,
			TROVE_context_id context_id,
			TROVE_op_id *out_op_id_p,
            PVFS_hint hints);
    
    int (*keyval_remove)(
			 TROVE_coll_id coll_id,
			 TROVE_handle handle,
			 TROVE_keyval_s *key_p,
                         TROVE_keyval_s *val_p,
			 TROVE_ds_flags flags,
			 TROVE_vtag_s *inout_vtag,
			 void *user_ptr,
			 TROVE_context_id context_id,
			 TROVE_op_id *out_op_id_p,
             PVFS_hint hints);
    
    int (*keyval_remove_list)(
			TROVE_coll_id coll_id,
			TROVE_handle handle,
			TROVE_keyval_s *key_array,
			TROVE_keyval_s *val_array,
                        int *error_array,
                        int count,
			TROVE_ds_flags flags,
			TROVE_vtag_s *inout_vtag,
			void *user_ptr,
			TROVE_context_id context_id,
			TROVE_op_id *out_op_id_p,
                        PVFS_hint hints);
    
    int (*keyval_validate)(
			   TROVE_coll_id coll_id,
			   TROVE_handle handle,
			   TROVE_ds_flags flags,
			   TROVE_vtag_s *inout_vtag,
			   void* user_ptr,
			   TROVE_context_id context_id,
			   TROVE_op_id *out_op_id_p,
               PVFS_hint hints);
    
    int (*keyval_iterate)(
			  TROVE_coll_id coll_id,
			  TROVE_handle handle,
			  TROVE_ds_position *inout_position_p,
			  TROVE_keyval_s *out_key_array,
			  TROVE_keyval_s *out_val_array,
			  int *inout_count_p,
			  TROVE_ds_flags flags,
			  TROVE_vtag_s *inout_vtag,
			  void *user_ptr,
			  TROVE_context_id context_id,
			  TROVE_op_id *out_op_id_p,
              PVFS_hint hints);
    
    int (*keyval_iterate_keys)(
			       TROVE_coll_id coll_id,
			       TROVE_handle handle,
			       TROVE_ds_position *inout_position_p,
			       TROVE_keyval_s *out_key_array,
			       int *inout_count_p,
			       TROVE_ds_flags flags,
			       TROVE_vtag_s *vtag,
			       void *user_ptr,
			       TROVE_context_id context_id,
			       TROVE_op_id *out_op_id_p,
                   PVFS_hint hints);
    
    int (*keyval_read_list)(
			    TROVE_coll_id coll_id,
			    TROVE_handle handle,
			    TROVE_keyval_s *key_array,
			    TROVE_keyval_s *val_array,
                            TROVE_ds_state *err_array,
			    int count,
			    TROVE_ds_flags flags,
			    TROVE_vtag_s *out_vtag,
			    void *user_ptr,
			    TROVE_context_id context_id,
			    TROVE_op_id *out_op_id_p,
                PVFS_hint hints);
    
    int (*keyval_write_list)(
			     TROVE_coll_id coll_id,
			     TROVE_handle handle,
			     TROVE_keyval_s *key_array,
			     TROVE_keyval_s *val_array,
			     int count,
			     TROVE_ds_flags flags,
			     TROVE_vtag_s *inout_vtag,
			     void *user_ptr,
			     TROVE_context_id context_id,
			     TROVE_op_id *out_op_id_p,
                 PVFS_hint hints);
    int (*keyval_flush)(
			TROVE_coll_id coll_id,
			TROVE_handle handle,
			TROVE_ds_flags flags,
			void *user_ptr,
			TROVE_context_id context_id,
			TROVE_op_id *out_op_id_p,
            PVFS_hint hints);
    int (*keyval_get_handle_info)(
        TROVE_coll_id coll_id,
        TROVE_handle handle,
        TROVE_ds_flags flags,
        TROVE_keyval_handle_info *info,
        void *user_ptr,
        TROVE_context_id context_id,
        TROVE_op_id *out_op_id_p,
        PVFS_hint hints);
};

struct TROVE_dspace_ops
{
    int (*dspace_create)(
			 TROVE_coll_id coll_id,
                         TROVE_handle_extent_array *extent_array,
			 TROVE_handle *handle,
			 TROVE_ds_type type,
			 TROVE_keyval_s *hint, /* TODO: figure out what this is! */
			 TROVE_ds_flags flags,
			 void *user_ptr,
			 TROVE_context_id context_id,
			 TROVE_op_id *out_op_id_p,
             PVFS_hint hints);

     int (*dspace_create_list)(
			 TROVE_coll_id coll_id,
                         TROVE_handle_extent_array *extent_array,
			 TROVE_handle *handle_array,
			 int count,
			 TROVE_ds_type type,
			 TROVE_keyval_s *hint, /* TODO: figure out what this is! */
			 TROVE_ds_flags flags,
			 void *user_ptr,
			 TROVE_context_id context_id,
			 TROVE_op_id *out_op_id_p,
                         PVFS_hint hints);
    
    int (*dspace_remove)(
			 TROVE_coll_id coll_id,
			 TROVE_handle handle,
			 TROVE_ds_flags flags,
			 void *user_ptr,
			 TROVE_context_id context_id,
			 TROVE_op_id *out_op_id_p,
             PVFS_hint hints);

    int (*dspace_remove_list)(
			 TROVE_coll_id coll_id,
			 TROVE_handle* handle_array,
                         TROVE_ds_state *error_array,
                         int count,
			 TROVE_ds_flags flags,
			 void *user_ptr,
			 TROVE_context_id context_id,
			 TROVE_op_id *out_op_id_p);


    int (*dspace_iterate_handles)(
			 	  TROVE_coll_id coll_id,
				  TROVE_ds_position *position_p,
			 	  TROVE_handle *handle_array,
				  int *inout_count_p,
		 		  TROVE_ds_flags flags,
				  TROVE_vtag_s *vtag,
				  void *user_ptr,
				  TROVE_context_id context_id,
				  TROVE_op_id *out_op_id_p);

    int (*dspace_verify)(
			 TROVE_coll_id coll_id,
			 TROVE_handle handle,
			 TROVE_ds_type *type, /* TODO: define types! */
			 TROVE_ds_flags flags,
			 void *user_ptr,
			 TROVE_context_id context_id,
			 TROVE_op_id *out_op_id_p,
             PVFS_hint hints);
    
    int (*dspace_getattr)(
			  TROVE_coll_id coll_id,
			  TROVE_handle handle,
			  TROVE_ds_attributes_s *ds_attr_p,
			  TROVE_ds_flags flags,
			  void *user_ptr,
			  TROVE_context_id context_id,
			  TROVE_op_id *out_op_id_p,
              PVFS_hint hints);

    int (*dspace_getattr_list)(
			  TROVE_coll_id coll_id,
                          int nhandles,
			  TROVE_handle *handle_array,
			  TROVE_ds_attributes_s *ds_attr_p,
                          TROVE_ds_state *error_array,
			  TROVE_ds_flags flags,
			  void *user_ptr,
			  TROVE_context_id context_id,
			  TROVE_op_id *out_op_id_p,
              PVFS_hint hints);
    
    int (*dspace_setattr)(
			  TROVE_coll_id coll_id,
			  TROVE_handle handle,
			  TROVE_ds_attributes_s *ds_attr_p, 
			  TROVE_ds_flags flags,
			  void *user_ptr,
			  TROVE_context_id context_id,
			  TROVE_op_id *out_op_id_p,
              PVFS_hint hints);
    
    int (*dspace_cancel)(
		       TROVE_coll_id coll_id,
		       TROVE_op_id ds_id,
		       TROVE_context_id context_id);

    int (*dspace_test)(
		       TROVE_coll_id coll_id,
		       TROVE_op_id ds_id,
		       TROVE_context_id context_id,
		       int *out_count_p,
		       TROVE_vtag_s *vtag,
		       void **returned_user_ptr_p,
		       TROVE_ds_state *out_state_p,
		       int max_idle_time_ms);
    
    int (*dspace_testsome)(
			   TROVE_coll_id coll_id,
			   TROVE_context_id context_id,
			   TROVE_op_id *ds_id_array,
			   int *inout_count_p,
			   int *out_index_array,
			   TROVE_vtag_s *vtag_array,
			   void **returned_user_ptr_array,
			   TROVE_ds_state *out_state_array,
			   int max_idle_time_ms);

    int (*dspace_testcontext)(
			   TROVE_coll_id coll_id,
                           TROVE_op_id *ds_id_array,
                           int *inout_count_p,
                           TROVE_ds_state *state_array,
                           void** user_ptr_array,
                           int max_idle_time_ms,
                           TROVE_context_id context_id);
};

struct TROVE_mgmt_ops
{
    int (*initialize)(
		      char *stoname,
		      TROVE_ds_flags flags);
    
    int (*finalize)(void);
    
    int (*storage_create)(
			  char *stoname,
			  void *user_ptr,
			  TROVE_op_id *out_op_id_p);
    
    int (*storage_remove)(
			  char *stoname,
			  void *user_ptr,
			  TROVE_op_id *out_op_id_p);
    
    int (*collection_create)(
			     /* char *stoname, */
			     char *collname,
			     TROVE_coll_id new_coll_id,
			     void *user_ptr,
			     TROVE_op_id *out_op_id_p);

    int (*collection_remove)(
			     /* char *stoname, */
			     char *collname,
			     void *user_ptr,
			     TROVE_op_id *out_op_id_p);
    
    int (*collection_lookup)(
			     /* char *stoname, */
			     char *collname,
			     TROVE_coll_id *coll_id_p,
			     void *user_ptr,
			     TROVE_op_id *out_op_id_p);

    int (*collection_clear)(TROVE_coll_id coll_id);

    int (*collection_iterate)(TROVE_ds_position *inout_position_p,
			      TROVE_keyval_s *name_array,
			      TROVE_coll_id *coll_id_array,
			      int *inout_count_p,
			      TROVE_ds_flags flags,
			      TROVE_vtag_s *vtag,
			      void *user_ptr,
			      TROVE_op_id *out_op_id_p);

    /* Note: setinfo and getinfo always return immediately */
    int (*collection_setinfo)(
                              TROVE_method_id method_id,
			      TROVE_coll_id coll_id,
			      TROVE_context_id context_id,
			      int option,
			      void *parameter);
    
    int (*collection_getinfo)(
			      TROVE_coll_id coll_id,
			      TROVE_context_id context_id,
			      TROVE_coll_getinfo_options opt,
			      void *parameter);
    
    int (*collection_seteattr)(
			       TROVE_coll_id coll_id,
			       TROVE_keyval_s *key_p,
			       TROVE_keyval_s *val_p,
			       TROVE_ds_flags flags,
			       void *user_ptr,
			       TROVE_context_id context_id,
			       TROVE_op_id *out_op_id_p);
    
    int (*collection_geteattr)(
			       TROVE_coll_id coll_id,
			       TROVE_keyval_s *key_p,
			       TROVE_keyval_s *val_p,
			       TROVE_ds_flags flags,
			       void *user_ptr,
			       TROVE_context_id context_id,
			       TROVE_op_id *out_op_id_p);
};

struct TROVE_context_ops
{
    int (*open_context)(
                        TROVE_coll_id coll_id,
                        TROVE_context_id *context_id);

    int (*close_context)(
                         TROVE_coll_id coll_id,
                         TROVE_context_id context_id);
};

/*
 * Local variables:
 *  c-indent-level: 4
 *  c-basic-offset: 4
 * End:
 *
 * vim: ts=8 sts=4 sw=4 expandtab
 */

#endif
