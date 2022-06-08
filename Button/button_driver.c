#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <asm/uaccess.h>
#include <linux/slab.h>
#include <linux/gpio.h>
#include <linux/io.h>
#include <linux/poll.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/sched.h>
#include <linux/wait.h>
#define BUTTON_MAJOR 220
#define BUTTON_NAME "BUTTON_DRIVER"
#define BCM2711_PERL_BASE 0xFE000000
#define GPIO_BASE (BCM2711_PERL_BASE + 0x200000)
#define GPIO_SIZE 256
char button_usage = 0;
static void *button_map;
volatile unsigned *button;
static char tmp_buf;
static int event_flag = 0;
DECLARE_WAIT_QUEUE_HEAD(waitqueue);
static irqreturn_t ind_interrupt_handler(int irq, void *pdata)
{
	int tmp_button1, tmp_button2, tmp_button3;
	tmp_buf = 0;
	if (irq == gpio_to_irq(17))
		tmp_buf = 1;
	else if (irq == gpio_to_irq(22))
		tmp_buf = 2;
	else if (irq == gpio_to_irq(27))
		tmp_buf = 3;
	wake_up_interruptible(&waitqueue);
	++event_flag;
	return IRQ_HANDLED;
}
static unsigned button_poll(struct file *mfile, struct poll_table_struct *pt)
{
	int mask = 0;
	poll_wait(mfile, &waitqueue, pt);
	if (event_flag > 0)
		mask |= (POLLIN | POLLRDNORM);
	event_flag = 0;
	return mask;
}
static int button_open(struct inode *minode, struct file *mfile)
{
	int result1, result2, result3;
	if (button_usage != 0)
		return -EBUSY;
	button_usage = 1;
	button_map = ioremap(GPIO_BASE, GPIO_SIZE);
	if (!button_map)
	{
		printk("error: mapping gpio memory");
		iounmap(button_map);
		return -EBUSY;
	}
	button = (volatile unsigned int *)button_map;
	// set inputMode GPIO 17 22 27
	*(button + 1) &= ~(0x7 << (3 * 7));
	*(button + 2) &= ~(0x7 << (3 * 2));
	*(button + 2) &= ~(0x7 << (3 * 7));
	// Poiing Edge 설정
	*(button + 22) |= (0x1 << 17);
	*(button + 22) |= (0x1 << 22);
	*(button + 22) |= (0x1 << 27);

	/* result1,2,3을 통해 1,2,3층 irq에 넣었는지 확인 */
	result1 = request_irq(gpio_to_irq(17), ind_interrupt_handler,
						  IRQF_TRIGGER_FALLING, "gpio_irq_button1", NULL);
	result2 = request_irq(gpio_to_irq(22), ind_interrupt_handler,
						  IRQF_TRIGGER_FALLING, "gpio_irq_button2", NULL);
	result3 = request_irq(gpio_to_irq(27), ind_interrupt_handler,
						  IRQF_TRIGGER_FALLING, "gpio_irq_button3", NULL);
	
	// 하나라도 실패한 값이 있으면 에러 출력후 종료
	if (result1 < 0 || result2 < 0 || result3 < 0)
	{
		printk("error: request_irq()");
		return -1;
	}
	return 0;
}
static int button_release(struct inode *minode, struct file *mfile)
{
	button_usage = 0;
	if (button)
		iounmap(button);
	free_irq(gpio_to_irq(17), NULL);
	free_irq(gpio_to_irq(22), NULL);
	free_irq(gpio_to_irq(27), NULL);
	return 0;
}

static int button_read(struct file *mfile, char *gdata, size_t length, loff_t *off_what)
{
	int result;
	printk("button_read = %d\n", tmp_buf);
	result = copy_to_user((void *)gdata, &tmp_buf, length); //user로부터 읽기 요청시 층 값 전송
	if (result < 0)
	{
		printk("error: copy_to_user()");
		return result;
	}
	return length;
}

static struct file_operations button_fops =
	{
		.owner = THIS_MODULE,
		.open = button_open,
		.release = button_release,
		.read = button_read,
		.poll = button_poll,
};
static int button_init(void)
{
	int result;
	result = register_chrdev(BUTTON_MAJOR, BUTTON_NAME, &button_fops);
	if (result < 0)
	{
		printk(KERN_WARNING "Can't get any major!\n");
		return result;
	}
	return 0;
}
static void button_exit(void)
{
	unregister_chrdev(BUTTON_MAJOR, BUTTON_NAME);
	printk("BUTTON module removed.\n");
}
module_init(button_init);
module_exit(button_exit);
MODULE_LICENSE("GPL");
