/*
 * Copyright (C) 2011 Freescale Semiconductor, Inc. All Rights Reserved.
 *
 * Copyright 2010-2011 Amazon.com, Inc. All rights reserved.
 * Manish Lachwani (lachwani@lab126.com)
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/spi/spi.h>
#include <linux/err.h>
#include <linux/pmic_external.h>
#include <linux/regulator/machine.h>
#include <linux/mfd/mc34708/core.h>
#include <mach/irqs.h>
#include "mx50_pins.h"

#include <mach/gpio.h>
/*
 * Convenience conversion.
 * Here atm, maybe there is somewhere better for this.
 */
#define mV_to_uV(mV) (mV * 1000)
#define uV_to_mV(uV) (uV / 1000)
#define V_to_uV(V) (mV_to_uV(V * 1000))
#define uV_to_V(uV) (uV_to_mV(uV) / 1000)

/* regulator standby mask */
#define GEN1_STBY_MASK		(1 << 1)
#define GEN2_STBY_MASK		(1 << 13)
#define PLL_STBY_MASK		(1 << 16)
#define USB2_STBY_MASK		(1 << 19)
#define USB_EN_MASK			(1 << 3)

#define REG_MODE_0_ALL_MASK	(GEN1_STBY_MASK | PLL_STBY_MASK\
				| GEN2_STBY_MASK | USB2_STBY_MASK)

#define SW1A_MODE_MASK		(0xf << 0)
#define SW2_MODE_MASK		(0xf << 14)
#define SW1A_MODE_VALUE		(0xc << 0)
#define SW2_MODE_VALUE		(0xc << 14)

#define REG_SW_1_2_MASK	(SW1A_MODE_MASK | SW2_MODE_MASK)
#define REG_SW_1_2_VALUE	(SW1A_MODE_VALUE | SW2_MODE_VALUE)

#define SW3_MODE_MASK		(0xf << 0)
#define SW4A_MODE_MASK		(0xf << 6)
#define SW4B_MODE_MASK		(0xf << 12)
#define SW5_MODE_MASK		(0xf << 18)

#define SW3_MODE_VALUE		(0xc << 0)
#define SW4A_MODE_VALUE		(0xc << 6)
#define SW4B_MODE_VALUE		(0xc << 12)
#define SW5_MODE_VALUE		(0xc << 18)

#define REG_SW_3_4_5_MASK	(SW3_MODE_MASK | SW4A_MODE_MASK\
				| SW4B_MODE_MASK | SW5_MODE_MASK)
#define REG_SW_3_4_5_VALUE	(SW3_MODE_VALUE | SW4A_MODE_VALUE\
					| SW4B_MODE_VALUE | SW5_MODE_VALUE)

#define SWBST_MODE_MASK		(0x3 << 5)
#define SWBST_MODE_VALUE	(0x1 << 5)

#define REG_SWBST_MODE_MASK	(SWBST_MODE_MASK)
#define REG_SWBST_MODE_VALUE	(SWBST_MODE_VALUE)

#define SWHOLD_MASK		(0x1 << 12)

#define SW1DVS_SHIFT		6
#define SW2DVS_SHIFT		20
#define SW4AHI_SHIFT		10
#define SWHOLD_SHIFT		12

struct mc34708;

static struct regulator_init_data sw1a_init = {
	.constraints = {
		.name = "SW1",
		.min_uV = 650000,
		.max_uV = 1437500,
		.valid_ops_mask = REGULATOR_CHANGE_VOLTAGE,
		.valid_modes_mask = 0,
		.always_on = 1,
		.boot_on = 1,
		.initial_state = PM_SUSPEND_MEM,
		.state_mem = {
			.uV = 850000,
			.mode = REGULATOR_MODE_NORMAL,
			.enabled = 1,
		},
	},
};

static struct regulator_init_data sw1b_init = {
	.constraints = {
		.name = "SW1B",
		.min_uV = 650000,
		.max_uV = 1437500,
		.valid_ops_mask = REGULATOR_CHANGE_VOLTAGE,
		.valid_modes_mask = 0,
		.always_on = 1,
		.boot_on = 1,
	},
};

