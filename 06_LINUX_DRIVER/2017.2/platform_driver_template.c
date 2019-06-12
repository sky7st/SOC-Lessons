#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/io.h>
#include <linux/interrupt.h>
#include <linux/device.h>  
#include <linux/platform_device.h>  
#include <linux/of_address.h>
#include <linux/of_device.h>
#include <linux/of_platform.h>
#include <linux/cdev.h>  
#include <linux/fs.h>  

#define IP_MAJOR		0     // 0: dynamic major number
#define IP_MINOR		0     // 0: dynamic minor number
#define IRQ_NUM 61
#define COMPATIBLE_LAB5_2 "xlnx,lab5-2-1.0"

#define DEVICENAME "MYIP"
#define IP_BASEADDRESS 0x43C00000

//===============================定義裝置編號=============================
static int IP_major = IP_MAJOR;  //主編號（類型編號）
static int IP_minor = IP_MINOR;  //次編號（同類型不同裝置）
module_param(IP_major, int, 0664);//設定存取模式,module_param(,,存取模式)
module_param(IP_minor, int, 0664);//設定存取模式,module_param(,,存取模式)

int value = 0;

struct myip_local
{
	int irq;
	unsigned long mem_start;
	unsigned long mem_end;
	void __iomem *base_addr;
} IP;

//=============================file operation=========================
// static int IP_open(struct inode *inode, struct file *file){
//     return nonseekable_open(inode, file);
// }

// static int IP_release(struct inode *inode, struct file *file){

//     return 0;
// }

// static int IP_read(struct file *file, char __user *buf, size_t size, loff_t *ppos){
//     value = ioread32(IP->base_addr+1);
//     mb();
//     printk("kernel:read data = %d\n",value);
//     copy_to_user(buf, &value,size);
//     return 0;
// }

// static int IP_write(struct file *file, char __user *buf, size_t size, loff_t *ppos){
//     copy_from_user(&value, buf, size);
//     printk("kernel:write data = %d\n",value);
//     iowrite32(value, IP->base_addr);
//     mb();
//     return 0;
// }

// static long IP_ioctl(struct file *file, unsigned int cmnd, unsigned long arg){
//     return 0;
// }

// static struct file_operations IP_fops = {
//     .owner          = THIS_MODULE,
//     .open           = IP_open,
//     .release        = IP_release,
//     .read           = IP_read,
//     .write          = IP_write,
//     .unlocked_ioctl = IP_ioctl
// };



static const struct of_device_id driver_match[] = {
	{.compatible = COMPATIBLE_LAB5_2 , },
	{/* end of table*/}
};
MODULE_DEVICE_TABLE(of, driver_match);
/*MODULE_DEVICE_TABLE(驅動裝置類型, match列表);  of:OpenFirmware*/

/* 
struct of_device_id{
	char name[32];
	char type[32];
	char compatible[128];

#ifdef __KERNEL__
	void *data;
#else
	kernel_ulong_t data;
#end if
};
*/

static irqreturn_t btn_handler(int irq, void *dev_id){
    printk("BTN Interrupt!!\n");
    return 0;
}

// static int IP_dev_setup(void){
// 	int ret;
// 	ret = platform_device_register(&my_device);
// 	if(ret){
// 		printk(KERN_ALERT "Platform_device_register failed\n");
// 		return ret;
// 	}
// 	return 0;
// }

