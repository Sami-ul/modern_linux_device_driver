#include <linux/fs.h> // alloc_chrdev_region(), unregister_chrdev_region()
#include <linux/init.h> // module_init(), module_exit()
#include <linux/module.h> // THIS_MODULE, MODULE_LICENSE(), printk(), KERN_INFO, KERN_ALERT
#include <linux/slab.h> // kmalloc(), kfree(), GFP_KERNEL
#include <linux/uaccess.h> // copy_to_user(), copy_from_user()
#include <linux/cdev.h>    // cdev; cdev_init(), cdev_add(), cdev_del()
#include <linux/device.h>  // class_create(), device_create()

#define BUFFER_SIZE 900 
#define DEVICE_NAME "modern_char_device"
#define CLASS_NAME "modern_char_class"

int times_opened = 0;
int times_closed = 0;
void* memptr;

dev_t dev_num; // holds dynamically assigned major and minor numbers
struct cdev my_cdev; // character device object
struct class *device_class;
struct device *device_instance;

static ssize_t char_driver_read(struct file *pfile, char __user *buffer, size_t length,
                         loff_t *offset) {
  /*
  we take the data from our buffer and copy it to the userspace buffer 
  using copy_to_user. we use this since kernel memory is protected

  length: length of the userspace buffer

  offset: offset user wants to read from
  */

  // clamping read length to prevent reading past the buffer
  if (*offset >= BUFFER_SIZE) return 0;
  if (*offset + length > BUFFER_SIZE) length = BUFFER_SIZE - *offset;

  // direct user-memory access requires copy_to_user
  if (copy_to_user(buffer, (char *) memptr+*offset, length) != 0) return -EFAULT;

  *offset += length;
  printk(KERN_INFO "%s read %zu bytes\n", DEVICE_NAME, length);
  return length;
}

static ssize_t char_driver_write(struct file *pfile, const char __user *buffer,
                          size_t length, loff_t *offset) {
  /*
  we copy the data in the users buffer to our buffer using copy_from_user
  since kernel memory is protected.
  */
  if (*offset >= BUFFER_SIZE) return 0;
  if (*offset + length > BUFFER_SIZE) length = BUFFER_SIZE - *offset;
  if (copy_from_user((char *) memptr+*offset, buffer, length) != 0) return -EFAULT;

  *offset += length;
  printk(KERN_INFO "%s wrote %zu bytes\n", DEVICE_NAME, length);
  return length;
}

static int char_driver_open(struct inode *pinode, struct file *pfile) {
  times_opened++;
  printk(KERN_INFO "%s has been opened %d times\n", DEVICE_NAME, times_opened);
  return 0;
}

static int char_driver_close(struct inode *pinode, struct file *pfile) {
  times_closed++;
  printk(KERN_INFO "%s has been closed %d times\n", DEVICE_NAME, times_closed);
  return 0;
}

static loff_t char_driver_seek(struct file *pfile, loff_t offset, int whence) {
  loff_t new_pos;
  switch (whence) {
  case 0: // SEEK_SET
    new_pos = offset;
    break;
  case 1: // SEEK_CUR
    new_pos = pfile->f_pos + offset;
    break;
  case 2: // SEEK_END
    new_pos = BUFFER_SIZE + offset;
    break;
  default:
    return -EINVAL;
  }
  
  if (new_pos < 0) new_pos = 0;
  else if (new_pos > BUFFER_SIZE) new_pos = BUFFER_SIZE;
  pfile -> f_pos = new_pos;
  return pfile->f_pos;
}

static struct file_operations char_driver_file_operations = {
    .owner = THIS_MODULE,
    .open = char_driver_open,
    .release = char_driver_close,
    .read = char_driver_read,
    .write = char_driver_write,
    .llseek = char_driver_seek
};

static char *modern_char_devnode(const struct device *dev, umode_t *mode) {
  // intercept device creation via this callback function 
  // to set permissions to 0666 (read and write for all) on the dynamically generated /dev/ node
  if (mode) {
    *mode = 0666;
  }
  return NULL;
}

static int __init char_driver_init(void) {
  // request a dynamic major/minor number from the kernel
  if (alloc_chrdev_region(&dev_num, 0, 1, DEVICE_NAME) < 0) {
    printk(KERN_ALERT "%s failed to allocate a major number\n", DEVICE_NAME);
    return -1;
  }

  // initialize the cdev struct and add it to the kernel
  cdev_init(&my_cdev, &char_driver_file_operations);
  if (cdev_add(&my_cdev, dev_num, 1) < 0) {
    unregister_chrdev_region(dev_num, 1);
    printk(KERN_ALERT "%s failed to add cdev\n", DEVICE_NAME);
    return -1;
  }

  // register the device class by creating the metadata category in /sys/class/
  device_class = class_create(CLASS_NAME);
  if (IS_ERR(device_class)) {
    cdev_del(&my_cdev);
    unregister_chrdev_region(dev_num, 1);
    printk(KERN_ALERT "%s failed to register device class\n", DEVICE_NAME);
    return PTR_ERR(device_class);
  }

  // bind the permissions callback
  device_class->devnode = modern_char_devnode;

  // notifies the udev listener to make the device node automatically in /dev/
  device_instance = device_create(device_class, NULL, dev_num, NULL, DEVICE_NAME);
  if (IS_ERR(device_instance)) {
    class_destroy(device_class);
    cdev_del(&my_cdev);
    unregister_chrdev_region(dev_num, 1);
    printk(KERN_ALERT "%s failed to create the device\n", DEVICE_NAME);
    return PTR_ERR(device_instance);
  }

  // allocate the buffer where we store our data
  memptr = kmalloc(BUFFER_SIZE, GFP_KERNEL);
  printk(KERN_INFO "%s initialized\n", DEVICE_NAME);
  return 0;
}

static void __exit char_driver_exit(void) {
  // clean up in reverse order of creation
  device_destroy(device_class, dev_num);
  class_destroy(device_class);
  cdev_del(&my_cdev);
  unregister_chrdev_region(dev_num, 1);

  kfree(memptr);
  printk(KERN_INFO "%s uninitialized\n", DEVICE_NAME);
}

module_init(char_driver_init);
module_exit(char_driver_exit);
MODULE_LICENSE("GPL");