// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Kendryte K230 OTP Support driver
 *
 * Copyright (C) 2024, Canaan Bright Sight Co., Ltd
 */

#include <linux/clk.h>
#include <linux/device.h>
#include <linux/io.h>
#include <linux/module.h>
#include <linux/nvmem-provider.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/delay.h>

#define OTP_BYPASS_ADDR_P 0x91102040
#define OTP_REG_OFFSET 0x0
#define OTP_USER_START_OFFSET 0x0

enum OTP_STATUS_E {
	OTP_BYPASS_STATUS = 0,
	OTP_MAX_STATUS = 1,
};

struct otp_priv {
	struct device *dev;
	void __iomem *base;
	struct nvmem_config *config;
};

static uint32_t be2le(uint32_t var)
{
	return (((0xff000000 & var) >> 24) | ((0x00ff0000 & var) >> 8) |
		((0x0000ff00 & var) << 8) | ((0x000000ff & var) << 24));
}

/**
 * @brief After the OTP is powered on, the driver can read the sysctl register SOC_BOOT_CTL
 *		to determine whether the OTP has been bypassed.
 *
 * @return true
 * @return false
 */
static bool sysctl_boot_get_otp_bypass(void)
{
	void __iomem *OTP_BYPASS_ADDR_V;

	OTP_BYPASS_ADDR_V = ioremap(OTP_BYPASS_ADDR_P, 4);
	if (readl(OTP_BYPASS_ADDR_V) & 0x10)
		return true;
	else
		return false;
}

static bool otp_get_status(enum OTP_STATUS_E eStatus)
{
	if (eStatus == OTP_BYPASS_STATUS)
		return sysctl_boot_get_otp_bypass();
	else
		return false;
}

/**
 * @brief OTP read operation, can only read specific areas,
 * including production information, reserved user space
 *
 * @param context
 * @param offset
 * @param val
 * @param bytes
 * @return int
 */
static int k230_otp_read(void *context, unsigned int offset, void *val,
			 size_t bytes)
{
	struct otp_priv *priv = context;
	uint32_t *outbuf = (uint32_t *)val;
	uint32_t *out32;
	uint32_t WORD_SIZE = priv->config->word_size;
	uint32_t wlen = bytes / WORD_SIZE;
	uint32_t word;
	uint32_t i = 0;

	if (true == otp_get_status(OTP_BYPASS_STATUS))
		return -1;

	if (wlen > 0) {
		memcpy(outbuf, (void *)(priv->base + OTP_REG_OFFSET + offset),
		       wlen * WORD_SIZE);

		out32 = (uint32_t *)outbuf;
		for (i = 0; i < wlen; ++i)
			*(out32 + i) = be2le(*(out32 + i));
	}

	if (bytes % WORD_SIZE != 0) {
		outbuf += wlen * WORD_SIZE;
		word = readl(priv->base + OTP_REG_OFFSET + offset +
			     wlen * WORD_SIZE);
		word = be2le(word);
		memcpy(outbuf, &word, bytes % WORD_SIZE);
	}

	return 0;
}

static struct nvmem_config kendryte_otp_nvmem_config = {
	.name = "kendryte_otp",
	.owner = THIS_MODULE,
	.read_only = true,
	.word_size = 4,
	.reg_read = k230_otp_read,
	// .reg_write = k230_otp_write,
	.size = 0x300,
};

static const struct of_device_id kendryte_otp_dt_ids[] = {
	{ .compatible = "canaan,k230-otp" },
	{},
};
MODULE_DEVICE_TABLE(of, kendryte_otp_dt_ids);

static int kendryte_otp_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct resource *res;
	struct otp_priv *priv;
	struct nvmem_device *nvmem;

	priv = devm_kzalloc(dev, sizeof(*priv), GFP_KERNEL);
	if (!priv)
		return -ENOMEM;

	priv->dev = dev;

	/* Get OTP base address register from DTS. */
	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	priv->base = devm_ioremap_resource(dev, res);
	if (IS_ERR(priv->base))
		return PTR_ERR(priv->base);

	kendryte_otp_nvmem_config.dev = dev;
	kendryte_otp_nvmem_config.priv = priv;
	priv->config = &kendryte_otp_nvmem_config;
	nvmem = devm_nvmem_register(dev, &kendryte_otp_nvmem_config);

	return PTR_ERR_OR_ZERO(nvmem);
}

static struct platform_driver kendryte_otp_driver = {
	.probe = kendryte_otp_probe,
	.driver = {
		.name = "kendryte_otp",
		.of_match_table = kendryte_otp_dt_ids,
	},
};
module_platform_driver(kendryte_otp_driver);

MODULE_DESCRIPTION("kendryte k230 otp driver");
MODULE_LICENSE("GPL");
