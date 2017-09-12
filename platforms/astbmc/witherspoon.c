/* Copyright 2017 IBM Corp.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or
 * implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <skiboot.h>
#include <device.h>
#include <console.h>
#include <chip.h>
#include <ipmi.h>
#include <psi.h>
#include <npu-regs.h>
#include <xscom.h>
#include <xscom-p9-regs.h>
#include <timebase.h>
#include <pci.h>
#include <pci-slot.h>
#include <phb4.h>

#include "astbmc.h"

static const struct slot_table_entry witherspoon_slot1[] = {
	{
		.etype = st_pluggable_slot,
		.location = ST_LOC_DEVFN(0,0),
		.name = "SLOT0"
	},
	{ .etype = st_end },
};

static const struct slot_table_entry witherspoon_slot2_shared[] = {
	{
		.etype = st_pluggable_slot,
		.location = ST_LOC_DEVFN(0,0),
		.name = "SLOT1"
	},
	{ .etype = st_end },
};

static const struct slot_table_entry witherspoon_slot3[] = {
	{
		.etype = st_pluggable_slot,
		.location = ST_LOC_DEVFN(0,0),
		.name = "SLOT2"
	},
	{ .etype = st_end },
};

static const struct slot_table_entry witherspoon_slot4[] = {
	{
		.etype = st_pluggable_slot,
		.location = ST_LOC_DEVFN(0,0),
		.name = "SLOT3"
	},
	{ .etype = st_end },
};

static const struct slot_table_entry witherspoon_gpu0[] = {
	{
		.etype = st_pluggable_slot,
		.location = ST_LOC_DEVFN(0x80,0),
		.name = "GPU0",
	},
	{ .etype = st_end },
};

static const struct slot_table_entry witherspoon_gpu1[] = {
	{
		.etype = st_pluggable_slot,
		.location = ST_LOC_DEVFN(0xa0,0),
		.name = "GPU1",
	},
	{ .etype = st_end },
};

static const struct slot_table_entry witherspoon_gpu2[] = {
	{
		.etype = st_pluggable_slot,
		.location = ST_LOC_DEVFN(0xc0,0),
		.name = "GPU2",
	},
	{ .etype = st_end },
};

static const struct slot_table_entry witherspoon_gpu3[] = {
	{
		.etype = st_pluggable_slot,
		.location = ST_LOC_DEVFN(0x60,0),
		.name = "GPU3",
	},
	{ .etype = st_end },
};

static const struct slot_table_entry witherspoon_gpu4[] = {
	{
		.etype = st_pluggable_slot,
		.location = ST_LOC_DEVFN(0x80,0),
		.name = "GPU4",
	},
	{ .etype = st_end },
};

static const struct slot_table_entry witherspoon_gpu5[] = {
	{
		.etype = st_pluggable_slot,
		.location = ST_LOC_DEVFN(0xa0,0),
		.name = "GPU5",
	},
	{ .etype = st_end },
};

static const struct slot_table_entry witherspoon_plx0_down[] = {
	{
		.etype = st_builtin_dev,
		.location = ST_LOC_DEVFN(0x4a,0),
		.children = witherspoon_gpu0,
		.name = "GPU0 down",
	},
	{
		.etype = st_builtin_dev,
		.location = ST_LOC_DEVFN(0x4b,0),
		.children = witherspoon_gpu1,
		.name = "GPU1 down",
	},
	{
		.etype = st_builtin_dev,
		.location = ST_LOC_DEVFN(0x4c,0),
		.children = witherspoon_gpu2,
		.name = "GPU2 down",
	},
	{ .etype = st_end },
};

static const struct slot_table_entry witherspoon_plx1_down[] = {
	{
		.etype = st_builtin_dev,
		.location = ST_LOC_DEVFN(0x44,0),
		.children = witherspoon_gpu3,
		.name = "GPU3 down",
	},
	{
		.etype = st_builtin_dev,
		.location = ST_LOC_DEVFN(0x45,0),
		.children = witherspoon_gpu4,
		.name = "GPU4 down",
	},
	{
		.etype = st_builtin_dev,
		.location = ST_LOC_DEVFN(0x4d,0),
		.children = witherspoon_gpu5,
		.name = "GPU5 down",
	},
	{ .etype = st_end },
};

static const struct slot_table_entry witherspoon_plx0_up[] = {
	{
		.etype = st_builtin_dev,
		.location = ST_LOC_DEVFN(0x20,0),
		.children = witherspoon_plx0_down,
	},
	{ .etype = st_end },
};

static const struct slot_table_entry witherspoon_plx1_up[] = {
	{
		.etype = st_builtin_dev,
		.location = ST_LOC_DEVFN(0x20,0),
		.children = witherspoon_plx1_down,
	},
	{ .etype = st_end },
};

static const struct slot_table_entry witherspoon_plx0_phb[] = {
	{
		.etype = st_builtin_dev,
		.location = ST_LOC_DEVFN(0,0),
		.children = witherspoon_plx0_up,
	},
	{ .etype = st_end },
};

static const struct slot_table_entry witherspoon_plx1_phb[] = {
	{
		.etype = st_builtin_dev,
		.location = ST_LOC_DEVFN(0,0),
		.children = witherspoon_plx1_up,
	},
	{ .etype = st_end },
};

/*
 * Slot numbering:
 *
 * slot 1 - x4 slot
 * slot 2 - shared slot, 8x to each chip's PHB3
 * slot 3 - 16x \w CAPI, second chip
 * slot 4 - 16x \w CAPI, first chip
 */

