/*
 * (C) 2001 Clemson University and The University of Chicago
 *
 * Changes by Acxiom Corporation to add protocol version to kernel
 * communication, Copyright � Acxiom Corporation, 2005.
 *
 * See COPYING in top-level directory.
 */

#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <inttypes.h>
#include <sys/ioctl.h>
#include <sys/poll.h>
#include <sys/uio.h>
#include <assert.h>
#include <sys/mman.h>

#include "pint-mem.h"
#include "pvfs2-types.h"
#include "pvfs2-debug.h"
#include "gossip.h"
#include "pint-dev.h"
#include "pvfs2-dev-proto.h"
#include "pvfs2-internal.h"

#ifdef WITH_LINUX_KMOD
static int setup_dev_entry(
    const char *dev_name);

static int parse_devices(
    const char *targetfile,
    const char *devname, 
    int *majornum);
#endif  /* WITH_LINUX_KMOD */

static int pdev_fd = -1;
static int32_t pdev_magic;
#ifdef WITH_LINUX_KMOD
static int32_t pdev_max_upsize;
#endif  /* WITH_LINUX_KMOD */
static int32_t pdev_max_downsize;

int32_t pvfs2_bufmap_total_size, pvfs2_bufmap_desc_size;
int32_t pvfs2_bufmap_desc_count, pvfs2_bufmap_desc_shift;

/* PINT_dev_initialize()
 *
 * initializes the device management interface
 *
 * returns 0 on success, -PVFS_error on failure
 */
int PINT_dev_initialize(
    const char *dev_name,
    int flags)
{
#ifdef WITH_LINUX_KMOD
    int ret = -1;
    char *debug_string = getenv("PVFS2_KMODMASK");
    int32_t debug_mask = 0;

    /* we have to be root to access the device */
    if ((getuid() != 0) && (geteuid() != 0))
    {
        gossip_err("Error: must be root to open pvfs2 device.\n");
        return (-(PVFS_EPERM|PVFS_ERROR_DEV));
    }

    /* setup /dev/ entry if needed */
    ret = setup_dev_entry(dev_name);
    if (ret < 0)
    {
        return (-(PVFS_ENODEV|PVFS_ERROR_DEV));
    }

    /* try to open the device */
    pdev_fd = open(dev_name, (O_RDWR | O_NONBLOCK));
    if (pdev_fd < 0)
    {
        switch(errno)
        {
            case EACCES:
                return(-(PVFS_EPERM|PVFS_ERROR_DEV));
            case ENOENT:
                return(-(PVFS_ENOENT|PVFS_ERROR_DEV));
            default:
                return(-(PVFS_ENODEV|PVFS_ERROR_DEV));
        }
    }

    /* run some ioctls to find out device parameters */
    ret = ioctl(pdev_fd, PVFS_DEV_GET_MAGIC, &pdev_magic);
    if (ret < 0)
    {
        gossip_err("Error: ioctl() PVFS_DEV_GET_MAGIC failure.\n");
        close(pdev_fd);
        return(-(PVFS_ENODEV|PVFS_ERROR_DEV));
    }

    ret = ioctl(pdev_fd, PVFS_DEV_GET_MAX_UPSIZE, &pdev_max_upsize);
    if (ret < 0)
    {
        gossip_err("Error: ioctl() PVFS_DEV_GET_MAX_UPSIZE failure.\n");
        close(pdev_fd);
        return(-(PVFS_ENODEV|PVFS_ERROR_DEV));
    }

    ret = ioctl(pdev_fd, PVFS_DEV_GET_MAX_DOWNSIZE, &pdev_max_downsize);
    if (ret < 0)
    {
        gossip_err("Error: ioctl() PVFS_DEV_GET_MAX_DOWNSIZE failure.\n");
        close(pdev_fd);
        return(-(PVFS_ENODEV|PVFS_ERROR_DEV));
    }

    /* set the debug mask through an ioctl */
    if (!debug_string)
        return 0;

    /* truncate it to a 32 bit mask since we dont have a need for more than that anyways */
    debug_mask = (int32_t) PVFS_kmod_eventlog_to_mask(debug_string);
    ret = ioctl(pdev_fd, PVFS_DEV_DEBUG, &debug_mask);
    if (ret < 0)
    {
        gossip_err("Error: ioctl() PVFS_DEV_DEBUG failure (debug mask to %x)\n", debug_mask);
        close(pdev_fd);
        return -(PVFS_ENODEV|PVFS_ERROR_DEV);
    }
#endif  /* WITH_LINUX_KMOD */
    return 0;
}

