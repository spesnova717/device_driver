/* vmelib.h */
/* KEK VMELIB compatible interface to vmedrv */
/* Created by Enomoto Sanshiro on 18 October 2005. */
/* Last updated by Enomoto Sanshiro on 18 October 2005. */


#ifndef __VMELIB_H_INCLUDED
#define __VMELIB_H_INCLUDED 1

#include <sys/types.h>


enum mode_t {
    VME_A16D16MODE,
    VME_A16D32MODE,
    VME_A24D16MODE,
    VME_A24D32MODE,
    VME_A32D16MODE,
    VME_A32D32MODE
};

enum dma_dev_t {
    MEMORY,
    FIFO
};


caddr_t vme_mapopen(int mode, off_t vmeaddr, size_t size);
int vme_mapclose(caddr_t addr, int mode);
void vme_mapprobe(off_t addr, size_t size, int mode);

int vme_dmaopen(int mode, int dma_dev);
int vme_dmaclose(int mode);
int vme_dmaread(int vmeaddr, void *buf, size_t count);
int vme_dmawrite(int vmeaddr, void *buf, size_t count);

int vme_intopen(int vector, int* intrno);
int vme_intclose(int intrno);
int vme_intenable(int intrno);
int vme_intdisable(int intrno);
int vme_intwait(int intrno, int* interrupt_count, int timeout);
int vme_intnowait(int intrno, int* interrupt_count);
int vme_intclear(int intrno);
int vme_intcheck(int intrno, int* interrupt_count);


#endif
