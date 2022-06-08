#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <asm/uaccess.h>
#include <linux/slab.h>
#include <linux/gpio.h>
//#include <mach/platform.h>
#include <linux/io.h>
#define FND_MAJOR 222
#define FND_NAME "FND_DRIVER"
#define BCM2711_PERL_BASE 0xFE000000
#define GPIO_BASE (BCM2711_PERL_BASE+0x200000)
#define GPIO_SIZE 256
char fnd_usage = 0;
static void *fnd_map;
volatile unsigned *fnd;
volatile int pin_num[] = {12, 13, 16, 19, 20, 21, 26};
volatile unsigned char fnd_data[3][8] = {
	{0x01, 0x00, 0x00, 0x01, 0x01, 0x01, 0x01, 0x01},	// 1층
	{0x00, 0x00, 0x01, 0x00, 0x00, 0x01, 0x00, 0x01},	// 2층
	{0x00, 0x00, 0x00, 0x00, 0x01, 0x01, 0x00, 0x01}	// 3층
};
static int fnd_open(struct inode *minode, struct file *mfile){
	int i;
	if (fnd_usage != 0)
		return -EBUSY;
	fnd_usage = 1;
	fnd_map = ioremap(GPIO_BASE, GPIO_SIZE);
	if (!fnd_map)
	{
		printk("error: mapping gpio memory");
		iounmap(fnd_map);
		return -EBUSY;
	}
	fnd = (volatile unsigned int *)fnd_map;
	for(i = 0; i < 7; ++i){
		*(fnd + (pin_num[i]/10)) &= ~(0x07 << (3 * (pin_num[i] % 10)));
		*(fnd + (pin_num[i]/10)) |= (0x01 << (3 * (pin_num[i] %10)));
	}
	return 0;
}
static int fnd_release(struct inode *minode, struct file *mfile)
{
	fnd_usage = 0;
	if (fnd)
		iounmap(fnd);
	return 0;
}
static int fnd_write(struct file *mfile, const char *gdata, size_t length, loff_t *off_what){
	char tmp_buf;
	int result;
	int i;
	result = copy_from_user(&tmp_buf, gdata, length);
	if (result < 0)
	{
		printk("Error: copy from user");
		return result;
	}
	for(i = 0; i < 7; ++i){
		*(fnd + 10) = (0x01 << pin_num[i]);	
		*(fnd + 7) = (fnd_data[(unsigned int)tmp_buf - 1][i] << pin_num[i]);
	}
	return length;
}
static struct file_operations fnd_fops =
{
	.owner = THIS_MODULE,
	.open = fnd_open,
	.release = fnd_release,
	.write = fnd_write,
};
static int fnd_init(void)
{
	int result;
	result = register_chrdev(FND_MAJOR, FND_NAME, &fnd_fops);
	if (result < 0)
	{
		printk(KERN_WARNING "Can't get any major number\n");
		return result;
	}
	return 0;
}
static void fnd_exit(void)
{
	unregister_chrdev(FND_MAJOR, FND_NAME);
	printk("FND module removed.\n");
}
module_init(fnd_init);
module_exit(fnd_exit);
MODULE_LICENSE("GPL");