/* PINT_dev_finalize()
 *
 * shuts down the device management interface
 *
 * no return value
 */
void PINT_dev_finalize(void)
{
    if (pdev_fd > -1)
    {
        close(pdev_fd);
        pdev_fd = -1;
    }
}

/* PINT_dev_get_mapped_regions()
 *
 * creates a set of memory buffers that are shared between user space and 
 * kernel space
 *
 * returns 0 on success, -PVFS_error on failure
 */
int PINT_dev_get_mapped_regions(int ndesc, struct PVFS_dev_map_desc *desc,
                                struct PINT_dev_params *params)
{
#ifdef WITH_LINUX_KMOD
    int i, ret = -1;
    uint64_t page_size = sysconf(_SC_PAGE_SIZE), total_size;
    void *ptr = NULL;
    int ioctl_cmd[2] = {PVFS_DEV_MAP, 0};
    int debug_on = 0;
    uint64_t debug_mask = 0;

    for (i = 0; i < ndesc; i++)
    {
        total_size = params[i].dev_buffer_size * params[i].dev_buffer_count;
        if (total_size < 0) 
        {
            gossip_err("Error:please provide sane values for device parameters.\n");
            break;
        }
        if (total_size % page_size != 0) 
        {
            gossip_err("Error: total device buffer size must be a multiple of system page size.\n");
            break;
        }
        if (total_size >= PVFS2_BUFMAP_MAX_TOTAL_SIZE)
        {
            gossip_err(
                "Error: total size (%llu) of device "
                "buffer must be < %llu MB.\n",
                llu(total_size), llu(PVFS2_BUFMAP_MAX_TOTAL_SIZE));
            break;
        }
        if (params[i].dev_buffer_size & (params[i].dev_buffer_size - 1))
        {
            gossip_err("Error: descriptor size must be a power of 2 (%llu)\n",
                        llu(params[i].dev_buffer_size));
            break;
        }
        /* we would like to use a memaligned region that is a multiple
         * of the system page size
         */
        ptr = PINT_mem_aligned_alloc(total_size, page_size);
        if (!ptr)
        {
            desc[i].ptr = NULL;
            break;
        }

#ifdef REDHAT_RELEASE_9
        /* fixes a corruption issue on linux 2.4 kernels where the buffers are
         * not being pinned in memory properly 
         */
        if(mlock( (const char *) ptr, total_size) != 0)
        { 
           gossip_err("Error: FAILED to mlock shared buffer\n");
           break;
        }
#endif
        desc[i].ptr  = ptr;
        desc[i].total_size = total_size;
        desc[i].size = params[i].dev_buffer_size;
        desc[i].count = params[i].dev_buffer_count;

        gossip_get_debug_mask(&debug_on, &debug_mask);
        gossip_set_debug_mask(1, GOSSIP_USER_DEV_DEBUG);
        gossip_debug(GOSSIP_USER_DEV_DEBUG,
            "[INFO]: Mapping pointer %p for I/O.\n", ptr);
        gossip_set_debug_mask(debug_on, debug_mask);

        /* ioctl to ask driver to map pages if needed */
        if (ioctl_cmd[i] != 0)
        {
            ret = ioctl(pdev_fd, ioctl_cmd[i], &desc[i]);
            if (ret < 0)
            {
                break;
            }
            pvfs2_bufmap_desc_count = params[i].dev_buffer_count;
            pvfs2_bufmap_desc_size  = params[i].dev_buffer_size;
            pvfs2_bufmap_total_size = total_size;
            pvfs2_bufmap_desc_shift = LOG2(pvfs2_bufmap_desc_size);
        }
    }
    if (i != ndesc) {
        int j;
        for (j = 0; j < i; j++) {
            if (desc[j].ptr) {
                PINT_mem_aligned_free(desc[j].ptr);
                desc[j].ptr = NULL;
            }
        }
        return -(PVFS_ENOMEM|PVFS_ERROR_DEV);
    }
#endif  /* WITH_LINUX_KMOD */
    return 0;
}

