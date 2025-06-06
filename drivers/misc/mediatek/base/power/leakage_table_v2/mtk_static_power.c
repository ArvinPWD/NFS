/*
 * Copyright (C) 2016 MediaTek Inc.
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

#include <linux/proc_fs.h>
#include <linux/debugfs.h>
#include <linux/export.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/math64.h>
#include <linux/module.h>
#include <linux/seq_file.h>
#include <linux/slab.h>

#include "mtk_common_static_power.h"
#include "mtk_spower_data.h"

#define SP_TAG "[Power/spower] "
#define SPOWER_LOG_NONE 0
#define SPOWER_LOG_WITH_PRINTK 1

/* #define SPOWER_LOG_PRINT SPOWER_LOG_WITH_PRINTK */
#define SPOWER_LOG_PRINT SPOWER_LOG_NONE

#define SPOWER_ERR(fmt, args...) pr_err(SP_TAG fmt, ##args)

#if (SPOWER_LOG_PRINT == SPOWER_LOG_NONE)
#define SPOWER_INFO(fmt, args...)
#elif (SPOWER_LOG_PRINT == SPOWER_LOG_WITH_PRINTK)
#define SPOWER_INFO(fmt, args...) pr_debug(SP_TAG fmt, ##args)
#endif

static struct sptab_s sptab[MTK_SPOWER_MAX];
static char static_power_buf[128];

#if defined(PRECISE_NODE)
static char static_power_buf_precise[128];
#endif

int interpolate(int x1, int x2, int x3, int y1, int y2)
{
	if (x1 == x2)
		return (y1 + y2) / 2;

#if defined(__LP64__) || defined(_LP64)
	return (long long)(x3 - x1) * (long long)(y2 - y1) /
		       (long long)(x2 - x1) +
	       y1;
#else
	return div64_s64((long long)(x3 - x1) * (long long)(y2 - y1),
			 (long long)(x2 - x1)) +
	       y1;
#endif
}

int interpolate_2d(struct sptab_s *tab, int v1, int v2, int t1, int t2,
		   int voltage, int degree)
{
	int c1, c2, p1, p2, p;

	if ((v1 == v2) && (t1 == t2)) {
		p = mA(tab, v1, t1);
	} else if (v1 == v2) {
		c1 = mA(tab, v1, t1);
		c2 = mA(tab, v1, t2);
		p = interpolate(deg(tab, t1), deg(tab, t2), degree, c1, c2);
	} else if (t1 == t2) {
		c1 = mA(tab, v1, t1);
		c2 = mA(tab, v2, t1);
		p = interpolate(mV(tab, v1), mV(tab, v2), voltage, c1, c2);
	} else {
		c1 = mA(tab, v1, t1);
		c2 = mA(tab, v1, t2);
		p1 = interpolate(deg(tab, t1), deg(tab, t2), degree, c1, c2);

		c1 = mA(tab, v2, t1);
		c2 = mA(tab, v2, t2);
		p2 = interpolate(deg(tab, t1), deg(tab, t2), degree, c1, c2);

		p = interpolate(mV(tab, v1), mV(tab, v2), voltage, p1, p2);
	}

	return p;
}

void interpolate_table(struct sptab_s *spt, int c1, int c2, int c3,
		       struct sptab_s *tab1, struct sptab_s *tab2)
{
	int v, t;

	/* avoid divid error, if we have bad raw data table */
	if (unlikely(c1 == c2)) {
		*spt = *tab1;
		SPOWER_INFO("sptab equal to tab1:%d/%d\n", c1, c3);
	} else {
		SPOWER_INFO("make sptab %d, %d, %d\n", c1, c2, c3);
		for (t = 0; t < tsize(spt); t++) {
			for (v = 0; v < vsize(spt); v++) {
				int *p = &mA(spt, v, t);

				p[0] = interpolate(c1, c2, c3, mA(tab1, v, t),
						   mA(tab2, v, t));

				if (v == 0 || v == vsize(spt) - 1)
					SPOWER_INFO("ma1, ma2=%d, %d, %d\n",
						    mA(tab1, v, t),
						    mA(tab2, v, t), p[0]);
			}
			SPOWER_INFO("\n");
		}
		SPOWER_INFO("make sptab done!\n");
	}
}

