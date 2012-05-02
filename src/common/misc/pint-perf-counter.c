/*
 * Copyright © Acxiom Corporation, 2005
 *
 * See COPYING in top-level directory.
 *
 */

#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <assert.h>
#include <stdio.h>
#include <time.h>
#include <pthread.h>
#include <math.h>
#include <gsl/gsl_linalg.h>
#include <gsl/gsl_multifit.h>



#include "pvfs2-types.h"
#include "pvfs2-util.h"
#include "pvfs2-internal.h"
#include "pint-perf-counter.h"
#include "server-config-mgr.h"
#include "pvfs2-debug.h"
#include "gossip.h"
#include "str-utils.h"
#include "pvfs2-mgmt.h"
#include "pint-cached-config.h"

int server_forecast_enable = 0;
//extern  int     migration_happen;


#define HISTORY 1

#define PINT_PERF_REALLOC_ARRAY(__pc, __tmp_ptr, __src_ptr, __new_history, __type) \
    {									\
	__tmp_ptr = (__type*)malloc(__new_history*sizeof(__type));	\
	if(!__tmp_ptr)							\
	    return(-PVFS_ENOMEM);					\
	memset(__tmp_ptr, 0, (__new_history*sizeof(__type)));		\
	memcpy(__tmp_ptr, __src_ptr,					\
	       (__pc->history_size*sizeof(__type)));			\
	free(__src_ptr);						\
	__src_ptr = __tmp_ptr;						\
    }

/** 
 * creates a new perf counter instance
 * \note key_array must not be freed by caller until after
 * PINT_perf_finalize()
 * \returns pointer to perf counter on success, NULL on failure
 */
struct PINT_perf_counter* PINT_perf_initialize(
					       struct PINT_perf_key* key_array) /**< NULL terminated array of keys */
{
    struct PINT_perf_counter* pc = NULL;
    struct PINT_perf_key* key = NULL;
    int i;
    struct timeval tv;

    pc = (struct PINT_perf_counter*)malloc(sizeof(struct PINT_perf_counter));
    if(!pc)
	{
	    return(NULL);
	}
    memset(pc, 0, sizeof(struct PINT_perf_counter));
    gen_mutex_init(&pc->mutex);
    pc->key_array = key_array;

    key = &key_array[pc->key_count];
    while(key->key_name)
	{
	    /* keys must be in order from zero */
	    if(key->key != pc->key_count)
		{
		    gossip_err("Error: PINT_perf_initialize(): key out of order.\n");
		    gen_mutex_destroy(&pc->mutex);
		    free(pc);
		    return(NULL);
		}
        
	    pc->key_count++;
	    key = &key_array[pc->key_count];
	}
    if(pc->key_count < 1)
	{
	    gossip_err("Error: PINT_perf_initialize(): no keys specified.\n");
	    gen_mutex_destroy(&pc->mutex);
	    free(pc);
	    return(NULL);
	}


    pc->history_size = PERF_DEFAULT_HISTORY_SIZE;

    /* allocate time arrays */
    pc->start_time_array_ms =
        (uint64_t*)malloc(PERF_DEFAULT_HISTORY_SIZE*sizeof(uint64_t));
    if(!pc->start_time_array_ms)
	{
	    gen_mutex_destroy(&pc->mutex);
	    free(pc);
	    return(NULL);
	}
    pc->interval_array_ms = 
        (uint64_t*)malloc(PERF_DEFAULT_HISTORY_SIZE*sizeof(uint64_t));
    if(!pc->interval_array_ms)
	{
	    free(pc->start_time_array_ms);
	    gen_mutex_destroy(&pc->mutex);
	    free(pc);
	    return(NULL);
	}
    memset(pc->start_time_array_ms, 0,
	   PERF_DEFAULT_HISTORY_SIZE*sizeof(uint64_t));
    memset(pc->interval_array_ms, 0,
	   PERF_DEFAULT_HISTORY_SIZE*sizeof(uint64_t));
    
    /* allocate value matrix */
    pc->value_matrix = (int64_t**)malloc(pc->key_count*sizeof(int64_t*));
    if(!pc->value_matrix)
	{
	    free(pc->start_time_array_ms);
	    free(pc->interval_array_ms);
	    gen_mutex_destroy(&pc->mutex);
	    free(pc);
	    return(NULL);
	}

    for(i=0; i<pc->key_count; i++)
	{
	    pc->value_matrix[i] =
		(int64_t*)malloc(pc->history_size*sizeof(int64_t));
	    if(!pc->value_matrix[i])
		{
		    for(i=i-1; i>= 0; i--)
			{
			    free(pc->value_matrix[i]);
			}
		    free(pc->value_matrix);
		    free(pc->start_time_array_ms);
		    free(pc->interval_array_ms);
		    gen_mutex_destroy(&pc->mutex);
		    free(pc);
		    return(NULL);
		}
	    memset(pc->value_matrix[i], 0, pc->history_size*sizeof(int64_t));
	}

    /* set initial timestamp */
    gettimeofday(&tv, NULL);
    pc->start_time_array_ms[0] = ((uint64_t)tv.tv_sec)*1000 +
	tv.tv_usec/1000;

    return(pc);
}



/*
 *create a new perf counter detail instancce
 *
 *return pointer to perf detail conter on success, NULL on failure
 * 
 */
struct PINT_perf_counter_detail* PINT_perf_initialize_detail(void)
{
    
