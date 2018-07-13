// Author: spesnova717

#include "FPGA.h"


MODULE_LICENSE("TUS Lab");
MODULE_AUTHOR("spesnova717");
MODULE_DESCRIPTION("Driver for PCI Express Xilinx FPGA.");

/* Forward Static PCI driver functions for kernel module */
static int probe(struct pci_dev *dev, const struct pci_device_id *id);
static void remove(struct pci_dev *dev);

// static int  init_chrdev (struct aclpci_dev *aclpci);


//Fill in kernel structures with a list of ids this driver can handle
static struct pci_device_id idTable[] = {
	{ PCI_DEVICE(VENDOR_ID, DEVICE_ID) },
	{ 0, },
};
MODULE_DEVICE_TABLE(pci, idTable);

static struct pci_driver fpgaDriver = {
	.name = DRIVER_NAME,
	.id_table = idTable,
	.probe = probe,
	.remove = remove,
};

struct file_operations fileOps = {
	.owner =    THIS_MODULE,
	.read =     fpga_read,
	.write =    fpga_write,
	.open =     fpga_open,
	.release =  fpga_close,
};


/*I/0 - should move to separate file at some point */
struct IOCmd_t {
	uint32_t cmd; //command word
	uint8_t barNum; //which bar we are read/writing to
	uint32_t devAddr; // address relative to BAR that we are read/writing to
	void * userAddr; // virtual address in user space to read/write from
};

//(struct pci_dev *dev, const struct pci_device_id *id)
//unsigned long barStart, barEnd, barLength;

void __iomem *pci_iomap_wc_range(struct pci_dev *dev,
             int bar,
                     unsigned long offset,
                             unsigned long maxlen)
{
  struct DevInfo_t *devInfo;
  resource_size_t start = pci_resource_start(devInfo->pciDev, 0);
  resource_size_t len = pci_resource_len(devInfo->pciDev, 0);
  unsigned long flags = pci_resource_flags(devInfo->pciDev, 0);


  if (flags & IORESOURCE_IO)
    return NULL;

  if (len <= offset || !start)
    return NULL;

  len -= offset;
  start += offset;
  if (maxlen && len > maxlen)
    len = maxlen;

  if (flags & IORESOURCE_MEM)
    return ioremap_wc(start, len);

  /* What? */
  return NULL;
}

void __iomem *pci_iomap_wc(struct pci_dev *dev, int bar, unsigned long maxlen)
{
  return pci_iomap_wc_range(dev, bar, 0, maxlen);
}

//#################################


	
//#################################
/*
ssize_t rw_dispatcher(struct file *filePtr, char __user *buf, size_t count, bool rwFlag){



	printk(KERN_INFO "[FPGA] rw_dispatcher: Entering function.\n");

	if (down_interruptible(&devInfo->sem)) {
		printk(KERN_WARNING "[FPGA] rw_dispatcher: Unable to get semaphore!\n");
		return -1;
	}

	copy_from_user(&iocmd, (void __user *) buf, sizeof(iocmd));

	//Map the device address to the iomaped memory
	startAddr = (void*) (devInfo->bar[iocmd.barNum] + iocmd.devAddr) ;

	printk(KERN_INFO "[FPGA] rw_dispatcher: Reading/writing %u bytes from user address 0x%p to device address %u.\n", (unsigned int) count, iocmd.userAddr, iocmd.devAddr);
	while (count > 0){
		bytesToTransfer = (count > BUFFER_SIZE) ? BUFFER_SIZE : count;


		if (rwFlag) {
			//First read from device into kernel memory 
			memcpy_fromio(devInfo->buffer, startAddr + bytesDone, bytesToTransfer);
      printk(KERN_INFO "[FPGA] count_read.\n");
			//Then into user space
			copy_to_user(iocmd.userAddr + bytesDone, devInfo->buffer, bytesToTransfer);
		}
		else{
			//First copy from user to buffer
			copy_from_user(devInfo->buffer, iocmd.userAddr + bytesDone, bytesToTransfer);
      printk(KERN_INFO "[FPGA] count_write.\n");
			//Then into the device
			memcpy_toio(startAddr + bytesDone, devInfo->buffer, bytesToTransfer);
		}
		bytesDone += bytesToTransfer;
		count -= bytesToTransfer;
	}
	up(&devInfo->sem);
	return bytesDone;
}
*/
int fpga_open(struct inode *inode, struct file *filePtr) {
	//Get a handle to our devInfo and store it in the file handle
	struct DevInfo_t * devInfo = 0;

	printk(KERN_INFO "[FPGA] fpga_open: Entering function.\n");

	devInfo = container_of(inode->i_cdev, struct DevInfo_t, cdev);

	if (down_interruptible(&devInfo->sem)) {
		printk(KERN_WARNING "[FPGA] fpga_open: Unable to get semaphore!\n");
		return -1;
	}

	filePtr->private_data = devInfo;

	//Record the PID of who opened the file
	//TODO: sort out where this is used
	devInfo->userPID = current->pid;

	//Return semaphore
	up(&devInfo->sem);

	if (down_interruptible(&devInfo->sem)) {
		printk(KERN_WARNING "[FPGA] fpga_open: Unable to get semaphore!\n");
		return -1;
	}

	up(&devInfo->sem);

	printk(KERN_INFO "[FPGA] fpga_open: Leaving function.\n");

	return 0;
}
int fpga_close(struct inode *inode, struct file *filePtr){

	struct DevInfo_t * devInfo = (struct DevInfo_t *)filePtr->private_data;

	if (down_interruptible(&devInfo->sem)) {
		printk(KERN_WARNING "[FPGA] fpga_close: Unable to get semaphore!\n");
		return -1;
	}

	//TODO: some checking of who is closing.
	up(&devInfo->sem);

	return 0;
}

