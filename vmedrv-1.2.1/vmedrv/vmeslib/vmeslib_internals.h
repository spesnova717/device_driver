/* vmeslib.h */
/* Created by Enomoto Sanshiro on 26 October 2005. */
/* Last updated by Enomoto Sanshiro on 20 July 2010. */


#ifndef __VMESLIB_PRIVATES_H_INCLUDED
#define __VMESLIB_PRIVATES_H_INCLUDED 1

#include <sys/types.h>


typedef struct vme_io_handle_t {
    int fd;
} VMEIO;


typedef struct vme_map_handle_t {
    int fd;
    unsigned vme_address;
    size_t size;
    unsigned map_base;
    unsigned map_offset;
    unsigned map_size;
    void* mapped_address;
    void* base;
} VMEMAP;


typedef struct vme_int_handle_t {
    int fd;
    int interrupt_number, vector;
} VMEINT;


#endif
