#include <linux/module.h>
#include <linux/init.h>
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/device.h>
#include <linux/pci.h>
#include <linux/interrupt.h>
#include <asm/dma.h>
#include <asm/current.h>
#include <asm/uaccess.h>

#define BAR_MMIO 0
#define VENDOR_ID 0x10ee
#define DEVICE_ID 0x7028
MODULE_LICENSE("Dual BSD/GPL");
MODULE_AUTHOR("fpga");
MODULE_DESCRIPTION("device driver for pci");
#define DRIVER_NAME "Virtex-7"
#define DEVICE_NAME "Virtex-7_XC7VX485T-2FFG1761C"
//#define PCI_DRIVER_DEBUG
//#define DETAIL_LOG
#ifdef  PCI_DRIVER_DEBUG
#define tprintk(fmt, ...) printk(KERN_ALERT "** (%3d) %-20s: " fmt, __LINE__,  __func__,  ## __VA_ARGS__)
#else
#define tprintk(fmt, ...) 
#endif

// 時間計測
static int print_timestamp() {
    struct timespec time;
    getnstimeofday(&time);
    return time.tv_sec * 1000000000L + time.tv_nsec;
}

int start_read;
int start_write;
int end_read;
int end_write;

// max device count (マイナー番号)
static int pci_devs_max = 1;

// pci_major = 0 ならば動的に割り当てられる
static unsigned int pci_major = 246;

// insmodした時に，pci_majorをコンソール上で得ることができる
module_param(pci_major, uint, 0);

static unsigned int pci_minor = 0; // static allocation
static dev_t pci_devt; // MKDEV(pci_major, pci_minor)
struct device_data {
        struct pci_dev *pdev;
        struct cdev *cdev;

        // for PCI mmio
        unsigned long mmio_base, mmio_flags, mmio_length;
        char *mmio_addr;
        //unsigned int mmio_memsize;
};
static struct device_data *dev_data;


// file_operations function
ssize_t pci_write(struct file *filp, const char __user *buf, size_t count, loff_t *f_ops)
{
        unsigned int write_num = 0;
	//char val;
        unsigned int val;
        int retval;        
        #ifdef DETAIL_LOG
        tprintk("[write] call!\n");
        #endif

        while(write_num < count) {            
		retval = get_user(val, &buf[write_num++]);
                #ifdef DETAIL_LOG
                tprintk("port io number:%lx\n", dev_data->mmio_addr + *f_ops);
		printk("[write]val =  %d\n", val);
                #endif
	if(retval) break; // 0 on success
	iowrite32(val, dev_data->mmio_addr + *f_ops);
	(*f_ops) = (*f_ops) + 4;
	}

return write_num;
}

ssize_t pci_read(struct file *filp, char __user *buf, size_t count, loff_t *f_ops)
{
        int retval;
	//char val = 0;
	unsigned int val = 0;
        unsigned int read_num = 0;
	//tprintk("count %d pos %lld\n", count, *f_ops);
        #ifdef DETAIL_LOG
	tprintk("[read] call!\n");
        #endif
	while (read_num < count) {
                
		val = ioread32(dev_data->mmio_addr + *f_ops);    // read 1 byte
                #ifdef DETAIL_LOG
                tprintk("port io number:%lx\n", dev_data->mmio_addr + *f_ops);
		printk("[read]val =  %d\n", val);
                #endif
		retval = put_user(val, &buf[read_num++]);
		if(retval) break; // 0 on success
		(*f_ops) = (*f_ops) + 4;
	}

return read_num;	
}


loff_t pci_llseek(struct file *filp, loff_t off, int whence)
{
        loff_t newpos =-1;
        tprintk("lseek whence:%d\n", whence);
        switch(whence) {
        
        case SEEK_SET:
                newpos = off;
        break;

        case SEEK_CUR:
                newpos = filp->f_pos + off;
        break;

        //case SEEK_END:
                //newpos = dev_data->mmio_memsize + off;
        //break;

        default: // can't happen //
                return -EINVAL;
        }

        if (newpos < 0) return -EINVAL;

        filp->f_pos = newpos;
        return newpos;
}