int sptab_lookup(struct sptab_s *tab, int voltage, int degree)
{
	int x1, x2, y1, y2, i;
	int mamper;

	/* lookup voltage */
	for (i = 0; i < vsize(tab); i++) {
		if (voltage <= mV(tab, i))
			break;
	}

	if (unlikely(voltage == mV(tab, i))) {
		x1 = x2 = i;
	} else if (unlikely(i == vsize(tab))) {
		x1 = vsize(tab) - 2;
		x2 = vsize(tab) - 1;
	} else if (i == 0) {
		x1 = 0;
		x2 = 1;
	} else {
		x1 = i - 1;
		x2 = i;
	}

	/* lookup degree */
	for (i = 0; i < tsize(tab); i++) {
		if (degree <= deg(tab, i))
			break;
	}

	if (unlikely(degree == deg(tab, i))) {
		y1 = y2 = i;
	} else if (unlikely(i == tsize(tab))) {
		y1 = tsize(tab) - 2;
		y2 = tsize(tab) - 1;
	} else if (i == 0) {
		y1 = 0;
		y2 = 1;
	} else {
		y1 = i - 1;
		y2 = i;
	}

	mamper = interpolate_2d(tab, x1, x2, y1, y2, voltage, degree);

	/*
	 * SPOWER_INFO("x1=%d, x2=%d, y1=%d, y2=%d\n", x1, x2, y1, y2);
	 * SPOWER_INFO("sptab_lookup-volt=%d, deg=%d, lkg=%d\n",
	 *                                 voltage, degree, mamper);
	 */
	return mamper;
}

