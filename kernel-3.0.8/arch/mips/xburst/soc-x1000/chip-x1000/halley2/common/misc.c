#include <linux/platform_device.h>
#include <linux/i2c.h>
#include <linux/i2c-gpio.h>
#include <linux/gpio_keys.h>
#include <linux/input.h>
#include <linux/leds.h>
//#include <linux/tsc.h>
#include <linux/spi/spi.h>
#include <linux/spi/spi_gpio.h>
//#include <linux/android_pmem.h>
#include <mach/platform.h>
#include <mach/jzsnd.h>
#include <mach/jzmmc.h>
#include <mach/jzssi.h>
#include <mach/jz_efuse.h>
#include <mach/jzeth_phy.h>
#include <gpio.h>
#include <linux/jz_dwc.h>
#include <linux/interrupt.h>
//#include <sound/jz-aic.h>
#include "board_base.h"

#ifdef CONFIG_LEDS_GPIO
#include <linux/leds.h>
#endif


#ifdef CONFIG_LEDS_GPIO

static struct gpio_led gpio_leds[] = {
#ifndef CONFIG_VIDEO_JZ_CIM_HOST
	{
		.name = "led-1",
		.gpio = GPIO_PA(8),
		.active_low = true,
		.default_state = LEDS_GPIO_DEFSTATE_OFF,
		.retain_state_suspended = true,
		.default_trigger = NULL,
	},
	{
		.name = "led-2",
		.gpio = GPIO_PA(9),
		.active_low = true,
		.default_state = LEDS_GPIO_DEFSTATE_OFF,
		.retain_state_suspended = true,
		.default_trigger = "timer",
	},
#endif
};

static struct gpio_led_platform_data gpio_led_info = {
	.leds = gpio_leds,
	.num_leds = ARRAY_SIZE(gpio_leds),
};

struct platform_device jz_leds_gpio = {
	.name = "leds-gpio",
	.id = -1,
	.dev = {
		.platform_data = &gpio_led_info,
	},
};
#endif

#ifdef CONFIG_JZ_MAC
#ifndef CONFIG_MDIO_GPIO
static struct jz_ethphy_feature ethphy_feature = {
#ifdef CONFIG_JZGPIO_PHY_RESET
	.phy_hwreset = {
		.gpio		= GMAC_PHY_PORT_GPIO,
		.active_level	= GMAC_PHY_ACTIVE_HIGH,
		.delaytime_msec	= GMAC_PHY_DELAYTIME,
	},
#endif
	.mdc_mincycle = 80, /* 80ns */
};

struct platform_device jz_mii_bus = {
	.name = "jz_mii_bus",
	.dev.platform_data = &ethphy_feature,
};
#else /* CONFIG_MDIO_GPIO */
static struct mdio_gpio_platform_data mdio_gpio_data = {
	.mdc = MDIO_MDIO_MDC_GPIO,
	.mdio = MDIO_MDIO_GPIO,
	.phy_mask = 0,
	.irqs = { 0 },
};

struct platform_device jz_mii_bus = {
	.name = "mdio-gpio",
	.dev.platform_data = &mdio_gpio_data,
};
#endif /* CONFIG_MDIO_GPIO */
struct platform_device jz_mac_device = {
	.name = "jz_mac",
	.dev.platform_data = &jz_mii_bus,
};
#endif /* CONFIG_JZ_MAC */

#ifdef CONFIG_JZ_EFUSE_V12
struct jz_efuse_platform_data jz_efuse_pdata = {
	/* supply 2.5V to VDDQ */
	.gpio_vddq_en_n = GPIO_EFUSE_VDDQ,
};
#endif

#ifdef CONFIG_JZ_EFUSE_V13
struct jz_efuse_platform_data jz_efuse_pdata = {
	/* supply 2.5V to VDDQ */
	.gpio_vddq_en_n = GPIO_EFUSE_VDDQ,
};
#endif

#if defined(GPIO_USB_ID) && defined(GPIO_USB_ID_LEVEL)
struct jzdwc_pin dwc2_id_pin = {
	.num = GPIO_USB_ID,
	.enable_level = GPIO_USB_ID_LEVEL,
};
#endif


#if defined(GPIO_USB_DETE) && defined(GPIO_USB_DETE_LEVEL)
struct jzdwc_pin dwc2_dete_pin = {
	.num = GPIO_USB_DETE,
	.enable_level = GPIO_USB_DETE_LEVEL,
};
#endif


#if defined(GPIO_USB_DRVVBUS) && defined(GPIO_USB_DRVVBUS_LEVEL) && !defined(USB_DWC2_DRVVBUS_FUNCTION_PIN)
struct jzdwc_pin dwc2_drvvbus_pin = {
	.num = GPIO_USB_DRVVBUS,
	.enable_level = GPIO_USB_DRVVBUS_LEVEL,
};
#endif
