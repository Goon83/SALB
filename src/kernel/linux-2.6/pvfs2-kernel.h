/*
 * (C) 2001 Clemson University and The University of Chicago
 *
 * See COPYING in top-level directory.
 */

/** \defgroup pvfs2linux PVFS2 Linux kernel support
 *
 *  The PVFS2 Linux kernel support allows PVFS2 volumes to be mounted and
 *  accessed through the Linux VFS (i.e. using standard I/O system calls).
 *  This support is only needed on clients that wish to mount the file system.
 *
 * @{
 */

/** \file
 *  Declarations and macros for the PVFS2 Linux kernel support.
 */

#ifndef __PVFS2KERNEL_H
#define __PVFS2KERNEL_H

#ifdef HAVE_NOWARNINGS_WHEN_INCLUDING_LINUX_CONFIG_H
#include <linux/config.h>
#endif

#ifdef PVFS2_LINUX_KERNEL_2_4

/* the 2.4 kernel requires us to manually set up modversions if needed */
#if CONFIG_MODVERSIONS==1
#define MODVERSIONS
#include <linux/modversions.h>
#endif 

#define __NO_VERSION__
#include <linux/version.h>
#include <linux/module.h>

#ifndef HAVE_SECTOR_T
typedef unsigned long sector_t;
#endif

#else /* !(PVFS2_LINUX_KERNEL_2_4) */

#include <linux/moduleparam.h>
#include <linux/vermagic.h>
#include <linux/statfs.h>
#include <linux/buffer_head.h>
#include <linux/backing-dev.h>
#include <linux/device.h>
#include <linux/mpage.h>
#include <linux/namei.h>
#include <linux/errno.h>

#endif /* PVFS2_LINUX_KERNEL_2_4 */

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/types.h>
#include <linux/fs.h>
#include <linux/vmalloc.h>

#include "pvfs2-config.h"
#include "pvfs2-debug.h"
#include "gossip.h"

#ifdef HAVE_AIO
#include <linux/aio.h>
#endif
#ifdef HAVE_POSIX_ACL_H
#include <linux/posix_acl.h>
#endif
#ifdef HAVE_POSIX_ACL_XATTR_H
#include <linux/posix_acl_xattr.h>
#endif
#ifdef HAVE_LINUX_COMPAT_H
#include <linux/compat.h>
#endif
#ifdef HAVE_LINUX_IOCTL32_H
#include <linux/ioctl32.h>
#endif
#ifdef HAVE_LINUX_SYSCALLS_H
#include <linux/syscalls.h>
#endif
#ifdef HAVE_LINUX_MOUNT_H
#include <linux/mount.h>
#endif
#include <asm/uaccess.h>
#include <asm/atomic.h>
#include <linux/uio.h>
#include <linux/sched.h>
#include <linux/mm.h>
#include <asm/atomic.h>
#include <linux/smp_lock.h>
#include <linux/wait.h>
#include <linux/dcache.h>
#include <linux/pagemap.h>
#include <linux/poll.h>
#include <linux/rwsem.h>
#include <asm/unaligned.h>
#ifdef HAVE_ASM_IOCTL32_H
#include <asm/ioctl32.h>
#endif

#ifdef HAVE_XATTR
#include <linux/xattr.h>
#ifdef HAVE_LINUX_XATTR_ACL_H
#include <linux/xattr_acl.h>
#endif
#endif

#ifdef HAVE_LINUX_EXPORTFS_H
#include <linux/exportfs.h>
#endif

#include "pint-dev-shared.h"
#include "pvfs2-dev-proto.h"
#include "pvfs2-types.h"

/*
  this attempts to disable the annotations used by the 'sparse' kernel
  source utility on systems that can't understand it by defining the
  used annotations away
*/
#ifndef __user
#define __user
#endif

#ifdef PVFS2_KERNEL_DEBUG
#define PVFS2_DEFAULT_OP_TIMEOUT_SECS       10
#else
#define PVFS2_DEFAULT_OP_TIMEOUT_SECS       20
#endif

#define PVFS2_DEFAULT_SLOT_TIMEOUT_SECS     1800 /* 30 minutes */

#define PVFS2_REQDEVICE_NAME          "pvfs2-req"

#define PVFS2_DEVREQ_MAGIC             0x20030529
#define PVFS2_LINK_MAX                 0x000000FF
#define PVFS2_PURGE_RETRY_COUNT        0x00000005
#define PVFS2_SEEK_END                 0x00000002
#define PVFS2_MAX_NUM_OPTIONS          0x00000004
#define PVFS2_MAX_MOUNT_OPT_LEN        0x00000080
#define PVFS2_MAX_FSKEY_LEN            64

#define MAX_DEV_REQ_UPSIZE (2*sizeof(int32_t) +   \
sizeof(uint64_t) + sizeof(pvfs2_upcall_t))
#define MAX_DEV_REQ_DOWNSIZE (2*sizeof(int32_t) + \
sizeof(uint64_t) + sizeof(pvfs2_downcall_t))

#define BITS_PER_LONG_DIV_8 (BITS_PER_LONG >> 3)

#define MAX_ALIGNED_DEV_REQ_UPSIZE                  \
(MAX_DEV_REQ_UPSIZE +                               \
((((MAX_DEV_REQ_UPSIZE / (BITS_PER_LONG_DIV_8)) *   \
   (BITS_PER_LONG_DIV_8)) +                         \
    (BITS_PER_LONG_DIV_8)) - MAX_DEV_REQ_UPSIZE))
#define MAX_ALIGNED_DEV_REQ_DOWNSIZE                \
(MAX_DEV_REQ_DOWNSIZE +                             \
((((MAX_DEV_REQ_DOWNSIZE / (BITS_PER_LONG_DIV_8)) * \
   (BITS_PER_LONG_DIV_8)) +                         \
    (BITS_PER_LONG_DIV_8)) - MAX_DEV_REQ_DOWNSIZE))

/* borrowed from irda.h */
#ifndef MSECS_TO_JIFFIES
#define MSECS_TO_JIFFIES(ms) (((ms)*HZ+999)/1000)
#endif

