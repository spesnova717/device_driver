/* vmedrv.c */
/* VME device driver for Bit3 Model 617/618/620 on Linux 2.2.x */
/* Created by Enomoto Sanshiro on 28 November 1999. */
/* Last updated by Enomoto Sanshiro on 14 September 2000 */


#define __KERNEL__
#define MODULE


#ifdef TRACE_CONFIG
#define DEBUG(x) x
#else
#define DEBUG(x)
#endif

#ifdef TRACE_MAP
#define DEBUG_MAP(x) x
#else
#define DEBUG_MAP(x)
#endif

#ifdef TRACE_INTERRUPT
#define DEBUG_INT(x) x
#else
#define DEBUG_INT(x)
#endif

#ifdef TRACE_DMA
#define DEBUG_DMA(x) x
#else
#define DEBUG_DMA(x)
#endif

#include <linux/autoconf.h>
#if defined(USE_MODVERSIONS) && USE_MODVERSIONS
#  ifdef CONFIG_MODVERSIONS
#    define MODVERSIONS
#    include <linux/modversions.h>
#  endif
#endif


#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/malloc.h>
#include <linux/ioctl.h>
#include <linux/mm.h>
#include <linux/fs.h>
#include <linux/sched.h>
#include <linux/config.h>
#include <linux/pci.h>
#include <linux/ioport.h>
#include <linux/errno.h>
#include <asm/io.h>
#include <asm/uaccess.h>
#include <asm/segment.h>
#include "vmedrv.h"
#include "vmedrv_params.h"
#include "vmedrv_conf.h"


struct pci_config_t {
    unsigned ioports[4];
    unsigned char irq;
};

struct interrupt_client_t {
    struct task_struct* task;
    int irq;
    int vector;
    int signal_id;
    int interrupt_count;
    int autodisable_flag;
    int vector_mask;
    struct interrupt_client_t* next;
};

struct bit3_config_t {
    int dev_id;
    unsigned char irq;
    unsigned io_node_io_base;
    unsigned long mapped_node_io_base;
    unsigned long mapping_registers_base;
    unsigned long dma_mapping_registers_base;
    unsigned long window_region_base;
    unsigned long physical_window_region_base;
    char window_status_table[bit3_NUMBER_OF_WINDOWS];
    struct interrupt_client_t* interrupt_client_list[vmeNUMBER_OF_IRQ_LINES];
    unsigned interrupt_enabling_flags;
    unsigned saved_interrupt_flags;
    void* dma_buffer;
    unsigned long dma_buffer_size;
    unsigned long dma_buffer_bus_address;
    unsigned long dma_buffer_mapped_pci_address;
    int is_ready;
};

struct dev_prop_t {
    unsigned address_modifier;
    unsigned dma_address_modifier;
    unsigned function_code;
    unsigned byte_swapping;
    unsigned mapping_flags;
    unsigned dma_mapping_flags;
    unsigned data_width;
    unsigned transfer_method;
    void* pio_buffer;
    int pio_window_index;
    int number_of_pio_windows;
    int mmap_window_index;
    int number_of_mmap_windows;
};


static int vmedrv_open(struct inode* inode, struct file* filep);
static int vmedrv_release(struct inode* inode, struct file* filep);
static ssize_t vmedrv_read(struct file* filep, char* buf, size_t count, loff_t* f_pos);
static ssize_t vmedrv_write(struct file* filep, const char* buf, size_t count, loff_t* f_pos);
static loff_t vmedrv_lseek(struct file* filep, loff_t offset, int whence);
static int vmedrv_ioctl(struct inode *inode, struct file *filep, unsigned int cmd, unsigned long arg);
static int vmedrv_mmap(struct file* filep, struct vm_area_struct* vma);
static void vmedrv_interrupt(int irq, void* dev_id, struct pt_regs* regs);


static int detect_pci_device(unsigned vendor, unsigned device, struct pci_config_t* pci_config);
static int initialize(struct pci_config_t* pci_config);
static int set_access_mode(struct dev_prop_t* dev_prop, int mode);
static int set_transfer_method(struct dev_prop_t* dev_prop, int method);

static int pio_read(struct dev_prop_t* dev_prop, char* buf, unsigned long vme_address, int count);
static int pio_write(struct dev_prop_t* dev_prop, const char* buf, unsigned long vme_address, int count);
static int prepare_pio(struct dev_prop_t* dev_prop);
static int allocate_windows(int number_of_windows);
static void free_windows(int window_index, int number_of_windows);
static unsigned map_windows(unsigned address, unsigned window_index, unsigned number_of_windows, unsigned flags);

static int enable_normal_interrupt(void);
static int disable_normal_interrupt(void);
static int enable_error_interrupt(void);
static int disable_error_interrupt(void);
static void save_interrupt_flags(void);
static void restore_interrupt_flags(void);
static int acknowledge_error_interrupt(unsigned local_status);
static int acknowledge_pt_interrupt(unsigned local_status);
static int acknowledge_dma_interrupt(unsigned dma_status);
static int acknowledge_pr_interrupt(unsigned remote_status);
static int acknowledge_vme_interrupt(unsigned interrupt_status);
static int register_interrupt_notification(struct task_struct* task, int irq, int vector, int signal_id);
static int unregister_interrupt_notification(struct task_struct* task, int irq, int vector);
static int wait_for_interrupt_notification(struct task_struct* task, int irq, int vector, int timeout);
static int check_interrupt_notification(int irq, int vector);
static int clear_interrupt_notification(int irq, int vector);
static int set_interrupt_autodisable(int irq, int vector);
static int set_vector_mask(int irq, int vector, int vector_mask);
static int reset_adapter(void);

static int dma_read(struct dev_prop_t* dev_prop, char* buf, unsigned long vme_adress, int count);
static int dma_write(struct dev_prop_t* dev_prop, const char* buf, unsigned long vme_adress, int count);
static int prepare_dma(struct dev_prop_t* dev_prop);
static int start_dma(struct dev_prop_t* dev_prop, unsigned long pci_address, unsigned long vme_address, unsigned long size, int direction);
static void* allocate_dma_buffer(unsigned long size);
static void release_dma_buffer(void);
static unsigned map_dma_windows(unsigned pci_address, unsigned size, unsigned dma_flags);
static int initiate_dma(struct dev_prop_t* dev_prop, unsigned mapped_pci_address, unsigned vme_address, unsigned size, int direction);
static int release_dma(void);


static struct file_operations vmedrv_fops = {
    vmedrv_lseek,      /* vmedrv_lseek */
    vmedrv_read,       /* vmedrv_read */
    vmedrv_write,      /* vmedrv_write */
    NULL,              /* vmedrv_readdir */
    NULL,              /* vmedrv_poll */
    vmedrv_ioctl,      /* vmedrv_ioctl */
    vmedrv_mmap,       /* vmedrv_mmap */
    vmedrv_open,       /* vmedrv_open */
    NULL,              /* vmedrv_flush */
    vmedrv_release,    /* vmedrv_release */
};

static struct wait_queue* vmedrv_dmadone_wait_queue = NULL;
static struct wait_queue* vmedrv_vmebusirq_wait_queue = NULL;

static struct pci_config_t pci_config;
static struct bit3_config_t bit3;


