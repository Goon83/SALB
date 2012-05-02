/*
 * (C) 2001 Clemson University and The University of Chicago
 *
 * See COPYING in top-level directory.
 */

#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/time.h>
#include <time.h>
#include <stdlib.h>
#include <getopt.h>

#include "pvfs2.h"
#include "pvfs2-mgmt.h"
#include "pint-sysint-utils.h"
#include "server-config.h"
#include "pvfs2-internal.h"

#ifndef PVFS2_VERSION
#define PVFS2_VERSION "Unknown"
#endif

struct options
{
    char* fs_path_hack;
    char* fs_path_real;
    char* mnt_point;
};

static struct options* parse_args(int argc, char* argv[]);
static void usage(int argc, char** argv);
static void print_mntent(
    struct PVFS_sys_mntent *entries, int num_entries);
static int print_config(PVFS_fs_id fsid);
static int noop_all_servers(PVFS_fs_id fsid);
static void print_error_details(PVFS_error_details * error_details);
static void print_root_check_error_details(PVFS_error_details * error_details);

int main(int argc, char **argv)
{
    int ret = -1, err = 0;
    int i;
    PVFS_fs_id cur_fs;
    const PVFS_util_tab* tab;
    struct options* user_opts = NULL;
    char pvfs_path[PVFS_NAME_MAX] = {0};
    PVFS_credentials creds;
    PVFS_sysresp_lookup resp_lookup;
    PVFS_error_details * error_details;
    struct PVFS_mgmt_setparam_value param_value;
    int count;

    /* look at command line arguments */
    user_opts = parse_args(argc, argv);
    if(!user_opts)
    {
	fprintf(stderr, "Error: failed to parse command line "
                "arguments.\n");
	usage(argc, argv);
	return(-1);
    }

    printf("\n(1) Parsing tab file...\n");
    tab = PVFS_util_parse_pvfstab(NULL);
    if (!tab)
    {
	PVFS_perror("PVFS_util_parse_pvfstab", ret);
        fprintf(stderr, "Failure: could not parse pvfstab.\n");
        return(-1);
    }

    printf("\n(2) Initializing system interface...\n");
    ret = PVFS_sys_initialize(GOSSIP_NO_DEBUG);
    if(ret < 0)
    {
	PVFS_perror("PVFS_sys_initialize", ret);
	fprintf(stderr, "Failure: could not initialize system "
                "interface.\n");
	return(-1);
    }

    printf("\n(3) Initializing each file system found "
           "in tab file: %s...\n", tab->tabfile_name);

    for(i=0; i<tab->mntent_count; i++)
    {
        print_mntent(&tab->mntent_array[i], 1);
        fflush(stdout);
        ret = PVFS_sys_fs_add(&tab->mntent_array[i]);
	printf("   %s: ", tab->mntent_array[i].mnt_dir);
	if(ret < 0)
	{
	    printf("FAILURE!\n");
            err = 1;
	}
	else
	{   
	    printf("Ok\n");
	}
    }
    fflush(stdout);
    if(err)
    {
        fprintf(stderr, "\nFailure: could not initialze at "
                "least one of the target file systems.\n");
    }
    printf("\n(4) Searching for %s in pvfstab...\n",
           user_opts->fs_path_real);

    /* translate local path into pvfs2 relative path */
    ret = PVFS_util_resolve(user_opts->fs_path_hack,
        &cur_fs, pvfs_path, PVFS_NAME_MAX);
    if(ret < 0)
    {
        fprintf(stderr, "Failure: could not find filesystem for %s "
                "in pvfs2tab %s\n", user_opts->fs_path_real, tab->tabfile_name);
        for (i = 0; i < tab->mntent_count; i++)
        {
            fprintf(stderr, "Entry %d: %s\n", i, tab->mntent_array[i].mnt_dir);
        }
        return(-1);
    }

    print_mntent(tab->mntent_array, tab->mntent_count);

    PVFS_util_gen_credentials(&creds);

    /* dump some key parts of the config file */
    ret = print_config(cur_fs);
    if(ret < 0)
    {
	PVFS_perror("print_config", ret);
	fprintf(stderr, "Failure: could not print configuration.\n");
	return(-1);
    }

    printf("\n(5) Verifying that all servers are responding...\n");

    /* send noop to everyone */
    ret = noop_all_servers(cur_fs);
    if(ret < 0)
    {
        fprintf(stderr, "Failure: could not communicate with "
                "one of the servers.\n");
        err = 1;
    }

    printf("\n(6) Verifying that fsid %ld is acceptable "
           "to all servers...\n",(long)cur_fs);

    ret = PVFS_mgmt_count_servers(
        cur_fs, &creds, PVFS_MGMT_IO_SERVER|PVFS_MGMT_META_SERVER, &count);
    if (ret < 0)
    {
	PVFS_perror("PVFS_mgmt_count_servers()", ret);
	return ret;
    }

    error_details = PVFS_error_details_new(count);
    if(!error_details)
    {
        PVFS_perror("PVFS_error_details_new", -ENOMEM);
        fprintf(stderr, "Failure: could not create error details\n");
        return(-1);
    }

    param_value.type = PVFS_MGMT_PARAM_TYPE_UINT64;
    param_value.u.value = (uint64_t)cur_fs;

    /* check that the fsid exists on all of the servers */
    /* TODO: we need a way to get information out about which server fails
     * in error cases here 
     */
    ret = PVFS_mgmt_setparam_all(
        cur_fs, &creds, PVFS_SERV_PARAM_FSID_CHECK,
        &param_value, error_details, NULL);
    if(ret < 0)
    {
	PVFS_perror("PVFS_mgmt_setparam_all", ret);
	fprintf(stderr, "Failure: not all servers accepted fsid %ld\n", 
	    (long)cur_fs);
        if (ret == -PVFS_EDETAIL)
            print_error_details(error_details);
        err = 1;
    }
    else
    {
        printf("\n   Ok; all servers understand fs_id %ld\n", (long) cur_fs);
    }

    printf("\n(7) Verifying that root handle is owned by one server...\n");    

    ret = PVFS_sys_lookup(cur_fs, "/", &creds,
                          &resp_lookup, PVFS2_LOOKUP_LINK_NO_FOLLOW, NULL);
    if(ret != 0)
    {
	PVFS_perror("PVFS_sys_lookup", ret);
	fprintf(stderr, "Failure: could not lookup root handle.\n");
	return(-1);
    }
    printf("\n   Root handle: %llu\n", llu(resp_lookup.ref.handle));

    param_value.type = PVFS_MGMT_PARAM_TYPE_UINT64;
    param_value.u.value = (uint64_t)resp_lookup.ref.handle;

    /* check that only one server controls root handle */
    /* TODO: we need a way to get information out about which server
     * failed in error cases here
     */
    ret = PVFS_mgmt_setparam_all(
        cur_fs, &creds, PVFS_SERV_PARAM_ROOT_CHECK,
        &param_value, error_details, NULL);

    if(ret < 0)
    {
	PVFS_perror("Failure: check root handle failed\n"
                    "PVFS_mgmt_setparam_all", ret);
        if(ret == -PVFS_EDETAIL) 
        {
            print_root_check_error_details(error_details);
        }

        err = 1;
    }

    PVFS_error_details_free(error_details);

    if (!err)
    {
        /* if we hit this point, then everything is ok */
        printf("     Ok; root handle is owned by exactly one server.\n");
        printf("\n");
    }

    PVFS_sys_finalize();

    printf("=============================================================\n");
    if (err)
    {
        printf("\nThe PVFS2 filesystem at %s appears to have "
               "problems.\n\n", user_opts->fs_path_real);
    }
    else
    {
        printf("\nThe PVFS2 filesystem at %s appears to be "
               "correctly configured.\n\n", user_opts->fs_path_real);
    }

    return(ret);
}