/************************************
 * valid pvfs2 kernel operation states
 *
 * unknown  - op was just initialized
 * waiting  - op is on request_list (upward bound)
 * inprogr  - op is in progress (waiting for downcall)
 * serviced - op has matching downcall; ok
 * purged   - op has to start a timer since client-core
              exited uncleanly before servicing op
 ************************************/
enum pvfs2_vfs_op_states {
    OP_VFS_STATE_UNKNOWN = 0,
    OP_VFS_STATE_WAITING = 1,
    OP_VFS_STATE_INPROGR = 2,
    OP_VFS_STATE_SERVICED = 4,
    OP_VFS_STATE_PURGED  = 8,
};

#define set_op_state_waiting(op)     ((op)->op_state = OP_VFS_STATE_WAITING)
#define set_op_state_inprogress(op)  ((op)->op_state = OP_VFS_STATE_INPROGR)
#define set_op_state_serviced(op)    ((op)->op_state = OP_VFS_STATE_SERVICED)
#define set_op_state_purged(op)      ((op)->op_state |= OP_VFS_STATE_PURGED)

#define op_state_waiting(op)     ((op)->op_state & OP_VFS_STATE_WAITING)
#define op_state_in_progress(op) ((op)->op_state & OP_VFS_STATE_INPROGR)
#define op_state_serviced(op)    ((op)->op_state & OP_VFS_STATE_SERVICED)
#define op_state_purged(op)      ((op)->op_state & OP_VFS_STATE_PURGED)

/* Defines for incrementing aio_ref_count */
#ifndef HAVE_AIO_VFS_SUPPORT
#define get_op(op)
#define put_op(op) op_release(op)
#define op_wait(op)
#else
#define get_op(op) \
    do {\
        atomic_inc(&(op)->aio_ref_count);\
        gossip_debug(GOSSIP_CACHE_DEBUG, "(get) Alloced OP (%p:%ld)\n", op, (unsigned long)(op)->tag);\
    } while(0)
#define put_op(op) \
    do {\
        if (atomic_sub_and_test(1, &(op)->aio_ref_count) == 1) {\
            gossip_debug(GOSSIP_CACHE_DEBUG, "(put) Releasing OP (%p:%ld)\n", op, (unsigned long)(op)->tag);\
            op_release(op);\
        }\
    } while(0)
#define op_wait(op) (atomic_read(&(op)->aio_ref_count) <= 2 ? 0 : 1)
#endif

/* Defines for controlling whether I/O upcalls are for async or sync operations */
enum PVFS_async_io_type 
{
     PVFS_VFS_SYNC_IO = 0,
    PVFS_VFS_ASYNC_IO = 1,
};

/************************************
 * pvfs2 kernel memory related flags
 ************************************/

#if ((defined PVFS2_KERNEL_DEBUG) && (defined CONFIG_DEBUG_SLAB))
#define PVFS2_CACHE_CREATE_FLAGS SLAB_RED_ZONE
#else
#define PVFS2_CACHE_CREATE_FLAGS 0
#endif /* ((defined PVFS2_KERNEL_DEBUG) && (defined CONFIG_DEBUG_SLAB)) */

#ifdef HAVE_SLAB_KERNEL_FLAG
# define PVFS2_CACHE_ALLOC_FLAGS (SLAB_KERNEL)
#else
# define PVFS2_CACHE_ALLOC_FLAGS (GFP_KERNEL)
#endif
#define PVFS2_GFP_FLAGS (GFP_KERNEL)
#define PVFS2_BUFMAP_GFP_FLAGS (GFP_KERNEL)

#ifdef CONFIG_HIGHMEM
#define pvfs2_kmap(page) kmap(page)
#define pvfs2_kunmap(page) kunmap(page)
#else
#define pvfs2_kmap(page) page_address(page)
#define pvfs2_kunmap(page) do {} while(0)
#endif /* CONFIG_HIGHMEM */

/* pvfs2 xattr and acl related defines */
#ifdef HAVE_XATTR
#define PVFS2_XATTR_INDEX_POSIX_ACL_ACCESS  1
#define PVFS2_XATTR_INDEX_POSIX_ACL_DEFAULT 2
#define PVFS2_XATTR_INDEX_TRUSTED           3
#define PVFS2_XATTR_INDEX_DEFAULT           4

#ifndef POSIX_ACL_XATTR_ACCESS
#define POSIX_ACL_XATTR_ACCESS	"system.posix_acl_access"
#endif
#ifndef POSIX_ACL_XATTR_DEFAULT
#define POSIX_ACL_XATTR_DEFAULT	"system.posix_acl_default"
#endif

#define PVFS2_XATTR_NAME_ACL_ACCESS  POSIX_ACL_XATTR_ACCESS
#define PVFS2_XATTR_NAME_ACL_DEFAULT POSIX_ACL_XATTR_DEFAULT
#define PVFS2_XATTR_NAME_TRUSTED_PREFIX "trusted."
#define PVFS2_XATTR_NAME_DEFAULT_PREFIX ""

#if !defined(PVFS2_LINUX_KERNEL_2_4) && defined(HAVE_GENERIC_GETXATTR)

extern int pvfs2_acl_chmod(struct inode *inode);
extern int pvfs2_init_acl(struct inode *inode, struct inode *dir);

extern struct xattr_handler *pvfs2_xattr_handlers[];
extern struct xattr_handler pvfs2_xattr_acl_default_handler, pvfs2_xattr_acl_access_handler;
extern struct xattr_handler pvfs2_xattr_trusted_handler;
extern struct xattr_handler pvfs2_xattr_default_handler;

#endif

static inline int convert_to_internal_xattr_flags(int setxattr_flags)
{
    int internal_flag = 0;

    /* Attribute must exist! */
    if (setxattr_flags & XATTR_REPLACE)
    {
        internal_flag = PVFS_XATTR_REPLACE;
    }
    /* Attribute must not exist */
    else if (setxattr_flags & XATTR_CREATE)
    {
        internal_flag = PVFS_XATTR_CREATE;
    }
    return internal_flag;
}

int pvfs2_xattr_set_trusted(struct inode *inode, 
    const char *name, const void *buffer, size_t size, int flags);
int pvfs2_xattr_get_trusted(struct inode *inode,
    const char *name, void *buffer, size_t size);