int init_module(void)
{
    int result = -ENODEV;
    int vender_id, device_id;
    const char* model_name = "";
    int index, number_of_devices;

    bit3.is_ready = 0;

    number_of_devices = sizeof(vmedrv_device_id_table) / sizeof(vmedrv_device_id_table[0]);
    for (index = 0; index < number_of_devices; index++) {
	vender_id = vmedrv_device_id_table[index].vender_id;
	device_id = vmedrv_device_id_table[index].device_id;	  
	model_name = vmedrv_device_id_table[index].model_name;

	result = detect_pci_device(vender_id, device_id, &pci_config);
	if (result == 0) {
	    break;
	}
    }
    if (result != 0) {
	printk(KERN_WARNING "%s: unable to find VME-PCI Bus Adapter.\n", vmedrv_name);
	return -ENODEV;
    }
    printk(KERN_INFO "%s: %s is detected at ioport 0x%04x on irq %d.\n", 
	   vmedrv_name, model_name, pci_config.ioports[0], pci_config.irq
    );

    result = register_chrdev(vmedrv_major, vmedrv_name, &vmedrv_fops);
    if (result < 0) {
        printk(KERN_WARNING "%s: can't get major %d\n", vmedrv_name, vmedrv_major);
        return result;
    }
    if (vmedrv_major == 0) {
        vmedrv_major = result;
    }

    printk(KERN_INFO "  I/O Mapped Node at 0x%04x.\n", pci_config.ioports[0]);
    printk(KERN_INFO "  Memory Mapped Node at 0x%04x.\n", pci_config.ioports[1]);
    printk(KERN_INFO "  Mapping Register at 0x%04x.\n", pci_config.ioports[2]);
    printk(KERN_INFO "  Remote Memory at 0x%04x.\n", pci_config.ioports[3]);

    if (check_region(pci_config.ioports[0], bit3_IO_NODE_IO_SIZE)) {
        printk(KERN_WARNING "%s: ioport region conflicted.\n", vmedrv_name);
        return -EBUSY;
    }

    result = initialize(&pci_config);
    if (result < 0) {
        printk(KERN_WARNING "%s: initialization fault.\n", vmedrv_name);
        return result;
    }

    result = request_irq(bit3.irq, vmedrv_interrupt, SA_INTERRUPT | SA_SHIRQ, vmedrv_name, &bit3.dev_id);
    if (result < 0) {
        printk(KERN_WARNING "%s: unable to request IRQ.\n", vmedrv_name);
        return result;
    }
    request_region(bit3.io_node_io_base, bit3_IO_NODE_IO_SIZE,  vmedrv_name);

    bit3.is_ready = 1;
    bit3.interrupt_enabling_flags = 0;
    bit3.dma_buffer_size = 0;

    printk(KERN_INFO "%s: successfully installed at 0x%04x on irq %d (major = %d).\n", 
        vmedrv_name, pci_config.ioports[0], pci_config.irq, vmedrv_major
    );

    return 0;
}


void cleanup_module(void)
{
    disable_normal_interrupt();
    disable_error_interrupt();

    release_dma_buffer();

    iounmap(bit3.mapped_node_io_base);
    iounmap(bit3.mapping_registers_base);
    iounmap(bit3.window_region_base);

    free_irq(bit3.irq, &bit3.dev_id);
    release_region(bit3.io_node_io_base, bit3_IO_NODE_IO_SIZE);
    unregister_chrdev(vmedrv_major, vmedrv_name);

    printk(KERN_INFO "%s: removed.\n", vmedrv_name);
}


static int vmedrv_open(struct inode* inode, struct file* filep)
{
    struct dev_prop_t* dev_prop;
    int minor_id, mode, method;

    if (! bit3.is_ready) {
        return -ENODEV;
    }

    filep->private_data = kmalloc(sizeof(struct dev_prop_t), GFP_KERNEL);
    if (filep->private_data == 0) {
        printk(KERN_WARNING "%s: can't allocate memory.", vmedrv_name);
        return -ENODEV;
    }
    dev_prop = filep->private_data;

    dev_prop->pio_buffer = 0;
    dev_prop->number_of_pio_windows = 0;
    dev_prop->number_of_mmap_windows = 0;

    minor_id = MINOR(inode->i_rdev);
    if (minor_id >= vmedrv_NUMBER_OF_MINOR_IDS) {
	return -ENODEV;
    }
    else {
        mode = minor_to_access_mode[minor_id];
	set_access_mode(dev_prop, mode);
       
	method = minor_to_transfer_method[minor_id];
	set_transfer_method(dev_prop, method);
    }
    
    MOD_INC_USE_COUNT;

    return 0;
}


static int vmedrv_release(struct inode* inode, struct file* filep)
{
    struct dev_prop_t* dev_prop;
    dev_prop = filep->private_data;

    unregister_interrupt_notification(current, 0, 0);

    if (dev_prop->number_of_pio_windows > 0) {
        free_windows(dev_prop->pio_window_index, dev_prop->number_of_pio_windows);
    }
    if (dev_prop->number_of_mmap_windows > 0) {
        free_windows(dev_prop->mmap_window_index, dev_prop->number_of_mmap_windows);
    }

    if (dev_prop->pio_buffer > 0) {
	kfree(dev_prop->pio_buffer);
    }

    kfree(dev_prop);

    MOD_DEC_USE_COUNT;

    return 0;
}


static ssize_t vmedrv_read(struct file* filep, char* buf, size_t count, loff_t* f_pos)
{
    struct dev_prop_t* dev_prop;
    unsigned long vme_address;
    int total_read_size, read_size, remainder_size;

    dev_prop = filep->private_data;
    vme_address = *f_pos;
    read_size = 0;
    total_read_size = 0;
    remainder_size = count;

    if ((count % dev_prop->data_width) != 0) {
	return -EINVAL;
    }

    while (remainder_size > 0) {
	if (dev_prop->transfer_method == tmDMA) {
	    read_size = dma_read(dev_prop, buf, vme_address, remainder_size);
	}
	else if (dev_prop->transfer_method == tmPIO) {
	    read_size = pio_read(dev_prop, buf, vme_address, remainder_size);
	}

	if (read_size < 0) {
	    return read_size;
	}
	else if (read_size == 0) {
	    break;
	}

	remainder_size -= read_size;
	total_read_size += read_size;
	vme_address += read_size;
	buf += read_size;
    }

    *f_pos += total_read_size;

    return total_read_size;
}


static ssize_t vmedrv_write(struct file* filep, const char* buf, size_t count, loff_t* f_pos)
{
    struct dev_prop_t* dev_prop;
    unsigned long vme_address;
    int total_written_size, written_size, remainder_size;

    dev_prop = filep->private_data;
    vme_address = *f_pos;
    written_size = 0;
    total_written_size = 0;
    remainder_size = count;

    if ((count % dev_prop->data_width) != 0) {
	return -EINVAL;
    }

    while (remainder_size > 0) {
	if (dev_prop->transfer_method == tmDMA) {
	    written_size = dma_write(dev_prop, buf, vme_address, remainder_size);
	}
	else if (dev_prop->transfer_method == tmPIO) {
	    written_size = pio_write(dev_prop, buf, vme_address, remainder_size);
	}

	if (written_size < 0) {
	    return written_size;
	}
	else if (written_size == 0) {
	    break;
	}

	remainder_size -= written_size;
	total_written_size += written_size;
	vme_address += written_size;
	buf += written_size;
    }

    *f_pos += total_written_size;

    return total_written_size;
}


static loff_t vmedrv_lseek(struct file* filep, loff_t offset, int whence)
{
    switch (whence) {
      case 0: /* SEEK_SET */
	filep->f_pos = offset;
	break;
      case 1: /* SEEK_CUR */
	filep->f_pos += offset;
	break;
      case 2: /* SEEK_END */
	return -EINVAL;
      default:
	return -EINVAL;
    };

    return filep->f_pos;
}