/* PINT_dev_put_mapped_regions()
 *
 * frees the set of memory buffers that were shared between user space and
 * kernel space.  MUST be called only after device is closed
 * (i.e. PINT_dev_finalize)
 */
void PINT_dev_put_mapped_regions(int ndesc, struct PVFS_dev_map_desc *desc)
{
    void *ptr;
    int i;
    
    assert(desc);

    for (i = 0; i < ndesc; i++)
    {
        ptr = (void *) desc[i].ptr;
        assert(ptr);

#ifdef REDHAT_RELEASE_9
        /* fixes a corruption issue on linux 2.4 kernels where the buffers are
         * not being pinned in memory properly
         */
        if(munlock( (const char *) ptr, desc[i].total_size) != 0)
        { 
           gossip_err("Error: FAILED to munlock shared buffer\n");
        }
#endif

        PINT_mem_aligned_free(ptr);
    }
}

/* PINT_dev_get_mapped_buffer()
 *
 * returns a memory buffer of size (pvfs2_bufmap_desc_size)
 * matching the specified buffer_index given a PVFS_dev_map_desc
 *
 * returns a valid desc addr on success, NULL on failure
 */
void *PINT_dev_get_mapped_buffer(
    enum pvfs_bufmap_type bm_type,
    struct PVFS_dev_map_desc *desc,
    int buffer_index)
{
    char *ptr;
    int desc_count, desc_size;

    if (bm_type != BM_IO && bm_type != BM_READDIR)
        return NULL;

    desc_count = (bm_type == BM_IO) ? 
                pvfs2_bufmap_desc_count :
                PVFS2_READDIR_DEFAULT_DESC_COUNT;
    desc_size  = (bm_type == BM_IO) ? 
                pvfs2_bufmap_desc_size : 
                PVFS2_READDIR_DEFAULT_DESC_SIZE;
    ptr =  (char *) desc[bm_type].ptr;
    return ((desc && ptr &&
             ((buffer_index > -1) &&
              (buffer_index < desc_count))) ?
            (ptr + (buffer_index * desc_size)) :
            NULL);
}

/* PINT_dev_test_unexpected()
 *
 * tests for the presence of unexpected messages
 *
 * returns number of completed unexpected messages on success,
 * -PVFS_error on failure
 */