int pvfs2_xattr_set_default(struct inode *inode, 
    const char *name, const void *buffer, size_t size, int flags);
int pvfs2_xattr_get_default(struct inode *inode,
    const char *name, void *buffer, size_t size);


#endif

#ifndef HAVE_STRUCT_XTVEC
/* Redefine xtvec structure so that we could move helper functions out of the define */
struct xtvec 
{
    __kernel_off_t xtv_off;  /* must be off_t */
    __kernel_size_t xtv_len; /* must be size_t */
};
#endif

/************************************
 * pvfs2 data structures
 ************************************/
typedef struct
{
    enum pvfs2_vfs_op_states op_state;
    uint64_t tag;

    pvfs2_upcall_t upcall;
    pvfs2_downcall_t downcall;

    wait_queue_head_t waitq;
    spinlock_t lock;

    int io_completed;
    wait_queue_head_t io_completion_waitq;

    /* upcalls requiring variable length trailers require that this struct
     * be in the request list even after client-core does a read() on the device
     * to dequeue the upcall 
     * if op_linger field goes to 0, we dequeue this op off the list.
     * else we let it stay. What gets passed to the read() is 
     * a) if op_linger field is = 1, pvfs2_kernel_op_t itself
     * b) else if = 0, we pass ->upcall.trailer_buf
     * We expect to have only a single upcall trailer buffer, so we expect callers with trailers
     * to set this field to 2 and others to set it to 1.
     */
    int32_t op_linger, op_linger_tmp;
    /* VFS aio fields */
    void *priv;/* used by the async I/O code to stash the pvfs2_kiocb structure */
    atomic_t aio_ref_count; /* used again for the async I/O code for deallocation */

    int attempts;

    struct list_head list;
} pvfs2_kernel_op_t;

/** per inode private pvfs2 info */
typedef struct
{
    PVFS_object_ref refn;
    char link_target[PVFS_NAME_MAX];
    PVFS_size blksize;
    /*
     * Reading/Writing Extended attributes need to acquire the appropriate
     * reader/writer semaphore on the pvfs2_inode_t structure.
     */
    struct rw_semaphore xattr_sem;

#ifdef PVFS2_LINUX_KERNEL_2_4
    struct inode *vfs_inode;
#else
    struct inode vfs_inode;
#endif
    sector_t last_failed_block_index_read;
    int error_code;
    int revalidate_failed;

    /* State of in-memory attributes not yet flushed to disk associated with this object */
    unsigned long pinode_flags;
    /* All allocated pvfs2_inode_t objects are chained to a list */
    struct list_head list;
} pvfs2_inode_t;

#define P_ATIME_FLAG 0
#define P_MTIME_FLAG 1
#define P_CTIME_FLAG 2
#define P_MODE_FLAG  3
#define P_INIT_FLAG  4

#define ClearAtimeFlag(pinode) clear_bit(P_ATIME_FLAG, &(pinode)->pinode_flags)
#define SetAtimeFlag(pinode)   set_bit(P_ATIME_FLAG, &(pinode)->pinode_flags)
#define AtimeFlag(pinode)      test_bit(P_ATIME_FLAG, &(pinode)->pinode_flags)

#define ClearMtimeFlag(pinode) clear_bit(P_MTIME_FLAG, &(pinode)->pinode_flags)
#define SetMtimeFlag(pinode)   set_bit(P_MTIME_FLAG, &(pinode)->pinode_flags)
#define MtimeFlag(pinode)      test_bit(P_MTIME_FLAG, &(pinode)->pinode_flags)

#define ClearCtimeFlag(pinode) clear_bit(P_CTIME_FLAG, &(pinode)->pinode_flags)
#define SetCtimeFlag(pinode)   set_bit(P_CTIME_FLAG, &(pinode)->pinode_flags)
#define CtimeFlag(pinode)      test_bit(P_CTIME_FLAG, &(pinode)->pinode_flags)

#define ClearModeFlag(pinode) clear_bit(P_MODE_FLAG, &(pinode)->pinode_flags)
#define SetModeFlag(pinode)   set_bit(P_MODE_FLAG, &(pinode)->pinode_flags)
#define ModeFlag(pinode)      test_bit(P_MODE_FLAG, &(pinode)->pinode_flags)

#define ClearInitFlag(pinode) clear_bit(P_INIT_FLAG, &(pinode)->pinode_flags)
#define SetInitFlag(pinode)   set_bit(P_INIT_FLAG, &(pinode)->pinode_flags)
#define InitFlag(pinode)      test_bit(P_INIT_FLAG, &(pinode)->pinode_flags)

/** mount options.  only accepted mount options are listed.
 */
typedef struct
{
    /** intr option (if set) is inspired by the nfs intr option that
     *  interrupts the operation in progress if a signal is received,
     *  and ignores the signal otherwise (if not set).
     */
    int intr;
    /** acl option (if set) is inspired by the ext2 acl option that
     * requires the file system to honor acl's 
     */
    int acl;
    /** suid option (if set) is inspired by the nfs mount option
    * that requires the file system to honor the setuid bit of a 
    * file if set. NOTE: this is disabled by default.
    */
    int suid;
    /** noatime option (if set) is inspired by the nfs mount option
    * that requires the file system to disable atime updates for all
    * files if set. NOTE: this is disabled by default.
    */
    int noatime;
    /** nodiratime option (if set) is inspired by the nfs mount option
    * that requires the file system to disable atime updates for
    * directories alone if set. NOTE: this is disabled by default.
    */
    int nodiratime;
} pvfs2_mount_options_t;

/** per superblock private pvfs2 info */
typedef struct
{
    PVFS_handle root_handle;
    PVFS_fs_id fs_id;
    int id;
    pvfs2_mount_options_t mnt_options;
    char data[PVFS2_MAX_MOUNT_OPT_LEN];
    char devname[PVFS_MAX_SERVER_ADDR_LEN];
    struct super_block *sb;
    int    mount_pending;
    struct list_head list;
    atomic_t pvfs2_inode_alloc_count;
    atomic_t pvfs2_inode_dealloc_count;
} pvfs2_sb_info_t;