/* noop_all_servers()
 *
 * sends a noop to all servers listed in the config file 
 *
 * returns -PVFS_error on failure, 0 on success
 */
static int noop_all_servers(PVFS_fs_id fsid)
{
    PVFS_credentials creds;
    int ret = -1;
    int count;
    PVFS_BMI_addr_t* addr_array;
    int i;
    int tmp;
 
    PVFS_util_gen_credentials(&creds);

    printf("\n   meta servers:\n");
    ret = PVFS_mgmt_count_servers(
        fsid, &creds, PVFS_MGMT_META_SERVER, &count);
    if (ret < 0)
    {
	PVFS_perror("PVFS_mgmt_count_servers()", ret);
	return ret;
    }
    addr_array = (PVFS_BMI_addr_t *) malloc(
        count * sizeof(PVFS_BMI_addr_t));
    if (addr_array == NULL)
    {
	perror("malloc");
	return -PVFS_ENOMEM;
    }

    ret = PVFS_mgmt_get_server_array(
        fsid, &creds, PVFS_MGMT_META_SERVER, addr_array, &count);
    if (ret < 0)
    {
	PVFS_perror("PVFS_mgmt_get_server_array()", ret);
	return ret;
    }

    for (i = 0; i < count; i++)
    {
	printf("   %s ",
               PVFS_mgmt_map_addr(fsid, &creds, addr_array[i], &tmp));
	ret = PVFS_mgmt_noop(fsid, &creds, addr_array[i], NULL);
	if (ret == 0)
	{
	    printf("Ok\n");
	}
	else
	{
	    printf("FAILURE: PVFS_mgmt_noop failed for server: %s\n",
                   PVFS_mgmt_map_addr(fsid, &creds, addr_array[i], &tmp));
	    return ret;
	}
    }
    free(addr_array);

    printf("\n   data servers:\n");
    ret = PVFS_mgmt_count_servers(
        fsid, &creds, PVFS_MGMT_IO_SERVER, &count);
    if (ret < 0)
    {
	PVFS_perror("PVFS_mgmt_count_servers()", ret);
	return ret;
    }
    addr_array = (PVFS_BMI_addr_t *)malloc(
        count * sizeof(PVFS_BMI_addr_t));
    if (addr_array == NULL)
    {
	perror("malloc");
	return -PVFS_ENOMEM;
    }

    ret = PVFS_mgmt_get_server_array(
        fsid, &creds, PVFS_MGMT_IO_SERVER, addr_array, &count);
    if (ret < 0)
    {
	PVFS_perror("PVFS_mgmt_get_server_array()", ret);
	return ret;
    }

    for (i = 0; i < count; i++)
    {
	printf("   %s ",
               PVFS_mgmt_map_addr(fsid, &creds, addr_array[i], &tmp));
	ret = PVFS_mgmt_noop(fsid, &creds, addr_array[i], NULL);
	if (ret == 0)
	{
	    printf("Ok\n");
	}
	else
	{
	    printf("Failure!\n");
	    return ret;
	}
    }
    free(addr_array);

    return(0);
}

