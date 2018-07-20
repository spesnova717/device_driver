#include <linux/init.h>
#include <linux/module.h>
#include <linux/pci.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>
#include <linux/sched.h>


#define VENDOR_ID 0x10ee
#define DEVICE_ID 0x8038

#define DRIVER_NAME "vcu108"
#define BOARD_NAME "fpga"
#define NUM_BARS 1  //we use up to BAR2


//fileio.c functions for device
int fpga_open(struct inode *inode, struct file *file);
int fpga_close(struct inode *inode, struct file *file);
ssize_t fpga_read(struct file *file, char __user *buf, size_t count, loff_t *pos);
ssize_t fpga_write(struct file *file, const char __user *buf, size_t count, loff_t *pos);
loff_t fpga_llseek(struct file *filp, loff_t off, int whence);

/* Maximum size of driver buffer (allocated with kalloc()).
 * Needed to copy data from user to kernel space */
static const size_t BUFFER_SIZE = PAGE_SIZE;

//Keep track of bits and bobs that we need for the driver
struct DevInfo_t {
  /* the kernel pci device data structure */
  struct pci_dev *pciDev;
  
  /* upstream root node */
  struct pci_dev *upstream;
  
  /* kernel's virtual addr. for the mapped BARs */
  void * __iomem bar[NUM_BARS];
  
  /* length of each memory region. Used for error checking. */
  size_t barLengths[NUM_BARS];

  /* temporary buffer. If allocated, will be BUFFER_SIZE. */
  char *buffer;
  
  /* Mutex for this device. */
  struct semaphore sem;
  
  /* PID of process that called open() */
  int userPID;
  
  /* character device */
  dev_t cdevNum;
  struct cdev cdev;
  struct class *myClass;
  struct device *device;

};
