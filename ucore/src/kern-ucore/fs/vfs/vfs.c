#include <types.h>
#include <stdio.h>
#include <string.h>
#include <slab.h>
#include <vfs.h>
#include <inode.h>
#include <sem.h>
#include <error.h>
#include <mod.h>

static semaphore_t bootfs_sem;
static struct inode *bootfs_node = NULL;

extern void vfs_devlist_init(void);

struct fs *__alloc_fs(int type)
{
	struct fs *fs;
	if ((fs = kmalloc(sizeof(struct fs))) != NULL) {
		// kprintf("Location of fs: %016x\n", fs);
		// kprintf("Location of type: %016x\n", &(fs->fs_type));
   		// kprintf("SIZE of it %d\n", sizeof(fs->fs_info));
		
		// kprintf("Location of the stack: %016x\n", &fs);
		// kprintf("LL %x %x %x %x %x %x\n", &fs->fs_info.__pipe_info, &fs->fs_info.__sfs_info,
        // 	&fs->fs_info.__sfatfs_info, &fs->fs_info.__sfatfs_info.sfatfs_buffer,
        //     &fs->fs_info.__sfatfs_info.fat, &fs->fs_info.__sfatfs_info.mutex_sem);
		// kprintf("SZ %d %d %d %d %d %d %d %d %d %d %d %d\n", sizeof(struct pipe_fs), sizeof(struct sfs_fs),
       	// 	sizeof(struct sfatfs_fs), sizeof(struct sfatfs_super), sizeof(struct device),
		// 	   sizeof(struct sfatfs_disk_inode), sizeof(semaphore_t), sizeof(atomic_t), sizeof(struct spinlock_s), 
		// 	   sizeof(wait_queue_t), sizeof(int), sizeof(bool));
		fs->fs_type = type;
	}
	return fs;
}

EXPORT_SYMBOL(__alloc_fs);

void vfs_init(void)
{
	sem_init(&bootfs_sem, 1);
	vfs_devlist_init();
	file_system_type_list_init();
}

static void lock_bootfs(void)
{
	down(&bootfs_sem);
}

static void unlock_bootfs(void)
{
	up(&bootfs_sem);
}

/*
 * Helper function for actually changing bootfs_inode.
 */
static void change_bootfs(struct inode *node)
{
	struct inode *old;
	lock_bootfs();
	{
		old = bootfs_node, bootfs_node = node;
	}
	unlock_bootfs();
	if (old != NULL) {
		vop_ref_dec(old);
	}
}

/*
 * Set bootfs_node.
 *
 * Bootfs_node is the inode used for beginning path translation of
 * pathnames starting with /.
 *
 * It is also incidentally the system's first current directory.
 */
int vfs_set_bootfs(char *fsname)
{
	struct inode *node = NULL;
	if (fsname != NULL) {
		char *s;
		if ((s = strchr(fsname, ':')) == NULL || s[1] != '\0') {
			return -E_INVAL;
		}
		int ret;
		if ((ret = vfs_chdir(fsname)) != 0) {
			kprintf("ret vfs_chdir %s, %d\n", fsname, ret);
			return ret;
		}
		if ((ret = vfs_get_curdir(&node)) != 0) {
			kprintf("ret vfs_get_curdir %s, %d\n", fsname, ret);
			return ret;
		}
	}
	
	change_bootfs(node);
	return 0;
}

/*
 * Get bootfs_inode.
 */
int vfs_get_bootfs(struct inode **node_store)
{
	struct inode *node = NULL;
	if (bootfs_node != NULL) {
		lock_bootfs();
		{
			if ((node = bootfs_node) != NULL) {
				vop_ref_inc(bootfs_node);
			}
		}
		unlock_bootfs();
	}
	if (node == NULL) {
		return -E_NOENT;
	}
	*node_store = node;
	return 0;
}

#define le2fstype(le, member)                         \
    to_struct((le), struct file_system_type, member)

static list_entry_t file_system_type_list;
static semaphore_t file_system_type_sem;

static void lock_file_system_type_list(void)
{
	down(&file_system_type_sem);
}

static void unlock_file_system_type_list(void)
{
	up(&file_system_type_sem);
}

static bool check_file_system_type_name_conflict(const char *name)
{
	list_entry_t *list = &file_system_type_list, *le = list;
	while ((le = list_next(le)) != list) {
		struct file_system_type *fstype =
		    le2fstype(le, file_system_type_link);
		if (strcmp(fstype->name, name) == 0) {
			return 0;
		}
	}
	return 1;
}

void file_system_type_list_init(void)
{
	list_init(&file_system_type_list);
	sem_init(&file_system_type_sem, 1);
}

int register_filesystem(const char *name, int (*mount) (const char *devname))
{
	assert(name != NULL);
	if (strlen(name) > FS_MAX_DNAME_LEN) {
		return -E_TOO_BIG;
	}

	int ret = -E_NO_MEM;
	char *s_name;
	if ((s_name = strdup(name)) == NULL) {
		return ret;
	}

	struct file_system_type *fstype;
	if ((fstype = kmalloc(sizeof(struct file_system_type))) == NULL) {
		goto failed_cleanup_name;
	}

	ret = -E_EXISTS;
	lock_file_system_type_list();
	if (!check_file_system_type_name_conflict(s_name)) {
		unlock_file_system_type_list();
		goto failed_cleanup_fstype;
	}
	fstype->name = s_name;
	fstype->mount = mount;

	list_add(&file_system_type_list, &(fstype->file_system_type_link));
	unlock_file_system_type_list();
	return 0;

failed_cleanup_fstype:
	kfree(fstype);
failed_cleanup_name:
	kfree(s_name);
	return ret;
}

int unregister_filesystem(const char *name)
{
	int ret = -E_EXISTS;
	lock_file_system_type_list();
	list_entry_t *list = &file_system_type_list, *le = list;
	while ((le = list_next(le)) != list) {
		struct file_system_type *fstype =
		    le2fstype(le, file_system_type_link);
		if (strcmp(fstype->name, name) == 0) {
			list_del(le);
			kfree((char *)fstype->name);
			kfree(fstype);
			ret = 0;
			break;
		}
	}

	unlock_file_system_type_list();
	return ret;
}

int do_mount(const char *devname, const char *fsname)
{
	int ret = -E_EXISTS;
	lock_file_system_type_list();
	list_entry_t *list = &file_system_type_list, *le = list;
	while ((le = list_next(le)) != list) {
		struct file_system_type *fstype =
		    le2fstype(le, file_system_type_link);
		if (strcmp(fstype->name, fsname) == 0) {
			assert(fstype->mount);
			ret = (fstype->mount) (devname);
			break;
		}
	}
	unlock_file_system_type_list();
	return ret;
}

int do_umount(const char *devname)
{
	return vfs_unmount(devname);
}

EXPORT_SYMBOL(unregister_filesystem);
EXPORT_SYMBOL(register_filesystem);