static const struct slot_table_entry witherspoon_phb_table[] = {
	ST_PHB_ENTRY(0, 0, witherspoon_slot4),
	ST_PHB_ENTRY(0, 3, witherspoon_slot2_shared),
	ST_PHB_ENTRY(0, 4, witherspoon_plx0_phb),

	ST_PHB_ENTRY(8, 0, witherspoon_slot3),
	ST_PHB_ENTRY(8, 3, witherspoon_slot2_shared),
	ST_PHB_ENTRY(8, 4, witherspoon_slot1),
	ST_PHB_ENTRY(8, 5, witherspoon_plx1_phb),

	{ .etype = st_end },
};

/*
 * Check if we got PCI slot information from the HDAT. If it's missing
 * then we have an old firmware without IOSLOT/SMP Link information.
 * Without these we don't know the NVLink topology (what GPU maps to
 * which NPU), so we can't do anything meaningful here.
 */
static bool witherspoon_probe(void)
{
	if (!dt_node_is_compatible(dt_root, "ibm,witherspoon"))
		return false;
	if (!dt_find_by_name(dt_root, "ibm,pcie-slots"))
		return false;

	/* Lot of common early inits here */
	astbmc_early_init();

	/* Setup UART for use by OPAL (Linux hvc) */
	uart_set_console_policy(UART_CONSOLE_OPAL);

	return true;
}

static bool old_witherspoon_probe(void)
{
	if (!dt_node_is_compatible(dt_root, "ibm,witherspoon"))
		return false;
	if (dt_find_by_name(dt_root, "ibm,pcie-slots"))
		return false;

	prerror("Old witherspoon firmware detected. NVLink is not supported\n");

	/* Lot of common early inits here */
	astbmc_early_init();

	/* Setup UART for use by OPAL (Linux hvc) */
	uart_set_console_policy(UART_CONSOLE_OPAL);

	slot_table_init(witherspoon_phb_table);

	return true;
}

static void phb4_activate_shared_slot_witherspoon(struct proc_chip *chip)
{
	uint64_t val;

	/*
	 * Shared slot activation is done by raising a GPIO line on the
	 * chip with the secondary slot. It will somehow activate the
	 * sideband signals between the slots.
	 * Need to wait 100us for stability.
	 */
	xscom_read(chip->id, P9_GPIO_DATA_OUT_ENABLE, &val);
	val |= PPC_BIT(2);
	xscom_write(chip->id, P9_GPIO_DATA_OUT_ENABLE, val);

	xscom_read(chip->id, P9_GPIO_DATA_OUT, &val);
	val |= PPC_BIT(2);
	xscom_write(chip->id, P9_GPIO_DATA_OUT, val);
	time_wait_us(100);
	prlog(PR_INFO, "Shared PCI slot activated\n");
}