int mtk_spower_make_table(struct sptab_s *spt, int voltage, int degree,
			  unsigned int id, struct sptab_list *all_tab[])
{
	int i, j;
	struct sptab_s *tab[MAX_TABLE_SIZE], *tab1, *tab2, *tspt;
	int wat; /* leakage that reads from efuse */
	int devinfo_domain;
	int c[MAX_TABLE_SIZE] = {0};
	int temp;

	/** FIXME, test only; please read efuse to assign. **/
	/* wat = 80; */
	/* voltage = 1150; */
	/* degree = 30; */

	SPOWER_INFO("spower_raw->table_size : %d\n", spower_raw->table_size);
	/* find out target domain's 3 raw table */
	for (i = 0; i < spower_raw->table_size; i++)
		tab[i] = &(all_tab[id]->tab_raw[i]);

	/* get leakage that reads from efuse */
	wat = spower_lkg_info[tab[0]->leakage_id].value;

	/** lookup tables which the chip type locates to **/
	for (i = 0; i < spower_raw->table_size; i++) {
		devinfo_domain = tab[i]->devinfo_domain;
		SPOWER_INFO("devinfo_domain : 0x%x\n", devinfo_domain);
		for (j = 0; j < MTK_SPOWER_MAX; j++) {
			/* get table of reference bank, and look up target in
			 * that table
			 */
			if (devinfo_domain & BIT(j)) {
				temp = (sptab_lookup(&(all_tab[j]->tab_raw[i]),
						     voltage, degree));
				/* SPOWER_INFO("cal table %d lkg %d\n", j,
				 * temp);
				 */
				c[i] += (temp *
					 all_tab[j]->tab_raw[i].instance) >>
					10;
				/* SPOWER_INFO("total lkg %d\n", c[i]); */
			}
		}
		SPOWER_INFO("done-->get c=%d\n", c[i]);
		if (wat >= c[i])
			break;
	}

	/** FIXME,
	 * There are only 2 tables are used to interpolate to form SPTAB.
	 * Thus, sptab takes use of the container which raw data is not used
	 *anymore.
	 **/

	if (i == spower_raw->table_size) {
		i = spower_raw->table_size - 1;
/** above all **/
#if defined(EXTER_POLATION)
		tab1 = tab[spower_raw->table_size - 2];
		tab2 = tab[spower_raw->table_size - 1];

		/** occupy the free container**/
		tspt = tab[spower_raw->table_size - 3];
#else  /* #if defined(EXTER_POLATION) */
		if (spower_raw->table_size - 1 >= 0)
			tspt = tab1 = tab2 = tab[spower_raw->table_size - 1];
		else
			tspt = tab1 = tab2 = tab[1];
#endif /* #if defined(EXTER_POLATION) */

		SPOWER_INFO("sptab max tab:%d/%d\n", wat, c[i]);
	} else if (i == 0) {
#if defined(EXTER_POLATION)
		/** below all **/
		tab1 = tab[0];
		tab2 = tab[1];

		/** occupy the free container**/
		tspt = tab[2];
#else  /* #if defined(EXTER_POLATION) */
		tspt = tab1 = tab2 = tab[0];
#endif /* #if defined(EXTER_POLATION) */

		SPOWER_INFO("sptab min tab:%d/%d\n", wat, c[i]);
	} else if (wat == c[i]) {
		/** just match **/
		tab1 = tab2 = tab[i];
		/** pointer duplicate  **/
		tspt = tab1;
		SPOWER_INFO("sptab equal to tab:%d/%d\n", wat, c[i]);
	} else {
		/** anyone **/
		tab1 = tab[i - 1];
		tab2 = tab[i];

		/** occupy the free container**/
		tspt = tab[(i + 1) % spower_raw->table_size];
		SPOWER_INFO("sptab interpolate tab:%d/%d, i:%d\n", wat, c[i],
			    i);
	}

	if (wat == 0) {
		/* force mc50 */
		tab1 = tab2 = tab[1];
		tspt = tab1;
		SPOWER_INFO("@@~ force mc50\n");
	}

	/** sptab needs to interpolate 2 tables. **/
	if (tab1 != tab2)
		interpolate_table(tspt, c[i - 1], c[i], wat, tab1, tab2);

	/** update to global data **/
	*spt = *tspt;

	return 0;
}

#if defined(MTK_SPOWER_UT)
void mtk_spower_ut(void)
{
	int v, t, p, i;

	for (i = 0; i < MTK_SPOWER_MAX; i++) {

		SPOWER_INFO("This is %s\n", spower_name[i]);

		/* new test case */
		v = 300;
		t = -50;
		p = mt_spower_get_leakage(i, v, t);

		v = 300;
		t = 150;
		p = mt_spower_get_leakage(i, v, t);

		v = 1500;
		t = -50;
		p = mt_spower_get_leakage(i, v, t);

		v = 1500;
		t = 150;
		p = mt_spower_get_leakage(i, v, t);

		v = 1150;
		t = 105;
		p = mt_spower_get_leakage(i, v, t);

		v = 700;
		t = 20;
		p = mt_spower_get_leakage(i, v, t);

		v = 820;
		t = 120;
		p = mt_spower_get_leakage(i, v, t);

		v = 650;
		t = 18;
		p = mt_spower_get_leakage(i, v, t);

		v = 600;
		t = 15;
		p = mt_spower_get_leakage(i, v, t);

		v = 550;
		t = 22;
		p = mt_spower_get_leakage(i, v, t);

		v = 550;
		t = 10;
		p = mt_spower_get_leakage(i, v, t);

		v = 400;
		t = 10;
		p = mt_spower_get_leakage(i, v, t);

		v = 320;
		t = 5;
		p = mt_spower_get_leakage(i, v, t);

		v = 220;
		t = 0;
		p = mt_spower_get_leakage(i, v, t);

		v = 80;
		t = -5;
		p = mt_spower_get_leakage(i, v, t);

		v = 0;
		t = -10;
		p = mt_spower_get_leakage(i, v, t);

		v = 1200;
		t = -10;
		p = mt_spower_get_leakage(i, v, t);

		v = 1200;
		t = -25;
		p = mt_spower_get_leakage(i, v, t);

		v = 1200;
		t = -28;
		p = mt_spower_get_leakage(i, v, t);

		v = 120;
		t = -39;
		p = mt_spower_get_leakage(i, v, t);

		v = 120;
		t = -120;
		p = mt_spower_get_leakage(i, v, t);

		v = 950;
		t = -80;
		p = mt_spower_get_leakage(i, v, t);

		v = 1000;
		t = 5;
		p = mt_spower_get_leakage(i, v, t);

		v = 1150;
		t = 10;
		p = mt_spower_get_leakage(i, v, t);

		SPOWER_INFO("%s efuse: %d\n", spower_name[i],
			    mt_spower_get_efuse_lkg(i));
		SPOWER_INFO("%s Done\n", spower_name[i]);
	}
}
#endif