static struct regulator_init_data sw2_init = {
	.constraints = {
		.name = "SW2",
		.min_uV = 650000,
		.max_uV = 1437500,
		.valid_ops_mask = REGULATOR_CHANGE_VOLTAGE,
		.always_on = 1,
		.boot_on = 1,
		.initial_state = PM_SUSPEND_MEM,
		.state_mem = {
			.uV = 950000,
			.mode = REGULATOR_MODE_NORMAL,
			.enabled = 1,
		},
	}
};

static struct regulator_init_data sw3_init = {
	.constraints = {
		.name = "SW3",
		.min_uV = 650000,
		.max_uV = 1425000,
		.valid_ops_mask = REGULATOR_CHANGE_VOLTAGE,
		.always_on = 1,
		.boot_on = 1,
		.initial_state = PM_SUSPEND_MEM,
		.state_mem = {
			.uV = 950000,
			.mode = REGULATOR_MODE_NORMAL,
			.enabled = 1,
		},
	}
};

static struct regulator_init_data sw4a_init = {
	.constraints = {
		.name = "SW4A",
		.min_uV = mV_to_uV(1200),
		.max_uV = mV_to_uV(3300),
		.valid_ops_mask = REGULATOR_CHANGE_VOLTAGE,
		.always_on = 1,
		.boot_on = 1,
	}
};

static struct regulator_init_data sw4b_init = {
	.constraints = {
		.name = "SW4B",
		.min_uV = mV_to_uV(1200),
		.max_uV = mV_to_uV(3300),
		.valid_ops_mask = REGULATOR_CHANGE_VOLTAGE,
		.always_on = 1,
		.boot_on = 1,
	}
};

static struct regulator_init_data sw5_init = {
	.constraints = {
		.name = "SW5",
		.min_uV = mV_to_uV(1200),
		.max_uV = mV_to_uV(1975),
		.valid_ops_mask = REGULATOR_CHANGE_VOLTAGE,
		.always_on = 1,
		.boot_on = 1,
	}
};

static struct regulator_init_data vrefddr_init = {
	.constraints = {
		.name = "VREFDDR",
		.always_on = 1,
		.boot_on = 1,
	}
};

static struct regulator_init_data vusb_init = {
	.constraints = {
		.name = "VUSB",
		.valid_ops_mask = REGULATOR_CHANGE_STATUS,
		.boot_on = 1,
		.always_on = 1,
	}
};

static struct regulator_init_data swbst_init = {
	.constraints = {
		.name = "SWBST",
		.boot_on = 1,
		.always_on = 1,
	}
};

static struct regulator_init_data vpll_init = {
	.constraints = {
		.name = "VPLL",
		.min_uV = mV_to_uV(1200),
		.max_uV = mV_to_uV(1800),
		.valid_ops_mask = REGULATOR_CHANGE_VOLTAGE,
		.boot_on = 1,
	},
};

static struct regulator_init_data vdac_init = {
	.constraints = {
		.name = "VDAC",
		.min_uV = mV_to_uV(2500),
		.max_uV = mV_to_uV(2775),
		.valid_ops_mask = REGULATOR_CHANGE_VOLTAGE,
		.boot_on = 1,
		.always_on = 1,
	}
};

static struct regulator_init_data vusb2_init = {
	.constraints = {
		.name = "VUSB2",
		.min_uV = mV_to_uV(2500),
		.max_uV = mV_to_uV(3000),
		.valid_ops_mask = REGULATOR_CHANGE_VOLTAGE,
		.boot_on = 1,
		.always_on = 1,
	}
};

static struct regulator_init_data vgen1_init = {
	.constraints = {
		.name = "VGEN1",
		.min_uV = mV_to_uV(1200),
		.max_uV = mV_to_uV(1550),
		.valid_ops_mask = REGULATOR_CHANGE_VOLTAGE,
		.always_on = 1,
	}
};

static struct regulator_init_data vgen2_init = {
	.constraints = {
		.name = "VGEN2",
		.min_uV = mV_to_uV(2500),
		.max_uV = mV_to_uV(3300),
		.valid_ops_mask = REGULATOR_CHANGE_VOLTAGE,
		.always_on = 1,
	}
};


