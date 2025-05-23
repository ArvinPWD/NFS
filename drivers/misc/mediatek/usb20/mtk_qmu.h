/*
 * Copyright (C) 2017 MediaTek Inc.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */

#ifndef _MTK_QMU_H_
#define _MTK_QMU_H_

#ifdef CONFIG_MTK_MUSB_QMU_SUPPORT

/* for musb_read/write api */
#include "mtk_musb.h"
#include "musb_debug.h"
#include "musb_io.h"

#include <linux/dmapool.h>

/* CUSTOM SETTING */
#define GPD_LEN_ALIGNED (64)	/* > gpd len (16) and cache line size aligned */
/* GPD_LEN_ALIGNED -
 * 16(should be sizeof(TGPD)
 */
#define GPD_EXT_LEN (48)
#define GPD_SZ (16)
#define DFT_MAX_GPD_NUM 36
#ifndef MUSB_QMU_LIMIT_SUPPORT
#define RXQ_NUM 8
#define TXQ_NUM 8
#else
#define RXQ_NUM MUSB_QMU_LIMIT_RXQ_NUM
#define TXQ_NUM MUSB_QMU_LIMIT_TXQ_NUM
#endif
#define MAX_QMU_EP RXQ_NUM
#define TXQ	0
#define RXQ	1
#define ISOC_EP_START_IDX 1

/* QMU SETTING */
#define NO_ZLP 0
#define HW_MODE 1
#define GPD_MODE 2
#define TXZLP GPD_MODE
/* #define TXZLP HW_MODE */
/*#define TXZLP NO_ZLP */

/* #define CFG_RX_ZLP_EN */
/* #define CFG_RX_COZ_EN */

#define CFG_CS_CHECK
/* #define CFG_EMPTY_CHECK */

#ifdef CONFIG_MTK_MUSB_DRV_36BIT
struct tx_haddr {
	u8 hiaddr;
	u8 reserved;
};

struct rx_haddr {
	u8 hiaddr;
};

union gpd_b14 {
	u8 ExtLength; /*Tx ExtLength for  TXGPD*/
	struct rx_haddr rx_haddr; /*Rx hi address for RXGPD*/
};

union gpd_w1 {
	u16 DataBufferLen; /*Rx Allow Length for RXGPD*/
	struct tx_haddr tx_haddr; /*Tx hi address for TXGPD */
};
#endif


/* TGPD */
struct TGPD {
	u8 flag;
	u8 chksum;
#ifdef CONFIG_MTK_MUSB_DRV_36BIT
	union gpd_w1 gpd_w1;
#else
	u16 DataBufferLen; /*Rx Allow Length */
#endif
	/* address field, 32-bit long */
	u32 pNext;
	u32 pBuf;

	u16 bufLen;
#ifdef CONFIG_MTK_MUSB_DRV_36BIT
	union gpd_b14 gpd_b14;
#else
	u8 ExtLength;
#endif
	u8 ZTepFlag;
};

struct _GPD_RANGE {
	struct TGPD *pNext;
	struct TGPD *pStart;
	struct TGPD *pEnd;
};

extern int mtk_host_qmu_concurrent;
extern int mtk_host_qmu_pipe_msk;
extern int mtk_host_qmu_force_isoc_restart;
extern int mtk_host_active_dev_cnt;
#ifdef CONFIG_MTK_UAC_POWER_SAVING
extern unsigned int low_power_timer_total_trigger_cnt;
extern unsigned int low_power_timer_total_wake_cnt;
extern int low_power_timer_mode2_option;
extern int low_power_timer_mode;
extern int usb_on_sram;
extern int audio_on_sram;
extern int use_mtk_audio;
extern int mtk_audio_request_sram
		(dma_addr_t *phys_addr, unsigned char **virt_addr,
		unsigned int length, void *user);
extern void mtk_audio_free_sram(void *user);
extern int gpd_switch_to_sram(struct device *dev);
extern void gpd_switch_to_dram(struct device *dev);
#endif
extern int mtk_qmu_dbg_level;	/* refer to musb_core.c */
extern int mtk_qmu_max_gpd_num;
extern int isoc_ep_end_idx;
extern int isoc_ep_gpd_count;
static inline int mtk_dbg_level(unsigned int level)
{
	return mtk_qmu_dbg_level >= level;
}

