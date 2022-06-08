#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <asm/uaccess.h>
#include <linux/slab.h>
#include <linux/gpio.h>
//#include <mach/platform.h>
#include <linux/io.h>
#include <linux/delay.h>
#define BUZZER_MAJOR 225
#define BUZZER_NAME "BUZZER_DRIVER"
#define BCM2711_PERL_BASE 0xFE000000
#define GPIO_BASE (BCM2711_PERL_BASE+0x200000)
#define GPIO_SIZE 256
char buzzer_usage = 0;
static void *buzzer_map;
volatile unsigned *buzzer;
volatile int BUZZER_PIN = 4;
static int buzzer_open(struct inode *minode, struct file *mfile) {
    unsigned char index;
    if (buzzer_usage != 0)
    	return -EBUSY;
    buzzer_usage = 1;
    // GPIO 
    buzzer_map = ioremap(GPIO_BASE, GPIO_SIZE);
    if (!buzzer_map) {
        printk("error: mapping gpio memory");
        iounmap(buzzer_map);
        return -EBUSY;
    }
    buzzer = (volatile unsigned int *)buzzer_map;
    *(buzzer) &= ~(0x07 << (3 * BUZZER_PIN));
    *(buzzer) |= (0x01 << (3 * BUZZER_PIN));
    return 0;
}
static int buzzer_release(struct inode *minode, struct file *mfile){
    buzzer_usage = 0;
    if (buzzer)
        iounmap(buzzer);
    return 0;
}
static int buzzer_write(struct file *mfile, const char *gdata, size_t length, loff_t
*off_what) {
    char tmp_buf;
    int result;
    result = copy_from_user(&tmp_buf, gdata, length);
    if (result < 0) {
        printk("Error: copy from user");
        return result;
    }
    printk("buzzer values = %0X\n", tmp_buf);
    //if(tmp_buf==0)
  //  	*(buzzer + 10) = (0x01 << BUZZER_PIN);
   // else
   // 	*(buzzer + 7) = (0x01 << BUZZER_PIN)
   int i=0;
    for(i=0; i < 2; i++){
	*(buzzer + 10) = 0x01 << BUZZER_PIN;
	mdelay(100);
	*(buzzer + 7) = 0x01 << BUZZER_PIN;
	mdelay(100);
    }
    return length;
}
static struct file_operations buzzer_fops = {
    .owner = THIS_MODULE,
    .open = buzzer_open,
    .release = buzzer_release,
    .write = buzzer_write,
};
static int buzzer_init(void) {
    int result;
    result = register_chrdev(BUZZER_MAJOR, BUZZER_NAME, &buzzer_fops);
    if (result < 0) {
        printk(KERN_WARNING "Can't get any major number\n");
        return result;
    }
    return 0;
}
static void buzzer_exit(void) {
    unregister_chrdev(BUZZER_MAJOR, BUZZER_NAME);
    printk("BUZZER module removed.\n");
}
module_init(buzzer_init);
module_exit(buzzer_exit);
MODULE_LICENSE("GPL");
