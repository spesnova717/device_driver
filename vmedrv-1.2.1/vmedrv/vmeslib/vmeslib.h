/* vmeslib.h */
/* Created by Enomoto Sanshiro on 26 October 2005. */
/* Last updated by Enomoto Sanshiro on 20 July 2010. */


#ifndef __VMESLIB_H_INCLUDED
#define __VMESLIB_H_INCLUDED 1

#include <sys/types.h>
#include "vmeslib_internals.h"


enum vme_access_mode_t {
    VME_A16D16, VME_A16D32,
    VME_A24D16, VME_A24D32,
    VME_A32D16, VME_A32D32
};

enum vme_transfer_mode_t {
    VME_NORMAL, VME_DMA, VME_NBDMA
};


#ifdef __cplusplus
extern "C" {
#endif


VMEIO* vme_open(int access_mode, int transfer_mode);
int vme_close(VMEIO* handle);
int vme_read(VMEIO* handle, unsigned vme_address, void* buffer, size_t count);
int vme_write(VMEIO* handle, unsigned vme_address, const void* buffer, size_t count);

VMEMAP* vme_mapopen(int access_mode, unsigned vme_address, size_t size);
int vme_mapclose(VMEMAP* map_handle);
#define vme_mapbase(map_handle) ((void*) map_handle->base)
#define vme_word16(map_handle, offset) ((short*) (map_handle->base+(offset)))
#define vme_word32(map_handle, offset) ((int*) (map_handle->base+(offset)))

VMEINT* vme_intopen(int intrrupt_number, int vector);
int vme_intclose(VMEINT* int_handle);
int vme_intsetautodisable(VMEINT* int_handle);
int vme_intsetvectormask(VMEINT* int_handle, int vector_mask);
int vme_intenable(VMEINT* int_handle);
int vme_intdisable(VMEINT* int_handle);
int vme_intwait(VMEINT* int_handle, int timeout_sec);
int vme_intnowait(VMEINT* int_handle);
int vme_intcheck(VMEINT* int_handle);
int vme_intclear(VMEINT* int_handle);

int vme_tryget(int access_mode, unsigned vme_address);


#ifdef __cplusplus
}
#endif


#endif