static int IP_probe(struct platform_device *pdev){
	struct device *dev = &pdev->dev;
	struct resource *r_mem;/* IO mem resources */
	struct resource *r_irq;/* Interrupt resources */
	struct myip_local *lp = NULL;

	int ret;

	unsigned int data = 0;

	//dev_info(dev, "Device regist\n");
	//IP_dev_setup();

	dev_info(dev, "Device Tree Probing\n");

	//註冊MEM(Device)
	r_mem = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if(!r_mem){
		dev_err(dev, "invalid address\n");
		return -ENODEV;
	}
	lp = (struct myip_local *)kmalloc(sizeof(struct myip_local), GFP_KERNEL);
	if(!lp){
		dev_err(dev, "Cound not allocate myip device\n");
		return -ENOMEM;
	}

	dev_set_drvdata(dev, lp);  //把dev的data指向lp
	lp->mem_start = r_mem->start;
	lp->mem_end = r_mem->end;

	if (!request_mem_region(lp->mem_start,
				lp->mem_end - lp->mem_start + 1,
				DEVICENAME)) {
		dev_err(dev, "Couldn't lock memory region at %p\n",
			(void *)lp->mem_start);
		ret = -EBUSY;
		goto error1;
	}

	//===========ioremap_nocache===========//記憶體映射,將實體address交給linux去分配
	lp->base_addr = ioremap_nocache(lp->mem_start, lp->mem_end - lp->mem_start + 1);
	if (!lp->base_addr) {
		dev_err(dev, "myip: Could not allocate iomem\n");
		ret = -EIO;
		goto error2;
	}

	//註冊IRQ
	// r_irq = platform_get_resource(pdev, IORESOURCE_IRQ, 0); //取得resource第0個IRQ type的resource
	// if(!r_irq){
	// 	dev_info(dev, "no IRQ found\n");
	// 	dev_info(dev, "myip at 0x%08x mapped to 0x%08x\n",
	// 		(unsigned int __force)lp->mem_start,
	// 		(unsigned int __force)lp->base_addr);
	// 	return 0;
	// }
	// lp->irq = r_irq->start;
	// printk(KERN_INFO "IRQ read from DTS entry as %d\n", lp->irq);

	lp->irq = platform_get_irq(pdev, 0);
	if (lp->irq < 0) {
		dev_err(dev, "invalid IRQ\n");
		return lp->irq;
	}
	ret = request_irq(lp->irq, btn_handler, IRQF_TRIGGER_RISING, DEVICENAME, lp);
	if(ret){
		dev_err(dev, "testmodule: Could not allocate interrupt %d.\n",
			lp->irq);
		goto error3;
	}
	dev_info(dev,"myip at 0x%08x mapped to 0x%08x, irq=%d\n",
		(unsigned int __force)lp->mem_start,
		(unsigned int __force)lp->base_addr,
		lp->irq);

	iowrite32(255 ,lp->base_addr);
	return 0;

error3:
	free_irq(lp->irq, lp);
error2:
	release_mem_region(lp->mem_start, lp->mem_end - lp->mem_start + 1);
error1:
	kfree(lp);
	dev_set_drvdata(dev, NULL);
	return ret;
}
/*
struct platform_device { //設備結構體信息
	const char * name; //該平台設備的名稱
	u32 id;
	struct device dev;
	u32 num_resources;
	struct resource *resource; //定義平台設備的資源
};
struct resource { 
	resource_size_t start; //定義資源的起始地址
	resource_size_t end; //定義資源的結束地址
	const char *name; //定義資源的名稱
	unsigned long flags; //定義資源的類型，例如MEM, IO ,IRQ, DMA類型
	struct resource *parent, *sibling, *child; //資源鍊錶指針
};

struct resource *platform_get_resource(struct platform_device *dev,
                                  unsigned int type, unsigned int num)
{
	int i;
	for (i = 0; i < dev->num_resources; i++) {
		struct resource *r = &dev->resource[i];
		if (type == resource_type(r) && num-- == 0)
			return r;
	}
	return NULL;
}
#define IORESOURCE_IO        0x00000100
#define IORESOURCE_MEM        0x00000200
#define IORESOURCE_IRQ        0x00000400
#define IORESOURCE_DMA        0x00000800

##linux system err : http://www-numi.fnal.gov/offline_software/srt_public_context/WebDocs/Errors/unix_system_errors.html

<linux/slab.h>
void *kmalloc(size_t size, int flags);   flags 常用 GFP_KERNEL

static inline void dev_set_drvdata(struct device *dev, void *data)
{
	dev->driver_data = data;
}
即把 dev的driver_data指向data

static inline void *dev_get_drvdata(struct device *dev)
{
	return dev->driver_data;
}
即回傳dev->driver_data


int request_irq(unsigned int irq, irq_handler_t handler, unsigned long irqflags, const char *devname, void *dev_id); 
irq 必須是Linux映射過的虛擬編號
irqflags: default : 0
#define IRQF_TRIGGER_NONE	0x00000000
#define IRQF_TRIGGER_RISING	0x00000001
#define IRQF_TRIGGER_FALLING	0x00000002
#define IRQF_TRIGGER_HIGH	0x00000004
#define IRQF_TRIGGER_LOW	0x00000008
#define IRQF_TRIGGER_MASK	(IRQF_TRIGGER_HIGH | IRQF_TRIGGER_LOW | \
				 IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING)
#define IRQF_TRIGGER_PROBE	0x00000010
*/

static int IP_remove(struct platform_device *pdev){

	printk(KERN_ALERT "Module remove start!. \n");
	struct device *dev = &pdev->dev;
	struct myip_local *lp = dev_get_drvdata(dev);
	free_irq(lp->irq, lp);
	release_mem_region(lp->mem_start, lp->mem_end - lp->mem_start + 1);
	kfree(lp);
	dev_set_drvdata(dev, NULL);

	//platform_device_unregister(&my_device);
    
	printk(KERN_ALERT "Module remove end!. \n");
	return 0;
}



static struct platform_driver my_driver = {
   .probe      = IP_probe,
   .remove     = IP_remove,
   .driver = {
      .name = DEVICENAME,
      .owner = THIS_MODULE,
      .of_match_table = driver_match,
   },
};


module_platform_driver(my_driver); 


/* 
</linux/platform_device.h>
struct platform_driver {
	int (*probe)(struct platform_device *);
	int (*remove)(struct platform_device *);
	void (*shutdown)(struct platform_device *);
	int (*suspend)(struct platform_device *, pm_message_t state);
	int (*suspend_late)(struct platform_device *, pm_message_t state);
	int (*resume_early)(struct platform_device *);
	int (*resume)(struct platform_device *);
	struct device_driver driver;
};
</linux/device.h>
struct device_driver {
	const char * name;
	struct bus_type * bus;
	struct kobject kobj;
	struct klist klist_devices;
	struct klist_node knode_bus;
	struct module * owner;
	const char * mod_name;
	struct module_kobject * mkobj;
	int (*probe) (struct device * dev);
	int (*remove) (struct device * dev);
	void (*shutdown) (struct device * dev);
	int (*suspend) (struct device * dev, pm_message_t state);
	int (*resume) (struct device * dev);
};*/
/* 
	module_platform_driver(platform_driver); <= in /linux/platform_device.h
=   module_driver(platform_driver, platform_driver_register, platform_driver_unregister); <= in /linux/device.h
=   static int __init __driver##_init(void){
		return platform_driver_register(&platform_driver);
	}
	module_init(__driver##_init);
	static void __exit __driver##_exit(void){
		return platform_driver_unregister(&platform_driver)
	}
	module_exit(__driver##_exit);
=   
*/

MODULE_AUTHOR("Yu Ming-Lin");
MODULE_DESCRIPTION("IP Driver");  /*此程式介紹與描述*/
MODULE_LICENSE("GPL");            /*程式 License*/