/** a temporary structure used only for sb mount time that groups the
 *  mount time data provided along with a private superblock structure
 *  that is allocated before a 'kernel' superblock is allocated.
*/
typedef struct
{
    void *data;
    PVFS_handle root_handle;
    PVFS_fs_id fs_id;
    int id;
} pvfs2_mount_sb_info_t;

/** PVFS2 specific structure that we use for constructing an opaque handle at the time 
 * an openg() system call that will be used at subsequent openfh system call
 * We stuff in enough information into this buffer that subsequent
 * openfh calls don't have to communicate with server. Padding is inserted so that
 * size of structure is the same and offset is also same on both 32 and 64
 * bit machines.
 */
typedef struct 
{
    PVFS_handle handle;
    PVFS_fs_id  fsid;
    int32_t     __pad1;
    PVFS_uid    owner;
    PVFS_gid    group;
    PVFS_permissions perms;
    int32_t     __pad2;
    PVFS_time   atime;
    PVFS_time   mtime;
    PVFS_time   ctime;
    PVFS_size   size;
    PVFS_ds_type objtype;
    uint32_t    mask;
} pvfs2_opaque_handle_t;
#ifdef __PINT_PROTO_ENCODE_OPAQUE_HANDLE
#define encode_int64_t(pptr,x) do { \
    *(int64_t*) *(pptr) = cpu_to_le64(*(x)); \
    *(pptr) += 8; \
} while (0)
#define decode_int64_t(pptr,x) do { \
    *(x) = le64_to_cpu(*(int64_t*) *(pptr)); \
    *(pptr) += 8; \
} while (0)

#define encode_int32_t(pptr,x) do { \
    *(int32_t*) *(pptr) = cpu_to_le32(*(x)); \
    *(pptr) += 4; \
} while (0)
#define decode_int32_t(pptr,x) do { \
    *(x) = le32_to_cpu(*(int32_t*) *(pptr)); \
    *(pptr) += 4; \
} while (0)

/* skip 4 bytes */
#define encode_skip4(pptr,x) do { \
    *(pptr) += 4; \
} while (0)
#define decode_skip4(pptr,x) do { \
    *(pptr) += 4; \
} while (0)

#define encode_pvfs2_opaque_handle_t(pptr,x) do {\
    encode_int64_t(pptr, &(x)->handle);\
    encode_int32_t(pptr, &(x)->fsid);\
    encode_skip4(pptr,);\
    encode_int32_t(pptr, &(x)->owner);\
    encode_int32_t(pptr, &(x)->group);\
    encode_int32_t(pptr, &(x)->perms);\
    encode_skip4(pptr,);\
    encode_int64_t(pptr, &(x)->atime);\
    encode_int64_t(pptr, &(x)->mtime);\
    encode_int64_t(pptr, &(x)->ctime);\
    encode_int64_t(pptr, &(x)->size);\
    encode_int32_t(pptr, &(x)->objtype);\
    encode_int32_t(pptr, &(x)->mask);\
} while (0)
#define decode_pvfs2_opaque_handle_t(pptr,x) do {\
    decode_int64_t(pptr, &(x)->handle);\
    decode_int32_t(pptr, &(x)->fsid);\
    decode_skip4(pptr,);\
    decode_int32_t(pptr, &(x)->owner);\
    decode_int32_t(pptr, &(x)->group);\
    decode_int32_t(pptr, &(x)->perms);\
    decode_skip4(pptr,);\
    decode_int64_t(pptr, &(x)->atime);\
    decode_int64_t(pptr, &(x)->mtime);\
    decode_int64_t(pptr, &(x)->ctime);\
    decode_int64_t(pptr, &(x)->size);\
    decode_int32_t(pptr, &(x)->objtype);\
    decode_int32_t(pptr, &(x)->mask);\
} while (0)
#endif

#ifdef HAVE_AIO_VFS_SUPPORT

/** structure that holds the state of any async I/O operation issued 
 *  through the VFS. Needed especially to handle cancellation requests
 *  or even completion notification so that the VFS client-side daemon
 *  can free up its vfs_request slots.
 */
typedef struct
{
    struct task_struct *tsk;/* the pointer to the task that initiated the AIO */
    struct kiocb *kiocb; /* pointer to the kiocb that kicked this operation */
    int buffer_index; /* buffer index that was used for the I/O */
    pvfs2_kernel_op_t *op; /* pvfs2 kernel operation type */
    struct iovec *iov; /* The user space buffers from/to which I/O is being staged */
    unsigned long nr_segs; /* number of elements in the iovector */
    int   rw; /* set to indicate the type of the operation */
    loff_t offset; /* file offset */
    size_t bytes_to_be_copied; /* and the count in bytes */
    ssize_t bytes_copied;
    int needs_cleanup;
} pvfs2_kiocb;

#endif

typedef struct pvfs2_stats {
    unsigned long cache_hits;
    unsigned long cache_misses;
    unsigned long reads;
    unsigned long writes;
} pvfs2_stats;

extern pvfs2_stats g_pvfs2_stats;

/*
  NOTE: See Documentation/filesystems/porting for information
  on implementing FOO_I and properly accessing fs private data
*/
static inline pvfs2_inode_t *PVFS2_I(
    struct inode *inode)
{
#ifdef PVFS2_LINUX_KERNEL_2_4
    return (pvfs2_inode_t *)inode->u.generic_ip;
#else
    return container_of(inode, pvfs2_inode_t, vfs_inode);
#endif
}

static inline pvfs2_sb_info_t *PVFS2_SB(
    struct super_block *sb)
{
#ifdef PVFS2_LINUX_KERNEL_2_4
    return (pvfs2_sb_info_t *)sb->u.generic_sbp;
#else
    return (pvfs2_sb_info_t *)sb->s_fs_info;
#endif
}

static inline PVFS_handle ino_to_pvfs2_handle(ino_t ino)
{
    return (PVFS_handle) ino;
}

static inline ino_t pvfs2_handle_to_ino(PVFS_handle handle)
{
    ino_t ino;

    ino = (ino_t) handle;
    if (sizeof(ino_t) < sizeof(PVFS_handle))
        ino ^= handle >> (sizeof(PVFS_handle) - sizeof(ino_t)) * 8;
    return ino;
}

