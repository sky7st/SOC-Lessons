//=========================定義include檔及define檔=======================
#define IP_MAJOR		0     // 0: dynamic major number
#define IP_MINOR		0     // 0: dynamic minor number
#define IP_BASEADDRESS 0x43C00000 //0x7A200000
#define SIZE_OF_DEVICE 0x4000*4


#include <linux/platform_device.h>    /* for platform driver functions */
#include <linux/interrupt.h>
#include <linux/sched.h>
#include <linux/kernel.h>
#include <linux/fs.h>                     //define file結構
#include <linux/types.h>
#include <linux/module.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/of_device.h>
#include <linux/of_platform.h>
#include <linux/ioport.h>
#include <linux/uaccess.h> 
//#include <asm/uaccess.h>                  //copy from/to user
#include <linux/ioctl.h>
//#include <asm/io.h>
#include <linux/io.h>

#define TX_IRQ_NUMBER 90
#define RX_IRQ_NUMBER 91




//===============================定義裝置編號=============================
static int IP_major = IP_MAJOR;  //主編號（類型編號）
static int IP_minor = IP_MINOR;  //次編號（同類型不同裝置）
module_param(IP_major, int, 0664);//設定存取模式,module_param(,,存取模式)
module_param(IP_minor, int, 0664);//設定存取模式,module_param(,,存取模式)

//=======================================================================

//===============================define device structure=============================
static struct IP_Driver { //註冊字元裝置配置
    dev_t IP_devt;
    int irq;
    struct class *class;
    //struct class_device *class_dev;
    struct device *class_dev;
    struct cdev *cdev;
} IP;
//=======================================================================
volatile unsigned long *IP_wBASEADDRESS; //預先宣告變數除存記憶體映射位址

unsigned int value=0;

//=============================file operation=========================
static int IP_open(struct inode *inode, struct file *file){
    return nonseekable_open(inode, file);
}

static int IP_release(struct inode *inode, struct file *file){

    return 0;
}

static ssize_t IP_read(struct file *file, char __user *buf, size_t size, loff_t *ppos){
    value = ioread32(IP_wBASEADDRESS+1);
    mb();
    printk("kernel:read data = %d\n",value);
    copy_to_user(buf, &value,size);
    return 0;
}


//static int IP_write(struct file *file,char __user *buf, size_t size, loff_t *ppos)
static ssize_t IP_write(struct file *file,const char __user *buf, size_t size, loff_t *ppos){
    copy_from_user(&value, buf, size);
    printk("kernel:write data = %d\n",value);
    iowrite32(value, IP_wBASEADDRESS);
    mb();
    return 0;
}

static long IP_ioctl(struct file *file, unsigned int cmnd, unsigned long arg){
    return 0;
}

static struct file_operations IP_fops = {
    .owner          = THIS_MODULE,
    .open           = IP_open,
    .release        = IP_release,
    .read           = IP_read,
    .write          = IP_write,
    .unlocked_ioctl = IP_ioctl
};

//==========================RX INIT HANDLER===========================
static irqreturn_t btn_handler(int irq, void *dev_id){

    printk("BTN Interrupt!!\n");
    return 0;
}


//=======================================================================