static int mc34708_regulator_init(struct mc34708 *mc34708)
{
	unsigned int value;

	pmic_read_reg(REG_MC34708_IDENTIFICATION, &value, 0xffffff);
	pr_info("PMIC MC34708 ID:0x%x\n", value);

	/* enable standby controll for mode0 regulators */
	pmic_read_reg(REG_MC34708_MODE_0, &value, 0xffffff);
	value &= ~(REG_MODE_0_ALL_MASK | USB_EN_MASK);
	value |= REG_MODE_0_ALL_MASK;
	pmic_write_reg(REG_MC34708_MODE_0, value, 0xffffff);

	/* enable standby controll for mode0 regulators */
	pmic_read_reg(REG_MC34708_SW_1_2_OP, &value, 0xffffff);
	value &= ~REG_SW_1_2_MASK;
	value |= REG_SW_1_2_VALUE;
	pmic_write_reg(REG_MC34708_SW_1_2_OP, value, 0xffffff);

	/* enable standby controll for mode0 regulators */
	pmic_read_reg(REG_MC34708_SW_3_4_5_OP, &value, 0xffffff);
	value &= ~REG_SW_3_4_5_MASK;
	value |= REG_SW_3_4_5_VALUE;
	pmic_write_reg(REG_MC34708_SW_3_4_5_OP, value, 0xffffff);

	/* enable standby controll for mode0 regulators */
	pmic_read_reg(REG_MC34708_SWBST, &value, 0xffffff);
	value &= ~REG_SWBST_MODE_MASK;
	value |= REG_SWBST_MODE_VALUE;
	pmic_write_reg(REG_MC34708_SWBST, value, 0xffffff);

	pr_info("Initializing mc34708 regulators for mx50 yoshime.\n");

	mc34708_register_regulator(mc34708, MC34708_SW1A, &sw1a_init);
	mc34708_register_regulator(mc34708, MC34708_SW1B, &sw1b_init);
	mc34708_register_regulator(mc34708, MC34708_SW2, &sw2_init);
	mc34708_register_regulator(mc34708, MC34708_SW3, &sw3_init);
	mc34708_register_regulator(mc34708, MC34708_SW4A, &sw4a_init);
	mc34708_register_regulator(mc34708, MC34708_SW4B, &sw4b_init);
	mc34708_register_regulator(mc34708, MC34708_SW5, &sw5_init);
	mc34708_register_regulator(mc34708, MC34708_SWBST, &swbst_init);
	mc34708_register_regulator(mc34708, MC34708_VPLL, &vpll_init);
	mc34708_register_regulator(mc34708, MC34708_VREFDDR, &vrefddr_init);
	mc34708_register_regulator(mc34708, MC34708_VDAC, &vdac_init);
	mc34708_register_regulator(mc34708, MC34708_VUSB, &vusb_init);
	mc34708_register_regulator(mc34708, MC34708_VUSB2, &vusb2_init);
	mc34708_register_regulator(mc34708, MC34708_VGEN1, &vgen1_init);
	mc34708_register_regulator(mc34708, MC34708_VGEN2, &vgen2_init);

	regulator_has_full_constraints();

	/* DVSSPEED for sw1 and sw2 */
	pmic_write_reg(REG_MC34708_SW_1_2_OP, (0x1 << SW1DVS_SHIFT), (0x3 << SW1DVS_SHIFT));
	pmic_write_reg(REG_MC34708_SW_1_2_OP, (0x1 << SW2DVS_SHIFT), (0x3 << SW2DVS_SHIFT));
	pmic_write_reg(REG_MC34708_SW_4_A_B, (0x3 << SW4AHI_SHIFT), (0x3 << SW4AHI_SHIFT));
	pmic_write_reg(REG_MC34708_USB_CONTROL, (0x1 << SWHOLD_SHIFT), (0x1 << SWHOLD_SHIFT));

	return 0;
}

static struct mc34708_platform_data mc34708_plat = {
	.init = mc34708_regulator_init,
};

static struct spi_board_info __initdata mc34708_spi_device = {
	.modalias = "pmic_spi",
	.irq = IOMUX_TO_IRQ(MX50_PIN_UART1_CTS),
	.max_speed_hz = 6000000,	/* max spi SCK clock speed in HZ */
	.bus_num = 3,
	.chip_select = 0,
	.platform_data = &mc34708_plat,
};

void pmic_vusb2_enable_vote(int enable,int source)
{
	return;
}
EXPORT_SYMBOL(pmic_vusb2_enable_vote);


int __init mx50_yoshime_init_mc34708(void)
{
	return spi_register_board_info(&mc34708_spi_device, 1);
}

