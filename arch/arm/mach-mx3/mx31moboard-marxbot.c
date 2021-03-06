/*
 * Copyright (C) 2009 Valentin Longchamp, EPFL Mobots group
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <linux/fsl_devices.h>
#include <linux/gpio.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/platform_device.h>
#include <linux/types.h>

#include <mach/common.h>
#include <mach/hardware.h>
#include <mach/imx-uart.h>
#include <mach/iomux-mx3.h>
#include <mach/mmc.h>

#include "devices.h"

static unsigned int marxbot_pins[] = {
	/* SDHC2 */
	MX31_PIN_PC_PWRON__SD2_DATA3, MX31_PIN_PC_VS1__SD2_DATA2,
	MX31_PIN_PC_READY__SD2_DATA1, MX31_PIN_PC_WAIT_B__SD2_DATA0,
	MX31_PIN_PC_CD2_B__SD2_CLK, MX31_PIN_PC_CD1_B__SD2_CMD,
	MX31_PIN_ATA_DIOR__GPIO3_28, MX31_PIN_ATA_DIOW__GPIO3_29,
	/* CSI */
	MX31_PIN_CSI_D4__CSI_D4, MX31_PIN_CSI_D5__CSI_D5,
	MX31_PIN_CSI_D6__CSI_D6, MX31_PIN_CSI_D7__CSI_D7,
	MX31_PIN_CSI_D8__CSI_D8, MX31_PIN_CSI_D9__CSI_D9,
	MX31_PIN_CSI_D10__CSI_D10, MX31_PIN_CSI_D11__CSI_D11,
	MX31_PIN_CSI_D12__CSI_D12, MX31_PIN_CSI_D13__CSI_D13,
	MX31_PIN_CSI_D14__CSI_D14, MX31_PIN_CSI_D15__CSI_D15,
	MX31_PIN_CSI_HSYNC__CSI_HSYNC, MX31_PIN_CSI_MCLK__CSI_MCLK,
	MX31_PIN_CSI_PIXCLK__CSI_PIXCLK, MX31_PIN_CSI_VSYNC__CSI_VSYNC,
	MX31_PIN_GPIO3_0__GPIO3_0, MX31_PIN_GPIO3_1__GPIO3_1,
	MX31_PIN_TXD2__GPIO1_28,
	/* USB OTG */
	MX31_PIN_USBOTG_DATA0__USBOTG_DATA0,
	MX31_PIN_USBOTG_DATA1__USBOTG_DATA1,
	MX31_PIN_USBOTG_DATA2__USBOTG_DATA2,
	MX31_PIN_USBOTG_DATA3__USBOTG_DATA3,
	MX31_PIN_USBOTG_DATA4__USBOTG_DATA4,
	MX31_PIN_USBOTG_DATA5__USBOTG_DATA5,
	MX31_PIN_USBOTG_DATA6__USBOTG_DATA6,
	MX31_PIN_USBOTG_DATA7__USBOTG_DATA7,
	MX31_PIN_USBOTG_CLK__USBOTG_CLK, MX31_PIN_USBOTG_DIR__USBOTG_DIR,
	MX31_PIN_USBOTG_NXT__USBOTG_NXT, MX31_PIN_USBOTG_STP__USBOTG_STP,
	MX31_PIN_USB_OC__GPIO1_30,
};

#define SDHC2_CD IOMUX_TO_GPIO(MX31_PIN_ATA_DIOR)
#define SDHC2_WP IOMUX_TO_GPIO(MX31_PIN_ATA_DIOW)

static int marxbot_sdhc2_get_ro(struct device *dev)
{
	return !gpio_get_value(SDHC2_WP);
}

static int marxbot_sdhc2_init(struct device *dev, irq_handler_t detect_irq,
		void *data)
{
	int ret;

	ret = gpio_request(SDHC2_CD, "sdhc-detect");
	if (ret)
		return ret;

	gpio_direction_input(SDHC2_CD);

	ret = gpio_request(SDHC2_WP, "sdhc-wp");
	if (ret)
		goto err_gpio_free;
	gpio_direction_input(SDHC2_WP);

	ret = request_irq(gpio_to_irq(SDHC2_CD), detect_irq,
		IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING,
		"sdhc2-card-detect", data);
	if (ret)
		goto err_gpio_free_2;

	return 0;

err_gpio_free_2:
	gpio_free(SDHC2_WP);
err_gpio_free:
	gpio_free(SDHC2_CD);

	return ret;
}

static void marxbot_sdhc2_exit(struct device *dev, void *data)
{
	free_irq(gpio_to_irq(SDHC2_CD), data);
	gpio_free(SDHC2_WP);
	gpio_free(SDHC2_CD);
}

static struct imxmmc_platform_data sdhc2_pdata = {
	.get_ro	= marxbot_sdhc2_get_ro,
	.init	= marxbot_sdhc2_init,
	.exit	= marxbot_sdhc2_exit,
};

static struct fsl_usb2_platform_data usb_pdata = {
	.operating_mode	= FSL_USB2_DR_DEVICE,
	.phy_mode	= FSL_USB2_PHY_ULPI,
};

#define OTG_PAD_CFG (PAD_CTL_DRV_MAX | PAD_CTL_SRE_FAST)
#define OTG_EN_B IOMUX_TO_GPIO(MX31_PIN_USB_OC)

static void marxbot_usbotg_init(void)
{
	mxc_iomux_set_pad(MX31_PIN_USBOTG_DATA0, OTG_PAD_CFG);
	mxc_iomux_set_pad(MX31_PIN_USBOTG_DATA1, OTG_PAD_CFG);
	mxc_iomux_set_pad(MX31_PIN_USBOTG_DATA2, OTG_PAD_CFG);
	mxc_iomux_set_pad(MX31_PIN_USBOTG_DATA3, OTG_PAD_CFG);
	mxc_iomux_set_pad(MX31_PIN_USBOTG_DATA4, OTG_PAD_CFG);
	mxc_iomux_set_pad(MX31_PIN_USBOTG_DATA5, OTG_PAD_CFG);
	mxc_iomux_set_pad(MX31_PIN_USBOTG_DATA6, OTG_PAD_CFG);
	mxc_iomux_set_pad(MX31_PIN_USBOTG_DATA7, OTG_PAD_CFG);
	mxc_iomux_set_pad(MX31_PIN_USBOTG_CLK, OTG_PAD_CFG);
	mxc_iomux_set_pad(MX31_PIN_USBOTG_DIR, OTG_PAD_CFG);
	mxc_iomux_set_pad(MX31_PIN_USBOTG_NXT, OTG_PAD_CFG);
	mxc_iomux_set_pad(MX31_PIN_USBOTG_STP, OTG_PAD_CFG);

	gpio_request(OTG_EN_B, "usb-udc-en");
	gpio_direction_output(OTG_EN_B, 0);
}

/*
 * system init for baseboard usage. Will be called by mx31moboard init.
 */
void __init mx31moboard_marxbot_init(void)
{
	printk(KERN_INFO "Initializing mx31marxbot peripherals\n");

	mxc_iomux_setup_multiple_pins(marxbot_pins, ARRAY_SIZE(marxbot_pins),
		"marxbot");

	mxc_register_device(&mxcsdhc_device1, &sdhc2_pdata);

	marxbot_usbotg_init();
	mxc_register_device(&mxc_otg_udc_device, &usb_pdata);
}
