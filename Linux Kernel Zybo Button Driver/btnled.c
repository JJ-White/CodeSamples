/*
 * btnled.c
 * 
 * Zybo button, switch, and led driver, for ES6 (Fontys HBO-ICT)
 */

// Includes
#include <linux/kernel.h>    	/* For doing kernel work */
#include <linux/module.h>   	/* For doing module work */
#include <linux/kobject.h>   	/* For using sysfs */
#include <linux/device.h>		/* For device specific functions */
#include <linux/stddef.h>		/* Standard C library definitions */
#include <linux/fs.h>           /* For devfs file operations */
#include <linux/kdev_t.h>       /* For getting minor number */
#include <asm/uaccess.h>        /* For put_user */
#include <asm/io.h>				/* For Register access (ioremap for example) */
#include <asm/errno.h>			/* For standard error codes */


// Defines
#define SYSFS_DIR  		"btnled"
#define SYSFS_FILE 		"conf"

#define DEV_NAME_LED    "led"   
#define MAJ_NR_LED      245
#define DEV_NAME_BTN    "button"   
#define MAJ_NR_BTN      246
#define DEV_NAME_SWH    "switch"   
#define MAJ_NR_SWH      247

#define DEV_BUF_LEN     128      

#define BTN_ADDR        0x41200000
#define NR_OF_BTNS      4
#define LED_ADDR        0x41210000
#define NR_OF_LEDS      4
#define SWH_ADDR        0x41220000
#define NR_OF_SWHS      4

// Function prototypes
int __init mod_init(void);
void __exit mod_exit(void);

static ssize_t sysfs_show(struct device *dev, struct device_attribute *attr, char *buffer);
static ssize_t sysfs_store(struct device *dev, struct device_attribute *attr, const char *buffer, size_t count);

static int device_open(struct inode *, struct file *);
static int device_release(struct inode *, struct file *);
static ssize_t device_read(struct file *, char *, size_t, loff_t *);
static ssize_t device_write(struct file *, const char *, size_t, loff_t *);

static void initLeds(void);
static int setLedOn(char lednr, bool state);
static int readState(char type, char nr);

// Module information
module_init(mod_init);
module_exit(mod_exit);
MODULE_LICENSE("Dual BSD/GPL");
MODULE_AUTHOR("__________________");
MODULE_DESCRIPTION("Zybo button driver");

// Device attributes
static DEVICE_ATTR(conf, S_IRUSR | S_IRGRP | S_IWUSR | S_IWGRP, sysfs_show, sysfs_store);
static struct attribute *attrs[] = {&dev_attr_conf.attr, NULL};
static struct attribute_group attr_group = {.attrs = attrs,};
static struct kobject *peek_poke_obj = NULL;

static struct file_operations fopsLed = {.read = device_read, .write = device_write, .open = device_open, .release = device_release};
//static struct file_operations fopsBtn = {.read = device_read, .write = device_write, .open = device_open, .release = device_release};
//static struct file_operations fopsSwh = {.read = device_read, .write = device_write, .open = device_open, .release = device_release};

// Global variables
static char sysfsBuffer[PAGE_SIZE + 1] = {'\0'};
static char ledstate = 0x00;

static int devOpen = 0; 
static char msg[DEV_BUF_LEN];
static char *msg_Ptr;
int major = 0;
int minor = 0;


// Methods

static void initLeds(void){
    int i = 0;
    for(i = 0; i < NR_OF_LEDS; i++)
        setLedOn(i, false);
}

static int setLedOn(
	char lednr,
	bool state)
{
    void __iomem *io_base = NULL;
    
    if((int)lednr > NR_OF_LEDS - 1){
        printk(KERN_INFO "btnled: Invalid LED assigned: %i\n", lednr);
        scnprintf(sysfsBuffer, PAGE_SIZE, "%d", -EINVAL);
        return -EINVAL;
    }
	
	io_base = ioremap(LED_ADDR, 1);
	if(io_base == NULL)
	{
		printk(KERN_NOTICE "btnled: Invalid memory region (0x%08x)!\n", LED_ADDR);
		scnprintf(sysfsBuffer, PAGE_SIZE, "%d", -EFAULT);
		return -EFAULT;
	}
	
	ledstate &= ~(1 << lednr);
	ledstate |= state << lednr;
	iowrite32(ledstate, io_base);
	scnprintf(sysfsBuffer, PAGE_SIZE, "%i %i", lednr, state);

	iounmap(io_base);
    return 0;
}

