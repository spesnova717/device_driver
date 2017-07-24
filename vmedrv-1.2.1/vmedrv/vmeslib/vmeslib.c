/* vmeslib.c */
/* Created by Enomoto Sanshiro on 26 October 2005. */
/* Last updated by Enomoto Sanshiro on 20 July 2010. */

#define _LARGEFILE64_SOURCE

#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include "vmedrv.h"
#include "vmeslib.h"

#if 0
#include <asm/page.h>
#else
#define PAGE_SHIFT	12
#define PAGE_SIZE	(1UL << PAGE_SHIFT)
#define PAGE_MASK	(~(PAGE_SIZE-1))
#endif


static const char* pio_device_file[] = {
    "/dev/vmedrv16d16", "/dev/vmedrv16d32",
    "/dev/vmedrv24d16", "/dev/vmedrv24d32",
    "/dev/vmedrv32d16", "/dev/vmedrv32d32"
};

static const char* dma_device_file[] = {
    "/dev/vmedrv16d16", "/dev/vmedrv16d32",
    "/dev/vmedrv24d16dma", "/dev/vmedrv24d32dma",
    "/dev/vmedrv32d16dma", "/dev/vmedrv32d32dma"
};

static const char* nbdma_device_file[] = {
    "/dev/vmedrv16d16", "/dev/vmedrv16d32",
    "/dev/vmedrv24d16nbdma", "/dev/vmedrv24d32nbdma",
    "/dev/vmedrv32d16nbdma", "/dev/vmedrv32d32nbdma"
};

static const char** device_file_table[] = {
    pio_device_file,
    dma_device_file,
    nbdma_device_file,
};


VMEIO* vme_open(int access_mode, int transfer_mode)
{
    const char* device_file;
    VMEIO* handle;

    handle = (VMEIO*) malloc(sizeof(VMEMAP));
    if (handle == NULL) {
	errno = ENOMEM;
	return NULL;
    }

    device_file = device_file_table[transfer_mode][access_mode];
    if ((handle->fd = open(device_file, O_RDWR)) == -1) {
	free(handle);
	return NULL;
    }

    return handle;
}

int vme_close(VMEIO* handle)
{
    if (handle == NULL) {
	errno = ENODEV;
	return -1;
    }

    close(handle->fd);
    
    free(handle);
    handle = NULL;

    return 0;
}

int vme_read(VMEIO* handle, unsigned vme_address, void* buffer, size_t count)
{
    int read_size;
    if (lseek64(handle->fd, vme_address, SEEK_SET) == -1) {
	return -1;
    }

    if ((read_size = read(handle->fd, buffer, count)) == -1) {
	return -1;
    }

    return read_size;
}

int vme_write(VMEIO* handle, unsigned vme_address, const void* buffer, size_t count)
{
    int written_size;
    if (lseek64(handle->fd, vme_address, SEEK_SET) == -1) {
	return -1;
    }

    if ((written_size = write(handle->fd, buffer, count)) == -1) {
	return -1;
    }

    return written_size;
}



VMEMAP* vme_mapopen(int mode, unsigned vme_address, size_t size)
{
    VMEMAP* handle;

    handle = (VMEMAP*) malloc(sizeof(VMEMAP));
    if (handle == NULL) {
	errno = ENOMEM;
	return NULL;
    }

    if ((handle->fd = open(pio_device_file[mode], O_RDWR)) == -1) {
	free(handle);
	return NULL;
    }

    handle->vme_address = vme_address;
    handle->size = size;

    handle->map_base = vme_address & PAGE_MASK;
    handle->map_offset = vme_address - handle->map_base;
    handle->map_size = size + handle->map_offset;

    handle->mapped_address = mmap(
	0, handle->map_size, PROT_WRITE, MAP_SHARED, 
	handle->fd, handle->map_base
    );
    if (handle->mapped_address == MAP_FAILED) {
	close(handle->fd);
	free(handle);
	return NULL;
    }

    handle->base = handle->mapped_address + handle->map_offset;

    return handle;
}

int vme_mapclose(VMEMAP* handle)
{
    if (handle == NULL) {
	errno = ENODEV;
	return -1;
    }

    munmap(handle->mapped_address, handle->map_size);
    close(handle->fd);

    free(handle);
    handle = NULL;

    return 0;
}