int PINT_dev_test_unexpected(
        int incount,
        int *outcount,
        struct PINT_dev_unexp_info *info_array,
        int max_idle_time)
{
    int ret = -1;
#ifdef WITH_LINUX_KMOD
    int avail = -1, i = 0;
    struct pollfd pfd;
    int32_t *magic = NULL;
    int32_t *proto_ver = NULL;
    uint64_t *tag = NULL;
    void *buffer = NULL;
    pvfs2_upcall_t *upc = NULL;

    if(incount < 1)
    {
        return(-PVFS_EINVAL);
    }

    /* prepare to read max upcall size (magic nr and tag included) */
    int read_size = pdev_max_upsize;
    
    *outcount = 0;

    pfd.fd = pdev_fd;
    pfd.events = POLLIN;

    gossip_debug(GOSSIP_USER_DEV_DEBUG, 
                 "[DEV]: Entered %s: incount: %d, timeout: %d\n",
                 __func__, incount, max_idle_time);

    do
    {
        /*
          poll to see if there is anything available on the device if
          we were given a max_idle_time.  if the max_idle_time is 0,
          skip the poll call and immediately try to read the device
        */
        if (max_idle_time)
        {
            do
            {
                pfd.revents = 0;
                avail = poll(&pfd, 1, max_idle_time);

            } while((avail < 0) && (errno == EINTR));

            if (avail < 0)
            {
                switch(errno)
                {
                    case EBADF:
                        ret = -(PVFS_EBADF|PVFS_ERROR_DEV);
                    case ENOMEM:
                        ret = -(PVFS_ENOMEM|PVFS_ERROR_DEV);
                    case EFAULT:
                        ret = -(PVFS_EFAULT|PVFS_ERROR_DEV);
                    default:
                        ret = -(PVFS_EIO|PVFS_ERROR_DEV);
                }
                goto dev_test_unexp_error;
            }

            /* device is emptied */
            if (avail == 0)
            {
                gossip_debug(GOSSIP_USER_DEV_DEBUG,
                             "[DEV]: Exiting %s: incount: %d, device empty!\n",
                             __func__, incount);
                return ((*outcount > 0) ? 1 : 0);
            }

            if (!(pfd.revents & POLLIN))
            {
                if (pfd.revents & POLLNVAL)
                {
                    return -(PVFS_EBADF|PVFS_ERROR_DEV);
                }
                continue;
            }

            /*
              once we have data to read, set the idle time to zero
              because we don't want to block on subsequent iterations
            */
            max_idle_time = 0;
        }

        /* prepare to read max upcall size, plus magic nr and tag */
        buffer = malloc(read_size);
        if (buffer == NULL)
        {
            ret = -(PVFS_ENOMEM|PVFS_ERROR_DEV);
            goto dev_test_unexp_error;
        }

        ret = read(pdev_fd, buffer, read_size);
        if (ret < 0)
        {
            /*
              EAGAIN is an error we can ignore in non-blocking mode;
              it just means that the device is emptied
            */
            if (errno == EAGAIN)
            {
                goto safe_exit;
            }
            ret = -(PVFS_EIO|PVFS_ERROR_DEV);
            goto dev_test_unexp_error;
        }

        if (ret == 0)
        {   
            /* assume we are done and return */
          safe_exit:
            free(buffer);
            gossip_debug(GOSSIP_USER_DEV_DEBUG,
                         "[DEV]: %s Exit: "
                         "incount: %d, outcount: %d, bytes available: %d\n",
                         __func__, incount, *outcount, avail);

            return ((*outcount > 0) ? 1 : 0);
        }

        /* make sure a payload is present */
        if (ret < (sizeof(int32_t) + sizeof(uint64_t) + 1))
        {
            gossip_err("Error: short message from device "
                       "(got %d bytes).\n", ret);

            ret = -(PVFS_EIO|PVFS_ERROR_DEV);
            goto dev_test_unexp_error;
        }

        proto_ver = (int32_t*)buffer;
        magic = (int32_t*)((unsigned long)buffer + sizeof(int32_t));
        tag = (uint64_t*)((unsigned long)buffer + 2*sizeof(int32_t));

        if(*magic != pdev_magic)
        {
            gossip_err("Error: magic numbers do not match.\n");
            ret = -(PVFS_EPROTO|PVFS_ERROR_DEV);
            goto dev_test_unexp_error;
        }
        if(*proto_ver != PVFS_KERNEL_PROTO_VERSION)
        {
            gossip_err("Error: protocol versions do not match.\n");
            gossip_err("Please check that your pvfs2 module "
                       "and pvfs2-client versions are consistent.\n");
            ret = -(PVFS_EPROTO|PVFS_ERROR_DEV);
            goto dev_test_unexp_error;
        }

        info_array[*outcount].size =
            (ret - 2*sizeof(int32_t) - sizeof(uint64_t));

        /* shift buffer up so caller doesn't see header info */
        info_array[*outcount].buffer = (void*)
            ((unsigned long)buffer + 2*sizeof(int32_t) + sizeof(uint64_t));
        info_array[*outcount].tag = *tag;

        upc = (pvfs2_upcall_t *) info_array[*outcount].buffer;
        /* if there is a trailer, allocate a buffer and issue another read */
        if (upc->trailer_size > 0)
        {
            upc->trailer_buf = malloc(upc->trailer_size);
            if (upc->trailer_buf == NULL)
            {
                ret = -(PVFS_ENOMEM|PVFS_ERROR_DEV);
                goto dev_test_unexp_error;
            }
            ret = read(pdev_fd, upc->trailer_buf, upc->trailer_size);
            if (ret < 0)
            {
                ret = -(PVFS_EIO|PVFS_ERROR_DEV);
                goto dev_test_unexp_error;
            }
        }

        (*outcount)++;

        /*
          keep going until we fill up the outcount or the device
          empties
        */

    } while((*outcount < incount) && avail);

    gossip_debug(GOSSIP_USER_DEV_DEBUG,
                 "[DEV]: %s Exit: "
                 "incount: %d, outcount: %d, bytes available: %d\n",
                 __func__, incount, *outcount, avail);

    return ((*outcount > 0) ? 1 : 0);

dev_test_unexp_error:

    /* release resources we created up to this point */
    for(i = 0; i < *outcount; i++)
    {
        upc = (pvfs2_upcall_t *) info_array[i].buffer;
        if (upc->trailer_buf)
        {
            free(upc->trailer_buf);
        }
        if (buffer)
        {
            free(buffer);
        }
    }

    *outcount = 0;
#endif  /* WITH_LINUX_KMOD */
    return ret;
}

