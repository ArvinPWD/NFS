/*
 * Copyright (C) 2017 MediaTek Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See http://www.gnu.org/licenses/gpl-2.0.html for more details.
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <asm/setup.h>

#if defined(CONFIG_MTK_PMIC) || defined(CONFIG_MTK_PMIC_NEW_ARCH)
#include <mt-plat/upmu_common.h>
#endif

#include <mtk_spm_internal.h>
#include <mtk_spm_suspend_internal.h>

#ifdef CONFIG_MTK_TINYSYS_SSPM_SUPPORT
#include <sspm_define.h>
#include <sspm_timesync.h>
#endif

#include <mtk_power_gs_api.h>
#include <mtk_sspm.h>

#define WORLD_CLK_CNTCV_L        (0x10017008)
#define WORLD_CLK_CNTCV_H        (0x1001700C)

#ifdef CONFIG_FPGA_EARLY_PORTING
__attribute__ ((weak))
unsigned int pmic_read_interface_nolock(unsigned int RegNum, unsigned int *val,
	unsigned int MASK, unsigned int SHIFT)
{
	pr_debug("[name:spm&]NO %s !!!\n", __func__);
	return 0;
}

__attribute__ ((weak))
unsigned int pmic_config_interface(unsigned int RegNum, unsigned int val,
	unsigned int MASK, unsigned int SHIFT)
{
	pr_debug("[name:spm&]NO %s !!!\n", __func__);
	return 0;
}
__attribute__ ((weak))
unsigned int pmic_config_interface_nolock(unsigned int RegNum, unsigned int val,
	unsigned int MASK, unsigned int SHIFT)
{
	pr_debug("[name:spm&]NO %s !!!\n", __func__);
	return 0;
}
#endif /* CONFIG_FPGA_EARLY_PORTING */

void spm_dump_world_clk_cntcv(void)
{
	u32 wlk_cntcv_l;
	u32 wlk_cntcv_h;

	/* SYS_TIMER counter value low and high */
	wlk_cntcv_l = _golden_read_reg(WORLD_CLK_CNTCV_L);
	wlk_cntcv_h = _golden_read_reg(WORLD_CLK_CNTCV_H);

	pr_debug("[name:spm&][SPM] wlk_cntcv_l = 0x%x, wlk_cntcv_h = 0x%x\n",
		wlk_cntcv_l, wlk_cntcv_h);
}

void spm_set_sysclk_settle(void)
{
	u32 settle;

	/* SYSCLK settle = MD SYSCLK settle but set it again for MD PDN */
	settle = spm_read(SPM_CLK_SETTLE);

	/* md_settle is keyword for suspend status */
#ifdef CONFIG_MTK_AEE_FEATURE
	aee_sram_printk("md_settle = %u, settle = %u\n",
		SPM_SYSCLK_SETTLE, settle);
#endif
	pr_debug("[name:spm&][SPM] md_settle = %u, settle = %u\n",
		SPM_SYSCLK_SETTLE, settle);
}

#if SPM_PMIC_DEBUG
static void spm_dump_power_gs_reg(void)
{
}
#endif /* SPM_PMIC_DEBUG */

#if !defined(CONFIG_FPGA_EARLY_PORTING)
static int mt_power_gs_dump_suspend_count = 2;
#endif
void spm_suspend_pre_process(int cmd, struct pwr_ctrl *pwrctrl)
{
#ifdef CONFIG_MTK_TINYSYS_SSPM_SUPPORT
	int ret;
	struct spm_data spm_d;
	unsigned int spm_opt = 0;

	memset(&spm_d, 0, sizeof(struct spm_data));
#endif

#ifdef SSPM_TIMESYNC_SUPPORT
	sspm_timesync_ts_get(&spm_d.u.suspend.sys_timestamp_h,
		&spm_d.u.suspend.sys_timestamp_l);
	sspm_timesync_clk_get(&spm_d.u.suspend.sys_src_clk_h,
		&spm_d.u.suspend.sys_src_clk_l);
#endif

#ifdef CONFIG_MTK_TINYSYS_SSPM_SUPPORT
	spm_d.u.suspend.spm_opt = spm_opt;

	ret = spm_to_sspm_command(cmd, &spm_d);
	if (ret < 0) {
#ifdef CONFIG_MTK_AEE_FEATURE
		aee_sram_printk("ret %d", ret);
#endif
		pr_debug("[name:spm&][SPM] ret %d", ret);
	}
#endif

#if !defined(CONFIG_FPGA_EARLY_PORTING)
	if (slp_dump_golden_setting || --mt_power_gs_dump_suspend_count >= 0)
		mt_power_gs_dump_suspend(slp_dump_golden_setting_type);
#endif

	/* dvfsrc_md_scenario_update(1); */

#if SPM_PMIC_DEBUG
	spm_dump_power_gs_reg();
#endif /* SPM_PMIC_DEBUG */
}

void spm_suspend_post_process(int cmd, struct pwr_ctrl *pwrctrl)
{
#ifdef CONFIG_MTK_TINYSYS_SSPM_SUPPORT
	int ret;
	struct spm_data spm_d;

	/* dvfsrc_md_scenario_update(0); */

#if SPM_PMIC_DEBUG
	spm_dump_power_gs_reg();
#endif /* SPM_PMIC_DEBUG */

	memset(&spm_d, 0, sizeof(struct spm_data));

#ifdef SSPM_TIMESYNC_SUPPORT
	sspm_timesync_ts_get(&spm_d.u.suspend.sys_timestamp_h,
		&spm_d.u.suspend.sys_timestamp_l);
	sspm_timesync_clk_get(&spm_d.u.suspend.sys_src_clk_h,
		&spm_d.u.suspend.sys_src_clk_l);
#endif

	ret = spm_to_sspm_command(cmd, &spm_d);
	if (ret < 0) {
#ifdef CONFIG_MTK_AEE_FEATURE
		aee_sram_printk("ret %d", ret);
#endif
		pr_debug("[name:spm&][SPM] ret %d", ret);
	}
#endif /* CONFIG_MTK_TINYSYS_SSPM_SUPPORT */
}

