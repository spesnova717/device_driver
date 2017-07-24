/* vmedrv.h */
/* VME device driver for Bit3 Model 617 on Linux 2.0.x */
/* Created by Enomoto Sanshiro on 28 November 1999. */
/* Last updated by Glenn H-S. on 28 May 2000 */


#ifndef __VMEDRV_H__
#define __VMEDRV_H__

#include <linux/ioctl.h>


typedef enum vmedrv_access_modes_t {
    VMEDRV_A16D16,
    VMEDRV_A16D32,
    VMEDRV_A24D16,
    VMEDRV_A24D32,
    VMEDRV_A32D16,
    VMEDRV_A32D32,
    vmedrv_NUMBER_OF_ACCESS_MODES
} vmedrv_access_mode_t;


typedef enum vmedrv_transfer_methods_t {
    VMEDRV_PIO,
    VMEDRV_DMA,
    vmedrv_NUMBER_OF_TRANSFER_METHODS
} vmedrv_transfer_methods_t;


/* 0 or negative signal_id is used to indicate wait-for-interrupt */
/* timeout is used only for wait-for-interrupt */
typedef struct vmedrv_interrupt_property_t {
    int irq;
    int vector;
    int signal_id;
    int timeout;
} vmedrv_interrupt_property_t;


typedef struct vmedrv_word_access_t {
    unsigned long address;
    unsigned long data;
} vmedrv_word_access_t;


#define VMEDRV_IOC_MAGIC 0xB3

#define VMEDRV_IOC_SET_ACCESS_MODE            _IOW(VMEDRV_IOC_MAGIC, 1, int)
#define VMEDRV_IOC_SET_TRANSFER_METHOD        _IOW(VMEDRV_IOC_MAGIC, 2, int)
#define VMEDRV_IOC_REGISTER_INTERRUPT         _IOW(VMEDRV_IOC_MAGIC, 3, vmedrv_interrupt_property_t)
#define VMEDRV_IOC_UNREGISTER_INTERRUPT       _IOW(VMEDRV_IOC_MAGIC, 4, vmedrv_interrupt_property_t)
#define VMEDRV_IOC_ENABLE_INTERRUPT           _IO(VMEDRV_IOC_MAGIC, 5)
#define VMEDRV_IOC_DISABLE_INTERRUPT          _IO(VMEDRV_IOC_MAGIC, 6)
#define VMEDRV_IOC_ENABLE_ERROR_INTERRUPT     _IO(VMEDRV_IOC_MAGIC, 7)
#define VMEDRV_IOC_DISABLE_ERROR_INTERRUPT    _IO(VMEDRV_IOC_MAGIC, 8)
#define VMEDRV_IOC_WAIT_FOR_INTERRUPT         _IOW(VMEDRV_IOC_MAGIC, 9, vmedrv_interrupt_property_t)


#endif