static inline PVFS_handle get_handle_from_ino(struct inode *inode)
{
#if defined(HAVE_IGET5_LOCKED) || defined(HAVE_IGET4_LOCKED)
    return PVFS2_I(inode)->refn.handle;
#else
    return ino_to_pvfs2_handle(inode->i_ino);
#endif
}

static inline PVFS_fs_id get_fsid_from_ino(struct inode *inode)
{
    return PVFS2_I(inode)->refn.fs_id;
}

static inline ino_t get_ino_from_handle(struct inode *inode)
{
    PVFS_handle handle;
    ino_t ino;

    handle = get_handle_from_ino(inode);
    ino = pvfs2_handle_to_ino(handle);
    return ino;
}

static inline ino_t get_parent_ino_from_dentry(struct dentry *dentry)
{
    return get_ino_from_handle(dentry->d_parent->d_inode);
}

static inline int is_root_handle(struct inode *inode)
{
    return PVFS2_SB(inode->i_sb)->root_handle == get_handle_from_ino(inode);
}

static inline int match_handle(PVFS_handle resp_handle, struct inode *inode)
{
    return resp_handle == get_handle_from_ino(inode);
}

/****************************
 * defined in pvfs2-cache.c
 ****************************/
int op_cache_initialize(void);
int op_cache_finalize(void);
pvfs2_kernel_op_t *op_alloc(int32_t type);
pvfs2_kernel_op_t *op_alloc_trailer(int32_t type);
char *get_opname_string(pvfs2_kernel_op_t *new_op);
void op_release(pvfs2_kernel_op_t *op);

int dev_req_cache_initialize(void);
int dev_req_cache_finalize(void);
void *dev_req_alloc(void);
void  dev_req_release(void *);

int pvfs2_inode_cache_initialize(void);
int pvfs2_inode_cache_finalize(void);
pvfs2_inode_t *pvfs2_inode_alloc(void);
void pvfs2_inode_release(pvfs2_inode_t *);

#ifdef HAVE_AIO_VFS_SUPPORT
int kiocb_cache_initialize(void);
int kiocb_cache_finalize(void);
pvfs2_kiocb* kiocb_alloc(void);
void kiocb_release(pvfs2_kiocb *ptr);
#else
static inline int kiocb_cache_initialize(void)
{
    return 0;
}
static inline int kiocb_cache_finalize(void)
{
    return 0;
}
#endif

/****************************
 * defined in pvfs2-mod.c
 ****************************/
void purge_inprogress_ops(void);

/****************************
 * defined in waitqueue.c
 ****************************/
int wait_for_matching_downcall(
    pvfs2_kernel_op_t * op);
int wait_for_cancellation_downcall(
    pvfs2_kernel_op_t * op);
void clean_up_interrupted_operation(
    pvfs2_kernel_op_t * op);
void purge_waiting_ops(void);

/****************************
 * defined in super.c
 ****************************/
#ifdef HAVE_FIND_INODE_HANDLE_SUPER_OPERATIONS
extern struct inode *pvfs2_sb_find_inode_handle(struct super_block *sb, 
        const struct file_handle *handle);
#endif
#ifdef PVFS2_LINUX_KERNEL_2_4
struct super_block* pvfs2_get_sb(
    struct super_block *sb,
    void *data,
    int silent);
#else
#ifdef HAVE_VFSMOUNT_GETSB
int pvfs2_get_sb(
    struct file_system_type *fst, int flags,
    const char *devname, void *data, 
    struct vfsmount *mnt);
#else
struct super_block *pvfs2_get_sb(
    struct file_system_type *fst, int flags,
    const char *devname, void *data);
#endif
#endif

void pvfs2_read_inode(
    struct inode *inode);

void pvfs2_kill_sb(struct super_block *sb);
int pvfs2_remount(
    struct super_block *sb,
    int *flags,
    char *data);

int fsid_key_table_initialize(void);
void fsid_key_table_finalize(void);

/****************************
 * defined in inode.c
 ****************************/
int pvfs2_set_inode(struct inode *inode, void *data);
uint32_t convert_to_pvfs2_mask(unsigned long lite_mask);
struct inode *pvfs2_get_custom_inode_common(
    struct super_block *sb,
    struct inode *dir,
    int mode,
    dev_t dev,
    PVFS_object_ref ref,
    int from_create);

/* In-core inodes are not being created on-disk */
#define pvfs2_get_custom_core_inode(sb, dir, mode, dev, ref) \
        pvfs2_get_custom_inode_common(sb, dir, mode, dev, ref, 0)
/* On-disk inodes are being created */
#define pvfs2_get_custom_inode(sb, dir, mode, dev, ref) \
        pvfs2_get_custom_inode_common(sb, dir, mode, dev, ref, 1)

int pvfs2_setattr(
    struct dentry *dentry,
    struct iattr *iattr);

#ifdef PVFS2_LINUX_KERNEL_2_4
int pvfs2_revalidate(
    struct dentry *dentry);
#else
int pvfs2_getattr(
    struct vfsmount *mnt,
    struct dentry *dentry,
    struct kstat *kstat);
#endif

/****************************
 * defined in xattr.c
 ****************************/
#ifdef HAVE_SETXATTR_CONST_ARG
int pvfs2_setxattr(struct dentry *dentry, const char *name,
		const void *value, size_t size, int flags);
#else
int pvfs2_setxattr(struct dentry *dentry, const char *name,
		void *value, size_t size, int flags);
#endif
ssize_t pvfs2_getxattr(struct dentry *dentry, const char *name,
		         void *buffer, size_t size);
ssize_t pvfs2_listxattr(struct dentry *dentry, char *buffer, size_t size);
int pvfs2_removexattr(struct dentry *dentry, const char *name);

/****************************
 * defined in namei.c
 ****************************/
struct inode *pvfs2_iget_common(
        struct super_block *sb,
        PVFS_object_ref *ref, int keep_locked);
#define pvfs2_iget(sb, ref)        pvfs2_iget_common(sb, ref, 0)
#define pvfs2_iget_locked(sb, ref) pvfs2_iget_common(sb, ref, 1)

#if defined(PVFS2_LINUX_KERNEL_2_4) || defined(HAVE_TWO_PARAM_PERMISSION)
int pvfs2_permission(struct inode *, int);
#else
int pvfs2_permission(struct inode *inode, 
					 int mask, struct nameidata *nd);
