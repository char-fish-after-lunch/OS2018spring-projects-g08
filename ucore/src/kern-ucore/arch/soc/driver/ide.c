#include <types.h>
#include <stdio.h>
#include <trap.h>
#include <picirq.h>
#include <fs.h>
#include <ide.h>
#include <arch.h>
#include <sem.h>
#include <assert.h>
#include <kio.h>
#include <string.h>

static int ide_wait_ready(unsigned short iobase, bool check_error) { return 0; }

void ide_init(void) {
    static_assert((SECTSIZE % 4) == 0);
    for (int ideno = 0; ideno < MAX_IDE; ++ideno) {
        ide_devices[ideno].valid = 0;
    }
#ifdef UCONFIG_SWAP
    ramdisk_init(SWAP_DEV_NO, &ide_devices[SWAP_DEV_NO]);
    assert(VALID_IDE(SWAP_DEV_NO));
#endif
    ramdisk_init(DISK0_DEV_NO, &ide_devices[DISK0_DEV_NO]);
    assert(VALID_IDE(DISK0_DEV_NO));

    ramdisk_init(DISK1_DEV_NO, &ide_devices[DISK1_DEV_NO]);
    assert(VALID_IDE(DISK1_DEV_NO));
}

bool ide_device_valid(unsigned short ideno) {
    return VALID_IDE(ideno); 
}

size_t ide_device_size(unsigned short ideno) {
    if (ide_device_valid(ideno)) {
        return ide_devices[ideno].size;
    }
    return 0;
}

int ide_read_secs(unsigned short ideno, uint32_t secno, void *dst,
                  size_t nsecs) {
    assert(nsecs <= MAX_NSECS && VALID_IDE(ideno));
    assert(secno < MAX_DISK_NSECS && secno + nsecs <= MAX_DISK_NSECS);
    return ide_devices[ideno].read_secs(&ide_devices[ideno], secno, dst, nsecs);
}

int ide_write_secs(unsigned short ideno, uint32_t secno, const void *src,
                   size_t nsecs) {
    assert(nsecs <= MAX_NSECS && VALID_IDE(ideno));
    assert(secno < MAX_DISK_NSECS && secno + nsecs <= MAX_DISK_NSECS);
    return ide_devices[ideno].write_secs(&ide_devices[ideno], secno, src,
                                         nsecs);
}