/* print_config()
 *
 * prints out config file information
 *
 * returns -PVFS_error on failure, 0 on success
 */
static int print_config(PVFS_fs_id fsid)
{
    PVFS_credentials creds;
    int i;
    int ret = -1;
    int tmp;
    int count;
    PVFS_BMI_addr_t *addr_array;
 
    PVFS_util_gen_credentials(&creds);

    printf("\n   meta servers:\n");
    ret = PVFS_mgmt_count_servers(
        fsid, &creds, PVFS_MGMT_META_SERVER, &count);
    if (ret < 0)
    {
	PVFS_perror("PVFS_mgmt_count_servers()", ret);
	return ret;
    }
    addr_array = (PVFS_BMI_addr_t *)malloc(
        count * sizeof(PVFS_BMI_addr_t));
    if (addr_array == NULL)
    {
	perror("malloc");
	return -PVFS_ENOMEM;
    }

    ret = PVFS_mgmt_get_server_array(
        fsid, &creds, PVFS_MGMT_META_SERVER, addr_array, &count);
    if (ret < 0)
    {
	PVFS_perror("PVFS_mgmt_get_server_array()", ret);
	return ret;
    }

    for (i=0; i<count; i++)
    {
	printf("   %s\n",
               PVFS_mgmt_map_addr(fsid, &creds, addr_array[i], &tmp));
    }
    free(addr_array);

    printf("\n   data servers:\n");
    ret = PVFS_mgmt_count_servers(
        fsid, &creds, PVFS_MGMT_IO_SERVER, &count);
    if (ret < 0)
    {
	PVFS_perror("PVFS_mgmt_count_servers()", ret);
	return ret;
    }
    addr_array = (PVFS_BMI_addr_t *)malloc(
        count * sizeof(PVFS_BMI_addr_t));
    if (addr_array == NULL)
    {
	perror("malloc");
	return -PVFS_ENOMEM;
    }

    ret = PVFS_mgmt_get_server_array(
        fsid, &creds, PVFS_MGMT_IO_SERVER, addr_array, &count);
    if (ret < 0)
    {
	PVFS_perror("PVFS_mgmt_get_server_array()", ret);
	return ret;
    }

    for(i=0; i<count; i++)
    {
	printf("   %s\n",
               PVFS_mgmt_map_addr(fsid, &creds, addr_array[i], &tmp));
    }
    free(addr_array);

    return 0;
}