#ifdef CONFIG_DEBUG_FS
static int static_power_show(struct seq_file *s, void *unused)
{
	seq_printf(s, "%s", static_power_buf);
	return 0;
}

static int static_power_open(struct inode *inode, struct file *file)
{
	return single_open(file, static_power_show, NULL);
}

static const struct file_operations static_power_operations = {
	.open = static_power_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};

#if defined(PRECISE_NODE)
static int static_power_precise_show(struct seq_file *s, void *unused)
{
	seq_printf(s, "%s", static_power_buf_precise);
	return 0;
}

static int static_power_precise_open(struct inode *inode, struct file *file)
{
	return single_open(file, static_power_precise_show, NULL);
}

static const struct file_operations static_power_precise_operations = {
	.open = static_power_precise_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};
#endif

#endif
#define PROC_FOPS_RO(name)					\
	static int name ## _proc_open(struct inode *inode,	\
		struct file *file)				\
	{							\
		return single_open(file, name ## _proc_show,	\
			PDE_DATA(inode));			\
	}							\
	static const struct file_operations name ## _proc_fops = {	\
		.owner		  = THIS_MODULE,			\
		.open		   = name ## _proc_open,		\
		.read		   = seq_read,				\
		.llseek		 = seq_lseek,				\
		.release		= single_release,		\
	}

#define PROC_ENTRY(name)	{__stringify(name), &name ## _proc_fops}

static int spower_lkg_proc_show(struct seq_file *m, void *v)
{
	seq_printf(m, "%s", static_power_buf);
	return 0;
}
PROC_FOPS_RO(spower_lkg);

