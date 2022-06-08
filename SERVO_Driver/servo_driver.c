#include <linux/module.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/io.h>
#include <linux/slab.h>
#include <linux/gpio.h>
#include <linux/delay.h>
#include <asm/uaccess.h>

#define SERVO_MAJOR    219
#define SERVO_NAME    "SERVO_DRIVER"

#define BCM2711_PERL_BASE 0xFE000000
#define GPIO_BASE    (BCM2711_PERL_BASE+0x200000)
#define GPIO_SIZE    256

char servo_usage = 0;
static void *servo_map;
volatile unsigned *servo;

static int servo_open(struct inode *inode, struct file *mfile) {
    if (servo_usage != 0)
        return -EBUSY;
    servo_usage = 1;

    servo_map = ioremap(GPIO_BASE, GPIO_SIZE);

    if (!servo_map) {
        printk("error: mapping gpio memory");
        iounmap(servo_map);
        return -EBUSY;
    }
    servo = (volatile unsigned int *) servo_map;

    *(servo + 1) &= ~(0x07 << (3 * 8));

    *(servo + 1) |= (0x01 << (3 * 8));

    *(servo + 10) = (0x01 << 18);
    return 0;
}

static int servo_release(struct inode *minode, struct file *mfile) {
    servo_usage = 0;

    if (servo) iounmap(servo);

    return 0;
}

static int servo_write(struct file *minode, const char *gdata, size_t length, loff_t *off_what) {
    char tmp_buf;
    int result;
    int i;

    result = copy_from_user(&tmp_buf, gdata, length);

    if (result < 0) {
        printk("Error: copy from user");
        return result;
    }
    printk("data from user: %d\n", tmp_buf);

    if (tmp_buf == '0') {
        *(servo + 10) = (0x01 << 18);
    } else if (tmp_buf == '1') {
        *(servo + 7) = (0x01 << 18);
    }
    return length;
}

static struct file_operations servo_fops = {
        .owner = THIS_MODULE,
        .open = servo_open,
        .release = servo_release,
        .write = servo_write,
};

static int servo_init(void) {
    int result;
    result = register_chrdev(SERVO_MAJOR, SERVO_NAME, &servo_fops);
    if (result < 0) {
        printk(KERN_WARNING
        "Can't get any major!\n");
        return result;
    }
    printk("servo module uploaded!.\n");
    return 0;
}

static void servo_exit(void) {
    unregister_chrdev(SERVO_MAJOR, SERVO_NAME);
    printk("servo module removed.\n");
}

module_init(servo_init);
module_exit(servo_exit);
MODULE_LICENSE("GPL");