static int readState(
    char type,
	char nr)
{
    int addr = 0x00000000;
    int max = 0;
    int val = 0;
	void __iomem *io_base = NULL;

    switch(type){
        case 'b': addr = BTN_ADDR; max = NR_OF_BTNS; break;
        case 's': addr = SWH_ADDR; max = NR_OF_SWHS; break;
        default:
        printk(KERN_INFO "btnled: Invalid type argument: %c\n", type);
		scnprintf(sysfsBuffer, PAGE_SIZE, "%d", -EINVAL);
        return -EINVAL;
    }
    
    if((int)nr > max - 1){
        printk(KERN_INFO "btnled: Invalid read nr: %c %i\n", type, nr);
        scnprintf(sysfsBuffer, PAGE_SIZE, "%d", -EINVAL);
        return -EINVAL;
    }
    
	io_base = ioremap(addr, 1);
	if(io_base == NULL)
	{
		printk(KERN_INFO "btnled: Invalid memory region (0x%08x)!\n", addr);
		scnprintf(sysfsBuffer, PAGE_SIZE, "%d", -EFAULT);
		return -EFAULT;
	}
	
	val = ioread32(io_base);
	iounmap(io_base);
	
	val = (val >> nr) & 1;
	scnprintf(sysfsBuffer, PAGE_SIZE, "%d", val);
	
	return val;

}

static ssize_t sysfs_show(
	struct device *dev, 
	struct device_attribute *attr, 
	char *buffer)
{
    return scnprintf(buffer, PAGE_SIZE, "%s", sysfsBuffer);
}

static ssize_t sysfs_store(
	struct device *dev, 
	struct device_attribute *attr, 
	const char *buffer, 
	size_t count)
{
	char mode = 0;
	int nr = 0;
	int state = -1;
	int params = 0;
	
	params = sscanf(buffer, "%c %i %i", &mode, &nr, &state);
	if(params < 2)
	{
		printk(KERN_NOTICE "btnled: Nr. of parameters invalid!\n");
		scnprintf(sysfsBuffer, PAGE_SIZE, "%d", -EINVAL);
		return -EINVAL;
	}
	
	switch(mode)
	{
		case 's':
		case 'b':
			readState(mode, nr);
			break;
		case 'l':
			setLedOn(nr, state);
			break;
		default:
			scnprintf(sysfsBuffer, PAGE_SIZE, "%d", -EINVAL);
			printk(KERN_NOTICE "btnled: Mode invalid: %c\n", mode);
			return -EINVAL;
	}
	
	return count;
}

static int device_open(
    struct inode *inode, 
    struct file *file)
{
    int state = -1;
    
    major = MAJOR(inode -> i_rdev);
    minor = MINOR(inode -> i_rdev);

    if (devOpen)
        return -EBUSY;
        
    switch(major){
        case MAJ_NR_LED:
            scnprintf(msg, DEV_BUF_LEN, "%i\n", (ledstate >> minor) & 1);
            break;
        case MAJ_NR_BTN:
            state = readState('b', minor);
            scnprintf(msg, DEV_BUF_LEN, "%04x\n", state);
            break;
        case MAJ_NR_SWH:
            state = readState('s', minor);
            scnprintf(msg, DEV_BUF_LEN, "%04x\n", state);
            break;
        default:
            printk(KERN_NOTICE "btnled: major number not supported: %i\n", major);
            return -EINVAL;
    }

    devOpen++;
    scnprintf(msg, DEV_BUF_LEN, "%i, %i\n", major, minor);
    //scnprintf(msg, DEV_BUF_LEN, "%04x\n", ledstate);
    msg_Ptr = msg;
    try_module_get(THIS_MODULE);

    return 0;
}