#endif

/*****************************
 * defined in file.c (shared file/dir operations)
 ****************************/
int pvfs2_file_open(
    struct inode *inode,
    struct file *file);
int pvfs2_file_release(
    struct inode *inode,
    struct file *file);
ssize_t pvfs2_inode_read(
    struct inode *inode,
    char *buf,
    size_t count,
    loff_t *offset,
    int copy_to_user,
    loff_t readahead_size);

/*****************************
 * defined in devpvfs2-req.c
 ****************************/

int     pvfs2_dev_init(void);
void    pvfs2_dev_cleanup(void);
int     is_daemon_in_service(void);
int     fs_mount_pending(PVFS_fs_id fsid);

/****************************
 * defined in pvfs2-utils.c
 ****************************/
int pvfs2_gen_credentials(
    PVFS_credentials *credentials);
PVFS_fs_id fsid_of_op(pvfs2_kernel_op_t *op);
int pvfs2_flush_inode(struct inode *inode);

int copy_attributes_to_inode(
    struct inode *inode,
    PVFS_sys_attr *attrs,
    char *symname);

ssize_t pvfs2_inode_getxattr(
        struct inode *inode, const char* prefix,
        const char *name, void *buffer, size_t size);
int pvfs2_inode_setxattr(struct inode *inode, const char* prefix,
        const char *name, const void *value, size_t size, int flags);
int pvfs2_inode_removexattr(struct inode *inode, const char* prefix,
        const char *name, int flags);
int pvfs2_inode_listxattr(struct inode *inode, char *, size_t);

int pvfs2_inode_getattr(
    struct inode *inode, uint32_t mask);

int pvfs2_inode_setattr(
    struct inode *inode,
    struct iattr *iattr);

struct inode *pvfs2_create_entry(
    struct inode *dir,
    struct dentry *dentry,
    const char *symname,
    int mode,
    int op_type,
    int *error_code);

int pvfs2_remove_entry(
    struct inode *dir,
    struct dentry *dentry);

int pvfs2_truncate_inode(
    struct inode *inode,
    loff_t size);

void pvfs2_inode_initialize(
    pvfs2_inode_t *pvfs2_inode);

void pvfs2_inode_finalize(
    pvfs2_inode_t *pvfs2_inode);

void pvfs2_op_initialize(
    pvfs2_kernel_op_t *op);

void pvfs2_make_bad_inode(
    struct inode *inode);

void mask_blocked_signals(
    sigset_t *orig_sigset);

void unmask_blocked_signals(
    sigset_t *orig_sigset);

#ifdef USE_MMAP_RA_CACHE
int pvfs2_flush_mmap_racache(
    struct inode *inode);
#endif

int pvfs2_unmount_sb(
    struct super_block *sb);

int pvfs2_cancel_op_in_progress(
    unsigned long tag);

PVFS_time pvfs2_convert_time_field(
    void *time_ptr);

#ifdef HAVE_FILL_HANDLE_INODE_OPERATIONS
int pvfs2_fill_handle(struct inode *inode, struct file_handle *handle);
#endif

int pvfs2_normalize_to_errno(PVFS_error error_code);

extern struct semaphore devreq_semaphore;
extern struct semaphore request_semaphore;
extern int debug;
extern int op_timeout_secs;
extern int slot_timeout_secs;
extern struct list_head pvfs2_superblocks;
extern spinlock_t pvfs2_superblocks_lock;
extern struct list_head pvfs2_request_list;
extern spinlock_t pvfs2_request_list_lock;
extern wait_queue_head_t pvfs2_request_list_waitq;
extern struct qhash_table *htable_ops_in_progress;

extern struct file_system_type pvfs2_fs_type;
extern struct address_space_operations pvfs2_address_operations;
extern struct backing_dev_info pvfs2_backing_dev_info;
extern struct inode_operations pvfs2_file_inode_operations;
extern struct file_operations pvfs2_file_operations;
extern struct inode_operations pvfs2_symlink_inode_operations;
extern struct inode_operations pvfs2_dir_inode_operations;
extern struct file_operations pvfs2_dir_operations;
extern struct dentry_operations pvfs2_dentry_operations;
extern struct file_operations pvfs2_devreq_file_operations;

/************************************
 * misc convenience macros
 ************************************/
#define add_op_to_request_list(op)                           \
do {                                                         \
    spin_lock(&op->lock);                                    \
    set_op_state_waiting(op);                                \
                                                             \
    spin_lock(&pvfs2_request_list_lock);                     \
    list_add_tail(&op->list, &pvfs2_request_list);           \
    spin_unlock(&pvfs2_request_list_lock);                   \
    spin_unlock(&op->lock);                                  \
    wake_up_interruptible(&pvfs2_request_list_waitq);        \
} while(0)

#define add_priority_op_to_request_list(op)                  \
do {                                                         \
    spin_lock(&op->lock);                                    \
    set_op_state_waiting(op);                                \
                                                             \
    spin_lock(&pvfs2_request_list_lock);                     \
    list_add(&op->list, &pvfs2_request_list);                \
    spin_unlock(&pvfs2_request_list_lock);                   \
    spin_unlock(&op->lock);                                  \
    wake_up_interruptible(&pvfs2_request_list_waitq);        \
} while(0)

#define remove_op_from_request_list(op)                      \
do {                                                         \
    struct list_head *tmp = NULL;                            \
    pvfs2_kernel_op_t *tmp_op = NULL;                        \
                                                             \
    spin_lock(&pvfs2_request_list_lock);                     \
    list_for_each(tmp, &pvfs2_request_list) {                \
        tmp_op = list_entry(tmp, pvfs2_kernel_op_t, list);   \
        if (tmp_op && (tmp_op == op)) {                      \
            list_del(&tmp_op->list);                         \
            break;                                           \
        }                                                    \
    }                                                        \
    spin_unlock(&pvfs2_request_list_lock);                   \
} while(0)

#define remove_op_from_htable_ops_in_progress(op)            \
do {                                                         \
    qhash_search_and_remove(htable_ops_in_progress,          \
                            &(op->tag));                     \
} while(0)