static int vmedrv_ioctl(struct inode* inode, struct file *filep, unsigned int cmd, unsigned long arg)
{
    struct dev_prop_t* dev_prop;
    int argument_size;
    int result = -EINVAL;
    int data = 0;
    struct vmedrv_interrupt_property_t interrupt_property;
    int irq = 0, vector = 0, signal_id = 0, timeout = 0;
    int vector_mask = 0xffff;

    if (_IOC_TYPE(cmd) != VMEDRV_IOC_MAGIC) {
        return -EINVAL;
    }

    /* read arguments from user area */
    if (
	(cmd == VMEDRV_IOC_SET_ACCESS_MODE) || 
	(cmd == VMEDRV_IOC_SET_TRANSFER_METHOD)
    ){
	get_user_ret(data, (int*) arg, -EFAULT);
    }
    else if (
	(cmd == VMEDRV_IOC_REGISTER_INTERRUPT) ||
	(cmd == VMEDRV_IOC_UNREGISTER_INTERRUPT) ||
	(cmd == VMEDRV_IOC_WAIT_FOR_INTERRUPT) ||
	(cmd == VMEDRV_IOC_CHECK_INTERRUPT) ||
	(cmd == VMEDRV_IOC_CLEAR_INTERRUPT) ||
	(cmd == VMEDRV_IOC_SET_INTERRUPT_AUTODISABLE) ||
	(cmd == VMEDRV_IOC_SET_VECTOR_MASK)
    ){
	argument_size = sizeof(struct vmedrv_interrupt_property_t);
	copy_from_user_ret(
	    &interrupt_property, (void *) arg, argument_size, -EFAULT
	);

	irq = interrupt_property.irq;
	vector = interrupt_property.vector;
	signal_id = interrupt_property.signal_id;
	timeout = interrupt_property.timeout;
	vector_mask = interrupt_property.vector_mask;
    }

    dev_prop = filep->private_data;

    switch (cmd) {
      case VMEDRV_IOC_SET_ACCESS_MODE:
	result = set_access_mode(dev_prop, data);
        break;
      case VMEDRV_IOC_SET_TRANSFER_METHOD:
	result = set_transfer_method(dev_prop, data);
        break;
      case VMEDRV_IOC_REGISTER_INTERRUPT:
	result = register_interrupt_notification(current, irq, vector, signal_id);
        break;
      case VMEDRV_IOC_UNREGISTER_INTERRUPT:
	result = unregister_interrupt_notification(current, irq, vector);
        break;
      case VMEDRV_IOC_WAIT_FOR_INTERRUPT:
	result = wait_for_interrupt_notification(current, irq, vector, timeout);
        break;
      case VMEDRV_IOC_CHECK_INTERRUPT:
	result = check_interrupt_notification(irq, vector);
        break;
      case VMEDRV_IOC_CLEAR_INTERRUPT:
	result = clear_interrupt_notification(irq, vector);
        break;
      case VMEDRV_IOC_SET_INTERRUPT_AUTODISABLE:
	result = set_interrupt_autodisable(irq, vector);
        break;
      case VMEDRV_IOC_ENABLE_INTERRUPT:
	result = enable_normal_interrupt();
        break;
      case VMEDRV_IOC_DISABLE_INTERRUPT:
	result = disable_normal_interrupt();
        break;
      case VMEDRV_IOC_SET_VECTOR_MASK:
	result = set_vector_mask(irq, vector, vector_mask);
        break;
      case VMEDRV_IOC_ENABLE_ERROR_INTERRUPT:
	result = enable_error_interrupt();
        break;
      case VMEDRV_IOC_DISABLE_ERROR_INTERRUPT:
	result = disable_error_interrupt();
        break;
      case VMEDRV_IOC_RESET_ADAPTER:
	result = reset_adapter();
        break;
      default:
	return -EINVAL;
    }

    return result;
}


static int vmedrv_mmap(struct file* filep, struct vm_area_struct* vma)
{
    unsigned long vme_address, size;
    unsigned long physical_address;
    struct dev_prop_t* dev_prop;

    dev_prop = filep->private_data;
    size = vma->vm_end - vma->vm_start;
    vme_address = vma->vm_offset;

    DEBUG_MAP(printk(KERN_DEBUG "mapping vme memory...\n"));
    DEBUG_MAP(printk(KERN_DEBUG "  vme address: 0x%lx\n", vme_address));
    DEBUG_MAP(printk(KERN_DEBUG "  mapping size: 0x%lx\n", size));

    if (vme_address & (PAGE_SIZE - 1)) {
        /* offset address must be aligned with the MMU page */
        return -ENXIO;
    }
    
    if (dev_prop->number_of_mmap_windows > 0) {
        /* already mapped */
        return -ENXIO;
    }

    dev_prop->number_of_mmap_windows = ((unsigned long) size - 1) / (unsigned long) bit3_WINDOW_SIZE + 1;
    dev_prop->mmap_window_index = allocate_windows(dev_prop->number_of_mmap_windows);
    if (dev_prop->mmap_window_index < 0) {
        dev_prop->number_of_mmap_windows = 0;
        return dev_prop->mmap_window_index;
    }

    DEBUG_MAP(printk(KERN_DEBUG "  map pages: %d\n", dev_prop->number_of_mmap_windows));
    DEBUG_MAP(printk(KERN_DEBUG "  window index: %d\n", dev_prop->mmap_window_index));

    map_windows(vme_address, size, dev_prop->mmap_window_index, dev_prop->mapping_flags);
    physical_address = (
        bit3.physical_window_region_base + 
	dev_prop->mmap_window_index * bit3_WINDOW_SIZE +
	(vme_address & bit3_PAGE_OFFSET_MASK)
    );

    DEBUG_MAP(printk(KERN_DEBUG "  physical address: 0x%lx\n", physical_address));

    if (remap_page_range(vma->vm_start, physical_address, size, vma->vm_page_prot) < 0) {
        return -EAGAIN;
    }

    DEBUG_MAP(printk(KERN_DEBUG "  mapped address: 0x%lx\n", vma->vm_start));

    vma->vm_file = filep;
#if 0
    filep->f_inode->i_count++;
#endif

    return 0;
}


static void vmedrv_interrupt(int irq, void* dev_id, struct pt_regs* regs)
{
    unsigned status;

    DEBUG_INT(printk(KERN_DEBUG "interrupt handled.\n"));
    //disable_normal_interrupt();

    /* check whether the PCI card is generating an interrupt */
    status = readb(bit3.mapped_node_io_base + regINTERRUPT_CONTROL);
    if (! (status & icINTERRUPT_ACTIVE)) {
	return;
    }

    /* checek for a error interrupt */
    status = readb(bit3.mapped_node_io_base + regLOCAL_STATUS);
    if (status & (lsERROR_BITS & ~lsRECEIVING_PR_INTERRUPT)) {
	acknowledge_error_interrupt(status);
	return;
    }
    
    /* check for a PR interrupt */
    if (status & lsRECEIVING_PR_INTERRUPT) {
	acknowledge_pr_interrupt(status);
	return;
    }

    /* check for a DMA DONE interrupt */
    status = readb(bit3.mapped_node_io_base + regDMA_COMMAND);
    if ((status & dcDMA_DONE_FLAG) && (status & dcENABLE_DMA_DONE_INTERRUPT)) {
	acknowledge_dma_interrupt(status);
	return;
    }

    /* check for a PT interrupt */
    status = readb(bit3.mapped_node_io_base + regREMOTE_STATUS);
    if (status & rsRECEIVING_PT_INTERRUPT) {
	acknowledge_pt_interrupt(status);
	return;
    }

    /* check for a VME backplane interrupt */    
    status = readb(bit3.mapped_node_io_base + regINTERRUPT_STATUS);
    if (status) {
	acknowledge_vme_interrupt(status);
	return;
    }

    printk(KERN_WARNING "%s: Unknown interrupt handled...\n", vmedrv_name);
}


