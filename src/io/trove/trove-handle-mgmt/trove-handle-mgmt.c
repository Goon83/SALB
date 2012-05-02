/* 
 * (C) 2001 Clemson University and The University of Chicago 
 *
 * See COPYING in top-level directory.
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <sys/time.h>

#include "trove.h"
#include "quickhash.h"
#include "extent-utils.h"
#include "trove-ledger.h"
#include "trove-handle-mgmt.h"
#include "gossip.h"
#include "gen-locks.h"
#include "pvfs2-internal.h"

/*
  this is an internal structure and shouldn't be used
  by anyone except this module
*/
typedef struct
{
    struct qlist_head hash_link;

    TROVE_coll_id coll_id;
    int have_valid_ranges;

    struct handle_ledger *ledger;
} handle_ledger_t;

static struct qhash_table *s_fsid_to_ledger_table = NULL;

/* these are based on code from src/server/request-scheduler.c */
static int hash_fsid(void *fsid, int table_size);
static int hash_fsid_compare(void *key, struct qlist_head *link);

static gen_mutex_t trove_handle_mutex = GEN_MUTEX_INITIALIZER;

/* trove_check_handle_ranges:
 *  internal function to verify that handles
 *  on disk match our assigned handles.
 *  this function is *very* expensive.
 *
 * coll_id: id of collection which we will verify
 * extent_list: llist of legal handle ranges/extents
 * ledger: a book-keeping ledger object
 *
 * returns 0 on success; -1 otherwise
 */
static int trove_check_handle_ranges(TROVE_coll_id coll_id,
                                     TROVE_context_id context_id,
                                     PINT_llist *extent_list,
                                     struct handle_ledger *ledger)
{
    int ret = -1, i = 0, count = 0, op_count = 0;
    TROVE_op_id op_id = 0;
    TROVE_ds_state state = 0;
    TROVE_ds_position pos = TROVE_ITERATE_START;
    static TROVE_handle handles[MAX_NUM_VERIFY_HANDLE_COUNT] =
        {TROVE_HANDLE_NULL};

    if (extent_list && ledger)
    {
        count = MAX_NUM_VERIFY_HANDLE_COUNT;

        while(count > 0)
        {
            ret = trove_dspace_iterate_handles(coll_id,&pos,handles,
                                               &count,0,NULL,NULL,
                                               context_id,&op_id);
            while(ret == 0)
            {
                ret = trove_dspace_test(coll_id,op_id,context_id,
                                        &op_count,NULL,NULL,&state,
                                        TROVE_DEFAULT_TEST_TIMEOUT);
            }

            /* check result of testing */
            if (ret < 0)
            {
                gossip_debug(GOSSIP_TROVE_DEBUG,
                             "dspace test of iterate_handles failed\n");
                return ret;
            }

            ret = 0;

            /* also check result of actual operation, in this case,
             * trove_dspace_iterate_handles
             */
            if(state < 0)
            {
                gossip_debug(GOSSIP_TROVE_DEBUG,
                             "trove_dspace_iterate_handles failed\n");
                return state;
            }

            /* look for special case of a blank fs */
            if ((count == 1) && (handles[0] == 0))
            {
                gossip_debug(GOSSIP_TROVE_DEBUG,
                             "* Trove: Assuming a blank filesystem\n");
                return ret;
            }

            if (count > 0)
            {
                for(i = 0; i != count; i++)
                {
                    /* check every item in our range list */
                    if (!PINT_handle_in_extent_list(extent_list,
                                                    handles[i]))
                    {
                        gossip_err(
                            "Error: handle %llu is invalid "
                            "(out of bounds)\n", llu(handles[i]));
                        return -1;
                    }

		    /* remove handle from trove-handle-mgmt */
		    ret = trove_handle_remove(ledger, handles[i]);
		    if (ret != 0)
                    {
			gossip_debug(
                            GOSSIP_TROVE_DEBUG, "could not remove "
                            "handle %llu\n", llu(handles[i]));
			break;
		    }
                }
                ret = ((i == count) ? 0 : -1);
            }
        }
    }
    return ret;
}

static int trove_map_handle_ranges( PINT_llist *extent_list,
                                   struct handle_ledger *ledger)
{
    int ret = -1;
    PINT_llist *cur = NULL;
    PVFS_handle_extent *cur_extent = NULL;
    int64_t total_handles=0;