/* print_mntent()
 *
 * prints out pvfstab information 
 *
 * no return value
 */
static void print_mntent(struct PVFS_sys_mntent *entries, int num_entries)
{
    int i, j;

    for (i = 0; i < num_entries; i++)
    {
        printf("\n   PVFS2 servers:");
        for (j=0; j<entries[i].num_pvfs_config_servers; j++) {
            printf(" %s", entries[i].pvfs_config_servers[j]);
            if (entries[i].num_pvfs_config_servers > 1) {
                if (entries[i].the_pvfs_config_server
                  == entries[i].pvfs_config_servers[j])
                    printf(" (active)");
                if (j < entries[i].num_pvfs_config_servers-1)
                    printf(",");
            }
        }
        printf("\n");
        printf("   Storage name: %s\n", entries[i].pvfs_fs_name);
        printf("   Local mount point: %s\n", entries[i].mnt_dir);
    }
    return;
}

/* parse_args()
 *
 * parses command line arguments
 *
 * returns pointer to options structure on success, NULL on failure
 */
static struct options* parse_args(int argc, char* argv[])
{
    char flags[] = "vm:";
    int one_opt = 0;
    int len;

    struct options* tmp_opts = NULL;
    int ret = -1;

    if (argc == 1)
    {
        usage(argc, argv);
        exit(EXIT_FAILURE);
    }

    /* create storage for the command line options */
    tmp_opts = (struct options *) malloc(sizeof(struct options));
    if (tmp_opts == NULL)
    {
	return(NULL);
    }
    memset(tmp_opts, 0, sizeof(struct options));

    /* look at command line arguments */
    while ((one_opt = getopt(argc, argv, flags)) != EOF)
    {
	switch (one_opt)
        {
            case('v'):
                printf("%s\n", PVFS2_VERSION);
                exit(0);
	    case('m'):
		/* taken from pvfs2-statfs.c */
		len = strlen(optarg)+1;
		tmp_opts->mnt_point = (char*)malloc(len+1);
		if (!tmp_opts->mnt_point)
		{
		    free(tmp_opts);
		    return NULL;
		}
		memset(tmp_opts->mnt_point, 0, len+1);
		ret = sscanf(optarg, "%s", tmp_opts->mnt_point);
		if (ret < 1){
		    free(tmp_opts);
		    return NULL;
		}
		/* TODO: dirty hack... fix later.  The remove_dir_prefix()
		 * function expects some trailing segments or at least
		 * a slash off of the mount point
		 */
		strcat(tmp_opts->mnt_point, "/");
		break;
	    case('?'):
		usage(argc, argv);
		exit(EXIT_FAILURE);
	}
    }

