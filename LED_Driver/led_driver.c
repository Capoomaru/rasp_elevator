#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <asm/uaccess.h>
#include <linux/slab.h>
#include <linux/gpio.h>
#include <linux/io.h>
#define LED_MAJOR 221
#define LED_NAME "LED_DRIVER"
#define BCM2711_PERI_BASE 0xFE000000
#define GPIO_BASE (BCM2711_PERI_BASE+0x200000)
#define GPIO_SIZE 256
char led_usage = 0;
static void *led_map;
volatile unsigned *led;
static int led_open(struct inode *minode, struct file *mfile)
{
	if (led_usage != 0)
		return -EBUSY;
	led_usage = 1;
	led_map = ioremap(GPIO_BASE, GPIO_SIZE);
	if (!led_map)
	{
		printk("error: mapping gpio memory");
		iounmap(led_map);
		return -EBUSY;
	}
	led = (volatile unsigned int *)led_map;
	*led &= ~(0x07);
	*led &= ~(0x07 << (3 * 1));
	*led |= (0x01 << (3 * 1));
	*led |= (0x01);
	return 0;
}
static int led_release(struct inode *minode, struct file *mfile)
{
	led_usage = 0;
	if (led)
		iounmap(led);
	return 0;
}
static int led_write(struct file *mfile, const char *gdata, size_t length, loff_t
	*off_what)
{
	char tmp_buf;
	int result;
	result = copy_from_user(&tmp_buf, gdata, length);
	if (result < 0)
	{
		printk("Error: copy from user");
		return result;
	}
	printk("data from app : %d\n", tmp_buf);
	// Control LED
	if (tmp_buf == 0){	// MOVE ON
		*(led + 7) = (0x01);
		*(led + 10) = (0x01 << 1);
	}
	else{	// MOVE OFF
		*(led + 10) = (0x01);
		*(led + 7) = (0x01 << 1);
	}
	return length;
}
static struct file_operations led_fops =
{
	.owner = THIS_MODULE,
	.open = led_open,
	.release = led_release,
	.write = led_write,
};
static int led_init(void)
{
	int result;
	result = register_chrdev(LED_MAJOR, LED_NAME, &led_fops);
	if (result < 0)
	{
		printk(KERN_WARNING "Can't get any major!\n");
		return result;
	}
	return 0;
}
static void led_exit(void)
{
	unregister_chrdev(LED_MAJOR, LED_NAME);
	printk("LED module removed.\n");
}
module_init(led_init);
module_exit(led_exit);
MODULE_LICENSE("GPL");
