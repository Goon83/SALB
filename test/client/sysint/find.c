/*
 * (C) 2001 Clemson University and The University of Chicago
 *
 * See COPYING in top-level directory.
 */

#include <stdio.h>
#include <stdlib.h>
#include <client.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include "pvfs2-util.h"
#include "pvfs2-internal.h"

/* TODO: this can be larger after system interface readdir logic
 * is in place to break up large readdirs into multiple operations
 */
#define MAX_NUM_DIRENTS    32


void print_at_depth(char *name, int depth);
int is_directory(PVFS_handle handle, PVFS_fs_id fs_id);
int directory_walk(PVFS_fs_id cur_fs,
                   char *start_dir, char *base_dir, int depth);

void print_at_depth(char *name, int depth)
{
    /* we ignore depth for now */
    if (name)
    {
        printf("****  %s\n",name);
    }
}

/*
  returns -1 on error; 0 if handle is not a directory;
  1 otherwise
*/
int is_directory(PVFS_handle handle, PVFS_fs_id fs_id)
{
    PVFS_object_ref pinode_refn;
    uint32_t attrmask;
    PVFS_credentials credentials;
    PVFS_sysresp_getattr getattr_response;

    memset(&getattr_response,0,sizeof(PVFS_sysresp_getattr));

    pinode_refn.handle = handle;
    pinode_refn.fs_id = fs_id;
    attrmask = PVFS_ATTR_SYS_ALL_NOSIZE;

    PVFS_util_gen_credentials(&credentials);
    if (PVFS_sys_getattr(pinode_refn, attrmask,
                         &credentials, &getattr_response, NULL))
    {
        fprintf(stderr,"Failed to get attributes on handle 0x%08llx "
                "(fs_id is %d)\n",llu(handle),fs_id);
        return -1;
    }
    return ((getattr_response.attr.objtype == PVFS_TYPE_DIRECTORY) ? 1 : 0);
}

int directory_walk(PVFS_fs_id cur_fs,
                   char *start_dir, char *base_dir, int depth)
{
    int i = 0;
    int is_dir = 0;
    char *cur_file = (char *)0;
    PVFS_handle cur_handle;
    PVFS_sysresp_lookup lk_response;
    PVFS_sysresp_readdir rd_response;
    char full_path[PVFS_NAME_MAX] = {0};
    char* name;
    PVFS_credentials credentials;
    PVFS_object_ref pinode_refn;
    PVFS_ds_position token;
    int pvfs_dirent_incount;

    gossip_debug(GOSSIP_CLIENT_DEBUG, "DIRECTORY WALK CALLED WITH "
                 "base %s | %s\n",base_dir,start_dir);

    memset(&lk_response,0,sizeof(PVFS_sysresp_lookup));

    if (base_dir)
    {
        strncpy(full_path,base_dir,PVFS_NAME_MAX);
        if (strlen(base_dir) > 1)
        {
            strcat(full_path,"/");
        }
        strncat(full_path,start_dir,PVFS_NAME_MAX);
    }
    else
    {
        strcpy(full_path,start_dir);
    }
    name = full_path;

    PVFS_util_gen_credentials(&credentials);
    if (PVFS_sys_lookup(cur_fs, name, &credentials,
                        &lk_response, PVFS2_LOOKUP_LINK_FOLLOW, NULL))
    {
        fprintf(stderr,"Failed to lookup %s on fs_id %d!\n",
                name,cur_fs);
        return 1;
    }

    print_at_depth(name,depth);

    pinode_refn.handle = lk_response.ref.handle;
    pinode_refn.fs_id = cur_fs;
    token = 0;
    pvfs_dirent_incount = MAX_NUM_DIRENTS;

    do
    {
        memset(&rd_response,0,sizeof(PVFS_sysresp_readdir));
        if (PVFS_sys_readdir(pinode_refn,
                             (!token ? PVFS_READDIR_START : token),
                             pvfs_dirent_incount,
                             &credentials, &rd_response, NULL))
        {
            fprintf(stderr,"Failed to perform readdir operation\n");
            return 1;
        }

        if (!rd_response.pvfs_dirent_outcount)
        {
            gossip_debug(GOSSIP_CLIENT_DEBUG,"No files found.\n");
            return 0;
        }

        gossip_debug(GOSSIP_CLIENT_DEBUG, "%d files found.\n",
                     rd_response.pvfs_dirent_outcount);
        for(i = 0; i < rd_response.pvfs_dirent_outcount; i++)
        {
            cur_file = rd_response.dirent_array[i].d_name;
            cur_handle = rd_response.dirent_array[i].handle;

            gossip_debug(GOSSIP_CLIENT_DEBUG,"Got handle %llu\n",
                         llu(cur_handle));

            is_dir = is_directory(cur_handle,
                                  cur_fs);
            switch(is_dir)
            {
                case -1:
                    /* if we had an error, warn */
                    gossip_err("Failed to get attributes.  Skipping "
                               "file %s\n", cur_file);
                    break;
                case 0:
                    /* if we have a normal file, print it */
                {
                    char buf[PVFS_NAME_MAX] = {0};
                    snprintf(buf,PVFS_NAME_MAX,"%s/%s",
                             ((strcmp(full_path,"/")) ?
                              full_path : ""),cur_file);
                    print_at_depth(buf,depth);
                }
                break;
                case 1:
                    /* if we have a dir, recurse */
                    if (directory_walk(cur_fs, cur_file,
                                       full_path,depth+1))
                    {
                        fprintf(stderr,"Failed directory walk at "
                                "depth %d\n", depth+1);
                        return 1;
                    }
                    break;
            }
        }

        token += rd_response.pvfs_dirent_outcount;
        if (rd_response.pvfs_dirent_outcount)
            free(rd_response.dirent_array);

    } while(rd_response.pvfs_dirent_outcount != 0);

    return 0;
}


int main(int argc, char **argv)
{
    int go_twice = 0;
    PVFS_fs_id cur_fs;
    int ret = -1;

    if (argc != 2)
    {
        if (argc == 3)
        {
            go_twice = atoi(argv[2]);
            if (go_twice != 1)
            {
                goto usage;
            }
            goto start_find;
        }
      usage:
        fprintf(stderr,"usage: %s <starting dir> [ 1 ]\n",argv[0]);
        fprintf(stderr,"This is not a full featured version of FIND(1L)\n");
        fprintf(stderr,"If the 3rd argument is a '1', find runs twice.\n");
        fprintf(stderr," (useful for lookup/dcache testing).\n");
        return 1;
    }

  start_find:

    ret = PVFS_util_init_defaults();
    if (ret < 0)
    {
	PVFS_perror("PVFS_util_init_defaults", ret);
	return (-1);
    }
    ret = PVFS_util_get_default_fsid(&cur_fs);
    if (ret < 0)
    {
	PVFS_perror("PVFS_util_get_default_fsid", ret);
	return (-1);
    }

    if (directory_walk(cur_fs, argv[1],NULL,0))
    {
        fprintf(stderr,"Failed to do directory walk\n");
        return 1;
    }

    if (go_twice)
    {
        if (directory_walk(cur_fs, argv[1],NULL,0))
        {
            fprintf(stderr,"Failed to do directory walk (2nd iteration)\n");
            return 1;
        }
    }

    if (PVFS_sys_finalize())
    {
        fprintf(stderr,"Failed to finalize PVFS\n");
        return 1;
    }

    return 0;
}