#define PVFS2_OP_INTERRUPTIBLE 1   /**< service_operation() is interruptible */
#define PVFS2_OP_PRIORITY      2   /**< service_operation() is high priority */
#define PVFS2_OP_CANCELLATION  4   /**< this is a cancellation */
#define PVFS2_OP_NO_SEMAPHORE  8   /**< don't acquire semaphore */
#define PVFS2_OP_ASYNC         16  /* Queue it, but don't wait */

int service_operation(pvfs2_kernel_op_t* op, const char* op_name, 
    int flags);

/** handles two possible error cases, depending on context.
 *
 *  by design, our vfs i/o errors need to be handled in one of two ways,
 *  depending on where the error occured.
 *
 *  if the error happens in the waitqueue code because we either timed
 *  out or a signal was raised while waiting, we need to cancel the
 *  userspace i/o operation and free the op manually.  this is done to
 *  avoid having the device start writing application data to our shared
 *  bufmap pages without us expecting it.
 *
 *  FIXME: POSSIBLE OPTIMIZATION:
 *  However, if we timed out or if we got a signal AND our upcall was never
 *  picked off the queue (i.e. we were in OP_VFS_STATE_WAITING), then we don't
 *  need to send a cancellation upcall. The way we can handle this is set error_exit
 *  to 2 in such cases and 1 whenever cancellation has to be sent and have handle_error
 *  take care of this situation as well..
 *
 *  if a pvfs2 sysint level error occured and i/o has been completed,
 *  there is no need to cancel the operation, as the user has finished
 *  using the bufmap page and so there is no danger in this case.  in
 *  this case, we wake up the device normally so that it may free the
 *  op, as normal.
 *
 *  \note the only reason this is a macro is because both read and write
 *  cases need the exact same handling code.
 */
#define handle_io_error()                                 \
do {                                                      \
    if(!op_state_serviced(new_op))                        \
    {                                                     \
        pvfs2_cancel_op_in_progress(new_op->tag);         \
        op_release(new_op);                               \
    }                                                     \
    else                                                  \
    {                                                     \
        wake_up_daemon_for_return(new_op);                \
    }                                                     \
    new_op = NULL;                                        \
    pvfs_bufmap_put(buffer_index);                        \
    buffer_index = -1;                                    \
} while(0)

#define get_interruptible_flag(inode)                     \
((PVFS2_SB(inode->i_sb)->mnt_options.intr ? PVFS2_OP_INTERRUPTIBLE : 0))

#define get_acl_flag(inode)                               \
(PVFS2_SB(inode->i_sb)->mnt_options.acl)

#define get_suid_flag(inode)                              \
(PVFS2_SB(inode->i_sb)->mnt_options.suid)

#ifdef USE_MMAP_RA_CACHE
#define clear_inode_mmap_ra_cache(inode)                  \
do {                                                      \
  gossip_debug(GOSSIP_INODE_DEBUG, "calling clear_inode_mmap_ra_cache on %llu\n",\
              llu(get_handle_from_ino(inode)));                         \
  pvfs2_flush_mmap_racache(inode);                        \
  gossip_debug(GOSSIP_INODE_DEBUG, "clear_inode_mmap_ra_cache finished\n");    \
} while(0)
#else
#define clear_inode_mmap_ra_cache(inode)
#endif /* USE_MMAP_RA_CACHE */

#define add_pvfs2_sb(sb)                                             \
do {                                                                 \
    gossip_debug(GOSSIP_SUPER_DEBUG, "Adding SB %p to pvfs2 superblocks\n", PVFS2_SB(sb));\
    spin_lock(&pvfs2_superblocks_lock);                              \
    list_add_tail(&PVFS2_SB(sb)->list, &pvfs2_superblocks);          \
    spin_unlock(&pvfs2_superblocks_lock);                            \
} while(0)

#define remove_pvfs2_sb(sb)                                          \
do {                                                                 \
    struct list_head *tmp = NULL;                                    \
    pvfs2_sb_info_t *pvfs2_sb = NULL;                                \
                                                                     \
    spin_lock(&pvfs2_superblocks_lock);                              \
    list_for_each(tmp, &pvfs2_superblocks) {                         \
        pvfs2_sb = list_entry(tmp, pvfs2_sb_info_t, list);           \
        if (pvfs2_sb && (pvfs2_sb->sb == sb)) {                      \
            gossip_debug(GOSSIP_SUPER_DEBUG, "Removing SB %p from pvfs2 superblocks\n",   \
                        pvfs2_sb);                                   \
            list_del(&pvfs2_sb->list);                               \
            break;                                                   \
        }                                                            \
    }                                                                \
    spin_unlock(&pvfs2_superblocks_lock);                            \
} while(0)

#define pvfs2_update_inode_time(inode) \
do { inode->i_mtime = inode->i_ctime = CURRENT_TIME; } while(0)


#ifdef PVFS2_LINUX_KERNEL_2_4
#define get_block_block_type long
#define pvfs2_lock_inode(inode) do {} while(0)
#define pvfs2_unlock_inode(inode) do {} while(0)
#define pvfs2_kernel_readpage block_read_full_page

static inline struct dentry *pvfs2_d_splice_alias(struct dentry *dentry, struct inode *inode)
{ 
    d_add(dentry, inode); 
    return dentry;
}

/*
  redhat 9 2.4.x kernels have to be treated almost like 2.6.x kernels
  so we special case them here
*/
#ifdef REDHAT_RELEASE_9
#define pvfs2_current_signal_lock current->sighand->siglock
#define pvfs2_current_sigaction current->sighand->action
#define pvfs2_recalc_sigpending recalc_sigpending
#define pvfs2_set_page_reserved(page) do {} while(0)
#define pvfs2_clear_page_reserved(page) do {} while(0)
#else
#define pvfs2_current_signal_lock current->sigmask_lock
#define pvfs2_current_sigaction current->sig->action
#define pvfs2_recalc_sigpending() recalc_sigpending(current)
#define pvfs2_set_page_reserved(page) SetPageReserved(page)
#define pvfs2_clear_page_reserved(page) \
do { ClearPageReserved(page); put_page(page); } while(0)
#endif /* REDHAT_RELEASE_9 */