#ifndef CONFIG_PCI
#error "Kernel doesn't support PCI BIOS."
#endif


static int detect_pci_device(unsigned vendor, unsigned device, struct pci_config_t* pci_config)
{
    struct pci_dev *dev;
    int base_index;

    if (! pcibios_present()) {
        printk(KERN_WARNING "%s: unable to find PCI bios.\n", vmedrv_name);
        return -ENODEV;
    }

    dev = NULL;
    dev = pci_find_device(vendor, device, dev);

    if (dev == NULL) {
        return -ENODEV;
    }

    for (base_index = 0; base_index < 4; base_index++) {
	pci_config->ioports[base_index] =
	    dev->base_address[base_index] & PCI_BASE_ADDRESS_IO_MASK;
    }
    pci_config->irq = dev->irq;

    return 0;
}


static int initialize(struct pci_config_t* pci_config)
{
    unsigned status;
    unsigned long offset;
    unsigned long size;

    /* read PCI configurations */
    bit3.irq = pci_config->irq;
    bit3.io_node_io_base = pci_config->ioports[bit3_IO_NODE_IO_BASE_INDEX];
    bit3.mapped_node_io_base = (unsigned long) ioremap(
        offset = pci_config->ioports[bit3_MAPPED_NODE_IO_BASE_INDEX], 
        size = bit3_MAPPED_NODE_IO_SIZE
    );
    bit3.mapping_registers_base = (unsigned long) ioremap(
        offset = pci_config->ioports[bit3_MAPPING_REGISTERS_BASE_INDEX], 
        size = bit3_MAPPING_REGISTERS_SIZE
    );
    bit3.window_region_base = (unsigned long) ioremap(
        offset = pci_config->ioports[bit3_WINDOW_REGION_BASE_INDEX], 
        size = bit3_WINDOW_REGION_SIZE
    );

    bit3.physical_window_region_base 
	= pci_config->ioports[bit3_WINDOW_REGION_BASE_INDEX];
    bit3.dma_mapping_registers_base 
	= bit3.mapping_registers_base + bit3_DMA_MAPPING_REGISTERS_BASE_OFFSET;

    DEBUG(printk(KERN_DEBUG "remapping pci memories...\n"));
    DEBUG(printk(KERN_DEBUG "  memory mapped node: --> 0x%08lx\n", bit3.mapped_node_io_base));
    DEBUG(printk(KERN_DEBUG "  mapping regs: --> 0x%08lx\n", bit3.mapping_registers_base));
    DEBUG(printk(KERN_DEBUG "  window base: --> 0x%08lx\n", bit3.window_region_base));
    
    /* check whether remote power is on */
#if 0
    status = inb(bit3.io_node_io_base + regLOCAL_STATUS);
#else
    status = readb(bit3.mapped_node_io_base + regLOCAL_STATUS);
#endif
    if (status & lsREMOTE_BUS_POWER_OFF) {
        printk(KERN_WARNING "%s: ERROR: VME chassis power is off.\n", vmedrv_name);
        printk(KERN_WARNING "  (or I/O cable is not connected, or SYSRESET is active.)\n");
	printk(KERN_WARNING "  Local Status Register: 0x%02x\n", status);
        return -EIO;
    }

    /* clear error caused by the power on transition */
#if 0
    inb(bit3.io_node_io_base  + regREMOTE_STATUS);
    outb(bit3.io_node_io_base  + regLOCAL_COMMAND, lcCLEAR_STATUS);
    status = inb(bit3.io_node_io_base  + regLOCAL_STATUS);
#else
    readb(bit3.mapped_node_io_base + regREMOTE_STATUS);
    writeb(lcCLEAR_STATUS, bit3.mapped_node_io_base + regLOCAL_COMMAND);
    status = readb(bit3.mapped_node_io_base + regLOCAL_STATUS);
#endif
    
    /* make sure no error bits are set */
    if (status & lsERROR_BITS) {
        if (status & lsINTERFACE_PARITY_ERROR) {
	    printk(KERN_WARNING "%s: ERROR: interface parity error.\n", vmedrv_name);
	}
        if (status & lsREMOTE_BUS_ERROR) {
	    printk(KERN_WARNING "%s: ERROR: remote bus error.\n", vmedrv_name);
	}
        if (status & lsRECEIVING_PR_INTERRUPT) {
	    printk(KERN_WARNING "%s: ERROR: receiving PR interrupt.\n", vmedrv_name);
	}
        if (status & lsINTERFACE_TIMEOUT) {
	    printk(KERN_WARNING "%s: ERROR: interface timed out.\n", vmedrv_name);
	}
        if (status & lsLRC_ERROR) {
	    printk(KERN_WARNING "%s: ERROR: LRC(Longitudinal Redundancy Check) error.\n", vmedrv_name);
	}
        if (status & lsREMOTE_BUS_POWER_OFF) {
	    printk(KERN_WARNING "%s: ERROR: remote bus power off or I/O cable is off.\n", vmedrv_name);
	}
        printk(KERN_WARNING "  Local Status Register: 0x%02x\n", status);

        return -EIO;
    }

    /* check adapter ID */    
    status = inb(bit3.io_node_io_base  + regADAPTER_ID);
    printk(KERN_DEBUG "  Adapter ID (I/O): 0x%02x\n", status);
    status = readb(bit3.mapped_node_io_base + regADAPTER_ID);
    printk(KERN_DEBUG "  Adapter ID (mem): 0x%02x\n", status);

    return 0;
}


static int set_access_mode(struct dev_prop_t* dev_prop, int mode)
{
    if (mode >= vmedrv_NUMBER_OF_ACCESS_MODES) {
	return -EINVAL;
    }

    dev_prop->address_modifier = vmedrv_config_table[mode].address_modifier;
    dev_prop->dma_address_modifier = vmedrv_config_table[mode].dma_address_modifier;
    dev_prop->data_width = vmedrv_config_table[mode].data_width;
    dev_prop->function_code = vmedrv_config_table[mode].function_code;
    dev_prop->byte_swapping = vmedrv_config_table[mode].byte_swapping;

    dev_prop->mapping_flags = (
        (dev_prop->address_modifier << bit3_AM_SHIFT) | 
        (dev_prop->function_code << bit3_FUNCTION_SHIFT) | 
        (dev_prop->byte_swapping << bit3_BYTESWAP_SHIFT) 
    );

    dev_prop->dma_mapping_flags = (
	(dev_prop->byte_swapping & bit3_DMA_BYTESWAP_MASK) << bit3_DMA_BYTESWAP_SHIFT
    );

    DEBUG(printk(KERN_DEBUG "setting access modes...\n"));
    DEBUG(printk(KERN_DEBUG "  address modifier: 0x%02x\n", dev_prop->address_modifier));
    DEBUG(printk(KERN_DEBUG "  data width: %d\n", dev_prop->data_width));
    DEBUG(printk(KERN_DEBUG "  function code: 0x%02x\n", dev_prop->function_code));
    DEBUG(printk(KERN_DEBUG "  byte swapping: 0x%02x\n", dev_prop->byte_swapping));

    return 0;
}