    struct PINT_perf_counter_detail* pc_detail=NULL;


    pc_detail = (struct PINT_perf_counter_detail *)malloc(sizeof(struct PINT_perf_counter_detail));
    if(!pc_detail)
	{
	    return(NULL);
	}
    memset(pc_detail, 0, sizeof(struct PINT_perf_counter_detail));
#if 0 
    /* count how many meta servers we have */
    ret = PINT_cached_config_count_servers(
					   get_server_config_struct(), coll_id, flags, &io_server_count);

    gossip_debug(GOSSIP_SERVER_DEBUG, "server count is %d\n",io_server_count);
    if(ret < 0)
	{
	    PVFS_perror("PVFS_mgmt_count_servers", ret);
	    return(ret);
	}
    /* allocate a 2 dimensional array for statistics */
    perf_load_matrix = (struct PVFS_mgmt_perf_stat**)malloc(
							    io_server_count*sizeof(struct PVFS_mgmt_perf_stat*));
    if(!perf_load_matrix)
	{
	    PVFS_perror("malloc", -1);
	    return(-1);
	}

    for(i=0; i<io_server_count; i++)
	{
	    perf_load_matrix[i] = (struct PVFS_mgmt_perf_stat *)
		malloc(HISTORY * sizeof(struct PVFS_mgmt_perf_stat));
	    if (perf_load_matrix[i] == NULL)
		{
		    PVFS_perror("malloc", -1);
		    return -1;
		}
	}

    /* build a list of servers to talk to */
    perf_load_addr_array = (PVFS_BMI_addr_t *)
	malloc(io_server_count * sizeof(PVFS_BMI_addr_t));
    if (perf_load_addr_array == NULL)
	{
	    PVFS_perror("malloc", -1);
	    return -1;
	}
    
    ret = PINT_cached_config_get_server_array(get_server_config_struct(),
					      coll_id,
					      flags,
					      perf_load_addr_array,
					      &io_server_count);
    if (ret < 0)
	{
	    PVFS_perror("PVFS_mgmt_get_server_array", ret);
	    return -1;
	}    
#endif
    /* initialize mutex*/
    gen_mutex_init(&pc_detail->mutex);
    /* initialize load value list to NULL */
    pc_detail->value_list = NULL;
    /* set hand_size to zero */
    pc_detail->handle_size = 0;
    
    return (pc_detail);
    //return (NULL);
}


/**
 * resets all counters within a perf counter instance, except for those that
 * have the PRESERVE bit set
 */
void PINT_perf_reset(
		     struct PINT_perf_counter* pc)
{
    int i;
    struct timeval tv;

    gen_mutex_lock(&pc->mutex);

    /* zero out all fields */
    memset(pc->start_time_array_ms, 0,
	   PERF_DEFAULT_HISTORY_SIZE*sizeof(uint64_t));
    memset(pc->interval_array_ms, 0,
	   PERF_DEFAULT_HISTORY_SIZE*sizeof(uint64_t));
    for(i=0; i<pc->key_count; i++)
	{
	    if(!(pc->key_array[i].flag & PINT_PERF_PRESERVE))
		{
		    memset(pc->value_matrix[i], 0, pc->history_size*sizeof(int64_t));
		}
	}

    /* set initial timestamp */
    gettimeofday(&tv, NULL);
    pc->start_time_array_ms[0] = ((uint64_t)tv.tv_sec)*1000 +
	tv.tv_usec/1000;

    gen_mutex_unlock(&pc->mutex);

    return;
}

/** 
 * destroys a perf counter instance
 */
void PINT_perf_finalize(
			struct PINT_perf_counter* pc)    /**< pointer to counter instance */
{
    int i;

    for(i=0; i<pc->key_count; i++)
	{
	    free(pc->value_matrix[i]);
	}
    free(pc->value_matrix);
    free(pc->start_time_array_ms);
    free(pc->interval_array_ms);
    gen_mutex_destroy(&pc->mutex);
    free(pc);
    pc = NULL;
    
    return;
}

/** 
 * destroys a perf counter detail instance
 */
void PINT_perf_finalize_detail(struct PINT_perf_counter_detail* pc_detail)    /**< pointer to counter instance */
{
    
    int i;
    struct PINT_perf_detail_item *itemp;
    struct PINT_perf_detail_item *nextp;
    
    itemp =pc_detail->value_list;
    for(i=0; i < (pc_detail->handle_size); i++)
	{
	    nextp = itemp->next;
	    free(itemp);
	    itemp = nextp;
	}
      
    
    gen_mutex_destroy(&pc_detail->mutex);
    free(pc_detail);
    pc_detail = NULL;
    
}


/**
 * performs an operation on the given key within a performance counter
 * \see PINT_perf_count macro
 */
void __PINT_perf_count(
		       struct PINT_perf_counter* pc,
		       int key, 
		       int64_t value,
		       enum PINT_perf_ops op)
{
    if(!pc)
	{
	    /* do nothing if perf counter is not initialized */
	    return;
	}

    gen_mutex_lock(&pc->mutex);
    
    if(key >= pc->key_count)
	{
	    gossip_err("Error: PINT_perf_count(): invalid key.\n");
	    return;
	}