#define fill_default_sys_attrs(sys_attr,type,mode)\
do                                                \
{                                                 \
    sys_attr.owner = current->fsuid;              \
    sys_attr.group = current->fsgid;              \
    sys_attr.size = 0;                            \
    sys_attr.perms = PVFS_util_translate_mode(mode,0); \
    sys_attr.objtype = type;                      \
    sys_attr.mask = PVFS_ATTR_SYS_ALL_SETABLE;    \
} while(0)

#else /* !(PVFS2_LINUX_KERNEL_2_4) */

#define get_block_block_type sector_t
#define pvfs2_lock_inode(inode) spin_lock(&inode->i_lock)
#define pvfs2_unlock_inode(inode) spin_unlock(&inode->i_lock)
#define pvfs2_current_signal_lock current->sighand->siglock
#define pvfs2_current_sigaction current->sighand->action
#define pvfs2_recalc_sigpending recalc_sigpending
#define pvfs2_kernel_readpage mpage_readpage
#define pvfs2_set_page_reserved(page) do {} while(0)
#define pvfs2_clear_page_reserved(page) do {} while(0)

static inline struct dentry* pvfs2_d_splice_alias(struct dentry *dentry, struct inode *inode)
{
    return d_splice_alias(inode, dentry);
}

#ifdef HAVE_CURRENT_FSUID 
#define fill_default_sys_attrs(sys_attr,type,mode)\
do                                                \
{                                                 \
    sys_attr.owner = current_fsuid();             \
    sys_attr.group = current_fsgid();             \
    sys_attr.size = 0;                            \
    sys_attr.perms = PVFS_util_translate_mode(mode,0); \
    sys_attr.objtype = type;                      \
    sys_attr.mask = PVFS_ATTR_SYS_ALL_SETABLE;    \
} while(0)
#else
#define fill_default_sys_attrs(sys_attr,type,mode)\
do                                                \
{ \
    sys_attr.owner = current->fsuid;              \
    sys_attr.group = current->fsgid;              \
    sys_attr.size = 0;                            \
    sys_attr.perms = PVFS_util_translate_mode(mode,0); \
    sys_attr.objtype = type;                      \
    sys_attr.mask = PVFS_ATTR_SYS_ALL_SETABLE;    \
} while(0)
#endif /* HAVE_CURRENT_FSUID */

#endif /* PVFS2_LINUX_KERNEL_2_4 */

#ifdef PVFS2_LINUX_KERNEL_2_4
/*
  based on code from 2.6.x's fs/libfs.c with required macro support
  from include/linux/list.h
*/
static inline int simple_positive(struct dentry *dentry)
{
    return dentry->d_inode && !d_unhashed(dentry);
}

#define list_for_each_entry(pos, head, member)             \
for (pos = list_entry((head)->next, typeof(*pos), member), \
  prefetch(pos->member.next);                              \
  &pos->member != (head);                                  \
  pos = list_entry(pos->member.next, typeof(*pos), member),\
  prefetch(pos->member.next))

static inline int simple_empty(struct dentry *dentry)
{
    struct dentry *child;
    int ret = 0;
    spin_lock(&dcache_lock);
    list_for_each_entry(child, &dentry->d_subdirs, d_child)
        if (simple_positive(child))
            goto out;
    ret = 1;
out:
    spin_unlock(&dcache_lock);
    return ret;
}

#if (PVFS2_LINUX_KERNEL_2_4_MINOR_VER < 19)
static inline int dcache_dir_open(struct inode *inode, struct file *file)
{
    static struct qstr cursor_name = {.len = 1, .name = "."};

    file->private_data = d_alloc(file->f_dentry, &cursor_name);

    return file->private_data ? 0 : -ENOMEM;
}

static inline int dcache_dir_close(struct inode *inode, struct file *file)
{
    dput(file->private_data);
    return 0;
}
#endif /* PVFS2_LINUX_KERNEL_2_4_MINOR_VER */

#endif /* PVFS2_LINUX_KERNEL_2_4 */

#ifdef HAVE_I_SEM_IN_STRUCT_INODE
#define pvfs2_inode_lock(__i) do \
{ down(&(__i)->i_sem); } while (0)
#define pvfs2_inode_unlock(__i) do \
{ up(&(__i)->i_sem); } while (0)
#else
#define pvfs2_inode_lock(__i) do \
{ mutex_lock(&(__i)->i_mutex); } while (0)
#define pvfs2_inode_unlock(__i) do \
{ mutex_unlock(&(__i)->i_mutex); } while (0)
#endif /* HAVE_I_SEM_IN_STRUCT_INODE */

static inline void pvfs2_i_size_write(struct inode *inode, loff_t i_size)
{
#ifndef HAVE_I_SIZE_WRITE
    inode->i_size = i_size;
#else
    #if BITS_PER_LONG==32 && defined(CONFIG_SMP)
    pvfs2_inode_lock(inode);
    #endif
    i_size_write(inode, i_size);
    #if BITS_PER_LONG==32 && defined(CONFIG_SMP)
    pvfs2_inode_unlock(inode);
    #endif
#endif
    return;
}

static inline loff_t pvfs2_i_size_read(struct inode *inode)
{
#ifndef HAVE_I_SIZE_READ
    return inode->i_size;
#else
    return i_size_read(inode);
#endif
}

static inline unsigned int diff(struct timeval *end, struct timeval *begin)
{
    if (end->tv_usec < begin->tv_usec) {
        end->tv_usec += 1000000; end->tv_sec--;
    }
    end->tv_sec  -= begin->tv_sec;
    end->tv_usec -= begin->tv_usec;
    return ((end->tv_sec * 1000000) + end->tv_usec);
}

#ifndef HAVE_KZALLOC
static inline void *kzalloc(size_t size, int flags)
{
	void * ptr;

	ptr = kmalloc(size, flags);
	if (ptr)
	{
            memset(ptr, 0, size);
	}
	return ptr;
}
#endif

#endif /* __PVFS2KERNEL_H */

/* @} */

/*
 * Local variables:
 *  c-indent-level: 4
 *  c-basic-offset: 4
 * End:
 *
 * vim: ts=8 sts=4 sw=4 expandtab
 */