static int set_transfer_method(struct dev_prop_t* dev_prop, int method)
{
    if (method == VMEDRV_PIO) {
	dev_prop->transfer_method = tmPIO;
        DEBUG(printk(KERN_DEBUG "transfer mode is set to PIO.\n"));
    }
    else if (method == VMEDRV_DMA) {
	if (dev_prop->dma_address_modifier == amINVALID) {
	    return -EINVAL;
	}
	dev_prop->transfer_method = tmDMA;
	DEBUG(printk(KERN_DEBUG "transfer mode is set to DMA.\n"));
    }
    else {
	return -EINVAL;
    }

    return 0;
}


static int pio_read(struct dev_prop_t* dev_prop, char* buf, unsigned long vme_address, int count)
{
    unsigned long offset_address;
    unsigned long size, read_size;
    unsigned window_address;
    void* pio_buf;
    unsigned long pio_buf_data_size, pio_buf_index;
    int result;

    /* allocate PIO buffer and mapping windows, if it has not been yet */
    if ((result = prepare_pio(dev_prop)) < 0) {
	return result;
    }

    /* map windows */
    offset_address = vme_address & bit3_PAGE_OFFSET_MASK;
    if (offset_address + count <= bit3_WINDOW_SIZE * dev_prop->number_of_pio_windows) {
        size = count;
    }
    else {
        size = bit3_WINDOW_SIZE * dev_prop->number_of_pio_windows - offset_address;
    }
    window_address = map_windows(vme_address, size, dev_prop->pio_window_index, dev_prop->mapping_flags);
    
    /* read from mapped windows */
    pio_buf = dev_prop->pio_buffer;
    pio_buf_data_size = 0;
    pio_buf_index = 0;
    for (read_size = 0; read_size < size; read_size += dev_prop->data_width) {
        if (dev_prop->data_width == dwWORD) {
	    ((unsigned short *) pio_buf)[pio_buf_index] = readw(window_address);
	}
	else if (dev_prop->data_width == dwLONG) {
	    ((unsigned int *) pio_buf)[pio_buf_index] = readl(window_address);
	}

	window_address += dev_prop->data_width;
	pio_buf_data_size += dev_prop->data_width;
	pio_buf_index++;

	if (pio_buf_data_size + dwLONG >= PIO_BUFFER_SIZE) {
	    copy_to_user(buf, pio_buf, pio_buf_data_size);
	    buf += pio_buf_data_size;
	    pio_buf_data_size = 0;
	    pio_buf_index = 0;
	}
    }

    copy_to_user(buf, pio_buf, pio_buf_data_size);

    return read_size;
}


static int pio_write(struct dev_prop_t* dev_prop, const char* buf, unsigned long vme_address, int count)
{
    unsigned long offset_address;
    unsigned long size, remain_size;
    unsigned window_address;
    void* pio_buf;
    unsigned long pio_buf_data_size, pio_buf_index;
    int result;

    /* allocate PIO buffer and mapping windows, if it has not been yet */
    if ((result = prepare_pio(dev_prop)) < 0) {
	return result;
    }

    /* map windows */
    offset_address = vme_address & bit3_PAGE_OFFSET_MASK;
    if (offset_address + count <= bit3_WINDOW_SIZE * dev_prop->number_of_pio_windows) {
        size = count;
    }
    else {
        size = bit3_WINDOW_SIZE * dev_prop->number_of_pio_windows - offset_address;
    }
    window_address = map_windows(vme_address, size, dev_prop->pio_window_index, dev_prop->mapping_flags);
    
    /* write to mapped windows */
    pio_buf = dev_prop->pio_buffer;
    pio_buf_data_size = 0;
    pio_buf_index = 0;
    for (remain_size = size; remain_size > 0; remain_size -= dev_prop->data_width) {
	if (pio_buf_data_size <= 0) {
	    if (remain_size < PIO_BUFFER_SIZE) {
		pio_buf_data_size = remain_size;
	    }
	    else {
		pio_buf_data_size = PIO_BUFFER_SIZE;
	    }
	    copy_from_user(pio_buf, buf, pio_buf_data_size);
	    buf += pio_buf_data_size;
	    pio_buf_index = 0;
	}

        if (dev_prop->data_width == dwWORD) {
	    writew(
		((unsigned short *) pio_buf)[pio_buf_index], window_address
	    );
	}
	else if (dev_prop->data_width == dwLONG) {
	    writel(
		((unsigned int *) pio_buf)[pio_buf_index], window_address
	    );
	}

	window_address += dev_prop->data_width;
	pio_buf_data_size -= dev_prop->data_width;
	pio_buf_index++;
    }

    return size;
}


static int prepare_pio(struct dev_prop_t* dev_prop)
{
    /* allocate PIO buffer, if it has not been allocated. */
    if (dev_prop->pio_buffer == 0) {
	dev_prop->pio_buffer = kmalloc(PIO_BUFFER_SIZE, GFP_KERNEL);
	if (dev_prop->pio_buffer == 0) {
	    printk(KERN_WARNING "%s: can't allocate PIO buffer.", vmedrv_name);
	    printk(KERN_WARNING "  requested size: %d\n", PIO_BUFFER_SIZE);
	    return -ENOMEM;
	}
    }    

    /* allocate PIO windows, if it has not been allocated. */
    if (dev_prop->number_of_pio_windows == 0) {
        dev_prop->pio_window_index = allocate_windows(PIO_WINDOW_PAGES);
	if (dev_prop->pio_window_index < 0) {
	    return dev_prop->pio_window_index;
	}
	dev_prop->number_of_pio_windows = PIO_WINDOW_PAGES;
    }

    return 0;
}

static int allocate_windows(int number_of_windows)
{
    int number_of_free_windows;
    int window_index = -ENOMEM;
    int i;

    number_of_free_windows = 0;
    for (i = 0; i < bit3_NUMBER_OF_WINDOWS; i++) {
        if (bit3.window_status_table[i] != 0) {
	    number_of_free_windows = 0;
	    continue;
	}

	if (number_of_free_windows == 0) {
	    window_index = i;
	}

	number_of_free_windows++;
	if (number_of_free_windows == number_of_windows) {
	    break;
	}
    }

    if (i == bit3_NUMBER_OF_WINDOWS) {
        return -ENOMEM;
    }
    
    for (i = 0; i < number_of_windows; i++) {
        bit3.window_status_table[window_index + i] = 1;
    }

    return window_index;
}


static void free_windows(int window_index, int number_of_windows)
{
    int i;
    for (i = 0; i < number_of_windows; i++) {
        bit3.window_status_table[window_index + i] = 0;
    }
}


static unsigned map_windows(unsigned address, unsigned size, unsigned window_index, unsigned flags) 
{
    unsigned base_address, offset_address;
    unsigned number_of_windows;
    unsigned window_address;
    int i;

    base_address = address & bit3_PAGE_BASE_MASK;
    offset_address = address & bit3_PAGE_OFFSET_MASK;
    number_of_windows = ((unsigned long) size - 1) / (unsigned long) bit3_WINDOW_SIZE + 1;
    window_address = bit3.window_region_base + bit3_WINDOW_SIZE * window_index + offset_address;

    for (i = 0; i < number_of_windows; i++) {
        writel(
	    base_address | flags, 
            bit3.mapping_registers_base + bit3_MAPPING_REGISTER_WIDTH * window_index
	);
        base_address += bit3_WINDOW_SIZE;
	window_index++;
    }

    return window_address;
}


static int enable_normal_interrupt(void)
{
    bit3.interrupt_enabling_flags |= icNORMAL_INTERRUPT_ENABLE;
    writeb(
	bit3.interrupt_enabling_flags,
        bit3.mapped_node_io_base + regINTERRUPT_CONTROL
    );

    DEBUG_INT(printk(KERN_DEBUG "normal interrupt enabled.\n"));

    return 0;
}