    switch(op)
	{
        case PINT_PERF_ADD:
            pc->value_matrix[key][0] = pc->value_matrix[key][0] + value;
            break;
        case PINT_PERF_SUB:
            pc->value_matrix[key][0] = pc->value_matrix[key][0] - value;
            break;
        case PINT_PERF_SET:
            pc->value_matrix[key][0] = value;
            break;
	}

    gen_mutex_unlock(&pc->mutex);
    return;
}

/**
 *performa an operation on the given handle within a performance counter
 *
 */
void __PINT_perf_count_detail(
			      struct PINT_perf_counter_detail* pc_detail,
			      int key,
			      PVFS_fs_id   fs_id,
			      PVFS_handle  handle,
			      int64_t value)
{
    
    int index;
    struct PINT_perf_detail_item *itemp;


    gossip_debug(
		 GOSSIP_FLOW_PROTO_DEBUG,
		 "perf count detail start in detail start :  handle: %llu, value: %lld, handle size: %d \n",
		 handle, value, pc_detail->handle_size);


    if(!pc_detail)
	{
	    return;
	}


    gossip_debug(
		 GOSSIP_FLOW_PROTO_DEBUG,
		 "perf count detail start :  handle: %llu, value: %lld, handle size: %d \n",
		 handle, value, pc_detail->handle_size);

    gen_mutex_lock(&pc_detail->mutex);


    itemp = pc_detail->value_list;
  
    
    for(index = 0;  index < pc_detail->handle_size; index++)
	{
	    
	    if(itemp == NULL){
		break;
	    }
	    
	    if(itemp->id == handle){
		if(key == PINT_PERF_WRITE){
		    itemp->value_write = itemp->value_write + value;		
		}else if(key == PINT_PERF_READ){
		    itemp->value_read = itemp->value_read + value;
		}
		    
		goto pc_count_detail_finish;
	    }

	    itemp = itemp->next;
	}


    gossip_debug(
		 GOSSIP_FLOW_PROTO_DEBUG,
		 "pc count detail find no handle exist:  index: %d, handle_size: %d\n",
		 index, pc_detail->handle_size);

    
    struct PINT_perf_detail_item *temp;
    
    temp = (struct PINT_perf_detail_item *)malloc(sizeof(struct PINT_perf_detail_item));
    if(temp == NULL){
	gossip_err("Error: PINT_perf_count_detail(): alloc memory error.\n");
	goto pc_count_detail_finish;
    }
    memset(temp, 0, sizeof(struct PINT_perf_detail_item));
    
    
    temp->id = handle;
    temp->fs_id = fs_id;     

    if(key == PINT_PERF_WRITE){
	temp->value_write = value;
    }else if(key == PINT_PERF_READ){
	temp->value_read =  value;
    }else{
	free(temp);
	goto pc_count_detail_finish;
    }
	
    gossip_debug(
		 GOSSIP_FLOW_PROTO_DEBUG,
		 "perf count detai temp->id: %llu\n",
		 temp->id);
    
    
    temp->next = pc_detail->value_list;
    pc_detail->value_list  = temp;
    
    
    pc_detail->handle_size = pc_detail->handle_size + 1;
    
 pc_count_detail_finish: 
    
    gen_mutex_unlock(&pc_detail->mutex);
    
    return;
    
}


#ifdef __PVFS2_DISABLE_PERF_COUNTERS__
#define PINT_perf_count(w,x,y,z) do{}while(0)
#define PINT_perf_count_detail(u,v,w,x,y,z) do{}while(0)
#else
#define PINT_perf_count __PINT_perf_count
#define PINT_perf_count_detail __PINT_perf_count_detail
#endif

/** 
 * rolls over the current history window
 */
void PINT_perf_rollover(
			struct PINT_perf_counter* pc)
{
    int i;
    struct timeval tv;
    uint64_t int_time;

    if(!pc)
	{
	    /* do nothing if perf counter is not initialized */
	    return;
	}

    gettimeofday(&tv, NULL);
    int_time = ((uint64_t)tv.tv_sec)*1000 + tv.tv_usec/1000;

    gen_mutex_lock(&pc->mutex);
    
    
    /* rotate all values back one */
    if(pc->history_size > 1)
	{
	    for(i=0; i<pc->key_count; i++)
		{
		    memmove(&pc->value_matrix[i][1], &pc->value_matrix[i][0],
			    ((pc->history_size-1)*sizeof(int64_t)));
		}
	    memmove(&pc->interval_array_ms[1], &pc->interval_array_ms[0],
		    ((pc->history_size-1)*sizeof(uint64_t)));

	    memmove(&pc->start_time_array_ms[1], &pc->start_time_array_ms[0],
		    ((pc->history_size-1)*sizeof(uint64_t)));

	    if(int_time > pc->start_time_array_ms[1])
		{
		    pc->interval_array_ms[1] = int_time - pc->start_time_array_ms[1];
		}
	    
	    /*   /\* */
	    /* 	     *Transform to M/s  */
	    /* 	     *\/ */
	    /* 	    if(pc->interval_array_ms[1] != 0){ */
	    /* 		pc->value_matrix[PINT_PERF_READ][1] = ((double) (pc->value_matrix[PINT_PERF_READ][1])/ 1048576.0) / ((double)(pc->interval_array_ms[1]) / 1000.0); */
	    /* 		pc->value_matrix[PINT_PERF_WRITE][1] =((double) (pc->value_matrix[PINT_PERF_WRITE][1])/ 1048576.0) / ((double)(pc->interval_array_ms[1])/1000.0); */
	    /* 	    }else{ */
	    /* 		pc->value_matrix[PINT_PERF_READ][1] = 0; */
	    /* 		pc->value_matrix[PINT_PERF_WRITE][1] = 0; */
	    /* 	    } */

	}
    