    if (extent_list && ledger)
    {
        cur = extent_list;
        while(cur)
        {
            cur_extent = PINT_llist_head(cur);
            if (!cur_extent)
            {
                break;
            }

	    ret = trove_handle_ledger_addextent(ledger, cur_extent);
	    if (ret != 0)
            {
		break;
            }

	    /* if, for example, you had a 'first' of 5 and a 'last' of
	     * 5, the difference is 0, but there is one handle */
	    total_handles += (cur_extent->last - cur_extent->first + 1);
            cur = PINT_llist_next(cur);
        }
	trove_handle_ledger_set_threshold(ledger, total_handles);
    }
    return ret;
}


static handle_ledger_t *get_or_add_handle_ledger(TROVE_coll_id coll_id)
{
    handle_ledger_t *ledger = NULL;
    struct qlist_head *hash_link = NULL;

    /* search for a matching entry */
    hash_link = qhash_search(s_fsid_to_ledger_table,&(coll_id));
    if (hash_link)
    {
        /* return it if it exists */
        ledger = qlist_entry(hash_link, handle_ledger_t, hash_link);
    }
    else
    {
        /* alloc, initialize, then return otherwise */
        ledger = (handle_ledger_t *)malloc(sizeof(handle_ledger_t));
        if (ledger)
        {
            ledger->coll_id = coll_id;
            ledger->have_valid_ranges = 0;
            ledger->ledger = trove_handle_ledger_init(coll_id,NULL);
            if (ledger->ledger)
            {
                qhash_add(s_fsid_to_ledger_table,
                          &(coll_id),&(ledger->hash_link));
            }
            else
            {
                free(ledger);
                ledger = NULL;
            }
        }
    }
    return ledger;
}

/* hash_fsid()
 *
 * hash function for fsids added to table
 *
 * returns integer offset into table
 */
static int hash_fsid(void *fsid, int table_size)
{
    /* TODO: update this later with a better hash function,
     * depending on what fsids look like, for now just modding
     *
     */
    unsigned long tmp = 0;
    TROVE_coll_id *real_fsid = (TROVE_coll_id *)fsid;

    tmp += (*(real_fsid));
    tmp = tmp%table_size;

    return ((int)tmp);
}

/* hash_fsid_compare()
 *
 * performs a comparison of a hash table entry to a given key
 * (used for searching)
 *
 * returns 1 if match found, 0 otherwise
 */
static int hash_fsid_compare(void *key, struct qlist_head *link)
{
    handle_ledger_t *ledger = NULL;
    TROVE_coll_id *real_fsid = (TROVE_coll_id *)key;

    ledger = qlist_entry(link, handle_ledger_t, hash_link);
    assert(ledger);

    if (ledger->coll_id == *real_fsid)
    {
        return(1);
    }
    return(0);
}

int trove_handle_mgmt_initialize()
{
    /*
      due to weird trove_initialize usages; this will always succeed
      unless the hash table initialization really fails.
    */
    int ret = 0;
    gen_mutex_lock(&trove_handle_mutex);

    if (s_fsid_to_ledger_table == NULL)
    {
        s_fsid_to_ledger_table = qhash_init(hash_fsid_compare,
                                            hash_fsid,67);
        ret = (s_fsid_to_ledger_table ? 0 : -1);
    }
    gen_mutex_unlock(&trove_handle_mutex);
    return ret;
}

int trove_set_handle_ranges(TROVE_coll_id coll_id,
                            TROVE_context_id context_id,
                            char *handle_range_str)
{
    int ret = -TROVE_EINVAL;
    PINT_llist *extent_list = NULL;
    handle_ledger_t *ledger = NULL;

    gen_mutex_lock(&trove_handle_mutex);
    if (handle_range_str)
    {
        extent_list = PINT_create_extent_list(handle_range_str);
        if (extent_list)
        {
            /*
              get existing ledger management struct if any;
              create otherwise
            */
            ledger = get_or_add_handle_ledger(coll_id);
            if (ledger)
            {
                /* assert the internal ledger struct is valid */
                assert(ledger->ledger);
		
		/* tell trove what are our valid ranges are */
		ret = trove_map_handle_ranges(
                    extent_list, ledger->ledger);
		if (ret != 0)
                {
                    gen_mutex_unlock(&trove_handle_mutex);
                    return ret;
                }

                ret = trove_check_handle_ranges(
                    coll_id,context_id,extent_list,ledger->ledger);
		if (ret != 0)
                {
                    gen_mutex_unlock(&trove_handle_mutex);
                    return ret;
                }
                else
                {
                    ledger->have_valid_ranges = 1;
                }
            }
            PINT_release_extent_list(extent_list);
        }
    }
    gen_mutex_unlock(&trove_handle_mutex);
    return ret;
}