//Pass-through to main dispatcher
ssize_t fpga_read(struct file *filePtr, char __user *buf, size_t count, loff_t *pos){
	struct DevInfo_t * devInfo = (struct DevInfo_t *) filePtr->private_data;
	//Read the command from the buffer
	struct IOCmd_t iocmd; 
	void * startAddr;

	size_t bytesDone = 0;
	size_t bytesToTransfer = 0;
	
	printk(KERN_INFO "[FPGA] read: Entering function.\n");

	if (down_interruptible(&devInfo->sem)) {
		printk(KERN_WARNING "[FPGA] read: Unable to get semaphore!\n");
		return -1;
	}

	copy_from_user(&iocmd, (void __user *) buf, sizeof(iocmd));	

	//Map the device address to the iomaped memory
	startAddr = (void*) (devInfo->bar[iocmd.barNum] + iocmd.devAddr) ;

	printk(KERN_INFO "[FPGA] read: Reading %u bytes from user address 0x%p \
			to device address %u.\n", \
			(unsigned int) count, iocmd.userAddr, iocmd.devAddr);

	while (count > 0){
		bytesToTransfer = (count > BUFFER_SIZE) ? BUFFER_SIZE : count;

		//First read from device into kernel memory 
		memcpy_fromio(devInfo->buffer, startAddr + bytesDone, bytesToTransfer);
      	printk(KERN_INFO "[FPGA] count_read.\n");
		//Then into user space
		copy_to_user(iocmd.userAddr + bytesDone, devInfo->buffer, bytesToTransfer);

		bytesDone += bytesToTransfer;
		count -= bytesToTransfer;
	}

	up(&devInfo->sem);
	return bytesDone;
}
ssize_t fpga_write(struct file *filePtr, const char __user *buf, size_t count, loff_t *pos){
	struct DevInfo_t * devInfo = (struct DevInfo_t *) filePtr->private_data;
	//Read the command from the buffer
	struct IOCmd_t iocmd; 
	void * startAddr;

	size_t bytesDone = 0;
	size_t bytesToTransfer = 0;

	printk(KERN_INFO "[FPGA] write: Entering function.\n");

	if (down_interruptible(&devInfo->sem)) {
		printk(KERN_WARNING "[FPGA] write: Unable to get semaphore!\n");
		return -1;
	}

	copy_from_user(&iocmd, (void __user *) buf, sizeof(iocmd));	

	//Map the device address to the iomaped memory
	startAddr = (void*) (devInfo->bar[iocmd.barNum] + iocmd.devAddr) ;

	printk(KERN_INFO "[FPGA] write: Writing %u bytes from user address 0x%p \
			to device address %u.\n", \
			(unsigned int) count, iocmd.userAddr, iocmd.devAddr);

	while (count > 0){
		bytesToTransfer = (count > BUFFER_SIZE) ? BUFFER_SIZE : count;

		//First copy from user to buffer
		copy_from_user(devInfo->buffer, iocmd.userAddr + bytesDone, bytesToTransfer);
        printk(KERN_INFO "[FPGA] count_write.\n");
		//Then into the device
		memcpy_toio(startAddr + bytesDone, devInfo->buffer, bytesToTransfer);
		
		bytesDone += bytesToTransfer;
		count -= bytesToTransfer;
	}
	up(&devInfo->sem);
	return bytesDone;
}