static int pci_open(struct inode *inode, struct file *file)
{
        file->private_data = NULL;
        return 0; // success
}

static int pci_close(struct inode *inode, struct file *file)
{

        if(file->private_data) {
                kfree(file->private_data);
                file->private_data = NULL;
        }

        return 0; // success
}

struct file_operations pci_fops = 
{
        .open = pci_open,
        .release = pci_close,
        .read = pci_read,
        .write = pci_write,
        .llseek = pci_llseek,
        .unlocked_ioctl = NULL,
};

//-----------------------------------------------------------------
// supported pci id type
static struct pci_device_id pci_ids[] =
{
        { PCI_DEVICE(VENDOR_ID, DEVICE_ID) },
        { 0, },
};

// export pci_device_id
MODULE_DEVICE_TABLE(pci, pci_ids);

//-----------------------------------------------------------------
// pci 初期化機能
// pciデバイスの有効化　＆　キャラクタデバイス登録
static int pci_probe (struct pci_dev *pdev, const struct pci_device_id *id)
{
        int err;
        char irq;
        int alloc_ret = 0;
        int cdev_err = 0;
        short vendor_id, device_id;

        //-----------------------------------------------------------------
        // config PCI
        // enable pci device
        err = pci_enable_device(pdev);
        if(err) {
                printk(KERN_ERR "can't enable pci device\n");
                goto error; 
        }
        #ifdef  PCI_DRIVER_DEBUG
        tprintk("PCI enabled for %s\n", DRIVER_NAME);
        #endif

        // request PCI region
        // bar 0 ... MMIO
        dev_data->mmio_base = pci_resource_start(pdev, BAR_MMIO);
        dev_data->mmio_length = pci_resource_len(pdev, BAR_MMIO);
        dev_data->mmio_flags = pci_resource_flags(pdev, BAR_MMIO);
        tprintk( "mmio_base: %lx, mmio_length: %lx, mmio_flags: %lx\n",
                        dev_data->mmio_base, dev_data->mmio_length, dev_data->mmio_flags);

        dev_data->mmio_addr = ioremap(dev_data->mmio_base, dev_data->mmio_length);
	tprintk("virtex7: Mapped Address -> (0x%llx)\n",(unsigned long long)dev_data->mmio_addr);
	tprintk("virtex7: Mapped Memory -> (0x%d)\n",dev_data->mmio_length);

        err = pci_request_region(pdev, BAR_MMIO, DRIVER_NAME);
        if(err) {
                printk(KERN_ERR "%s :error pci_request_region MMIO\n", __func__);
                goto error; 
        }
        //iowrite8(0xff,dev_data->mmio_addr);     //初期化
        //===================試し書き込み===================
        int val1;
        int size;
        int i;

        start_write = print_timestamp();
        for (i = 0; i < 262145; i++) {
        iowrite32(0x12345677, dev_data->mmio_addr + i * 4);
        
        }
        val1 = ioread32(dev_data->mmio_addr);

        printk("vall=%x\n", val1);
        end_write = print_timestamp();

        printk("Transfer_Time[Write] = %d\n",end_write - start_write);

        start_read = print_timestamp();
        for (size = 0; size < 16384; size++) {
        val1 = ioread32(dev_data->mmio_addr + size * 4);
        }
        end_read = print_timestamp();
        printk("Transfer_Time[Read] = %d\n",end_read - start_read);

        //printk("val=%x\n", val);
        //===================試し書き込み===================
        /*//===================試し書き込み===================
        unsigned char val;
        int size;
        start_write = print_timestamp();
        for (size = 0; size < 262140*4; size++) {
        iowrite8(0xA, dev_data->mmio_addr + size);
        
        }
        val = ioread8(dev_data->mmio_addr + 262140*4);
        printk("vall=%x\n", val);
        end_write = print_timestamp();

        printk("%d\n",end_write - start_write);

        start_read = print_timestamp();
        for (size = 0; size < 262140*4; size++) {
        val = ioread8(dev_data->mmio_addr + size);
        }
        end_read = print_timestamp();
        printk("%d\n",end_read - start_read);
        *///printk("val=%x\n", val);
        //===================試し書き込み===================

        // PCI configuration data　の探索
        // define at include/uapi/linux/pci_regs.h
        pci_read_config_word(pdev, VENDOR_ID, &vendor_id);
        pci_read_config_word(pdev, DEVICE_ID, &device_id);
        tprintk("PCI Vendor ID:%x, Device ID:%x\n", vendor_id, device_id);

        dev_data->pdev = pdev;
        //dev_data->mmio_memsize = MMIO_DATASIZE;

        tprintk("sucess allocate i/o region\n");

        //-----------------------------------------------------------------
        // config irq 
        // get irq number
        irq = pdev->irq; // same as pci_read_config_byte(pdev, PCI_INTERRUPT_LINE, &irq);
        tprintk("device irq: %d\n", irq);
        //irq error処理いるならここに記述

        //-----------------------------------------------------------------
        // キャラクタデバイス登録
        // メジャー番号割当
        alloc_ret = alloc_chrdev_region(&pci_devt, pci_minor, pci_devs_max, DRIVER_NAME);
        if(alloc_ret) goto error;

        pci_major = MAJOR(pci_devt);

        dev_data->cdev = (struct cdev*)kmalloc(sizeof(struct cdev), GFP_KERNEL);
        if(!dev_data->cdev) goto error;

        cdev_init(dev_data->cdev, &pci_fops);
        dev_data->cdev->owner = THIS_MODULE;

        cdev_err = cdev_add(dev_data->cdev, pci_devt, pci_devs_max);
        if(cdev_err) goto error;

        tprintk("%s driver(major %d) installed.\n", DRIVER_NAME, pci_major);
        
        start_write = print_timestamp();
        for (i = 0; i < 16384; i++) {
        iowrite32(0x12345678, dev_data->mmio_addr + i * 4);
        
        }
        val1 = ioread32(dev_data->mmio_addr + 65536);

        printk("vall=%x\n", val1);
        end_write = print_timestamp();

        printk("after=%d\n",end_write - start_write);




        return 0;

error:
        tprintk("PCI load error\n");
        if(cdev_err == 0) cdev_del(dev_data->cdev);

        if(alloc_ret == 0) unregister_chrdev_region(pci_devt, pci_devs_max);

        return -1;
}