static int device_release(
    struct inode *inode, 
    struct file *file)
{
    devOpen--;
    module_put(THIS_MODULE);
    return 0;
}

static ssize_t device_read(
    struct file *filp,
    char *buffer,
    size_t length,
    loff_t * offset)
{
  int size = 0;

  if (*msg_Ptr == 0)
    return 0;

  while (length && *msg_Ptr) {
    put_user(*(msg_Ptr++), buffer++);

    length--;
    size++;
  }

  return size;
}

static ssize_t device_write(
    struct file *filp, 
    const char *buff, 
    size_t len, 
    loff_t * off)
{
    char tempBuff[len];
    int ret = 0;
    int a = 0;
	int params = 0;
    
    ret = copy_from_user(tempBuff, buff, len);
    if(ret != 0){
        printk(KERN_NOTICE "btnled: copy_from_user failed %i\n", ret);
        return -EAGAIN;
    }
    
    params = sscanf(buff, "%i", &a);
	if(params != 1)
	{
		printk(KERN_NOTICE "btnled: Nr. of parameters invalid!\n");
		scnprintf(sysfsBuffer, PAGE_SIZE, "%d", -EINVAL);
		return -EINVAL;
	}
    
    switch(major){
        case MAJ_NR_LED:
            setLedOn(minor, a);
            break;
        case MAJ_NR_BTN:
            printk(KERN_NOTICE "btnled: this operation isn't supported on a button.\n");
            return -EINVAL; 
        case MAJ_NR_SWH:
            printk(KERN_NOTICE "btnled: this operation isn't supported on a switch.\n");
            return -EINVAL;
        default:
            printk(KERN_NOTICE "btnled: major number not supported: %i\n", major);
            return -EINVAL;
    }

	return len;
}

int __init mod_init(void)
{    
    int result = 0;
    
    // sysfs init
    peek_poke_obj = kobject_create_and_add(SYSFS_DIR, kernel_kobj);
    if (peek_poke_obj == NULL)
    {
        printk(KERN_ERR "%s kobject_create_and_add failed\n", SYSFS_FILE);
        return -ENOMEM;
    }
    
    result = sysfs_create_group(peek_poke_obj, &attr_group);
    if (result != 0)
    {
        printk(KERN_ERR "%s sysfs_create_group failed with result %d\n", SYSFS_FILE, result);
        kobject_put(peek_poke_obj);
        return -ENOMEM;
    }
    printk(KERN_INFO "/sys/kernel/%s/%s created\n", SYSFS_DIR, SYSFS_FILE);
    
    // devfs init
    result = register_chrdev(MAJ_NR_LED, DEV_NAME_LED, &fopsLed);
    if (result < 0) 
        printk(KERN_ALERT "registering led device failed with %d\n", result);
    
    // Not working for multiple files
    //result = register_chrdev(MAJ_NR_BTN, DEV_NAME_BTN, &fopsBtn);
    //if (result < 0) 
    //    printk(KERN_ALERT "registering button device failed with %d\n", result);
    //
    //result = register_chrdev(MAJ_NR_SWH, DEV_NAME_SWH, &fopsSwh);
    //if (result < 0) 
    //    printk(KERN_ALERT "registering switch device failed with %d\n", result);
    
  
    initLeds();
    if(result < 0){
        kobject_put(peek_poke_obj);
        unregister_chrdev(MAJ_NR_LED, DEV_NAME_LED);
        unregister_chrdev(MAJ_NR_BTN, DEV_NAME_BTN);
        unregister_chrdev(MAJ_NR_SWH, DEV_NAME_SWH);    
    }
    return result;
}

void __exit mod_exit(void)
{
    initLeds();
    
    unregister_chrdev(MAJ_NR_LED, DEV_NAME_LED);
    unregister_chrdev(MAJ_NR_BTN, DEV_NAME_BTN);
    unregister_chrdev(MAJ_NR_SWH, DEV_NAME_SWH);
    
    kobject_put(peek_poke_obj);
    printk(KERN_INFO "/sys/kernel/%s/%s removed\n", SYSFS_DIR, SYSFS_FILE);
}