static int disable_normal_interrupt(void)
{
    bit3.interrupt_enabling_flags &= ~icNORMAL_INTERRUPT_ENABLE;
    writeb(
	bit3.interrupt_enabling_flags,
        bit3.mapped_node_io_base + regINTERRUPT_CONTROL
    );

    DEBUG_INT(printk(KERN_DEBUG "normal interrupt disabled.\n"));

    return 0;
}


static int enable_error_interrupt(void)
{
    bit3.interrupt_enabling_flags |= icERROR_INTERRUPT_ENABLE;
    writeb(
	bit3.interrupt_enabling_flags,
        bit3.mapped_node_io_base + regINTERRUPT_CONTROL
    );

    return 0;
}


static int disable_error_interrupt(void)
{
    bit3.interrupt_enabling_flags &= ~icERROR_INTERRUPT_ENABLE;
    writeb(
	bit3.interrupt_enabling_flags,
        bit3.mapped_node_io_base + regINTERRUPT_CONTROL
    );

    return 0;
}


static void save_interrupt_flags(void)
{
    bit3.saved_interrupt_flags = bit3.interrupt_enabling_flags;
    DEBUG_INT(printk(
	KERN_DEBUG "interrupt flags are saved: 0x%02x\n", 
	bit3.interrupt_enabling_flags
    ));
}


static void restore_interrupt_flags(void)
{
    if (bit3.saved_interrupt_flags != bit3.interrupt_enabling_flags) {
	bit3.interrupt_enabling_flags = bit3.saved_interrupt_flags;
	writeb(
	    bit3.interrupt_enabling_flags,
	    bit3.mapped_node_io_base + regINTERRUPT_CONTROL
	);	
    }

    DEBUG_INT(printk(
	KERN_DEBUG "interrupt flags are restored: 0x%02x\n", 
	bit3.interrupt_enabling_flags
    ));
}


static int acknowledge_error_interrupt(unsigned local_status)
{
    unsigned remote_status;

    DEBUG_INT(printk(KERN_DEBUG "error interrupt handled...\n"));
    DEBUG_INT(printk(KERN_DEBUG "  local status = 0x%02x\n", local_status));

    writeb(lcCLEAR_STATUS, bit3.mapped_node_io_base + regLOCAL_COMMAND);
    if (local_status & lsINTERFACE_TIMEOUT) {
	/* flush the interface error */
	remote_status = readb(bit3.mapped_node_io_base + regREMOTE_STATUS);
        DEBUG_INT(printk(KERN_DEBUG "  remote status = 0x%02x\n", remote_status));
    }

    return 0;
}


static int acknowledge_pt_interrupt(unsigned local_status)
{
    DEBUG_INT(printk(KERN_DEBUG "PT interrupt handled...\n"));

    writeb(lcCLEAR_PR_INTERRUPT, bit3.mapped_node_io_base + regLOCAL_COMMAND);

    return 0;
}


static int acknowledge_dma_interrupt(unsigned dma_status)
{
    DEBUG_INT(printk(KERN_DEBUG "DMA interrupt handled...\n"));

    /* clear the DMA Command Register */
    writeb(0, bit3.mapped_node_io_base + regDMA_COMMAND);  

    /* wake up the process */
    wake_up_interruptible(&vmedrv_dmadone_wait_queue);

    return 0;
}


static int acknowledge_pr_interrupt(unsigned remote_status)
{
    DEBUG_INT(printk(KERN_DEBUG "PR interrupt handled...\n"));

    writeb(
        rcCLEAR_PT_INTERRUPT,
	bit3.mapped_node_io_base + regREMOTE_COMMAND_1
    );
    
    return 0;
}


static int acknowledge_vme_interrupt(unsigned interrupt_status)
{
    unsigned irq, vector;
    struct interrupt_client_t* interrupt_client;
    struct task_struct* task;
    int signal_id;
    int priv;

    DEBUG_INT(printk(KERN_DEBUG "VME interrupt handled...\n"));

    for (irq = 1; irq < vmeNUMBER_OF_IRQ_LINES; irq++) {
        /* check whether this IRQ is asserted */
        if (! (interrupt_status & (0x0001 << irq))) {
	    continue;
	}

	DEBUG_INT(printk(KERN_DEBUG "  irq = %d\n", irq));

	/* acknowledge IRQ request (send IACK) */
	writeb(irq, bit3.mapped_node_io_base + regREMOTE_COMMAND_1);
	vector = readw(bit3.mapped_node_io_base + regIACK_READ);
	
	DEBUG_INT(printk(KERN_DEBUG "  vector = 0x%04x\n", vector));

	/* check whether appropriate handler is installed */
	interrupt_client = bit3.interrupt_client_list[irq];
	if (interrupt_client == 0) {
	    DEBUG_INT(printk(KERN_DEBUG "  client is not registered.\n"));
	    continue;
	}

	/* send signal to registered processes */
	while (interrupt_client != 0) {
	    if ((vector & interrupt_client->vector_mask) == (interrupt_client->vector & interrupt_client->vector_mask)) {
		if (interrupt_client->autodisable_flag != 0) {
		    disable_normal_interrupt();
                    DEBUG_INT(printk(KERN_DEBUG "  auto-disabled.\n"));
		}

	        task = interrupt_client->task;
		signal_id = interrupt_client->signal_id;
		if (signal_id > 0) {
		    send_sig(signal_id, task, priv = 1);
                    DEBUG_INT(printk(KERN_DEBUG "  send signal.\n"));
		}
		else {
		    interrupt_client->interrupt_count++;
		    wake_up_interruptible(&vmedrv_vmebusirq_wait_queue);
                    DEBUG_INT(printk(KERN_DEBUG "  wake up.\n"));
		}
	    }
	    interrupt_client = interrupt_client->next;
	}
    }
    
    DEBUG_INT(printk(KERN_DEBUG "now exit vme interrupt handling routine.\n"));

    return 0;
}


static int register_interrupt_notification(struct task_struct* task, int irq, int vector, int signal_id)
{
    struct interrupt_client_t* interrupt_client;

    if ((irq < 1) || (irq >= vmeNUMBER_OF_IRQ_LINES)) {
        return -EINVAL;
    }

    interrupt_client = kmalloc(sizeof(struct interrupt_client_t), GFP_KERNEL);
    if (interrupt_client == 0) {
        printk(KERN_WARNING "%s: can't allocate memory for interrupt client entry.\n", vmedrv_name);
        return -ENOMEM;
    }

    interrupt_client->task = task;
    interrupt_client->irq = irq;
    interrupt_client->vector = vector;
    interrupt_client->signal_id = signal_id;
    interrupt_client->interrupt_count = 0;
    interrupt_client->autodisable_flag = 0;
    interrupt_client->vector_mask = 0xffff;

    interrupt_client->next = bit3.interrupt_client_list[irq];
    bit3.interrupt_client_list[irq] = interrupt_client;

    DEBUG_INT(printk(KERN_DEBUG 
        "vme interrupt is registered, "
	"irq: %d, vector: 0x%04x, pid: %d, signal: %d.\n",
	irq, vector, task->pid, signal_id
    ));

    return 0;
}


