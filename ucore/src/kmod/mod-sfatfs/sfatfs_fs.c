#include <stdio.h>
#include <string.h>
#include <slab.h>
#include <list.h>
#include <fs.h>
#include <vfs.h>
#include <dev.h>
#include <sfatfs.h>
#include <inode.h>
#include <iobuf.h>
#include <bitmap.h>
#include <error.h>
#include <assert.h>
#include <kio.h>

/*
 * Write Block 0 & 1 back to IDE if it's dirty
 */
static int
sfatfs_sync(struct fs *fs) {
    struct sfatfs_fs *sfatfs = fsop_info(fs, sfatfs);
    //cache
	lock_sfatfs_fs(sfatfs);
    {
        list_entry_t *list = &(sfatfs->inode_list), *le = list;
        while ((le = list_next(le)) != list) {
            struct sfatfs_inode *sin = le2sfatin(le, inode_link);
            vop_fsync(info2node(sin, sfatfs_inode));
        }
    }
    unlock_sfatfs_fs(sfatfs);
	//block 0 & 1,meta data
    int ret;
    if (sfatfs->super_dirty) {
        sfatfs->super_dirty = 0;
        if ((ret = sfatfs_sync_super(sfatfs)) != 0) {
            sfatfs->super_dirty = 1;
            return ret;
        }
        if ((ret = sfatfs_sync_freemap(sfatfs)) != 0) {
            sfatfs->super_dirty = 1;
            return ret;
        }
    }
    return 0;
}
//Get root inode
static struct inode *
sfatfs_get_root(struct fs *fs) {
    struct inode *node;
    int ret;
    if ((ret = sfatfs_load_inode(fsop_info(fs, sfatfs), &node, SFATFS_RECORD_ROOT)) != 0) {
        panic("load sfatfs root failed: %e", ret);
    }
	return node;
}

static int
sfatfs_unmount(struct fs *fs) {
    struct sfatfs_fs *sfatfs = fsop_info(fs, sfatfs);
    if (!list_empty(&(sfatfs->inode_list))) {
        return -E_BUSY;
    }
    assert(!sfatfs->super_dirty);
    kfree(sfatfs->fat);
    kfree(sfatfs->sfatfs_buffer);
    kfree(sfatfs->hash_list);
    kfree(sfatfs);
    return 0;
}
//sync the fs
static void
sfatfs_cleanup(struct fs *fs) {
    struct sfatfs_fs *sfatfs = fsop_info(fs, sfatfs);
    uint32_t blocks = sfatfs->super.blocks, unused_blocks = sfatfs->super.unused_blocks;
    kprintf("sfatfs: cleanup: '%s' (%d/%d/%d)\n", sfatfs->super.info,
            blocks - unused_blocks, unused_blocks, blocks);
    int i, ret;
    for (i = 0; i < 32; i ++) {
        if ((ret = fsop_sync(fs)) == 0) {
            break;
        }
    }
    if (ret != 0) {
        warn("sfatfs: sync error: '%s': %e.\n", sfatfs->super.info, ret);
    }
}
/*
 * Read a block with index 'blkno' into blk_buffer
 * Used by function sfatfs_do_mount() to fetch the meta data(block 0 & 1) of the fs.
 */
static int
sfatfs_init_read(struct device *dev, uint32_t blkno, void *blk_buffer) {
    struct iobuf __iob, *iob = iobuf_init(&__iob, blk_buffer, SFATFS_BLKSIZE, blkno * SFATFS_BLKSIZE);
    return dop_io(dev, iob, 0);
}
/*
 * Mount sfatfs to device 'dev', store the fs to 'fs_store' when return.
 * It allocates a new fs struct, fetch the meta data from IDE drive.
 */
