#include <linux/module.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/io.h>
#include <linux/slab.h>
#include <linux/gpio.h>
#include <linux/delay.h>
#include <asm/uaccess.h>

#define DC_MAJOR    224
#define DC_NAME    "DC_DRIVER"

#define BCM2711_PERL_BASE 0xFE000000
#define GPIO_BASE    (BCM2711_PERL_BASE+0x200000)
#define GPIO_SIZE    256

char dc_usage = 0;
static void *dc_map;
volatile unsigned *dc;

static int dc_open(struct inode *inode, struct file *mfile) {
    if (dc_usage != 0)
        return -EBUSY;
    dc_usage = 1;

    dc_map = ioremap(GPIO_BASE, GPIO_SIZE);

    if (!dc_map) {
        printk("error: mapping gpio memory");
        iounmap(dc_map);
        return -EBUSY;
    }
    dc = (volatile unsigned int *) dc_map;

    *(dc + 1) &= ~(0x07 << (3 * 8));
    *(dc + 2) &= ~(0x07 << (3 * 3));
    *(dc + 2) &= ~(0x07 << (3 * 4));

    *(dc + 1) |= 0x01 << (3 * 8);
    *(dc + 2) |= 0x01 << (3 * 3);
    *(dc + 2) |= 0x01 << (3 * 4);

    *(dc + 10) = (0x01 << 18);
    *(dc + 10) = (0x01 << 23);
    *(dc + 10) = (0x01 << 24);

    return 0;
}

static int dc_release(struct inode *minode, struct file *mfile) {
    dc_usage = 0;

    if (dc) iounmap(dc);

    return 0;
}

static int dc_write(struct file *minode, const char *gdata, size_t length, loff_t *off_what) {
    char tmp_buf;
    int result;

    result = copy_from_user(&tmp_buf, gdata, length);

    if (result < 0) {
        printk("Error: copy from user");
        return result;
    }
    printk("data from user: %c\n", tmp_buf);

    if (tmp_buf == '0') {
	*(dc + 10) = (0x01 << 18);
	*(dc + 10) = (0x01 << 23);
	*(dc + 10) = (0x01 << 24);
    } else if (tmp_buf == '1') {
        *(dc + 10) = (0x01 << 24);
        *(dc + 7) = (0x01 << 23);
        *(dc + 7) = (0x01 << 18);
    } else if (tmp_buf == '2') {
        *(dc + 7) = (0x01 << 24);
        *(dc + 7) = (0x01 << 23);
        *(dc + 10) = (0x01 << 18);
    }
    return length;
}

static struct file_operations dc_fops = {
        .owner = THIS_MODULE,
        .open = dc_open,
        .release = dc_release,
        .write = dc_write,
};

static int dc_init(void) {
    int result;
    result = register_chrdev(DC_MAJOR, DC_NAME, &dc_fops);
    if (result < 0) {
        printk(KERN_WARNING
        "Can't get any major!\n");
        return result;
    }
    printk("dc module uploaded!.\n");
    return 0;
}

static void dc_exit(void) {
    unregister_chrdev(DC_MAJOR, DC_NAME);
    printk("dc module removed.\n");
}

module_init(dc_init);
module_exit(dc_exit);
MODULE_LICENSE("GPL");