static int unregister_interrupt_notification(struct task_struct* task, int irq, int vector)
{
    struct interrupt_client_t *interrupt_client, *prev_interrupt_client;
    struct interrupt_client_t *removed_interrupt_client;
    int process_id;
    int irq_index;

    process_id = task->pid;
    for (irq_index = 1; irq_index < vmeNUMBER_OF_IRQ_LINES; irq_index++) {
        interrupt_client = bit3.interrupt_client_list[irq_index];
	prev_interrupt_client = 0;

	while (interrupt_client != 0) {
	    if (
		(process_id == interrupt_client->task->pid) &&
		((irq == 0) || (irq == interrupt_client->irq)) &&
		((vector == 0) || (vector == interrupt_client->vector))
	    ){
		DEBUG_INT(printk(KERN_DEBUG 
		    "vme interrupt is unregistered, "
		    "irq: %d, vector: 0x%04x, pid: %d.\n",
		    interrupt_client->irq, interrupt_client->vector, process_id
		));

	        removed_interrupt_client = interrupt_client;
		interrupt_client = interrupt_client->next;
                if (prev_interrupt_client == 0) {
		    bit3.interrupt_client_list[irq_index] = interrupt_client;
		}
		else {
		    prev_interrupt_client->next = interrupt_client;
		}

		kfree(removed_interrupt_client);
	    }
	    else {
  	        prev_interrupt_client = interrupt_client;
		interrupt_client = interrupt_client->next;
	    }
	}
    }

    return 0;
}


static int wait_for_interrupt_notification(struct task_struct* task, int irq, int vector, int timeout)
{
    unsigned long flags;
    long remaining_time;
    int process_id;
    struct interrupt_client_t *interrupt_client;
    
    timeout *= HZ;
    
    /* find matching registered interrupt signature */
    process_id = task->pid;
    interrupt_client = bit3.interrupt_client_list[irq];
    while (interrupt_client != 0) {
	if (
	    (process_id == interrupt_client->task->pid) &&
	    (vector == interrupt_client->vector) &&
	    (interrupt_client->signal_id <= 0)
	){
	    break;
	}
	interrupt_client = interrupt_client->next;
    }
    if (interrupt_client == 0)
    {
	printk(KERN_WARNING "%s: no interrupt is registered to wait for\n", vmedrv_name);
	return -ENODEV;
    }
    
    /* now process or wait for interrupt */
    save_flags(flags);
    cli();
    if (interrupt_client->interrupt_count == 0) {
	remaining_time = interruptible_sleep_on_timeout(&vmedrv_vmebusirq_wait_queue, timeout);
	restore_flags(flags);
	
	if (remaining_time == 0) {
	    return -ETIMEDOUT;
	}
	if (interrupt_client->interrupt_count == 0) {
	    return -EINTR;
	}
    }
    else {
	restore_flags(flags);
    }
    
    interrupt_client->interrupt_count = 0;
    
    return 1;
}
  

static int check_interrupt_notification(int irq, int vector)
{
    int result = 0;
    struct interrupt_client_t *interrupt_client;

    interrupt_client = bit3.interrupt_client_list[irq];
    while (interrupt_client != 0) {
	if (vector == interrupt_client->vector) {
	    result += interrupt_client->interrupt_count;

	}
	interrupt_client = interrupt_client->next;
    }

    return result;
}
  

static int clear_interrupt_notification(int irq, int vector)
{
    struct interrupt_client_t *interrupt_client;

    interrupt_client = bit3.interrupt_client_list[irq];
    while (interrupt_client != 0) {
	if (vector == interrupt_client->vector) {
	    interrupt_client->interrupt_count = 0;
	}
	interrupt_client = interrupt_client->next;
    }

    return 0;
}

  
static int set_interrupt_autodisable(int irq, int vector)
{
    struct interrupt_client_t *interrupt_client;

    interrupt_client = bit3.interrupt_client_list[irq];
    while (interrupt_client != 0) {
	if (vector == interrupt_client->vector) {
	    interrupt_client->autodisable_flag = 1;
	}
	interrupt_client = interrupt_client->next;
    }

    return 0;
}

  
static int set_vector_mask(int irq, int vector, int vector_mask)
{
    struct interrupt_client_t *interrupt_client;

    interrupt_client = bit3.interrupt_client_list[irq];
    while (interrupt_client != 0) {
	if (vector == interrupt_client->vector) {
	    interrupt_client->vector_mask = vector_mask;
	}
	interrupt_client = interrupt_client->next;
    }

    return 0;
}

static int reset_adapter(void)
{
    writel(
	rcRESET_ADAPTER, 
	bit3.mapped_node_io_base + regREMOTE_COMMAND_1
    );

    return 0;
}

  
static int dma_read(struct dev_prop_t* dev_prop, char* buf, unsigned long vme_address, int count)
{
    unsigned long pci_address, size;
    int direction;
    int result;

    if ((result = prepare_dma(dev_prop)) < 0) {
	return result;
    }

    /* set transfer size and direction */
    if (count > bit3.dma_buffer_size) {
	size = bit3.dma_buffer_size;
    }
    else {
	size = count;
    }
    direction = tdREAD;

    pci_address = bit3.dma_buffer_mapped_pci_address;

    result = start_dma(dev_prop, pci_address, vme_address, size, direction);
    if (result < 0) {
	return result;
    }

    copy_to_user(buf, bit3.dma_buffer, size);

    return size;
}


static int dma_write(struct dev_prop_t* dev_prop, const char* buf, unsigned long vme_address, int count)
{
    unsigned long pci_address, size;
    int direction;
    int result;

    if ((result = prepare_dma(dev_prop)) < 0) {
	return result;
    }

    /* set transfer size and direction */
    if (count > bit3.dma_buffer_size) {
	size = bit3.dma_buffer_size;
    }
    else {
	size = count;
    }
    direction = tdWRITE;

    copy_from_user(bit3.dma_buffer, buf, size);

    pci_address = bit3.dma_buffer_mapped_pci_address;
    result = start_dma(dev_prop, pci_address, vme_address, size, direction);

    return (result < 0) ? result : size;
}


static int prepare_dma(struct dev_prop_t* dev_prop)
{
    /* allocate and map DMA buffer, if not it has not been allocated yet.*/
    if (bit3.dma_buffer_size == 0) {
	if (allocate_dma_buffer(DMA_BUFFER_SIZE) <= 0) {
	    printk(KERN_WARNING "%s: can't allocate dma buffer.\n", vmedrv_name);
	    printk(KERN_WARNING "  requested size: %d\n", DMA_BUFFER_SIZE);
	    return -ENOMEM;
	}
	bit3.dma_buffer_mapped_pci_address = map_dma_windows(
	     bit3.dma_buffer_bus_address, 
	     bit3.dma_buffer_size, 
             /* byte swapping = */ 0
	);
    }

    return 0;
}


static int start_dma(struct dev_prop_t* dev_prop, unsigned long pci_address, unsigned long vme_address, unsigned long size, int direction)
{
    int flags;
    unsigned status;

    /* set DMA registers, and start */
    save_flags(flags);  // initiate_dma() calls cli()
    initiate_dma(dev_prop, pci_address, vme_address, size, direction);
    DEBUG_DMA(printk(KERN_DEBUG "Now start dma transfer...\n"));

    /* wait DMA DONE interrupt */
    interruptible_sleep_on(&vmedrv_dmadone_wait_queue);
    restore_flags(flags);

    /* release DMA settings */
    DEBUG_DMA(printk(KERN_DEBUG "dma transfer finished.\n"));    
    status = release_dma();
    if (status) {
	printk(KERN_WARNING "%s: dma transfer faild.\n", vmedrv_name);
	printk(KERN_WARNING "  status: 0x%02x\n", status);
	return -EIO;
    }

    return 0;
}