static int
sfatfs_do_mount(struct device *dev, struct fs **fs_store) {
    if (dev->d_blocksize != SFATFS_BLKSIZE) {
        return -E_NA_DEV;
    }

    /* allocate fs structure */
    struct fs *fs;
    if ((fs = alloc_fs(sfatfs)) == NULL) {
        return -E_NO_MEM;
    }
    // kprintf("location of fs outside: %016lx\n", fs);
    // kprintf("loc of type %016lx\n", &(fs->fs_type));
    // kprintf("SIZE of it %d\n", sizeof(fs->fs_info));

	// 	kprintf("LL %x %x %x %x %x %x\n", &fs->fs_info.__pipe_info, &fs->fs_info.__sfs_info,
    //     	&fs->fs_info.__sfatfs_info, &fs->fs_info.__sfatfs_info.sfatfs_buffer,
    //         &fs->fs_info.__sfatfs_info.fat, &fs->fs_info.__sfatfs_info.mutex_sem);
	// 	kprintf("SZ %d %d %d %d %d %d %d %d %d %d %d %d\n", sizeof(struct pipe_fs), sizeof(struct sfs_fs),
    //    		sizeof(struct sfatfs_fs), sizeof(struct sfatfs_super), sizeof(struct device),
	// 		   sizeof(struct sfatfs_disk_inode), sizeof(semaphore_t), sizeof(atomic_t), sizeof(struct spinlock_s), 
	// 		   sizeof(wait_queue_t), sizeof(int), sizeof(bool));
    struct sfatfs_fs *sfatfs = fsop_info(fs, sfatfs);
    sfatfs->dev = dev;

    int ret = -E_NO_MEM;

    void *sfatfs_buffer;
    if ((sfatfs->sfatfs_buffer = sfatfs_buffer = kmalloc(SFATFS_BLKSIZE)) == NULL) {
        goto failed_cleanup_fs;
    }

    /* load and check superblock */
    if ((ret = sfatfs_init_read(dev, SFATFS_BLKN_SUPER, sfatfs_buffer)) != 0) {
        goto failed_cleanup_sfatfs_buffer;
    }

    ret = -E_INVAL;

    struct sfatfs_super *super = sfatfs_buffer;
    if (super->magic != SFATFS_MAGIC) {
        kprintf("sfatfs: wrong magic in superblock. (%08x should be %08x).\n",
                super->magic, SFATFS_MAGIC);
        goto failed_cleanup_sfatfs_buffer;
    }
    if (super->blocks > dev->d_blocks) {
        kprintf("sfatfs: fs has %u blocks, device has %u blocks.\n",
                super->blocks, dev->d_blocks);
        goto failed_cleanup_sfatfs_buffer;
    }
	/* Init sfatfs, copy all the dir entries */
    super->info[SFATFS_MAX_INFO_LEN] = '\0';
    sfatfs->super = *super;
	struct sfatfs_disk_inode *dentrys;
	uint16_t sizeinbyte = sizeof(struct sfatfs_disk_inode) * super->ndirentry;
	if ((sfatfs->dinos = dentrys = kmalloc(sizeof(struct sfatfs_disk_inode) * SFATFS_FILE_NENTRY)) == NULL) {
		goto failed_cleanup_sfatfs_buffer;
	}
	unsigned char *fcp = ((unsigned char *)sfatfs_buffer) + 512;
	unsigned char *tcp = (unsigned char *)dentrys;
	while (sizeinbyte --) *tcp ++ = *fcp ++;

    ret = -E_NO_MEM;

    uint32_t i;

    /* alloc and initialize hash list */
    list_entry_t *hash_list;
    if ((sfatfs->hash_list = hash_list = kmalloc(sizeof(list_entry_t) * SFATFS_HLIST_SIZE)) == NULL) {
        goto failed_cleanup_sfatfs_buffer;
    }
    for (i = 0; i < SFATFS_HLIST_SIZE; i ++) {
        list_init(hash_list + i);
    }

    /* load and check freemap(FAT) */
	uint16_t *fat;
	if ((sfatfs->fat = fat = kmalloc(sizeof(uint16_t) * SFATFS_BLK_NENTRY)) == NULL) {
		goto failed_cleanup_hash_list;
	}
    if ((ret = sfatfs_init_read(dev, SFATFS_BLKN_FAT, fat)) != 0) {
        goto failed_cleanup_fat;
    }
    uint32_t blocks = sfatfs->super.blocks, unused_blocks = 0;
	uint16_t *tfat;
	for (tfat = fat; tfat < fat + sfatfs->super.blocks; tfat ++) {
		if (*tfat == FAT_FREE) unused_blocks ++;
	}
    assert(unused_blocks == sfatfs->super.unused_blocks);

    /* and other fields */
    sfatfs->super_dirty = 0;
    sem_init(&(sfatfs->fs_sem), 1);
    sem_init(&(sfatfs->io_sem), 1);
    sem_init(&(sfatfs->mutex_sem), 1);
    list_init(&(sfatfs->inode_list));
    kprintf("sfatfs: mount: '%s' (%d/%d/%d)\n", sfatfs->super.info,
            blocks - unused_blocks, unused_blocks, blocks);

    fs->fs_sync = sfatfs_sync;
    fs->fs_get_root = sfatfs_get_root;
    fs->fs_unmount = sfatfs_unmount;
    fs->fs_cleanup = sfatfs_cleanup;
    *fs_store = fs;
    return 0;

failed_cleanup_fat:
	kfree(fat);
failed_cleanup_hash_list:
    kfree(hash_list);
failed_cleanup_sfatfs_buffer:
    kfree(sfatfs_buffer);
failed_cleanup_fs:
    kfree(fs);
    return ret;
}
//Up-levvel interface
int
sfatfs_mount(const char *devname) {
    return vfs_mount(devname, sfatfs_do_mount);
}