    /* reset times for next interval */
    pc->start_time_array_ms[0] = int_time;
    pc->interval_array_ms[0] = 0;
    
    for(i=0; i<pc->key_count; i++)
	{
	    /* reset next interval's value, unless preserve flag set */
	    if(!(pc->key_array[i].flag & PINT_PERF_PRESERVE))
		{
		    pc->value_matrix[i][0] = 0;
		}
	}
    
    gen_mutex_unlock(&pc->mutex);
    
    return;
}




/** 
 * Print the detail information
 */
void PINT_gather_detail(
			struct PINT_perf_counter_detail* pc_detail)
{
    int i;
    struct PINT_perf_detail_item* startp;


    if(!pc_detail)
	{
	    /* do nothing if perf counter is not initialized */
	    return;
	}
    gen_mutex_lock(&pc_detail->mutex);

    

    startp = pc_detail->value_list;


    gossip_debug(
		 GOSSIP_FLOW_PROTO_DEBUG,
		 "pc_detail : handle size  %d, \n", pc_detail->handle_size
		 );
    

    for (i = 0; i < pc_detail->handle_size; i++){
	
	gossip_debug(
		     GOSSIP_FLOW_PROTO_DEBUG,
		     "perf_update_do_work->gather detail: fs_id: %d file_handle:  %llu  read: %lld  write:%lld \n",
		     startp->fs_id, startp->id, startp->value_read, startp->value_write);
	startp = startp->next;
	 
    }
    
    
    gen_mutex_unlock(&pc_detail->mutex);
    return;
}


int   PINT_reset_perf_counter_detail(struct PINT_perf_counter_detail* pc_detail)
{

    struct PINT_perf_detail_item *cur_p, *next_p;
	
    int i;
	
    gen_mutex_lock(&pc_detail->mutex);

    cur_p = pc_detail->value_list;
    for (i = 0; i < pc_detail->handle_size; i++){
	next_p = cur_p->next;
	free(cur_p);
	cur_p = next_p;
    }
	
    pc_detail->value_list = NULL;
    pc_detail->handle_size = 0;

    gen_mutex_unlock(&pc_detail->mutex);
    
    return 0;    

}




int   PINT_delete_perf_counter_detail_item(
					   struct PINT_perf_counter_detail* pc_detail, PVFS_handle id)
{
    /*
      struct PINT_perf_detail_item *cur_p, *pre_p;
	
      int i;
	
      gen_mutex_lock(&pc_detail->mutex);

      cur_p = pc_detail->value_list;
      pre_p = cur_p;
	
      for (i = 0; i < pc_detail->handle_size; i++){
      if (id  == (cur_p->id)){
      pre_p = cur_p->next;
      free(cur_p);
      pc_detail->handle_size - 1;
      break;
      }
      pre_p = cur_p;
      cur_p = cur_p->next;
      }
	
	
      gen_mutex_unlock(&pc_detail->mutex);
    
      return 0;    
    */
    return 0;
}



struct PINT_perf_detail_item  PINT_choose_heavy_one(
						    struct PINT_perf_counter_detail* pc_detail, 
						    double load_diff, double interval, 
						    PVFS_handle rt_handle, double high_load, 
						    double low_load, char *access_size, 
						    double global_max_load,
						    int    *candidate_f)
{
    struct PINT_perf_detail_item choose_load_item, *startp;
    double    max_load;
    int i;
    double    r1, r2;
    
    memset(&choose_load_item, 0, sizeof(struct PINT_perf_detail_item));
    if(!pc_detail){
	return choose_load_item;
    }

    gen_mutex_lock(&pc_detail->mutex);
    startp = pc_detail->value_list;
    max_load = 0;
    
    
    int find = -1;
    double temp_load;
    /* Fix me to add size of file */
    for (i = 0; i < pc_detail->handle_size; i++){
        temp_load = (startp->value_read + startp->value_write);
        temp_load = (double)( temp_load / (1024.0 * 1024.0));
        temp_load = temp_load / (interval);
        
	//        gossip_debug( 
	//           GOSSIP_LB_DEBUG, 
	//           "index: %d, temp_load: %g, load_diff: %g \n, rt_h= %llu ", i, temp_load, load_diff, rt_handle);     
        //printf();
        //printf("%llu \n", rt_handle);
        
	r1 = PINT_load_vs_response_function(high_load, access_size, global_max_load)+PINT_load_vs_response_function(low_load, access_size, global_max_load);
	r2 = PINT_load_vs_response_function(high_load - temp_load, access_size, global_max_load)+PINT_load_vs_response_function(low_load+temp_load, access_size, global_max_load);

/* 	gossip_debug(  */
/* 		     GOSSIP_SERVER_DEBUG,  */
/* 		     "index: %d, temp_load: %g, load_diff: %g \n, rt_h= %llu, r1 = %f, r2= %f \n", i, temp_load, load_diff, rt_handle, r1, r2);      */
	
	if ((temp_load < load_diff) 
	    && (max_load < temp_load)
	    && (startp->id != rt_handle) 
	    ){
	    
/* 	    gossip_debug(  */
/* 			 GOSSIP_LB_DEBUG,  */
/* 			 "index: %d, temp_load: %g, load_diff: %g \n, rt_h= %llu, r1 = %f, r2= %f \n", i, temp_load, load_diff, rt_handle, r1, r2);      */
	    
	    // printf("%llu \n", rt_handle);
	    find = 1;
	    *candidate_f = 1;
	    max_load = temp_load;//(startp->value_read + startp->value_write);
	    choose_load_item.value_read = startp->value_read;
	    choose_load_item.value_write = startp->value_write;
	    choose_load_item.fs_id = startp->fs_id;
	    choose_load_item.id = startp->id;
        }
        startp = startp->next;
	
	
    }
    gen_mutex_unlock(&pc_detail->mutex);
    
    if (find != -1){
        gossip_debug( 
                     GOSSIP_SERVER_DEBUG, 
                     "find: %d, max_load: %g \n ", find, max_load);     
	return choose_load_item;    
    }else{
	*candidate_f = 0;
        choose_load_item.id = 0;
        return choose_load_item;
    }
}