int spower_procfs_init(void)
{
	struct proc_dir_entry *dir = NULL;
	int i;

	struct pentry {
		const char *name;
		const struct file_operations *fops;
	};

	const struct pentry entries[] = {
		PROC_ENTRY(spower_lkg),
	};

	dir = proc_mkdir("leakage", NULL);

	if (!dir) {
		pr_notice("fail to create /proc/leakage @ %s()\n",
								__func__);
		return -ENOMEM;
	}

	for (i = 0; i < ARRAY_SIZE(entries); i++) {
		if (!proc_create
		    (entries[i].name, 0664, dir, entries[i].fops))
			pr_notice("%s(), create /proc/leakage/%s failed\n",
				__func__, entries[i].name);
	}
	return 0;
}
static unsigned int mtSpowerInited;
int mt_spower_init(void)
{
#ifndef WITHOUT_LKG_EFUSE
	int devinfo = 0;
	unsigned int temp_lkg;
#endif
	int i;
	unsigned int v_of_fuse;
	int t_of_fuse;
	unsigned int idx = 0;
	char *p_buf = static_power_buf;
#if defined(PRECISE_NODE)
	char *p_buf_precise = static_power_buf_precise;
#endif

	/* Group FF,TT,SS tables of all the banks together */
	struct sptab_list *tab[MTK_SPOWER_MAX];

#ifdef SPOWER_NOT_READY
	/* FIX ME */
	return 0;
#endif

	if (mtSpowerInited == 1)
		return 0;

	for (i = 0; i < MTK_SPOWER_MAX; i++)
		tab[i] = kmalloc(sizeof(struct sptab_list), GFP_KERNEL);

	/* avoid side effect from multiple invocation */
	if (tab_validate(&sptab[0]))
		return 0;

#ifndef WITHOUT_LKG_EFUSE
	for (i = 0; i < MTK_LEAKAGE_MAX; i++) {
		devinfo = (int)get_devinfo_with_index(
			spower_lkg_info[i].devinfo_idx);
		temp_lkg =
			(devinfo >> spower_lkg_info[i].devinfo_offset) & 0xff;
		SPOWER_INFO("[Efuse] %s => 0x%x\n", spower_lkg_info[i].name,
			    temp_lkg);
		/* if has leakage info in efuse, get the final leakage */
		/* if no leakage info in efuse, spower_lkg_info[i].value will
		 * use default lkg
		 */

		if (temp_lkg != 0) {
			temp_lkg = (int)devinfo_table[temp_lkg];
			spower_lkg_info[i].value =
				(int)(temp_lkg * spower_lkg_info[i].v_of_fuse);
		}
		/*
		 * Fix Me
		 */
#if defined(CONFIG_MACH_MT6893) || defined(TRIGEAR_LEAKAGE)
		if (i == MTK_B_LEAKAGE)
			spower_lkg_info[i].value *= 0.47;
#endif
		SPOWER_INFO("[Efuse Leakage] %s => 0x%x\n",
			    spower_lkg_info[i].name, temp_lkg);
		SPOWER_INFO("[Final Leakage] %s => %d\n",
			    spower_lkg_info[i].name, spower_lkg_info[i].value /
			    1000);
	}
#endif
	SPOWER_INFO("spower table construct\n");
	/** structurize the raw data **/
	for (i = 0; i < MTK_SPOWER_MAX; i++) {
		spower_tab_construct(tab[i]->tab_raw, &spower_raw[i], i);
		SPOWER_INFO("table %d done\n", tab[i]->tab_raw[0].spower_id);
	}

	for (i = 0; i < MTK_SPOWER_MAX; i++) {
		SPOWER_INFO("%s\n", spower_name[i]);
		idx = tab[i]->tab_raw[0].leakage_id;
		v_of_fuse = spower_lkg_info[idx].v_of_fuse;
		t_of_fuse = spower_lkg_info[idx].t_of_fuse;
		mtk_spower_make_table(&sptab[i], v_of_fuse, t_of_fuse,
				      (unsigned int)i, tab);
		if (tab[i]->tab_raw[0].print_leakage == true) {
			p_buf += sprintf(p_buf, "%d/",
					 (spower_lkg_info[idx].value / 1000 /
					  tab[i]->tab_raw[0].instance));
#if defined(PRECISE_NODE)
			p_buf_precise += sprintf(p_buf_precise, "%d.%d/",
				DIV_ROUND_CLOSEST(spower_lkg_info[idx].value,
						tab[i]->tab_raw[0].instance *
						100) / 10,
				DIV_ROUND_CLOSEST(spower_lkg_info[idx].value,
						tab[i]->tab_raw[0].instance *
						100) % 10
			);
#endif
		}
	}
	p_buf += sprintf(p_buf, "\n");
#if defined(PRECISE_NODE)
	p_buf_precise += sprintf(p_buf_precise, "\n");
#endif

#if defined(MTK_SPOWER_UT)
	SPOWER_INFO("Start SPOWER UT!\n");
	mtk_spower_ut();
	SPOWER_INFO("End SPOWER UT!\n");
#endif

	/* print static_power_buf and generate debugfs node */
	SPOWER_ERR("%s", static_power_buf);

#ifdef CONFIG_DEBUG_FS
	debugfs_create_file("static_power", S_IFREG | 0400, NULL, NULL,
			    &static_power_operations);
#endif

#if defined(PRECISE_NODE)
	SPOWER_ERR("%s", static_power_buf_precise);
#endif

#ifdef CONFIG_DEBUG_FS
#if defined(PRECISE_NODE)
	debugfs_create_file("static_power_precise", S_IFREG | 0400, NULL, NULL,
			    &static_power_precise_operations);
#endif
#endif

	spower_procfs_init();
	for (i = 0; i < MTK_SPOWER_MAX; i++)
		kfree(tab[i]);

	mtSpowerInited = 1;
	return 0;
}
module_init(mt_spower_init);