//==========================cdev init()     =============================
static int IP_setup_cdev(struct IP_Driver *IP_p){

    int ret, err;

    IP_p->IP_devt = MKDEV(IP_major,IP_minor); //向 kernel 取出 major/minor number

    if(IP_major){
	//register_chrdev_region 靜態取得 major number
        ret = register_chrdev_region(IP_p->IP_devt, 1, "IP-Driver");
    }else{
	//alloc_chrdev_region 動態取得 major number
        ret = alloc_chrdev_region(&IP_p->IP_devt, IP_minor, 1, "IP-Driver");
        IP_major = MAJOR(IP_p->IP_devt);//向 kernel 取出 major number
        IP_minor = MINOR(IP_p->IP_devt);//向 kernel 取出 minor number
    }
    if(ret <0)
        return ret;

    //--在「/sys/class/IP-Driver/IP-Driver/dev」新建立驅動程式資訊與規則檔---
    IP_p->class = class_create(THIS_MODULE, "IP-Driver");//登記 class , 讓驅動程式支援 udev
    if(IS_ERR(IP_p->class)){	//透過 IS_ERR() 來判斷class_create函式呼叫的成功與否
        printk("IP_setup_cdev: Can't create IP Driver class!\n");
        ret = PTR_ERR(IP_p->class);
        goto error1;
    }
    
    IP_p->cdev = cdev_alloc();//配置cdev的函式
    if(NULL == IP_p->cdev){
        printk("IP_setup_cdev: Can't alloc IP Driver cdev!\n");
        ret = -ENOMEM;
        goto error2;
    }
    
    IP_p->cdev->owner = THIS_MODULE;
    IP_p->cdev->ops = &IP_fops;

    err = cdev_add(IP_p->cdev, IP_p->IP_devt, 1);//向 kernel 登記驅動程式
    if(err){
        printk("IP_setup_cdev: Can't add IP cdev to system!\n");
        ret = -EAGAIN;
        goto error2;
    }

    //--建立「/sys/class/IP-Driver/IP-Driver」裝置名稱---
    IP_p->class_dev = device_create(IP_p->class, NULL, IP_p->IP_devt, NULL, "IP-Driver");

    if(IS_ERR(IP_p->class_dev)){ //透過 IS_ERR() 來判斷device_create函式呼叫的成功與否
        printk("IP_setup_cdev: Can't create IP class_dev to system!\n");
        ret = PTR_ERR(IP_p->class_dev);
        goto error3;
    }
    printk("IP-Driver_class_dev info: IP-Driver (%d:%d)\n",MAJOR(IP_p->IP_devt), MINOR(IP_p->IP_devt));
    
    //================request mem region======================
    //保留位址範圍,(記憶體位址,byte單位的範圍大小,引數裝置名稱)
    if(!request_mem_region(IP_BASEADDRESS, SIZE_OF_DEVICE*4,"IP"))
    {
        printk("err:Request_mem_region\n");
        return -ENODEV;
    }
    //===========ioremap_nocache===========//記憶體映射,將實體address交給linux去分配
    IP_wBASEADDRESS = (unsigned long *)ioremap_nocache(IP_BASEADDRESS, SIZE_OF_DEVICE*4);

    printk("IP > IP_ioport_write    : %p\n", IP_wBASEADDRESS);

 

    return 0;

    error3:
        cdev_del(IP_p->cdev);
    error2:
        class_destroy(IP_p->class);
    error1:
        unregister_chrdev_region(IP_p->IP_devt, 1);
        return ret;
}
//=======================================================================
static void IP_remove_cdev(struct IP_Driver *IP_p){

    device_unregister(IP_p->class_dev);
    cdev_del(IP_p->cdev);
    class_destroy(IP_p->class);
    unregister_chrdev_region(IP_p->IP_devt, 1);
    //==============ioremap_nocache==================
    iounmap((void *)IP_wBASEADDRESS);//取消記憶體映射

    release_mem_region(IP_BASEADDRESS, SIZE_OF_DEVICE*4);//取消物理記憶體位址保留

}
//=======================================================================

static int IP_init(void){
    int ret=0;
    //struct resource resource;
    printk(KERN_ALERT "hello XDC. \n");
    
    IP_setup_cdev(&IP);
    
    //Interrupt initial (interrupt number,working function,irqflags,devname,dev_id)
    
    //Ip_p->irq = irq_of_parse_and_map(node, 0);
    //ret = request_irq(29,btn_handler,IRQF_TRIGGER_RISING,"IP-Driver",NULL);
    //printk("IRQ RET:%d", ret);
    //ret = request_irq(68,btn_handler,IRQF_TRIGGER_RISING,"IP-Rx_ISR",THIS_MODULE);
    //if(ret < 0)
    //    pr_err("%s\n", "request_irq failed");
    //printk("Rx miniuart !!!!!!\n\n");


    iowrite32(0x00,IP_wBASEADDRESS); //write data to memory(value , address)
    
    return ret;
}

static int IP_probe(struct platform_device *ofdev){
    //int irq, ret;
    int ret = 0;
    struct resource *res;
    printk(KERN_INFO "START IP_init()\n");
    IP_init();
     
    res = platform_get_resource(ofdev, IORESOURCE_IRQ, 0);
    if (!res) {
      printk(KERN_INFO "could not get platform IRQ resource.\n");
      goto fail_irq;
    }
    IP.irq = res->start;
    printk(KERN_INFO "IRQ read form DTS entry as %d\n", IP.irq);
    ret = request_irq(IP.irq, btn_handler, IRQF_TRIGGER_RISING, "IP-Driver", &IP);
    if(ret)
    {
      printk(KERN_INFO "can't get assigned irq: %d\n", IP.irq);
      goto fail_irq;
    }
   return 0;

fail_irq:
   return -1;

}

static const struct of_device_id mydriver_of_match[] = {
   { .compatible = "xlnx,myip-lab5-2-1.0", },
   { /* end of list */ },
};
MODULE_DEVICE_TABLE(of, mydriver_of_match);

//static void IP_exit(void){
static int IP_exit(struct platform_device *pdev){
    printk(KERN_ALERT "Close Module. \n");
    IP_remove_cdev(&IP);

    free_irq(IP.irq, NULL);
    return 0;
}

static struct platform_driver mydrive_of_driver = {
   .probe      = IP_probe,
   .remove     = IP_exit,
   .driver = {
      .name = "xlnx,lab5-2-1.0",
      .owner = THIS_MODULE,
      .of_match_table = mydriver_of_match,
   },
};



module_platform_driver(mydrive_of_driver);            /*模組安裝之啟動函式*/
/*module_exit(IP_exit);  */           /*模組卸載之啟動函式*/

MODULE_DESCRIPTION("IP Driver");  /*此程式介紹與描述*/
MODULE_LICENSE("GPL");            /*程式 License*/