void PINT_perf_elim_trend(double *hist_load, double *temp_ts, int *t_diff){
    int i;
    int diff = 1; 		/* Assume diff= 1 at first */
	
    for (i = 0;  i < ((PERF_DEFAULT_HISTORY_SIZE - 1)- diff); i++){
	if (diff == 0){
	    temp_ts[i] = hist_load[i];
	}else{
	    temp_ts[i] = hist_load[i] - hist_load[i + diff];
	}
    }   
	
    *t_diff = diff;
}

void PINT_perf_elim_season(double *temp_ts,  double *load_ts, int t_diff, int *s_diff){
    int i;
    int diff = 0;		/* Assume diff = 0 at first */
	
    for (i = 0; i < (((PERF_DEFAULT_HISTORY_SIZE - 1)- t_diff) - diff); i++){ 
	if (diff == 0){
	    load_ts[i] = temp_ts[i]; 
	}else{
	    load_ts[i] = temp_ts[i+diff] - temp_ts[i]; 
	}
    } 
	
    *s_diff = diff;
}


void PINT_perf_smoother(double *hist_load, double *load_ts, int *t_diff, int *s_diff){
    double temp_ts[PERF_DEFAULT_HISTORY_SIZE - 1];

    /* Remove trend */
    PINT_perf_elim_trend(hist_load, temp_ts, t_diff);

    /* Remove season */
    PINT_perf_elim_season(temp_ts,  load_ts, *t_diff, s_diff);
}

/*
 *  Solve Ax = b
 */
int line_equation(int N, double *A, double *b, double *x)
{
    gsl_matrix_view tmp_A = gsl_matrix_view_array(A, N, N);
    gsl_vector_view tmp_b = gsl_vector_view_array(b, N);
    gsl_vector *tmp_x = gsl_vector_alloc(N);
    int s, i;
    gsl_permutation *p = gsl_permutation_alloc(N);
    
    gsl_linalg_LU_decomp(&tmp_A.matrix, p, &s);
    gsl_linalg_LU_solve(&tmp_A.matrix, p, &tmp_b.vector, tmp_x);
	
    for (i = 0; i < N; i++) {
	x[i] = gsl_vector_get(tmp_x, i);
    }
	
    gsl_permutation_free(p);
    gsl_vector_free(tmp_x);

    return 0;
}

/*
 * Estimate cofficients of equation
 */
int ar_estimate(int n, double *R, double *ar_coff)
{
    double *A=(double *)malloc(n * n * sizeof(double));
    double *b=(double *)malloc(n * sizeof(double));
	
    int k, p;
	
    for (k = 0; k < n; k++) {
	for (p = 0; p < n; p++) {
	    A[k*n + p] = R[abs(k-p)];
	}
	b[k] = R[k+1];
    }
	
    line_equation(n, A, b, ar_coff);
	
    free(A);
    free(b);

    return 0;
}


double lse_estimate(double *history_load, int history_load_length, int min_aic_index, double *residual){
	

    int           i, j;
    gsl_matrix *X, *cov;
    gsl_vector *y, *c;
    int           n, dim;
    double     chisq;

    /* Fix me: when history_load_length- min_aic_index<min_aic_index */
    n = history_load_length - min_aic_index;
    //n = min_aic_index;
    
    //dim = min_aic_index + 1;
    dim = min_aic_index;

    
    //printf("n= %d, dim= %d\n", n, dim);
    
    X = gsl_matrix_alloc(n, dim);
    y = gsl_vector_alloc(n);
    c = gsl_vector_alloc(dim);
    cov = gsl_matrix_alloc(dim, dim);
	
    for (i = 0 ; i < n; i++){
	//gsl_matrix_set(X, i, 0, 1.0);
	gsl_vector_set(y,  i,  history_load[i]); 
	for (j = 0 ; j < dim; j++){
	    gsl_matrix_set(X, i, j, history_load[ (i + 1) + j]);
	}
    }
	
    {
	gsl_multifit_linear_workspace * work = gsl_multifit_linear_alloc(n, dim);
	gsl_multifit_linear (X, y, c, cov, &chisq, work);
	gsl_multifit_linear_free (work);
    }
    
    *residual = chisq; 

#define C(i) (gsl_vector_get(c,(i)))
    double pre_value = 0;
    for (i = 0; i < dim; i++){
        
	pre_value = pre_value +  C(i) * history_load[i];
    }
    return pre_value;
}