/*
 * trove_set_handle_timeout: controls how long a handle, once freed,
 * will sit on the sidelines before returning to the pool of
 * avaliable handles 
 */
int trove_set_handle_timeout(TROVE_coll_id coll_id,
                             TROVE_context_id context_id,
                             struct timeval *timeout)
{
    int ret = -1;
    handle_ledger_t *ledger = NULL;

    gen_mutex_lock(&trove_handle_mutex);
    ledger = get_or_add_handle_ledger(coll_id);
    if (ledger)
    {
	/* assert the internal ledger struct is valid */
	assert(ledger->ledger);

	/*
          tell trove how long the timeout should be.
          if 0 is specified, use a reasonable default value
        */
        timeout->tv_sec = ((timeout->tv_sec == 0) ?
                           TROVE_DEFAULT_HANDLE_PURGATORY_SEC :
                           timeout->tv_sec);
	ret = trove_ledger_set_timeout(ledger->ledger, timeout);

        gossip_debug(GOSSIP_TROVE_DEBUG, "- set handle re-use "
                     "timeout to %d seconds (ret=%d)\n",
                     (int)timeout->tv_sec, ret);
    }
    gen_mutex_unlock(&trove_handle_mutex);
    return ret;
}

TROVE_handle trove_handle_alloc(TROVE_coll_id coll_id)
{
    handle_ledger_t *ledger = NULL;
    struct qlist_head *hash_link = NULL;
    TROVE_handle handle = TROVE_HANDLE_NULL;

    gen_mutex_lock(&trove_handle_mutex);
    hash_link = qhash_search(s_fsid_to_ledger_table,&(coll_id));
    if (hash_link)
    {
        ledger = qlist_entry(hash_link, handle_ledger_t, hash_link);
        if (ledger && (ledger->have_valid_ranges == 1))
        {
            handle = trove_ledger_handle_alloc(ledger->ledger);
        }
    }
    gen_mutex_unlock(&trove_handle_mutex);
    return handle;
}

TROVE_handle trove_handle_alloc_from_range(
    TROVE_coll_id coll_id,
    TROVE_handle_extent_array *extent_array)
{
    handle_ledger_t *ledger = NULL;
    struct qlist_head *hash_link = NULL;
    TROVE_handle handle = TROVE_HANDLE_NULL;
    int i = 0;

    gen_mutex_lock(&trove_handle_mutex);
    hash_link = qhash_search(s_fsid_to_ledger_table, &(coll_id));
    if (hash_link)
    {
        ledger = qlist_entry(hash_link, handle_ledger_t, hash_link);
        if (ledger && (ledger->have_valid_ranges == 1))
        {
            for(i = 0; i < extent_array->extent_count; i++)
            {
                handle = trove_ledger_handle_alloc_from_range(
                    ledger->ledger, &(extent_array->extent_array[i]));
                if (handle != TROVE_HANDLE_NULL)
                {
                    break;
                }
            }
        }
    }
    gen_mutex_unlock(&trove_handle_mutex);
    return handle;
}

int trove_handle_peek(
    TROVE_coll_id coll_id,
    TROVE_handle *out_handle_array,
    int max_num_handles,
    int *returned_handle_count)
{
    int ret = -TROVE_EINVAL;
    handle_ledger_t *ledger = NULL;
    struct qlist_head *hash_link = NULL;

    if (!out_handle_array || !returned_handle_count)
    {
        return ret;
    }

    gen_mutex_lock(&trove_handle_mutex);
    hash_link = qhash_search(s_fsid_to_ledger_table,&(coll_id));
    if (hash_link)
    {
        ledger = qlist_entry(hash_link, handle_ledger_t, hash_link);
        if (ledger && (ledger->have_valid_ranges == 1))
        {
            ret = trove_ledger_peek_handles(
                ledger->ledger, out_handle_array,
                max_num_handles, returned_handle_count);
        }
    }
    gen_mutex_unlock(&trove_handle_mutex);
    return ret;
}