    if (optind != (argc ))
    {
	usage(argc, argv);
	exit(EXIT_FAILURE);
    }

    /* get the path of the file system, this one has a trailing slash
     * tacked on, see comment below for why 
     */
    tmp_opts->fs_path_hack = (char *) malloc(strlen(argv[argc-1]) + 2);
    if (tmp_opts->fs_path_hack == NULL)
    {
	free(tmp_opts);
	return NULL;
    }
    ret = sscanf(argv[argc-1], "%s", tmp_opts->fs_path_hack);
    if (ret < 1)
    {
	free(tmp_opts->fs_path_hack);
	free(tmp_opts);
	return NULL;
    }
    /* TODO: this is a hack... fix later.  The remove_dir_prefix()
     * function expects some trailing segments or at least a slash
     * off of the mount point
     */
    strcat(tmp_opts->fs_path_hack, "/");
    
    /* also preserve the real path, to use in print statements elsewhre */
    tmp_opts->fs_path_real = (char *) malloc(strlen(argv[argc-1]) + 2);
    if (tmp_opts->fs_path_real == NULL)
    {
	free(tmp_opts->fs_path_hack);
	free(tmp_opts);
	return NULL;
    }
    ret = sscanf(argv[argc-1], "%s", tmp_opts->fs_path_real);
    if (ret < 1)
    {
	free(tmp_opts->fs_path_hack);
	free(tmp_opts->fs_path_real);
	free(tmp_opts);
	return NULL;
    }
 
    return(tmp_opts);
}

static void print_root_check_error_details(PVFS_error_details * error_details)
{
    int i = 0;
    int owners = 0;
    int fatal_errors = 0;
    /* find # of servers that report ownership of root */
    for(i = 0; i < error_details->count_used; ++i)
    {
        if(error_details->error[i].error == 0)
        {
            ++owners;
        }
        else if(error_details->error[i].error != -PVFS_ENOENT)
        {
            ++fatal_errors;
        }
    }

    /* now figure out what error to print */
    if(owners == 0)
    {
        fprintf(stderr, "\n\tExactly one server must own the root handle.\n"
                        "\tIn this setup, no servers own the root handle.\n");
    }
    else if(owners > 1)
    {
        fprintf(stderr, "\n\tExactly one server must own the root handle.\n"
                        "\tIn this setup, multiple servers report "
                        "ownership of root handle.\n");
    }

    fprintf(stderr, "\nPer-server errors:\n");
    for(i = 0; i < error_details->count_used; ++i)
    {
        char perrorstr[100];
        if(error_details->error[i].error == 0)
        {
            fprintf(stderr, "\tServer: %s: Reports ownership of root handle\n",
                    BMI_addr_rev_lookup(error_details->error[i].addr));
        }
        else if(error_details->error[i].error != -PVFS_ENOENT)
        {
            PVFS_strerror_r(error_details->error[i].error, perrorstr, 100);
        }

    }
}

static void print_error_details(PVFS_error_details * error_details)
{
    int i = 0;
    fprintf(stderr, "\tPer-server errors:\n");
    for(i = 0; i < error_details->count_used; ++i)
    {
        char perrorstr[100];
        if(error_details->error[i].error != 0)
        {
            PVFS_strerror_r(error_details->error[i].error, perrorstr, 100);
            fprintf(stderr, "\tServer: %s: %s\n",
                    BMI_addr_rev_lookup(error_details->error[i].addr),
                    perrorstr);
        }
    }
}

static void usage(int argc, char** argv)
{
    fprintf(stderr, "%s version %s\n\n", argv[0], PVFS2_VERSION);
    fprintf(stderr, "Usage  : %s -m file_system_path\n", argv[0]);
    fprintf(stderr, "Example: %s -m /mnt/pvfs2\n", argv[0]);
    return;
}

/*
 * Local variables:
 *  c-indent-level: 4
 *  c-basic-offset: 4
 * End:
 *
 * vim: ts=8 sts=4 sw=4 expandtab
 */