static int setup_chrdev(struct DevInfo_t *devInfo){
	/*
	Setup the /dev/deviceName to allow user programs to read/write to the driver.
	*/

	int devMinor = 0;
	int devMajor = 0; 
	int devNum = -1;

	int result = alloc_chrdev_region(&devInfo->cdevNum, devMinor, 1 /* one device*/, BOARD_NAME);
	if (result < 0) {
		printk(KERN_WARNING "Can't get major ID\n");
		return -1;
	}
	devMajor = MAJOR(devInfo->cdevNum);
	devNum = MKDEV(devMajor, devMinor);
	
	//Initialize and fill out the char device structure
	cdev_init(&devInfo->cdev, &fileOps);
	devInfo->cdev.owner = THIS_MODULE;
	devInfo->cdev.ops = &fileOps;
	result = cdev_add(&devInfo->cdev, devNum, 1 /* one device */);
	if (result) {
		printk(KERN_NOTICE "Error %d adding char device for FPGA driver with major/minor %d / %d", result, devMajor, devMinor);
		return -1;
	}

	return 0;
}
unsigned long barStart, barEnd, barLength;
static int map_bars(struct DevInfo_t *devInfo){
	/*
	Map the device memory regions into kernel virtual address space.
	Report their sizes in devInfo.barLengths
	*/
	int ct = 0;
	//unsigned long barStart, barEnd, barLength;
	for (ct = 0; ct < NUM_BARS; ct++){
		printk(KERN_INFO "[FPGA] Trying to map BAR #%d of %d.\n", ct, NUM_BARS);
		barStart = pci_resource_start(devInfo->pciDev, ct);
		barEnd = pci_resource_end(devInfo->pciDev, ct);
		barLength = barEnd - barStart + 1;

		devInfo->barLengths[ct] = barLength;

		//Check for empty BAR
		if (!barStart || !barEnd) {
			devInfo->barLengths[ct] = 0;
			printk(KERN_INFO "[FPGA] Empty BAR #%d.\n", ct);
			continue;
		}

		//Check for messed up BAR
		if (barLength < 1) {
			printk(KERN_WARNING "[FPGA] BAR #%d length is less than 1 byte.\n", ct);
			continue;
		}

		// If we have a valid bar region then map the device memory or
		// IO region into kernel virtual address space  
    //
    // startAddr = (void*) (devInfo->bar[iocmd.barNum] + iocmd.devAddr) ;
		devInfo->bar[ct] = pci_iomap(devInfo->pciDev, ct, barLength);
    //devInfo->bar[ct] = ioremap(devInfo->pciDev, barLength);


		if (!devInfo->bar[ct]) {
			printk(KERN_WARNING "[FPGA] Could not map BAR #%d.\n", ct);
			return -1;
		}

		printk(KERN_INFO "[FPGA] BAR%d mapped at 0x%p with length %lu.\n", ct, devInfo->bar[ct], barLength);
	}
	return 0;
}  

static int unmap_bars(struct DevInfo_t * devInfo){
	/* Release the mapped BAR memory */
	int ct = 0;
	for (ct = 0; ct < NUM_BARS; ct++) {
		if (devInfo->bar[ct]) {
			pci_iounmap(devInfo->pciDev, devInfo->bar[ct]);
			devInfo->bar[ct] = NULL;
		}
	}
	return 0;
}

static int print_timestamp() {
    struct timespec time;
 
    getnstimeofday(&time);
    return time.tv_sec * 1000000000L + time.tv_nsec;
}

int start_read;
int start_write;
int end_read;
int end_write;

