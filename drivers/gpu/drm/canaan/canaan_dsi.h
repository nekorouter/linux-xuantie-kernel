/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (C) 2022, Canaan Bright Sight Co., Ltd
 *
 * All enquiries to https://www.canaan-creative.com/
 *
 */

#ifndef _CANAAN_DSI_H_
#define _CANAAN_DSI_H_

#include <drm/drm_connector.h>
#include <drm/drm_encoder.h>
#include <drm/drm_mipi_dsi.h>

struct canaan_dsi {
	struct drm_connector connector;
	struct drm_encoder encoder;
	struct mipi_dsi_host host;

	struct clk *bus_clk;
	struct clk *mod_clk;
	struct regmap *regs;
	struct regulator *regulator;
	struct reset_control *reset;
	struct phy *dphy;

	struct device *dev;
	struct mipi_dsi_device *device;
	struct drm_device *drm;
	struct drm_panel *panel;
	struct drm_bridge *bridge;

	void __iomem *base;
	u32 phy_freq;
	u32 clk_freq;
};

static inline struct canaan_dsi *host_to_canaan_dsi(struct mipi_dsi_host *host)
{
	return container_of(host, struct canaan_dsi, host);
};

static inline struct canaan_dsi *
connector_to_canaan_dsi(struct drm_connector *connector)
{
	return container_of(connector, struct canaan_dsi, connector);
};

static inline struct canaan_dsi *
encoder_to_canaan_dsi(const struct drm_encoder *encoder)
{
	return container_of(encoder, struct canaan_dsi, encoder);
};

void k230_dsi_config_4lan_phy(struct canaan_dsi *dsi, uint32_t m, uint32_t n,
			      uint8_t vco, uint8_t hsfreq);

#endif /* _CANAAN_DSI_H_ */