#define LOG_EMERG		0
#define LOG_ALERT		1
#define LOG_CRIT		2
#define LOG_ERR		3
#define LOG_WARN	4
#define LOG_NOTICE		5
#define LOG_INFO		6
#define LOG_DBG		7

#define QMU_DBG_ON
#ifdef QMU_DBG_ON
#define QMU_ERR(format, args...) do {if (mtk_dbg_level(LOG_ERR)) \
	pr_notice("QMU_ERR,<%s %d>, " format, __func__, __LINE__, ## args);  } \
	while (0)
#define QMU_WARN(format, args...) do {if (mtk_dbg_level(LOG_WARN)) \
	pr_notice("QMU_WARN,<%s %d>, " format, __func__, __LINE__, ## args); } \
	while (0)
#define QMU_INFO(format, args...) do {if (mtk_dbg_level(LOG_INFO)) \
	pr_notice("QMU_INFO,<%s %d>, " format, __func__, __LINE__, ## args); } \
	while (0)
#define QMU_DBG(format, args...) do {if (mtk_dbg_level(LOG_DBG)) \
	pr_notice("QMU_DBG,<%s %d>, " format, __func__, __LINE__, ## args);  } \
	while (0)
#else
#define QMU_ERR(format, args...) ((void)0)
#define QMU_WARN(format, args...) ((void)0)
#define QMU_INFO(format, args...) ((void)0)
#define QMU_DBG(format, args...) ((void)0)
#endif



/* QMU macros */
#define USB_HW_QMU_OFF	0x0000
#define USB_HW_QUCS_OFF	0x0300
#define USB_HW_QIRQ_OFF	0x0400
#define USB_HW_QDBG_OFF	0x04F0

#define MGC_O_QMU_QCR0	0x0000
#define MGC_O_QMU_QCR2	0x0008
#define MGC_O_QMU_QCR3	0x000C

#define MGC_O_QMU_RQCSR0	0x0010
#define MGC_O_QMU_RQSAR0	0x0014
#define MGC_O_QMU_RQCPR0	0x0018
#define MGC_O_QMU_RQCSR(n) (MGC_O_QMU_RQCSR0+0x0010*((n)-1))
#define MGC_O_QMU_RQSAR(n) (MGC_O_QMU_RQSAR0+0x0010*((n)-1))
#define MGC_O_QMU_RQCPR(n) (MGC_O_QMU_RQCPR0+0x0010*((n)-1))

#define MGC_O_QMU_RQTR_BASE	0x0090
#define MGC_O_QMU_RQTR(n)		(MGC_O_QMU_RQTR_BASE+0x4*((n)-1))
#define MGC_O_QMU_RQLDPR0		0x0100
#define MGC_O_QMU_RQLDPR(n)	(MGC_O_QMU_RQLDPR0+0x4*((n)-1))

#define MGC_O_QMU_TQCSR0	0x0200
#define MGC_O_QMU_TQSAR0	0x0204
#define MGC_O_QMU_TQCPR0	0x0208
#define MGC_O_QMU_TQCSR(n) (MGC_O_QMU_TQCSR0+0x0010*((n)-1))
#define MGC_O_QMU_TQSAR(n) (MGC_O_QMU_TQSAR0+0x0010*((n)-1))
#define MGC_O_QMU_TQCPR(n) (MGC_O_QMU_TQCPR0+0x0010*((n)-1))

#define MGC_O_QMU_QAR		0x0300
#define MGC_O_QUCS_USBGCSR	0x0000
#define MGC_O_QIRQ_QISAR		0x0000
#define MGC_O_QIRQ_QIMR		0x0004
#define MGC_O_QIRQ_QIMCR		0x0008
#define MGC_O_QIRQ_QIMSR		0x000C
#define MGC_O_QIRQ_IOCDISR    0x0030
#define MGC_O_QIRQ_TEPEMPR	0x0060
#define MGC_O_QIRQ_TEPEMPMR	0x0064
#define MGC_O_QIRQ_TEPEMPMCR	0x0068
#define MGC_O_QIRQ_TEPEMPMSR	0x006C
#define MGC_O_QIRQ_REPEMPR	0x0070
#define MGC_O_QIRQ_REPEMPMR	0x0074
#define MGC_O_QIRQ_REPEMPMCR	0x0078
#define MGC_O_QIRQ_REPEMPMSR	0x007C

#define MGC_O_QIRQ_RQEIR		0x0090
#define MGC_O_QIRQ_RQEIMR		0x0094
#define MGC_O_QIRQ_RQEIMCR	0x0098
#define MGC_O_QIRQ_RQEIMSR	0x009C
#define MGC_O_QIRQ_REPEIR		0x00A0
#define MGC_O_QIRQ_REPEIMR	0x00A4
#define MGC_O_QIRQ_REPEIMCR	0x00A8
#define MGC_O_QIRQ_REPEIMSR	0x00AC
#define MGC_O_QIRQ_TQEIR		0x00B0
#define MGC_O_QIRQ_TQEIMR		0x00B4
#define MGC_O_QIRQ_TQEIMCR	0x00B8
#define MGC_O_QIRQ_TQEIMSR	0x00BC
#define MGC_O_QIRQ_TEPEIR		0x00C0
#define MGC_O_QIRQ_TEPEIMR	0x00C4
#define MGC_O_QIRQ_TEPEIMCR	0x00C8
#define MGC_O_QIRQ_TEPEIMSR	0x00CC

#define MGC_O_QDBG_DFCR	0x0000
#define MGC_O_QDBG_DFMR	0x0004

/* brief Queue Control value Definition */
#define DQMU_QUE_START	0x00000001
#define DQMU_QUE_RESUME	0x00000002
#define DQMU_QUE_STOP		0x00000004
#define DQMU_QUE_ACTIVE	0x00008000

/*brief USB QMU Special Control USBGCSR value Definition*/
#define USB_QMU_Tx0_EN			0x00000001
#define USB_QMU_Tx_EN(n)			(USB_QMU_Tx0_EN<<((n)-1))
#define USB_QMU_Rx0_EN			0x00010000
#define USB_QMU_Rx_EN(n)			(USB_QMU_Rx0_EN<<((n)-1))
#define USB_QMU_HIFEVT_EN			0x00000100
#define USB_QMU_HIFCMD_EN			0x01000000
#define DQMU_SW_RESET		0x00010000
#define DQMU_CS16B_EN		0x80000000
#define DQMU_TQ0CS_EN		0x00010000
#define DQMU_TQCS_EN(n)	(DQMU_TQ0CS_EN<<((n)-1))
#define DQMU_RQ0CS_EN		0x00000001
#define DQMU_RQCS_EN(n)	(DQMU_RQ0CS_EN<<((n)-1))
#define DQMU_TX0_ZLP		0x01000000
#define DQMU_TX_ZLP(n)		(DQMU_TX0_ZLP<<((n)-1))
#define DQMU_TX0_MULTIPLE	0x00010000
#define DQMU_TX_MULTIPLE(n)	(DQMU_TX0_MULTIPLE<<((n)-1))
#define DQMU_T0Q_GDP_ZLP	0x00000100
#define DQMU_TQ_GDP_ZLP(n)	(DQMU_T0Q_GDP_ZLP<<((n)-1))
#define DQMU_RX0_MULTIPLE	0x00010000
#define DQMU_RX_MULTIPLE(n)	(DQMU_RX0_MULTIPLE<<((n)-1))
#define DQMU_RX0_ZLP		0x01000000
#define DQMU_RX_ZLP(n)		(DQMU_RX0_ZLP<<((n)-1))
#define DQMU_RX0_COZ		0x00000100
#define DQMU_RX_COZ(n)		(DQMU_RX0_COZ<<((n)-1))

#define DQMU_M_TXEP_ERR	0x10000000
#define DQMU_M_TXQ_ERR	0x08000000
#define DQMU_M_RXEP_ERR	0x04000000
#define DQMU_M_RXQ_ERR	0x02000000
#define DQMU_M_RQ_EMPTY	0x00020000
#define DQMU_M_TQ_EMPTY	0x00010000
#define DQMU_M_RX0_EMPTY	0x00000001
#define DQMU_M_RX_EMPTY(n)	(DQMU_M_RX0_EMPTY<<((n)-1))
#define DQMU_M_TX0_EMPTY	0x00000001
#define DQMU_M_TX_EMPTY(n)	(DQMU_M_TX0_EMPTY<<((n)-1))
#define DQMU_M_RX0_DONE	0x00000100
#define DQMU_M_RX_DONE(n)	(DQMU_M_RX0_DONE<<((n)-1))
#define DQMU_M_TX0_DONE	0x00000001
#define DQMU_M_TX_DONE(n)	(DQMU_M_TX0_DONE<<((n)-1))

#define DQMU_M_RX0_ZLP_ERR	0x01000000
#define DQMU_M_RX_ZLP_ERR(n)	(DQMU_M_RX0_ZLP_ERR<<((n)-1))
#define DQMU_M_RX0_LEN_ERR	0x00000100
#define DQMU_M_RX_LEN_ERR(n)	(DQMU_M_RX0_LEN_ERR<<((n)-1))
#define DQMU_M_RX0_GPDCS_ERR		0x00000001
#define DQMU_M_RX_GPDCS_ERR(n)	(DQMU_M_RX0_GPDCS_ERR<<((n)-1))

#define DQMU_M_TX0_LEN_ERR	0x00010000
#define DQMU_M_TX_LEN_ERR(n)	(DQMU_M_TX0_LEN_ERR<<((n)-1))
#define DQMU_M_TX0_GPDCS_ERR	0x00000100
#define DQMU_M_TX_GPDCS_ERR(n)	(DQMU_M_TX0_GPDCS_ERR<<((n)-1))
#define DQMU_M_TX0_BDCS_ERR		0x00000001
#define DQMU_M_TX_BDCS_ERR(n)	(DQMU_M_TX0_BDCS_ERR<<((n)-1))

#define DQMU_M_TX0_EP_ERR		0x00000001
#define DQMU_M_TX_EP_ERR(n)	(DQMU_M_TX0_EP_ERR<<((n)-1))

#define DQMU_M_RX0_EP_ERR		0x00000001
#define DQMU_M_RX_EP_ERR(n)	(DQMU_M_RX0_EP_ERR<<((n)-1))
#define DQMU_M_RQ_DIS_IOC(n)   (0x100<<((n)-1))

#define MGC_ReadQMU8(base, _offset) \
	musb_readb(base, (USB_HW_QMU_OFF + _offset))

#define MGC_ReadQUCS8(base, _offset) \
	musb_readb(base, (USB_HW_QUCS_OFF + _offset))

#define MGC_ReadQIRQ8(base, _offset) \
	musb_readb(base, (USB_HW_QIRQ_OFF + _offset))

#define MGC_ReadQMU16(base, _offset) \
	musb_readw(base, (USB_HW_QMU_OFF + _offset))

#define MGC_ReadQUCS16(base, _offset) \
	musb_readw(base, (USB_HW_QUCS_OFF + _offset))

#define MGC_ReadQIRQ16(base, _offset) \
	musb_readw(base, (USB_HW_QIRQ_OFF + _offset))
#define MGC_ReadQMU32(base, _offset) \
	musb_readl(base, (USB_HW_QMU_OFF + _offset))

#define MGC_ReadQUCS32(base, _offset) \
	musb_readl(base, (USB_HW_QUCS_OFF + _offset))

#define MGC_ReadQIRQ32(base, _offset) \
	musb_readl(base, (USB_HW_QIRQ_OFF + _offset))

#define MGC_WriteQMU32(base, _offset, _data) \
	musb_writel(base, (USB_HW_QMU_OFF + _offset), _data)

#define MGC_WriteQUCS32(base, _offset, _data) \
	musb_writel(base, (USB_HW_QUCS_OFF + _offset), _data)

#define MGC_WriteQIRQ32(base, _offset, _data) \
	musb_writel(base, (USB_HW_QIRQ_OFF + _offset), _data)

u8 PDU_calcCksum(u8 *data, int len);

/* brief Define DMAQ GPD format */
#define TGPD_FLAGS_HWO              0x01
#define TGPD_IS_FLAGS_HWO(_pd)  \
	(((struct TGPD *)_pd)->flag & TGPD_FLAGS_HWO)
#define TGPD_SET_FLAGS_HWO(_pd)  \
	(((struct TGPD *)_pd)->flag |= TGPD_FLAGS_HWO)
#define TGPD_CLR_FLAGS_HWO(_pd)  \
	(((struct TGPD *)_pd)->flag &= (~TGPD_FLAGS_HWO))
#define TGPD_FORMAT_BDP             0x02
#define TGPD_IS_FORMAT_BDP(_pd) \
	(((struct TGPD *)_pd)->flag & TGPD_FORMAT_BDP)
#define TGPD_SET_FORMAT_BDP(_pd)  \
	(((struct TGPD *)_pd)->flag |= TGPD_FORMAT_BDP)
#define TGPD_CLR_FORMAT_BDP(_pd)  \
	(((struct TGPD *)_pd)->flag &= (~TGPD_FORMAT_BDP))

#define TGPD_SET_FLAG(_pd, _flag)  \
			(((struct TGPD *)_pd)->flag = \
			(((struct TGPD *)_pd)->flag&(~TGPD_FLAGS_HWO))|(_flag))
#define TGPD_GET_FLAG(_pd)  \
	(((struct TGPD *)_pd)->flag & TGPD_FLAGS_HWO)
#define TGPD_SET_CHKSUM(_pd, _n)  \
	(((struct TGPD *)_pd)->chksum = PDU_calcCksum((u8 *)_pd, _n))
#define TGPD_SET_CHKSUM_HWO(_pd, _n)  \
	(((struct TGPD *)_pd)->chksum = PDU_calcCksum((u8 *)_pd, _n)-1)
#define TGPD_GET_CHKSUM(_pd)        (((struct TGPD *)_pd)->chksum)
#define TGPD_SET_FORMAT(_pd, _fmt)  \
			(((struct TGPD *)_pd)->flag =\
			(((struct TGPD *)_pd)->flag&(~TGPD_FORMAT_BDP))|(_fmt))
#define TGPD_GET_FORMAT(_pd)  \
	(((((struct TGPD *)_pd)->flag & TGPD_FORMAT_BDP)>>1))

#ifdef CONFIG_MTK_MUSB_DRV_36BIT
#define TGPD_SET_DataBUF_LEN(_pd, _len)  \
	(((struct TGPD *)_pd)->gpd_w1.DataBufferLen = _len)
#define TGPD_ADD_DataBUF_LEN(_pd, _len)  \
	(((struct TGPD *)_pd)->gpd_w1.DataBufferLen += _len)
#define TGPD_GET_DataBUF_LEN(_pd)  \
	(((struct TGPD *)_pd)->gpd_w1.DataBufferLen)
#else
#define TGPD_SET_DataBUF_LEN(_pd, _len)  \
	(((struct TGPD *)_pd)->DataBufferLen = _len)
#define TGPD_ADD_DataBUF_LEN(_pd, _len)  \
	(((struct TGPD *)_pd)->DataBufferLen += _len)
#define TGPD_GET_DataBUF_LEN(_pd)       (((struct TGPD *)_pd)->DataBufferLen)
#endif

#ifdef CONFIG_MTK_MUSB_DRV_36BIT
#define TGPD_SET_NEXT(_pd, _next)  \
	(((struct TGPD *)_pd)->pNext = (u32)_next)
#define TGPD_SET_NEXT_TXHI(_pd, _next)  \
	do {	\
		((struct TGPD *) _pd)->gpd_w1.tx_haddr.hiaddr &= 0x0F;	\
		((struct TGPD *) _pd)->gpd_w1.tx_haddr.hiaddr |=\
							((u8)_next << 4); \
	} while (0)

#define TGPD_SET_NEXT_RXHI(_pd, _next)	\
	do {	\
		((struct TGPD *) _pd)->gpd_b14.rx_haddr.hiaddr &= 0x0F; \
		((struct TGPD *) _pd)->gpd_b14.rx_haddr.hiaddr |=\
							((u8)_next << 4); \
	} while (0)

#define TGPD_GET_NEXT(_pd)  \
	((uintptr_t)((struct TGPD *)_pd)->pNext)
#define TGPD_GET_NEXT_TXHI(_pd)  \
	((uintptr_t)((struct TGPD *)_pd)->gpd_w1.tx_haddr.hiaddr >> 4)
#define TGPD_GET_NEXT_RXHI(_pd)  \
	((uintptr_t)((struct TGPD *)_pd)->gpd_b14.rx_haddr.hiaddr >> 4)
#define TGPD_GET_NEXT_TX(_pd)  \
	((struct TGPD *)(TGPD_GET_NEXT(_pd) |  (TGPD_GET_NEXT_TXHI(_pd) << 32)))
#define TGPD_GET_NEXT_RX(_pd)  \
	((struct TGPD *)(TGPD_GET_NEXT(_pd) |  (TGPD_GET_NEXT_RXHI(_pd) << 32)))
#define TGPD_SET_DATA(_pd, _data)  \
	(((struct TGPD *)_pd)->pBuf = (u32)_data)
#define TGPD_SET_DATA_TXHI(_pd, _next)	\
	do {	\
		((struct TGPD *)_pd)->gpd_w1.tx_haddr.hiaddr &= 0xF0; \
		((struct TGPD *)_pd)->gpd_w1.tx_haddr.hiaddr |=\
							((u8)_next & 0x0F); \
	} while (0)

#define TGPD_SET_DATA_RXHI(_pd, _next)	\
	do {	\
		((struct TGPD *)_pd)->gpd_b14.rx_haddr.hiaddr &= 0xF0; \
		((struct TGPD *)_pd)->gpd_b14.rx_haddr.hiaddr |=\
					((u8)_next & 0x0F); \
	} while (0)

#define TGPD_GET_DATA(_pd)		((uintptr_t)((struct TGPD *)_pd)->pBuf)
#define TGPD_GET_DATA_TXHI(_pd)  \
	((uintptr_t)((struct TGPD *)_pd)->gpd_w1.tx_haddr.hiaddr & 0x0F)
#define TGPD_GET_DATA_RXHI(_pd)  \
	((uintptr_t)((struct TGPD *)_pd)->gpd_b14.rx_haddr.hiaddr & 0x0F)
#define TGPD_GET_DATA_TX(_pd)  \
	((struct TGPD *)(TGPD_GET_DATA(_pd) |  (TGPD_GET_DATA_TXHI(_pd) << 32)))
#define TGPD_GET_DATA_RX(_pd)  \
	((struct TGPD *)(TGPD_GET_DATA(_pd) |  (TGPD_GET_DATA_RXHI(_pd) << 32)))
#else
#define TGPD_GET_NEXT_TX(_pd)		TGPD_GET_NEXT(_pd)
#define TGPD_GET_NEXT_RX(_pd)		TGPD_GET_NEXT(_pd)

#define TGPD_SET_NEXT(_pd, _next)  \
	(((struct TGPD *)_pd)->pNext = (u32)(uintptr_t)((struct TGPD *)_next))
#define TGPD_GET_NEXT(_pd)  \
	((struct TGPD *)(uintptr_t)((struct TGPD *)_pd)->pNext)

#define TGPD_GET_DATA_TX(_pd)		TGPD_GET_DATA(_pd)
#define TGPD_GET_DATA_RX(_pd)		TGPD_GET_DATA(_pd)

#define TGPD_SET_DATA(_pd, _data)  \
	(((struct TGPD *)_pd)->pBuf = (u32)(uintptr_t)_data)
#define TGPD_GET_DATA(_pd)  \
	((u8 *)(uintptr_t)((struct TGPD *)_pd)->pBuf)
#endif

#define TGPD_SET_BUF_LEN(_pd, _len) (((struct TGPD *)_pd)->bufLen = _len)
#define TGPD_ADD_BUF_LEN(_pd, _len) (((struct TGPD *)_pd)->bufLen += _len)
#define TGPD_GET_BUF_LEN(_pd)       (((struct TGPD *)_pd)->bufLen)

#ifdef CONFIG_MTK_MUSB_DRV_36BIT
#define TGPD_SET_EXT_LEN(_pd, _len)  \
	(((struct TGPD *)_pd)->gpd_b14.ExtLength = _len)
#define TGPD_GET_EXT_LEN(_pd)  \
	(((struct TGPD *)_pd)->gpd_b14.ExtLength)
#else
#define TGPD_SET_EXT_LEN(_pd, _len) (((struct TGPD *)_pd)->ExtLength = _len)
#define TGPD_GET_EXT_LEN(_pd)        (((struct TGPD *)_pd)->ExtLength)
#endif

#define TGPD_SET_EPaddr(_pd, _EP)  (((struct TGPD *)_pd)->ZTepFlag = \
				(((struct TGPD *)_pd)->ZTepFlag&0xF0)|(_EP))
#define TGPD_GET_EPaddr(_pd)        (((struct TGPD *)_pd)->ZTepFlag & 0x0F)

#define TGPD_FORMAT_TGL             0x10
#define TGPD_IS_FORMAT_TGL(_pd)  \
	((((struct TGPD *)_pd)->ZTepFlag & TGPD_FORMAT_TGL))
#define TGPD_SET_FORMAT_TGL(_pd)  \
	((((struct TGPD *)_pd)->ZTepFlag |= TGPD_FORMAT_TGL))
#define TGPD_CLR_FORMAT_TGL(_pd)  \
	((((struct TGPD *)_pd)->ZTepFlag &= (~TGPD_FORMAT_TGL)))
#define TGPD_FORMAT_ZLP             0x20
#define TGPD_IS_FORMAT_ZLP(_pd)  \
	((((struct TGPD *)_pd)->ZTepFlag & TGPD_FORMAT_ZLP))
#define TGPD_SET_FORMAT_ZLP(_pd)  \
	((((struct TGPD *)_pd)->ZTepFlag |= TGPD_FORMAT_ZLP))
#define TGPD_CLR_FORMAT_ZLP(_pd)  \
	((((struct TGPD *)_pd)->ZTepFlag &= (~TGPD_FORMAT_ZLP)))
#define TGPD_SET_TGL(_pd, _TGL)  \
	(((struct TGPD *)_pd)->ZTepFlag |= ((_TGL) ? 0x10 : 0x00))
#define TGPD_GET_TGL(_pd)        (((struct TGPD *)_pd)->ZTepFlag & 0x10 ? 1:0)
#define TGPD_SET_ZLP(_pd, _ZLP)  \
	(((struct TGPD *)_pd)->ZTepFlag |= ((_ZLP) ? 0x20 : 0x00))
#define TGPD_GET_ZLP(_pd)        (((struct TGPD *)_pd)->ZTepFlag & 0x20 ? 1:0)

#define TGPD_FLAG_IOC				0x80
#define TGPD_SET_IOC(_pd)	(((struct TGPD *)_pd)->flag |= TGPD_FLAG_IOC)
#define TGPD_CLR_IOC(_pd)	(((struct TGPD *)_pd)->flag &= (~TGPD_FLAG_IOC))

extern void qmu_destroy_gpd_pool(struct device *dev);
extern int qmu_init_gpd_pool(struct device *dev);
extern void qmu_reset_gpd_pool(u32 ep_num, u8 isRx);
extern bool mtk_is_qmu_enabled(u8 EP_Num, u8 isRx);
extern void mtk_qmu_enable(struct musb *musb, u8 EP_Num, u8 isRx);
extern void mtk_qmu_insert_task(u8 EP_Num, u8 isRx,
				dma_addr_t buf, u32 length, u8 zlp, u8 isioc);
extern void mtk_qmu_resume(u8 EP_Num, u8 isRx);
extern void qmu_done_rx(struct musb *musb, u8 ep_num);
extern void qmu_done_tx(struct musb *musb, u8 ep_num);
extern void mtk_disable_q(struct musb *musb, u8 ep_num, u8 isRx);
extern void mtk_qmu_irq_err(struct musb *musb, u32 qisar);
extern void flush_ep_csr(struct musb *musb, u8 ep_num, u8 isRx);
extern void mtk_qmu_stop(u8 ep_num, u8 isRx);

#define QMU_RX_SPLIT_BLOCK_SIZE (32*1024)
#define QMU_RX_SPLIT_THRE	(64*1024)
extern u32 qmu_used_gpd_count(u8 isRx, u32 num);
extern u32 qmu_free_gpd_count(u8 isRx, u32 num);
extern void h_qmu_done_rx(struct musb *musb, u8 ep_num);
extern void h_qmu_done_tx(struct musb *musb, u8 ep_num);
#endif
#endif
