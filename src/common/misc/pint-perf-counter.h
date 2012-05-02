/*
 * Copyright © Acxiom Corporation, 2005
 *
 * See COPYING in top-level directory.
 */

#ifndef __PINT_PERF_COUNTER_H
#define __PINT_PERF_COUNTER_H

#include "pvfs2-types.h"
#include "pvfs2-mgmt.h"
#include "gen-locks.h"



enum {
PERF_DEFAULT_TIME_INTERVAL_SECS = 300,
PERF_DEFAULT_HISTORY_SIZE       = 81,
};

/** flag that indicates that values for a particular key should be preserved
 * across rollover rather than reset to 0
 */
#define PINT_PERF_PRESERVE 1

/* TODO: this may be moved in the long term; it is an enumeration of keys
 * that pvfs2-server supports (used by trove and flow counters)
 */
enum PINT_server_perf_keys
{
    PINT_PERF_READ = 0,
    PINT_PERF_WRITE = 1,
    PINT_PERF_METADATA_READ = 2,
    PINT_PERF_METADATA_WRITE = 3,
    PINT_PERF_METADATA_DSPACE_OPS = 4,
    PINT_PERF_METADATA_KEYVAL_OPS = 5,
    PINT_PERF_REQSCHED = 6
};

/** enumeration of valid measurement operations */
enum PINT_perf_ops
{
    PINT_PERF_ADD = 0,
    PINT_PERF_SUB = 1,
    PINT_PERF_SET = 2,
};

/** enumeration of runtime options */
enum PINT_perf_option
{
    PINT_PERF_HISTORY_SIZE = 1,  /**< sets/gets the history size */
    PINT_PERF_KEY_COUNT = 2      /**< gets the key coung (cannot be set) */
};

/** describes a single key to be stored in the perf counter interface */
struct PINT_perf_key
{
    const char* key_name;    /**< string name for key */
    int key;           /**< integer representation of key */
    int flag;          /**< flags that modify behavior of values in this key */
};

/** struct representing a perf counter instance */
struct PINT_perf_counter
{
    gen_mutex_t mutex;
    struct PINT_perf_key* key_array;     /**< keys (provided by initialize()) */
    int key_count;                       /**< number of keys */
    int history_size;                    /**< number of history intervals */
    /** matrix of statistics, first dimension is key, second is history */
    int64_t** value_matrix; 
    uint64_t* start_time_array_ms;        /**< array of start times */
    uint64_t* interval_array_ms;          /**< array of interval lengths */
};

/** server-wide perf counter structure */
extern struct PINT_perf_counter *PINT_server_pc;

struct PINT_perf_detail_item
{
    PVFS_handle  id;
    PVFS_fs_id fs_id;
    int64_t      value_read;
    int64_t      value_write;
    struct PINT_perf_detail_item *next;
};

/** struct representing a perf counter instance per handle (or datafile) */
/** matrix of statistics, first dimension is handle, second is
 *  performance(read and write)
 *  ------------------------
 *     id       read  write  next
 *  handle1      xx    xx    ->handle2
 *  handle2      xx    xx    NULL
 *  .......      ...   ...
 *  ------------------------
 */
struct PINT_perf_counter_detail
{
    gen_mutex_t mutex;
    int handle_size;            /* file handle count */
    struct PINT_perf_detail_item* value_list; 
};


/** server-wide perf counter structure */
extern struct PINT_perf_counter_detail* PINT_server_pc_detail;

/* Global forcast load */
extern double global_forecast_load;


/* added to record migration state
 *    0: no migration(default)
 *    1: migration
 */
extern int    mig_state; 



//extern struct PVFS_mgmt_perf_stat ** perf_load_matrix;
//extern PVFS_BMI_addr_t * perf_load_addr_array;   
//extern int io_server_count;

struct PINT_perf_counter* PINT_perf_initialize(
    struct PINT_perf_key* key_array);
 
struct PINT_perf_counter_detail* PINT_perf_initialize_detail(void);

//void PINT_perf_finalize_detail(struct PINT_perf_counter_detail* pc_detail);
void PINT_perf_finalize_detail(struct PINT_perf_counter_detail* pc_detail);    /**< pointer to counter instance */

void PINT_perf_finalize(
    struct PINT_perf_counter* pc);

void PINT_perf_reset(
    struct PINT_perf_counter* pc);

void __PINT_perf_count(
    struct PINT_perf_counter* pc,
    int key, 
    int64_t value,
    enum PINT_perf_ops op);

void __PINT_perf_count_detail(
    struct PINT_perf_counter_detail* pc_detail,
    int key,
    PVFS_fs_id fs_id,
    PVFS_handle  handle,
    int64_t value);



#ifdef __PVFS2_DISABLE_PERF_COUNTERS__
    #define PINT_perf_count(w,x,y,z) do{}while(0)
    #define PINT_perf_count_detail(w,x,y,z) do{}while(0)
#else
    #define PINT_perf_count __PINT_perf_count
    #define PINT_perf_count_detail __PINT_perf_count_detail
#endif

void PINT_perf_rollover(
    struct PINT_perf_counter* pc);

void PINT_gather_detail(
    struct PINT_perf_counter_detail* pc_detail);

int   PINT_reset_perf_counter_detail(struct PINT_perf_counter_detail* pc_detail);
int   PINT_delete_perf_counter_detail_item(
					   struct PINT_perf_counter_detail* pc_detail, PVFS_handle id);
struct PINT_perf_detail_item  PINT_choose_heavy_one(struct PINT_perf_counter_detail* pc_detail, 
						    double load_diff, double interval, 
						    PVFS_handle rt_handle, double high_load, 
						    double low_load, char *access_size, 
						    double global_max_load, int *candidate_f);
double lse_estimate(double *history_load, int history_load_length, int min_aic_index, double *residual);
int line_equation(int N, double *A, double *b, double *x);
int ar_estimate(int n, double *R, double *ar_coff);
double lse_estimate(double *history_load, int history_load_length, int min_aic_index, double *residual);
int line_equation(int N, double *A, double *b, double *x);
int ar_estimate(int n, double *R, double *ar_coff);
double PINT_perf_ar_predict(struct PINT_perf_counter* pc, int migration_happen, int load_collection_happen, int candidate_find);
double PINT_load_vs_response_function(double load, char *access_size, double g_max_load);

void PINT_perf_smoother(double *hist_load, double *load_ts, int *t_diff, int *s_diff);
void PINT_perf_elim_trend(double *hist_load, double *temp_ts, int *t_diff);
void PINT_perf_elim_season(double *temp_ts,  double *load_ts, int t_diff, int *s_diff);

int PINT_perf_set_info(
    struct PINT_perf_counter* pc,
    enum PINT_perf_option option,
    unsigned int arg);

int PINT_perf_get_info(
    struct PINT_perf_counter* pc,
    enum PINT_perf_option option,
    unsigned int* arg);

void PINT_perf_retrieve(
    struct PINT_perf_counter* pc,        
    int64_t** value_matrix,
    uint64_t* start_time_array_ms,       
    uint64_t* interval_array_ms,         
    int max_key,                         
    int max_history);     

char* PINT_perf_generate_text(
    struct PINT_perf_counter* pc,
    int max_size);


#endif /* __PINT_PERF_COUNTER_H */

/*
 * Local variables:
 *  c-indent-level: 4
 *  c-basic-offset: 4
 * End:
 *
 * vim: ts=8 sts=4 sw=4 expandtab
 */