/* return 0, means sptab is not yet ready. */
/* vol unit should be mv */
int mt_spower_get_leakage(int dev, unsigned int vol, int deg)
{
	int ret;

	if (!tab_validate(&sptab[dev]))
		return 0;

	if (vol > mV(&sptab[dev], VSIZE - 1))
		vol = mV(&sptab[dev], VSIZE - 1);
	else if (vol < mV(&sptab[dev], 0))
		vol = mV(&sptab[dev], 0);

	if (deg > deg(&sptab[dev], TSIZE - 1))
		deg = deg(&sptab[dev], TSIZE - 1);
	else if (deg < deg(&sptab[dev], 0))
		deg = deg(&sptab[dev], 0);

	ret = sptab_lookup(&sptab[dev], (int)vol, deg) >> 10;

	SPOWER_INFO("%s-dev=%d,volt=%d, deg=%d, lkg=%d\n", __func__,
		    dev, vol, deg, ret);
	return ret;
}
EXPORT_SYMBOL(mt_spower_get_leakage);

int mt_spower_get_leakage_uW(int dev, unsigned int vol, int deg)
{
	int ret;

	if (dev < 0)
		return 0;

	if (!tab_validate(&sptab[dev]))
		return 0;

	if (vol > mV(&sptab[dev], VSIZE - 1))
		vol = mV(&sptab[dev], VSIZE - 1);
	else if (vol < mV(&sptab[dev], 0))
		vol = mV(&sptab[dev], 0);

	if (deg > deg(&sptab[dev], TSIZE - 1))
		deg = deg(&sptab[dev], TSIZE - 1);
	else if (deg < deg(&sptab[dev], 0))
		deg = deg(&sptab[dev], 0);

	ret = sptab_lookup(&sptab[dev], (int)vol, deg);

	SPOWER_INFO("%s-dev=%d,volt=%d, deg=%d, lkg=%d\n", __func__,
		    dev, vol, deg, ret);
	return ret;
}
EXPORT_SYMBOL(mt_spower_get_leakage_uW);

int mt_spower_get_efuse_lkg(int dev)
{
	unsigned int id = 0;

	if (dev >= MTK_SPOWER_MAX || dev < 0)
		return 0;

	id = spower_raw[dev].leakage_id;
	if (id >= MTK_SPOWER_MAX)
		return 0;
#if 0
	int devinfo = 0, efuse_lkg = 0, efuse_lkg_mw = 0;
	int leakage_id = spower_raw[dev].leakage_id;

	devinfo = (int) get_devinfo_with_index(devinfo_idx[dev]);
	efuse_lkg = (devinfo >> devinfo_offset[dev]) & 0xff;
	efuse_lkg_mw = (efuse_lkg == 0) ? default_leakage[leakage_id] :
		(int) (devinfo_table[efuse_lkg] * V_OF_FUSE / 1000);

	return efuse_lkg_mw;
#endif
	return spower_lkg_info[id].value / 1000;
}
EXPORT_SYMBOL(mt_spower_get_efuse_lkg);