/* PINT_dev_release_unexpected()
 *
 * releases the resources associated with an unexpected device message
 *
 * returns 0 on success, -PVFS_error on failure
 */
int PINT_dev_release_unexpected(
        struct PINT_dev_unexp_info *info)
{
    int ret = -PVFS_EINVAL;
    void *buffer = NULL;

    if (info && info->buffer)
    {
        /* index backwards header size off of the buffer before freeing */
        buffer = (void*)((unsigned long)info->buffer - 2*sizeof(int32_t) - 
                         sizeof(uint64_t));
        free(buffer);

        ret = 0;
    }

    memset(info, 0, sizeof(struct PINT_dev_unexp_info));
    return ret;
}

/* PINT_dev_write_list()
 *
 * writes a set of buffers into the device
 *
 * returns 0 on success, -PVFS_error on failure
 */
int PINT_dev_write_list(
    void **buffer_list,
    int *size_list,
    int list_count,
    int total_size,
    enum PINT_dev_buffer_type buffer_type,
    PVFS_id_gen_t tag)
{
    struct iovec io_array[8];
    int io_count = 3;
    int i;
    int ret = -1;
    int32_t proto_ver = PVFS_KERNEL_PROTO_VERSION;
    
    /* lets be reasonable about list size :) */
    /* two vecs are taken up by magic nr and tag */
    if (list_count > 7)
    {
        return (-(PVFS_EINVAL|PVFS_ERROR_DEV));
    }

    /* even though we are ignoring the buffer_type for now, 
     * make sure that the caller set it to a sane value 
     */
    if (buffer_type != PINT_DEV_EXT_ALLOC &&
        buffer_type != PINT_DEV_PRE_ALLOC)
    {
        return (-(PVFS_EINVAL|PVFS_ERROR_DEV));
    }

    if (size_list[0] > pdev_max_downsize)
    {
        return(-(PVFS_EMSGSIZE|PVFS_ERROR_DEV));
    }

    io_array[0].iov_base = &proto_ver;
    io_array[0].iov_len = sizeof(int32_t);
    io_array[1].iov_base = &pdev_magic;
    io_array[1].iov_len = sizeof(int32_t);
    io_array[2].iov_base = &tag;
    io_array[2].iov_len = sizeof(uint64_t);

    for (i=0; i<list_count; i++)
    {
        io_array[i+3].iov_base = buffer_list[i];
        io_array[i+3].iov_len = size_list[i];
        io_count++;
    }

    ret = writev(pdev_fd, io_array, io_count);

    return ((ret < 0) ? -(PVFS_EIO|PVFS_ERROR_DEV) : 0);
}

/* PINT_dev_remount()
 *
 * asks the kernel to re-issues upcall mount operations to refill the
 * dynamic mount information to the pvfs2-client-core
 *
 * returns 0 on success, -PVFS_error on failure
 */
int PINT_dev_remount(void)
{
    int ret = -PVFS_EINVAL;

#ifdef WITH_LINUX_KMOD
    if (pdev_fd > -1)
    {
        ret = ((ioctl(pdev_fd, PVFS_DEV_REMOUNT_ALL, NULL) < 0) ?
               -PVFS_ERROR_DEV : 0);
        if (ret)
        {
            gossip_err("Error: ioctl PVFS_DEV_REMOUNT_ALL failure\n");
        }
    }
#endif  /* WITH_LINUX_KMOD */
    return ret;
}

/* PINT_dev_write()
 *
 * writes a buffer into the device
 *
 * returns 0 on success, -PVFS_error on failure
 */