static void* allocate_dma_buffer(unsigned long size)
{
    if (size > bit3_DMA_MAPPING_SIZE) {
	size = bit3_DMA_MAPPING_SIZE;
    }

    bit3.dma_buffer = kmalloc(size, GFP_KERNEL);
    if (bit3.dma_buffer > 0) {
	bit3.dma_buffer_size = size;
	bit3.dma_buffer_bus_address = virt_to_bus(bit3.dma_buffer);

	DEBUG_DMA(printk(KERN_DEBUG "dma buffer is allocated.\n"));
	DEBUG_DMA(printk(KERN_DEBUG "  size: 0x%lx.\n", bit3.dma_buffer_size));
	DEBUG_DMA(printk(KERN_DEBUG "  virtual address: 0x%08lx.\n", (long) bit3.dma_buffer));
	DEBUG_DMA(printk(KERN_DEBUG "  bus address: 0x%08lx.\n", bit3.dma_buffer_bus_address));
    }

    return bit3.dma_buffer;
}


static void release_dma_buffer(void)
{
    if (bit3.dma_buffer_size > 0) {
	kfree(bit3.dma_buffer);
	DEBUG_DMA(printk(KERN_DEBUG "dma buffer is released.\n"));
    }
}


static unsigned map_dma_windows(unsigned pci_address, unsigned size, unsigned flags) 
{
    unsigned base_address, offset_address;
    unsigned number_of_windows;
    unsigned window_index;
    unsigned mapping_register_address;
    unsigned mapped_pci_address;
    int i;

    base_address = pci_address & bit3_DMA_PAGE_BASE_MASK;
    offset_address = pci_address & bit3_DMA_PAGE_OFFSET_MASK;
    number_of_windows = ((unsigned long) (size - 1)) / (unsigned long) bit3_DMA_WINDOW_SIZE + 1;
    if (offset_address > 0) {
        number_of_windows += 1;
    }

    window_index = 0;
    mapping_register_address = (
	bit3.dma_mapping_registers_base + 
	bit3_DMA_MAPPING_REGISTER_WIDTH * window_index
    );

    DEBUG_DMA(printk(KERN_DEBUG "writing dma mapping registers...\n"));
    for (i = 0; i < number_of_windows; i++) {
        writel(
	    base_address | flags, 
	    mapping_register_address
	);

	DEBUG_DMA(printk(
	     KERN_DEBUG "  reg: 0x%08x, value: 0x%08x\n", 
	     mapping_register_address, 
	     base_address | flags
	));

        base_address += bit3_DMA_WINDOW_SIZE;
	mapping_register_address += bit3_DMA_MAPPING_REGISTER_WIDTH;
    }

    mapped_pci_address = window_index << bit3_DMA_MAPPING_REGISTER_INDEX_SHIFT;
    mapped_pci_address |= offset_address;

    DEBUG_DMA(printk(KERN_DEBUG "  mapped pci address: 0x%08x\n", mapped_pci_address));

    return mapped_pci_address;
}


static int initiate_dma(struct dev_prop_t* dev_prop, unsigned mapped_pci_address, unsigned vme_address, unsigned size, int direction)
{
    unsigned remainder_count, packet_count;
    unsigned dma_register_value, remote_command2_value;
    
    DEBUG_DMA(printk(KERN_DEBUG "setting dma parameters...\n"));

    /* program the Local DMA Command Register */
    dma_register_value = dcENABLE_DMA_DONE_INTERRUPT;
    if (direction == tdREAD) {
	dma_register_value |= dcDMA_TRANSFER_DIRECTION_READ;
    }
    else {
	dma_register_value |= dcDMA_TRANSFER_DIRECTION_WRITE;
    }
    if (dev_prop->data_width == dwWORD) {
	dma_register_value |= dcDMA_WORD_LONGWORD_SELECT_WORD;
    }
    else {
	dma_register_value |= dcDMA_WORD_LONGWORD_SELECT_LONGWORD;
    }
    writeb(
	dma_register_value,
	bit3.mapped_node_io_base + regDMA_COMMAND
    );
    DEBUG_DMA(printk(KERN_DEBUG "  dma reg value: 0x%02x\n", dma_register_value));

    /* program the Local DMA Address Register */
    writeb(
	(mapped_pci_address >> 0) & 0x000000ff,
	bit3.mapped_node_io_base + regDMA_PCI_ADDRESS_2_7
    );
    writeb(
	(mapped_pci_address >> 8) & 0x000000ff,
	bit3.mapped_node_io_base + regDMA_PCI_ADDRESS_8_15
    );
    writeb(
	(mapped_pci_address >> 16) & 0x000000ff,
	bit3.mapped_node_io_base + regDMA_PCI_ADDRESS_16_23
    );
    DEBUG_DMA(printk(KERN_DEBUG "  mapped pci address: 0x%08x\n", mapped_pci_address));
	
    /* load the Remote DMA Address Register */
    writeb(
	(vme_address >> 0) & 0x000000ff,
	bit3.mapped_node_io_base + regDMA_VME_ADDRESS_0_7
    );
    writeb(
	(vme_address >> 8) & 0x000000ff,
	bit3.mapped_node_io_base + regDMA_VME_ADDRESS_8_15
    );
    writeb(
	(vme_address >> 16) & 0x000000ff,
	bit3.mapped_node_io_base + regDMA_VME_ADDRESS_16_23
    );
    writeb(
	(vme_address >> 24) & 0x000000ff,
	bit3.mapped_node_io_base + regDMA_VME_ADDRESS_24_31
    );

    DEBUG_DMA(printk(KERN_DEBUG "  vme address: 0x%08x\n", vme_address));

    /* load the Remainder/Packet Count Register */
    remainder_count = size % bit3_DMA_PACKET_SIZE;
    packet_count = size / bit3_DMA_PACKET_SIZE;
    writeb(
	remainder_count,
	bit3.mapped_node_io_base + regDMA_REMOTE_REMAINDER_COUNT
    );
    writeb(
	remainder_count,
	bit3.mapped_node_io_base + regDMA_REMAINDER_COUNT
    );
    writeb(
	(packet_count >> 0) & 0x00ff,
	bit3.mapped_node_io_base + regDMA_PACKET_COUNT_0_7
    );
    writeb(
	(packet_count >> 8) & 0x00ff,
	bit3.mapped_node_io_base + regDMA_PACKET_COUNT_8_15
    );
    DEBUG_DMA(printk(KERN_DEBUG "  remainder count: 0x%02x\n", remainder_count));
    DEBUG_DMA(printk(KERN_DEBUG "  packet count: 0x%04x\n", packet_count));

    /* program the other CSRs */
    writeb(
	dev_prop->dma_address_modifier,
	bit3.mapped_node_io_base + regADDRESS_MODIFIER
    );
    remote_command2_value = rcBLOCK_MODE_DMA | rcDISABLE_INTERRUPT_PASSING;
    writeb(
        remote_command2_value,
	bit3.mapped_node_io_base + regREMOTE_COMMAND_2
    );
    DEBUG_DMA(printk(KERN_DEBUG "  dma am code: 0x%02x\n", dev_prop->dma_address_modifier));
    DEBUG_DMA(printk(KERN_DEBUG "  remote command 2: 0x%02x\n", remote_command2_value));

    /* enable normal interrupt*/
    save_interrupt_flags();
    enable_normal_interrupt();
    cli();

    /* now, start the DMA transfer */
    writeb(
	dma_register_value | dcSTART_DMA,
	bit3.mapped_node_io_base + regDMA_COMMAND
    );

    return 0;
}


static int release_dma(void)
{
    unsigned status;

    status = readb(bit3.mapped_node_io_base + regLOCAL_STATUS);
    status &= lsERROR_BITS;

    /* restore VME interrupt flags */
    restore_interrupt_flags();
    writeb(
	0,
	bit3.mapped_node_io_base + regREMOTE_COMMAND_2
    );
    
    return status;
}