/*
 *Perform ar predict model
 * 
 */
#define LOAD_HISTORY (PERF_DEFAULT_HISTORY_SIZE - 1)
#define MAX_ORDER    (LOAD_HISTORY / 2)
double PINT_perf_ar_predict(struct PINT_perf_counter* pc, int migration_happen, int load_collection_happen, int candidate_find)
{
    double   hist_load[LOAD_HISTORY];           /* raw history load */
    double   load_ts[LOAD_HISTORY];	        /* load  series  */
    int      load_ts_count;
    double   load_mean = 0;
    
    double   coeff[MAX_ORDER][MAX_ORDER];       /* coeff of each order */
    double   r[MAX_ORDER];		        /* correlation */
    int      i, j;
    int      max_order;
    int      t_diff;
    int      s_diff;

    static double residual;
    double aic;
    double aic_min;
    
    static double   load_predict;
    static  int       order_select;
    static  int       min_aic_index;
    struct timeval start_time;
    struct timeval end_time;
    static double  time_consume;

    if(!pc){
        return 0;
    }

    /* read the historyload from pc to hist_load */
    gen_mutex_lock(&pc->mutex);
    for(i = 0; i < LOAD_HISTORY; i++)
	{
	    /* Change load to M/s */
	    hist_load[i] =(double) (pc->value_matrix[PINT_PERF_READ][i + 1] + pc->value_matrix[PINT_PERF_WRITE][i + 1]);
	    hist_load[i] =(double) (hist_load[i]/ (1024.0*1024.0));
	    if (pc->interval_array_ms[i+1] != 0){
		hist_load[i] =(double) hist_load[i] / ((double)pc->interval_array_ms[i+1]/(1000.0));
	    }else{
		hist_load[i] = 0;
	    }
	}
    gen_mutex_unlock(&pc->mutex);
    
    gossip_debug(GOSSIP_LB_DEBUG, ",re=%g, fe=%g, aic_o=%d, f_o=%d, lch=%d, migh=%d, caf=%d\n", hist_load[0], load_predict, min_aic_index, order_select, load_collection_happen, migration_happen, candidate_find);
    
    /* 
     *    Eliminate trend and season of hist_load.
     *     load_ts: load time series
     *        t_diff:  difference lag for trend
     *       s_diff:  difference lag for seacon
     */
    PINT_perf_smoother(hist_load, load_ts, &t_diff, &s_diff);

    /* The count of time series */
    load_ts_count = LOAD_HISTORY - t_diff - s_diff;

    /* Normalization  */
    load_mean = 0;
    for (i = 0; i < load_ts_count; i++){
	load_mean = load_mean + load_ts[i];
    }
    load_mean = load_mean / load_ts_count;
    for (i = 0 ; i < load_ts_count; i++){
	load_ts[i] = load_ts[i] - load_mean;
    }

    /* Set max order  */
    if (MAX_ORDER < load_ts_count){
	max_order = MAX_ORDER  / 2;   
    }else{
	max_order = load_ts_count / 2;
    }
	
    /* Compute autocorrelation of load_ts*/
    memset(r,  0,  sizeof(r));
    for (i  = 0;  i < max_order;  i++){
	for (j = 0;  j < (load_ts_count  - i);  j++){
	    r[i] = r[i] +  load_ts[j] * load_ts[j+i];
	}
	r[i] = r[i] / load_ts_count;
    }

    /* when r[0] =0, all value is zero */
    if (r[0] == 0) 
        return 0; 

    /* for (i = 0;  i < max_order; i++){ */
    /* 	if (r[0] == 0){ */
    /* 		/\* when r[0] =0, all value is zero *\/ */
    /* 		return 0; */
    /* 	}else{ */
    /* 		r[i] = r[i] / r[0]; */
    /* 	} */
    /* } */

    gettimeofday(&start_time, NULL);
    
    aic_min = 0xFFFFFFFF;
    min_aic_index = 1;
    
    double tmp;
    for (i = 1; i < max_order; i++){
	ar_estimate(i, r, coeff[i]);
	tmp = 0;
	for (j = 0; j < i; j++){
	    tmp = tmp + coeff[i][j] * r[j+1];
	}
	tmp = r[0] - tmp;
		
	/* Use BIC here, change to AIC:  log(load_ts_count) = 2 */
	aic = log(tmp) + ((double) (log(load_ts_count) * i))  / load_ts_count;
	if (aic < aic_min){
	    aic_min = aic;
	    min_aic_index = i;
	}
    }
    
    /* Add mean */
    for (i = 0 ;  i < load_ts_count; i++)
	{
	    load_ts[i] = load_ts[i] + load_mean;
	}
    
    load_predict =  lse_estimate(load_ts,  load_ts_count,  min_aic_index,  &residual);
    order_select = min_aic_index;

    double load_predict_tmp;
    double residual_tmp;
    for ( i =  (min_aic_index - 1) ;  i >= 1;  i--){
	load_predict_tmp = lse_estimate(load_ts, load_ts_count, i, &residual_tmp);
	if(residual > residual_tmp){
	    residual = residual_tmp;
	    load_predict = load_predict_tmp;
	    order_select = i;
            
	}else{
	    break;
	}
    }
	
    /*Add mean  */
    //load_predict = load_predict + load_mean;
    
    /* Add season */
    if (s_diff != 0){
	load_predict = load_predict + load_ts[load_ts_count - s_diff];
    }
    
    /* Add trend */
    if (t_diff != 0){
	load_predict = load_predict + hist_load[0];
    }
    
    /* Record time */
    gettimeofday(&end_time, NULL);
    time_consume = (end_time.tv_sec - start_time.tv_sec) + (end_time.tv_usec - start_time.tv_usec) /1000000.0;

    /* Set it to current load for test*/
    if (server_forecast_enable == 0){
        return hist_load[0]; //load_predict;
    }else{
        return load_predict;
    }
}