static int probe(struct pci_dev *dev, const struct pci_device_id *id) {
		/*
		From : http://www.makelinux.net/ldd3/chp-12-sect-1
		This function is called by the PCI core when it has a struct pci_dev that it thinks this driver wants to control.
		A pointer to the struct pci_device_id that the PCI core used to make this decision is also passed to this function. 
		If the PCI driver claims the struct pci_dev that is passed to it, it should initialize the device properly and return 0. 
		If the driver does not want to claim the device, or an error occurs, it should return a negative error value.
		*/

		//Initalize driver info 
		struct DevInfo_t *devInfo = 0;

		printk(KERN_INFO "[FPGA] Entered driver probe function.\n");
		printk(KERN_INFO "[FPGA] vendor = 0x%x, device = 0x%x \n", dev->vendor, dev->device); 

		//Allocate and zero memory for devInfo

		devInfo = kzalloc(sizeof(struct DevInfo_t), GFP_KERNEL);
		if (!devInfo) {
			printk(KERN_WARNING "Couldn't allocate memory for device info!\n");
			return -1;
		}

		//Copy in the pci device info
		devInfo->pciDev = dev;

		//Save the device info itself into the pci driver
		dev_set_drvdata(&dev->dev, (void*) devInfo);

		//Setup the char device
		setup_chrdev(devInfo);    

		//Initialize other fields
		devInfo->userPID = -1;
		devInfo->buffer = kmalloc (BUFFER_SIZE * sizeof(char), GFP_KERNEL);

		//Enable the PCI
		if (pci_enable_device(dev)){
			printk(KERN_WARNING "[FPGA] pci_enable_device failed!\n");
			return -1;
		}

		pci_set_master(dev);
		pci_request_regions(dev, DRIVER_NAME);

		//Memory map the BAR regions into virtual memory space
		map_bars(devInfo);

		//TODO: proper error catching and memory releasing
		sema_init(&devInfo->sem, 1);

		int val[2048];
		int val_res[2048];
		int t;
		/*
     * Kenel Dump
    for(t=0;t<2000;t++){
			val[t] = 0x12345678;
      val_res[t] = 0;
		}
*/
    start_write = print_timestamp();
		memcpy_toio(devInfo->bar[0], val, 4096);
    memcpy_toio(devInfo->bar[0], val, 4096);
    memcpy_toio(devInfo->bar[0], val, 4096);
    memcpy_toio(devInfo->bar[0], val, 4096);
    memcpy_toio(devInfo->bar[0], val, 4096);
    memcpy_toio(devInfo->bar[0], val, 4096);
    memcpy_toio(devInfo->bar[0], val, 4096);
    memcpy_toio(devInfo->bar[0], val, 4096);
    memcpy_toio(devInfo->bar[0], val, 4096);
    memcpy_toio(devInfo->bar[0], val, 4096);
    memcpy_toio(devInfo->bar[0], val, 4096);
    memcpy_toio(devInfo->bar[0], val, 4096);
    memcpy_toio(devInfo->bar[0], val, 4096);
    memcpy_toio(devInfo->bar[0], val, 4096);
    memcpy_toio(devInfo->bar[0], val, 4096);
    memcpy_toio(devInfo->bar[0], val, 4096);
    end_write = print_timestamp();

    start_read = print_timestamp();
		memcpy_fromio(val_res, devInfo->bar[0], 4096);
    memcpy_fromio(val_res, devInfo->bar[0], 4096);
    memcpy_fromio(val_res, devInfo->bar[0], 4096);
    memcpy_fromio(val_res, devInfo->bar[0], 4096);
    memcpy_fromio(val_res, devInfo->bar[0], 4096);
    memcpy_fromio(val_res, devInfo->bar[0], 4096);
    memcpy_fromio(val_res, devInfo->bar[0], 4096);
    memcpy_fromio(val_res, devInfo->bar[0], 4096);
    memcpy_fromio(val_res, devInfo->bar[0], 4096);
    memcpy_fromio(val_res, devInfo->bar[0], 4096);
    memcpy_fromio(val_res, devInfo->bar[0], 4096);
    memcpy_fromio(val_res, devInfo->bar[0], 4096);
    memcpy_fromio(val_res, devInfo->bar[0], 4096);
    memcpy_fromio(val_res, devInfo->bar[0], 4096);
    memcpy_fromio(val_res, devInfo->bar[0], 4096);
    memcpy_fromio(val_res, devInfo->bar[0], 4096);
    end_read = print_timestamp();

		//printk("memcpy_fromio = 0x%lx\n",val_res[1024]);
    printk("write = %d\n",end_write - start_write);
    printk("read = %d\n",end_read - start_read);
		return 0;

}

static void remove(struct pci_dev *dev) {

	struct DevInfo_t *devInfo = 0;
	
	printk(KERN_INFO "[FPGA] Entered FPGA driver remove function.\n");
	
	devInfo = (struct DevInfo_t*) dev_get_drvdata(&dev->dev);
	if (devInfo == 0) {
		printk(KERN_WARNING "[FPGA] remove: devInfo is 0");
		return;
	}

	//Clean up the char device
	cdev_del(&devInfo->cdev);
	unregister_chrdev_region(devInfo->cdevNum, 1);

	//Release memory
	unmap_bars(devInfo);

	//TODO: does order matter here?
	pci_release_regions(dev);
	pci_disable_device(dev);

	kfree(devInfo->buffer);
	kfree(devInfo);

}

static int fpga_init(void){
	printk(KERN_INFO "[FPGA] Loading FPGA driver!\n");
	return pci_register_driver(&fpgaDriver);
}

static void fpga_exit(void){
	printk(KERN_INFO "[FPGA] Exiting FPGA driver!\n");
	pci_unregister_driver(&fpgaDriver);
}

module_init(fpga_init);
module_exit(fpga_exit);
