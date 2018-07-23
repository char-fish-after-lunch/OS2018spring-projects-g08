#include <assert.h>
#include <types.h>
#include <fs.h>
#include <ramdisk.h>
#include <stdio.h>
#include <kio.h>
#include <string.h>
#include <memlayout.h>


#define MIN(x, y) (((x) < (y)) ? (x) : (y))


bool check_initrd() {
    if (initrd_begin == initrd_end) {
        kprintf("Warning: No Initrd!\n");
        return 0;
    }
    kprintf("Initrd: 0x%08x - 0x%08x, size: 0x%08x\n", initrd_begin,
            initrd_end - 1, initrd_end - initrd_begin);
    return 1;
}

bool check_initrd_cp() {
    if (initrd_cp_begin == initrd_cp_end) {
        kprintf("Warning: No Initrd_cp!\n");
        return 0;
    }
    kprintf("Initrd_cp: 0x%08x - 0x%08x, size: 0x%08x\n", initrd_cp_begin,
            initrd_cp_end - 1, initrd_cp_end - initrd_cp_begin);
    return 1;
}

bool check_swaprd() {
    if (swaprd_begin == swaprd_end) {
        kprintf("Warning: No Swaprd!\n");
        return 0;
    }
    kprintf("Swaprd: 0x%08x - 0x%08x, size: 0x%08x\n", swaprd_begin,
            swaprd_end - 1, swaprd_end - swaprd_begin);
    return 1;
}

static int ramdisk_read(struct ide_device *dev, size_t secno, void *dst,
                        size_t nsecs) {
    nsecs = MIN(nsecs, dev->size - secno);
    if (nsecs < 0) return -1;
    memcpy(dst, (void *)(dev->iobase + secno * SECTSIZE), nsecs * SECTSIZE);
    return 0;
}

static int ramdisk_write(struct ide_device *dev, size_t secno, const void *src,
                         size_t nsecs) {
    nsecs = MIN(nsecs, dev->size - secno);
    if (nsecs < 0) return -1;
    memcpy((void *)(dev->iobase + secno * SECTSIZE), src, nsecs * SECTSIZE);
    return 0;
}

void ramdisk_init(int devno, struct ide_device *dev) {
    memset(dev, 0, sizeof(struct ide_device));
    assert(INITRD_SIZE() % SECTSIZE == 0);
    kprintf("INITRD_SIZE():0x%x\n", INITRD_SIZE());
    assert(INITRD_CP_SIZE() % SECTSIZE == 0);
    kprintf("INITRD_CP_SIZE():0x%x\n", INITRD_CP_SIZE());
#ifdef UCONFIG_SWAP    
    assert(SWAPRD_SIZE() % SECTSIZE == 0);
    kprintf("SWAPRD_SIZE():0x%x\n", SWAPRD_SIZE());
#endif

    if (devno == SWAP_DEV_NO) {
    #ifndef UCONFIG_SWAP
        panic("Swap is not config!\n");
    #endif
        if (CHECK_SWAPRD_EXIST()) {
            dev->valid = 1;
            dev->sets = ~0;
            dev->size = (unsigned int)SWAPRD_SIZE() / SECTSIZE;
            dev->iobase = (void *) DISK_SWAP_VBASE;
            strcpy(dev->model, "KERN_INITRD");
            dev->read_secs = ramdisk_read;
            dev->write_secs = ramdisk_write;
	    }
    } else if (devno == DISK0_DEV_NO) {
        if (CHECK_INITRD_EXIST()) {
            dev->valid = 1;
            dev->sets = ~0;
            dev->size = (unsigned int)INITRD_SIZE() / SECTSIZE;
            dev->iobase = (void *) DISK_FS_VBASE;
            strcpy(dev->model, "KERN_INITRD");
            dev->read_secs = ramdisk_read;
            dev->write_secs = ramdisk_write;
        }
    } else if (devno == DISK1_DEV_NO) {
        if (CHECK_INITRD_CP_EXIST()) {
            dev->valid = 1;
            dev->sets = ~0;
            dev->size = (unsigned int)INITRD_CP_SIZE() / SECTSIZE;
            dev->iobase = (void *) DISK2_FS_VBASE;
            strcpy(dev->model, "KERN_INITRD_CP");
            dev->read_secs = ramdisk_read;
            dev->write_secs = ramdisk_write;
        }
    } else {
        panic("Device Not Found");
    }
}