static void phb4_pre_pci_fixup_witherspoon(void)
{
	struct pci_slot *slot0, *slot1;
	struct proc_chip *chip0, *chip1;
	uint8_t p0 = 0, p1 = 0;

	/*
	 * Detect if a x16 card is present on the shared slot and
	 * do some extra configuration if it is.
	 *
	 * The shared slot, a.k.a "Slot 2" in the documentation, is
	 * connected to PEC2 phb index 3 on both chips. From skiboot,
	 * it looks like two x8 slots, each with its own presence bit.
	 *
	 * Here is the matrix of possibilities for the presence bits:
	 *
	 * slot0 presence     slot1 presence
	 *    0                  0               => no card
	 *    1                  0               => x8 or less card detected
	 *    1                  1               => x16 card detected
	 *    0                  1               => invalid combination
	 *
	 * We only act if a x16 card is detected ('1 1' combination above).
	 *
	 * One issue is that we don't really know if it is a
	 * shared-slot-compatible card (such as Mellanox CX5) or
	 * a 'normal' x16 PCI card. We activate the shared slot in both cases,
	 * as it doesn't seem to hurt.
	 *
	 * If the card is a normal x16 PCI card, the link won't train on the
	 * second slot (nothing to do with the shared slot activation), the
	 * procedure will timeout, thus adding some delay to the boot time.
	 * Therefore the recommendation is that we shouldn't use a normal
	 * x16 card on the shared slot of a witherspoon.
	 *
	 * Plugging a x8 or less adapter on the shared slot should work
	 * like any other physical slot.
	 */
	chip0 = next_chip(NULL);
	chip1 = next_chip(chip0);
	if (!chip1 || next_chip(chip1)) {
		prlog(PR_WARNING,
			"Unexpected number of chips, skipping shared slot detection\n");
		return;
	}

	/* the shared slot is connected to PHB3 on both chips */
	slot0 = pci_slot_find(phb4_get_opal_id(chip0->id, 3));
	slot1 = pci_slot_find(phb4_get_opal_id(chip1->id, 3));
	if (slot0 && slot1) {
		if (slot0->ops.get_presence_state)
			slot0->ops.get_presence_state(slot0, &p0);
		if (slot1->ops.get_presence_state)
			slot1->ops.get_presence_state(slot1, &p1);
		if (p0 == 1 && p1 == 1)
			phb4_activate_shared_slot_witherspoon(chip1);
	}
}

static void witherspoon_pre_pci_fixup(void)
{
	phb4_pre_pci_fixup_witherspoon();
}

/* The only difference between these is the PCI slot handling */

DECLARE_PLATFORM(witherspoon) = {
	.name			= "Witherspoon",
	.probe			= witherspoon_probe,
	.init			= astbmc_init,
	.pre_pci_fixup		= witherspoon_pre_pci_fixup,
	.start_preload_resource	= flash_start_preload_resource,
	.resource_loaded	= flash_resource_loaded,
	.bmc			= &astbmc_openbmc,
	.cec_power_down         = astbmc_ipmi_power_down,
	.cec_reboot             = astbmc_ipmi_reboot,
	.elog_commit		= ipmi_elog_commit,
	.exit			= ipmi_wdt_final_reset,
	.terminate		= ipmi_terminate,

	.pci_get_slot_info	= map_pci_dev_to_slot,
};

DECLARE_PLATFORM(old_witherspoon) = {
	.name			= "Witherspoon (old)",
	.probe			= old_witherspoon_probe,
	.init			= astbmc_init,
	.pre_pci_fixup		= witherspoon_pre_pci_fixup,
	.start_preload_resource	= flash_start_preload_resource,
	.resource_loaded	= flash_resource_loaded,
	.bmc			= &astbmc_openbmc,
	.cec_power_down         = astbmc_ipmi_power_down,
	.cec_reboot             = astbmc_ipmi_reboot,
	.elog_commit		= ipmi_elog_commit,
	.exit			= ipmi_wdt_final_reset,
	.terminate		= ipmi_terminate,

	.pci_get_slot_info	= slot_table_get_slot_info,
	.pci_probe_complete	= check_all_slot_table,
};