double PINT_load_vs_response_function(double load, char *access_size, double g_max_load)
{

	double load2 ;
	double load3 ;

	load = load / g_max_load;

	load2 = load * load;
	load3 = load2 * load;

	if ( (strcmp("64k", access_size) == 0) )
	{
		return (-0.45 + 2.65 * load - 4.58 * load2   + 2.57 * load3 );
	}

	if ( (strcmp("128k", access_size) == 0) )
	{
		return (-0.28 + 1.96 * load - 3.69 * load2  + 2.25 * load3);
	}

	if ( (strcmp("256k", access_size) == 0) )
	{
		return (-0.65 + 3.90 * load - 6.67 * load2  + 3.79 * load3);
	}

	
	if ( (strcmp("512k", access_size) == 0) )
	{
		return (-4.71 + 22.23 * load - 33.64 * load2  + 16.62 * load3);
	}


	if ( (strcmp("1024k", access_size) == 0) )
	{
		return (-55.97 + 214.39 * load - 272.05 * load2  + 114.66 * load3);
	}

	
	if ( (strcmp("2048k", access_size) == 0) )
	{
		return (84.83 + -190.36 * load - 101.89 * load2  + 5.78 * load3);
	}
	
	return 0;
}

/**
 * sets runtime tunable performance counter options 
 * \returns 0 on success, -PVFS_error on failure
 */
int PINT_perf_set_info(
		       struct PINT_perf_counter* pc,
		       enum PINT_perf_option option,
		       unsigned int arg)
{
    uint64_t* tmp_unsigned;
    int64_t* tmp_signed;
    int i;

    if(!pc)
	{
	    /* do nothing if perf counter is not initialized */
	    return 0;
	}

    gen_mutex_lock(&pc->mutex);
    switch(option)
	{
        case PINT_PERF_HISTORY_SIZE:
            if(arg <= pc->history_size)
		{
		    pc->history_size = arg;
		}
            else
		{
		    /* we have to reallocate everything */
		    /* NOTE: these macros will return error if needed, and
		     * counter instance will still be operational
		     */
		    PINT_PERF_REALLOC_ARRAY(pc,
					    tmp_unsigned,
					    pc->start_time_array_ms,
					    arg,
					    uint64_t);
		    PINT_PERF_REALLOC_ARRAY(pc,
					    tmp_unsigned,
					    pc->interval_array_ms,
					    arg,
					    uint64_t);
		    for(i=0; i<pc->key_count; i++)
			{
			    PINT_PERF_REALLOC_ARRAY(pc,
						    tmp_signed,
						    pc->value_matrix[i],
						    arg,
						    int64_t);
			}
		    pc->history_size = arg;
		}
            break;
        default:
            gen_mutex_unlock(&pc->mutex);
            return(-PVFS_EINVAL);
	}
    
    gen_mutex_unlock(&pc->mutex);
    return(0);
}

/**
 * retrieves runtime tunable performance counter options 
 * \returns 0 on success, -PVFS_error on failure
 */
int PINT_perf_get_info(
		       struct PINT_perf_counter* pc,
		       enum PINT_perf_option option,
		       unsigned int* arg)
{
    if(!pc)
	{
	    /* do nothing if perf counter is not initialized */
	    return (0);
	}

    gen_mutex_lock(&pc->mutex);
    switch(option)
	{
        case PINT_PERF_HISTORY_SIZE:
            *arg = pc->history_size;
            break;
        case PINT_PERF_KEY_COUNT:
            *arg = pc->key_count;
            break;
        default:
            gen_mutex_unlock(&pc->mutex);
            return(-PVFS_EINVAL);
	}
    
    gen_mutex_unlock(&pc->mutex);
    return(0);
}

/**
 * retrieves measurement history
 */