int trove_handle_peek_from_range(
    TROVE_coll_id coll_id,
    TROVE_handle_extent_array *extent_array,
    TROVE_handle *out_handle_array,
    int max_num_handles,
    int *returned_handle_count)
{
    int ret = -TROVE_EINVAL, i = 0;
    handle_ledger_t *ledger = NULL;
    struct qlist_head *hash_link = NULL;

    if (!extent_array || !out_handle_array || !returned_handle_count)
    {
        return ret;
    }

    gen_mutex_lock(&trove_handle_mutex);
    hash_link = qhash_search(s_fsid_to_ledger_table,&(coll_id));
    if (hash_link)
    {
        ledger = qlist_entry(hash_link, handle_ledger_t, hash_link);
        if (ledger && (ledger->have_valid_ranges == 1))
        {
            for(i = 0; i < extent_array->extent_count; i++)
            {
                ret = trove_ledger_peek_handles_from_extent(
                    ledger->ledger, &(extent_array->extent_array[i]),
                    out_handle_array, max_num_handles,
                    returned_handle_count);
                /*
                  if we get any handles back, just return, even if
                  it's not the full amount requested
                */
                if (ret == 0)
                {
                    assert(*returned_handle_count > 0);
                    break;
                }
            }
        }
    }
    gen_mutex_unlock(&trove_handle_mutex);
    return ret;
}

int trove_handle_set_used(TROVE_coll_id coll_id, TROVE_handle handle)
{
    int ret = -1;
    handle_ledger_t *ledger = NULL;
    struct qlist_head *hash_link = NULL;

    gen_mutex_lock(&trove_handle_mutex);
    hash_link = qhash_search(s_fsid_to_ledger_table,&(coll_id));
    if (hash_link)
    {
        ledger = qlist_entry(hash_link, handle_ledger_t, hash_link);
        if (ledger)
        {
            ret = trove_handle_remove(ledger->ledger,handle);
        }
    }
    gen_mutex_unlock(&trove_handle_mutex);
    return ret;
}

int trove_handle_free(TROVE_coll_id coll_id, TROVE_handle handle)
{
    int ret = -1;
    handle_ledger_t *ledger = NULL;
    struct qlist_head *hash_link = NULL;

    gen_mutex_lock(&trove_handle_mutex);
    hash_link = qhash_search(s_fsid_to_ledger_table,&(coll_id));
    if (hash_link)
    {
        ledger = qlist_entry(hash_link, handle_ledger_t, hash_link);
        if (ledger)
        {
            ret = trove_ledger_handle_free(ledger->ledger, handle);
        }
    }
    gen_mutex_unlock(&trove_handle_mutex);
    return ret;
}

/* trove_handle_get_statistics()
 *
 * retrieves handle usage statistics from given collection; right now
 * this simply means returning the count of free handles
 *
 * returns 0 on success, -1 on error
 */
int trove_handle_get_statistics(TROVE_coll_id coll_id, uint64_t* free_count)
{
    handle_ledger_t *ledger = NULL;
    struct qlist_head *hash_link = NULL;

    gen_mutex_lock(&trove_handle_mutex);
    hash_link = qhash_search(s_fsid_to_ledger_table,&(coll_id));
    if (hash_link)
    {
        ledger = qlist_entry(hash_link, handle_ledger_t, hash_link);
        if (ledger)
        {
	    trove_handle_ledger_get_statistics(ledger->ledger, 
		free_count);
            gen_mutex_unlock(&trove_handle_mutex);
	    return(0);
        }
	else
	{
            gen_mutex_unlock(&trove_handle_mutex);
	    return(-PVFS_ENOENT);
	}
    }
    else
    {
        gen_mutex_unlock(&trove_handle_mutex);
	return(-PVFS_ENOENT);
    }
}

int trove_handle_mgmt_finalize()
{
    int i;
    handle_ledger_t *ledger = NULL;
    struct qlist_head *hash_link = NULL;

    gen_mutex_lock(&trove_handle_mutex);
    /*
      this is an exhaustive and slow iterate.  speed this up
      if 'finalize' is something that will be done frequently.
    */
    for (i = 0; i < s_fsid_to_ledger_table->table_size; i++)
    {
        do
        {
            hash_link =
                qhash_search_and_remove_at_index(s_fsid_to_ledger_table, i);
            if (hash_link)
            {
                ledger = qlist_entry(hash_link, handle_ledger_t, hash_link);
                assert(ledger);
                assert(ledger->ledger);

                trove_handle_ledger_free(ledger->ledger);
                free(ledger);
            }
        } while(hash_link);
    }
    qhash_finalize(s_fsid_to_ledger_table);
    s_fsid_to_ledger_table = NULL;

    gen_mutex_unlock(&trove_handle_mutex);
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
