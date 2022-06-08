#include <linux/module.h>
#define INCLUDE_VERMAGIC
#include <linux/build-salt.h>
#include <linux/elfnote-lto.h>
#include <linux/vermagic.h>
#include <linux/compiler.h>

BUILD_SALT;
BUILD_LTO_INFO;

MODULE_INFO(vermagic, VERMAGIC_STRING);
MODULE_INFO(name, KBUILD_MODNAME);

__visible struct module __this_module
__section(".gnu.linkonce.this_module") = {
	.name = KBUILD_MODNAME,
	.init = init_module,
#ifdef CONFIG_MODULE_UNLOAD
	.exit = cleanup_module,
#endif
	.arch = MODULE_ARCH_INIT,
};

#ifdef CONFIG_RETPOLINE
MODULE_INFO(retpoline, "Y");
#endif

static const struct modversion_info ____versions[]
__used __section("__versions") = {
	{ 0xd0b811ee, "module_layout" },
	{ 0x6bc3fbc0, "__unregister_chrdev" },
	{ 0xa463a9c2, "__register_chrdev" },
	{ 0x92d5838e, "request_threaded_irq" },
	{ 0x1d37eeed, "ioremap" },
	{ 0x3dcf1ffa, "__wake_up" },
	{ 0xc1514a3b, "free_irq" },
	{ 0x96f22c0c, "gpiod_to_irq" },
	{ 0x36e6c49c, "gpio_to_desc" },
	{ 0xedc03953, "iounmap" },
	{ 0x51a910c0, "arm_copy_to_user" },
	{ 0x2cfde9a2, "warn_slowpath_fmt" },
	{ 0x92997ed8, "_printk" },
	{ 0xb1ad28e0, "__gnu_mcount_nc" },
};

MODULE_INFO(depends, "");


MODULE_INFO(srcversion, "A678E0707260E477420F6A0");