int PINT_dev_write(void *buffer,
                   int size,
                   enum PINT_dev_buffer_type buffer_type,
                   PVFS_id_gen_t tag)
{
    return PINT_dev_write_list(
        &buffer, &size, 1, size, buffer_type, tag);
}

/* PINT_dev_memalloc()
 *
 * allocates a memory buffer optimized for transfer into the device
 *
 * returns pointer to buffer on success, NULL on failure
 */
void *PINT_dev_memalloc(int size)
{
    /* no optimizations yet */
    return malloc(size);
}

/* PINT_dev_memfree()
 *
 * frees a memory buffer that was allocated with PINT_dev_memalloc()
 *
 * no return value
 */
void PINT_dev_memfree(void *buffer, int size)
{
    free(buffer);
}

#ifdef WITH_LINUX_KMOD
/* setup_dev_entry()
 *
 * sets up the device file
 *
 * returns 0 on success, -1 on failure
 */
static int setup_dev_entry(const char *dev_name)
{
    int majornum = -1;
    int ret = -1;
    struct stat dev_stat;

    ret = parse_devices("/proc/devices", "pvfs2-req", &majornum);
    if (ret < 0)
    {
        gossip_err("Error: unable to parse device file.\n");
        return -1;
    }

    if (majornum == -1)
    {
        gossip_err("Error: could not setup device %s.\n", dev_name);
        gossip_err("Error: did you remember to load the kernel module?\n");
        return -1;
    }

    if (!access(dev_name, F_OK))
    {
        /* device file already exists */
        ret = stat(dev_name, &dev_stat);
        if (ret != 0)
        {
            gossip_err("Error: could not stat %s.\n", dev_name);
            return -1;
        }

        if (S_ISCHR(dev_stat.st_mode) &&
            (major(dev_stat.st_rdev) == majornum))
        {
            /*
              the device file already has the correct major number;
              we're done
            */
            return 0;
        }
        else
        {
            /* the device file is incorrect; unlink it */
            ret = unlink(dev_name);
            if (ret != 0)
            {
                gossip_err("Error: could not unlink old %s\n", dev_name);
                return -1;
            }
        }
    }

    /* if we hit this point, then we need to create a new device file */
    ret = mknod(dev_name, (S_IFCHR | S_IRUSR | S_IWUSR),
                makedev(majornum, 0));
    if (ret != 0)
    {
        gossip_err("Error: could not create new %s device entry.\n",
                   dev_name);
    }
    return ret;
}

/* parse_devices()
 *
 * parses a file in the /proc/devices format looking for an entry for
 * the given "devname".  If found, "majornum" is filled in with the
 * major number of the device.  Else "majornum" is set to -1.
 *
 * returns 0 on successs, -1 on failure
 */
static int parse_devices(
    const char *targetfile,
    const char *devname, 
    int *majornum)
{
    char line_buf[256];
    char dev_buf[256];
    int major_buf = -1;
    FILE *devfile = NULL;
    int ret = -1;

    /* initialize for safety */
    *majornum = -1;

    /* open up the file to parse */
    devfile = fopen(targetfile, "r");
    if (!devfile)
    {
        gossip_err("Error: could not open %s.\n", targetfile);
        return -1;
    }

    /* scan every line until we get a match or end of file */
    while (fgets(line_buf, sizeof(line_buf), devfile))
    {
        /*
          sscanf is safe here as long as the target string is at least
          as large as the source
        */
        ret = sscanf(line_buf, " %d %s ", &major_buf, dev_buf);
        if (ret == 2)
        {
            /*
              this line is the correct format; see if it matches the
              devname
            */
            if (strncmp(devname, dev_buf, sizeof(dev_buf)) == 0)
            {
                *majornum = major_buf;
                
                /*
                  don't break out; it doesn't cost much to scan the
                  whole thing, and we want the last entry if
                  somehow(?)  there are two
                */
            }
        }
    }
    fclose(devfile);
    return 0;
}
#endif  /* WITH_LINUX_KMOD */

/*
 * Local variables:
 *  c-indent-level: 4
 *  c-basic-offset: 4
 * End:
 *
 * vim: ts=8 sts=4 sw=4 expandtab
 */