void PINT_perf_retrieve(
			struct PINT_perf_counter* pc,        /**< performance counter */
			int64_t** value_matrix, /**< 2d matrix to fill in with measurements */
			uint64_t* start_time_array_ms,       /**< array of start times */
			uint64_t* interval_array_ms,         /**< array of interval lengths */
			int max_key,                         /**< max key value (1st dimension) */
			int max_history)                     /**< max history (2nd dimension) */
{
    int i;
    int tmp_max_key;
    int tmp_max_history;
    struct timeval tv;
    uint64_t int_time;

    if(!pc)
	{
	    /* do nothing if perf counter is not initialized */
	    return;
	}

    gen_mutex_lock(&pc->mutex);

    /* it isn't very safe to allow the caller to ask for more keys than are
     * available, because they will probably overrun key array bounds when
     * interpretting results
     */
    assert(max_key <= pc->key_count);
    
    tmp_max_key = PVFS_util_min(max_key, pc->key_count);
    tmp_max_history = PVFS_util_min(max_history, pc->history_size);

    if(max_key > pc->key_count || max_history > pc->history_size)
	{
	    /* zero out value matrix, we won't use all fields */
	    for(i=0; i<max_key; i++)
		{
		    memset(value_matrix[i], 0, (max_history*sizeof(int64_t)));
		}
	}

    if(max_history > pc->history_size)
	{
	    /* zero out time arrays, we won't use all fields */
	    memset(start_time_array_ms, 0, (max_history*sizeof(uint64_t)));
	    memset(interval_array_ms, 0, (max_history*sizeof(uint64_t)));
	}

    /* copy data out */
    for(i=0; i<tmp_max_key; i++)
	{
	    memcpy(value_matrix[i], pc->value_matrix[i],
		   (tmp_max_history*sizeof(int64_t)));
	}
    memcpy(start_time_array_ms, pc->start_time_array_ms,
	   (tmp_max_history*sizeof(uint64_t)));
    memcpy(interval_array_ms, pc->interval_array_ms,
	   (tmp_max_history*sizeof(uint64_t)));
    
    gen_mutex_unlock(&pc->mutex);

    /* fill in interval length for newest interval */
    gettimeofday(&tv, NULL);
    int_time = ((uint64_t)tv.tv_sec) * 1000 + tv.tv_usec / 1000;
    if(int_time > start_time_array_ms[0])
	{
	    interval_array_ms[0] = int_time - start_time_array_ms[0];
	}
    
    return;
}

char* PINT_perf_generate_text(
			      struct PINT_perf_counter* pc,
			      int max_size)
{
    int total_size = 0;
    int line_size = 0;
    int actual_size = 0;
    char* tmp_str;
    char* position;
    int i, j;
    uint64_t int_time;
    struct timeval tv;
    time_t tmp_time_t;
    struct tm tmp_tm;
    int ret;

    gen_mutex_lock(&pc->mutex);
    
    line_size = 26 + (24*pc->history_size); 
    total_size = (pc->key_count+2)*line_size + 1;
    
    actual_size = PVFS_util_min(total_size, max_size);

    if((actual_size/line_size) < 3)
	{
	    /* don't bother trying to display anything, can't fit any results in
	     * that size
	     */
	    return(NULL);
	}

    tmp_str = (char*)malloc(actual_size*sizeof(char));
    if(!tmp_str)
	{
	    gen_mutex_unlock(&pc->mutex);
	    return(NULL);
	}
    position = tmp_str;

    /* start times */
    sprintf(position, "%-24.24s: ", "Start times (hr:min:sec)");
    position += 25;
    for(i=0; i<pc->history_size; i++)
	{
	    if(pc->start_time_array_ms[i])
		{
		    tmp_time_t = pc->start_time_array_ms[i]/1000;
		    localtime_r(&tmp_time_t, &tmp_tm);
		    strftime(position, 11, "  %H:%M:%S", &tmp_tm);
		    position += 10;
		    sprintf(position, ".%03u", 
			    (unsigned)(pc->start_time_array_ms[i]%1000));
		    position += 4;
		}
	    else
		{
		    sprintf(position, "%14d", 0);
		    position += 14;
		}
	}
    sprintf(position, "\n");
    position++;

    /* fill in interval length for newest interval */
    gettimeofday(&tv, NULL);
    int_time = ((uint64_t)tv.tv_sec) * 1000 + tv.tv_usec / 1000;
    if(int_time > pc->start_time_array_ms[0])
	{
	    pc->interval_array_ms[0] = int_time - pc->start_time_array_ms[0];
	}

    /* intervals */
    sprintf(position, "%-24.24s:", "Intervals (hr:min:sec)");
    position += 25;
    for(i=0; i<pc->history_size; i++)
	{
	    if(pc->interval_array_ms[i])
		{
		    tmp_time_t = pc->interval_array_ms[i]/1000;
		    gmtime_r(&tmp_time_t, &tmp_tm);
		    strftime(position, 11, "  %H:%M:%S", &tmp_tm);
		    position += 10;
		    sprintf(position, ".%03u", 
			    (unsigned)(pc->interval_array_ms[i]%1000));
		    position += 4;
		}
	    else
		{
		    sprintf(position, "%14d", 0);
		    position += 14;
		}

	}
    sprintf(position, "\n");
    position++;

    sprintf(position, "-------------------------");
    position += 25;
    for(i=0; i<pc->history_size; i++)
	{
	    sprintf(position, "--------------");
	    position += 14;
	}
    sprintf(position, "\n");
    position++;

    /* values */
    for(i=0; i<pc->key_count; i++)
	{
	    sprintf(position, "%-24.24s:", pc->key_array[i].key_name);
	    position += 25;
	    for(j=0; j<pc->history_size; j++)
		{
		    ret = snprintf(position, 15, " %13lld", lld(pc->value_matrix[i][j]));
		    if(ret >= 15)
			{
			    sprintf(position, "%14.14s", "Overflow!");
			}
		    position += 14;
		}
	    sprintf(position, "\n");
	    position++;
	}

    gen_mutex_unlock(&pc->mutex);

    return(tmp_str);
}








/*
 * Local variables:
 *  c-indent-level: 4
 *  c-basic-offset: 4
 * End:
 *
 * vim: ts=8 sts=4 sw=4 expandtab
 */