//==================================complete==============================================
static void pci_remove(struct pci_dev *pdev)
{
        cdev_del(dev_data->cdev);
        unregister_chrdev_region(pci_devt, pci_devs_max);

        free_irq(dev_data->pdev->irq, dev_data->pdev);

        iounmap(dev_data->mmio_addr);

        pci_release_region(dev_data->pdev, BAR_MMIO);
        pci_disable_device(dev_data->pdev);

        tprintk("%s driver (major %d) unloaded\n", DRIVER_NAME, pci_major);
}

static struct pci_driver pci_driver =
{
        .name = DRIVER_NAME,
        .id_table = pci_ids,
        .probe = pci_probe,
        .remove = pci_remove,
};

//-----------------------------------------------------------------
// module init/exit

static int __init pci_init(void)
{
        dev_data = kmalloc(sizeof(struct device_data), GFP_KERNEL);
        if(!dev_data) {
                printk(KERN_ERR "cannot allocate device data memory\n");
                return -1; // 他の値に戻るべきか？
        }
        printk(KERN_ERR "Xilinx: Xilinx driver Revision: 0.717\n");
        printk(KERN_ERR "Virtex-7_XC7VX485T-2FFG1761C　Driver Loaded!!!\n");

        return pci_register_driver(&pci_driver);
 
}

static void __exit pci_exit(void)
{
        pci_unregister_driver(&pci_driver);
        kfree(dev_data);
}

module_init(pci_init);
module_exit(pci_exit);