VMEINT* vme_intopen(int interrupt_number, int vector)
{
    VMEINT* handle;
    struct vmedrv_interrupt_property_t interrupt_property;
    int result;

    handle = (VMEINT*) malloc(sizeof(VMEMAP));
    if (handle == NULL) {
	errno = ENOMEM;
	return NULL;
    }

    if ((handle->fd = open("/dev/vmedrv", O_RDWR)) == -1) {
	free(handle);
	return NULL;
    }

    interrupt_property.irq = interrupt_number;
    interrupt_property.vector = vector;
    interrupt_property.signal_id = 0;
    result = ioctl(
	handle->fd, VMEDRV_IOC_REGISTER_INTERRUPT, &interrupt_property
    );
    if (result == -1) {
	close(handle->fd);
	free(handle);
	return NULL;
    }

    handle->interrupt_number = interrupt_number;
    handle->vector = vector;

    return handle;
}

int vme_intclose(VMEINT* handle)
{
    if (handle == NULL) {
	errno = ENODEV;
	return -1;
    }

    close(handle->fd);

    free(handle);
    handle = NULL;

    return 0;
}

int vme_intsetautodisable(VMEINT* handle)
{
    struct vmedrv_interrupt_property_t interrupt_property;
    int result;

    interrupt_property.irq = handle->interrupt_number;
    interrupt_property.vector = handle->vector;

    result = ioctl(
	handle->fd, VMEDRV_IOC_SET_INTERRUPT_AUTODISABLE, &interrupt_property
    );

    return (result < 0) ? -1 : 0;
}

int vme_intsetvectormask(VMEINT* handle, int vector_mask)
{
    struct vmedrv_interrupt_property_t interrupt_property;
    int result;

    interrupt_property.irq = handle->interrupt_number;
    interrupt_property.vector = handle->vector;
    interrupt_property.vector_mask = vector_mask;

    result = ioctl(
	handle->fd, VMEDRV_IOC_SET_VECTOR_MASK, &interrupt_property
    );

    return (result < 0) ? -1 : 0;
}

int vme_intenable(VMEINT* handle)
{
    if (ioctl(handle->fd, VMEDRV_IOC_ENABLE_INTERRUPT) == -1) {
	return -1;
    }

    return 0;
}

int vme_intdisable(VMEINT* handle)
{
    if (ioctl(handle->fd, VMEDRV_IOC_DISABLE_INTERRUPT) == -1) {
	return -1;
    }

    return 0;
}

int vme_intwait(VMEINT* handle, int timeout_sec)
{
    struct vmedrv_interrupt_property_t interrupt_property;
    int result;

    interrupt_property.irq = handle->interrupt_number;
    interrupt_property.vector = handle->vector;
    interrupt_property.timeout = timeout_sec;

    result = ioctl(
	handle->fd, VMEDRV_IOC_WAIT_FOR_INTERRUPT, &interrupt_property
    );
    if (result < 0) {
	if (errno == ETIMEDOUT) {
	    result = 0;
	}
	else {
	    result = -1;
	}
    }

    return result;
}

int vme_intnowait(VMEINT* handle)
{
    int interrupt_count;

    if ((interrupt_count = vme_intcheck(handle)) == -1) {
	return -1;
    }

    if (interrupt_count > 0) {
	if (vme_intclear(handle) == -1) {
	    return -1;
	}
    }

    return interrupt_count;
}

int vme_intcheck(VMEINT* handle)
{
    struct vmedrv_interrupt_property_t interrupt_property;
    int result;

    interrupt_property.irq = handle->interrupt_number;
    interrupt_property.vector = handle->vector;

    result = ioctl(
	handle->fd, VMEDRV_IOC_CHECK_INTERRUPT, &interrupt_property
    );
    if (result == -1) {
	return -1;
    }

    return result;  /* number of interrupts being requested */
}

int vme_intclear(VMEINT* handle)
{
    struct vmedrv_interrupt_property_t interrupt_property;
    int result;

    interrupt_property.irq = handle->interrupt_number;
    interrupt_property.vector = handle->vector;

    result = ioctl(
	handle->fd, VMEDRV_IOC_CLEAR_INTERRUPT, &interrupt_property
    );

    return result;
}



int vme_tryget(int access_mode, unsigned vme_address)
{
    const char* device_file;
    int fd;
    vmedrv_word_access_t word_access;

    device_file = device_file_table[VME_NORMAL][access_mode];
    if ((fd = open(device_file, O_RDWR)) == -1) {
	return -1;
    }

    word_access.address = vme_address;
    if (ioctl(fd, VMEDRV_IOC_PROBE, &word_access) == -1) {
	close(fd);
	errno = EIO;
	return -1;
    }

    close(fd);

    return 0;
}
