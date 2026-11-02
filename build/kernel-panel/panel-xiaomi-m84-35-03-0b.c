// SPDX-License-Identifier: GPL-2.0-only
// Copyright (c) 2026 FIXME
// Generated with linux-mdss-dsi-panel-driver-generator from vendor device tree:
//   Copyright (c) 2013, The Linux Foundation. All rights reserved. (FIXME)

#include <linux/backlight.h>
#include <linux/delay.h>
#include <linux/device-id/of.h>
#include <linux/gpio/consumer.h>
#include <linux/module.h>

#include <video/mipi_display.h>

#include <drm/display/drm_dsc.h>
#include <drm/display/drm_dsc_helper.h>
#include <drm/drm_mipi_dsi.h>
#include <drm/drm_modes.h>
#include <drm/drm_panel.h>
#include <drm/drm_probe_helper.h>

struct panel_m84_35_03_0bp {
	struct drm_panel panel;
	struct mipi_dsi_device *dsi;
	struct drm_dsc_config dsc;
	struct gpio_desc *reset_gpio;
};

static inline
struct panel_m84_35_03_0bp *to_panel_m84_35_03_0bp(struct drm_panel *panel)
{
	return container_of_const(panel, struct panel_m84_35_03_0bp, panel);
}

static void panel_m84_35_03_0bp_reset(struct panel_m84_35_03_0bp *ctx)
{
	gpiod_set_value_cansleep(ctx->reset_gpio, 0);
	usleep_range(10000, 11000);
	gpiod_set_value_cansleep(ctx->reset_gpio, 1);
	usleep_range(10000, 11000);
	gpiod_set_value_cansleep(ctx->reset_gpio, 0);
	usleep_range(15000, 16000);
}

static int panel_m84_35_03_0bp_on(struct panel_m84_35_03_0bp *ctx)
{
	struct mipi_dsi_multi_context dsi_ctx = { .dsi = ctx->dsi };

	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0x00, 0x00);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xff, 0x82, 0x01, 0x01);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0x00, 0x80);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xff, 0x82, 0x01);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0x00, 0xb0);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xca, 0x01, 0x01, 0x04);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0x00, 0xb4);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xca, 0x02);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0x00, 0xa0);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xca, 0x06, 0x06, 0x06);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0x00, 0x80);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xca,
				     0xc5, 0xcf, 0xc4, 0xb8, 0xad, 0xa3, 0x9c,
				     0x96, 0x90, 0x8c, 0x87, 0x83);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0x00, 0x90);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xca,
				     0xfe, 0xff, 0x66, 0xf7, 0xff, 0xff, 0xfa,
				     0xff, 0x66);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0x00, 0xbe);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xb4, 0x20);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0x00, 0x93);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xc5, 0x61);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0x00, 0x97);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xc5, 0x61);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0x00, 0x9e);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xc5, 0x05);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0x00, 0xb6);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xc5, 0x57, 0x57);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0x00, 0x94);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xc5, 0x44);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0x00, 0x98);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xc5, 0x44);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0x00, 0x9b);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xc5, 0x45);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0x00, 0x9d);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xc5, 0x45);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0x00, 0xad);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xc5, 0x83);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0x00, 0xa4);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xf3, 0x0b);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0x00, 0x00);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xfa, 0x02);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0x00, 0xb6);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xc5, 0x57, 0x57);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0x00, 0xa4);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xf3, 0x0b);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0x00, 0x00);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xfa, 0x5a);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0x00, 0xa4);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xf3, 0x0b);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0x00, 0x00);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xfa, 0x01);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0x00, 0xb6);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xc5, 0x55, 0x55);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0x00, 0xa4);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xf3, 0x0b);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0x00, 0x00);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xfa, 0x5a);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0x00, 0x00);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xff, 0x00, 0x00, 0x00);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0x00, 0x80);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xff, 0x00, 0x00);
	mipi_dsi_dcs_set_display_brightness_multi(&dsi_ctx, 0x04e6);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, MIPI_DCS_WRITE_CONTROL_DISPLAY,
				     0x2c);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, MIPI_DCS_WRITE_POWER_SAVE, 0x00);
	mipi_dsi_usleep_range(&dsi_ctx, 10000, 11000);
	mipi_dsi_dcs_exit_sleep_mode_multi(&dsi_ctx);
	mipi_dsi_msleep(&dsi_ctx, 120);
	mipi_dsi_dcs_set_display_on_multi(&dsi_ctx);
	mipi_dsi_msleep(&dsi_ctx, 20);
	mipi_dsi_dcs_set_tear_on_multi(&dsi_ctx, MIPI_DSI_DCS_TEAR_MODE_VBLANK);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0x00, 0x00);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0x1c, 0x01);

	return dsi_ctx.accum_err;
}

static int panel_m84_35_03_0bp_off(struct panel_m84_35_03_0bp *ctx)
{
	struct mipi_dsi_multi_context dsi_ctx = { .dsi = ctx->dsi };

	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0x28, 0x00);
	mipi_dsi_msleep(&dsi_ctx, 20);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0x10, 0x00);
	mipi_dsi_msleep(&dsi_ctx, 120);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0x00, 0x00);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xf7, 0x5a, 0xa5, 0x95, 0x27);

	return dsi_ctx.accum_err;
}

static int panel_m84_35_03_0bp_prepare(struct drm_panel *panel)
{
	struct panel_m84_35_03_0bp *ctx = to_panel_m84_35_03_0bp(panel);
	struct device *dev = &ctx->dsi->dev;
	struct drm_dsc_picture_parameter_set pps;
	int ret;

	panel_m84_35_03_0bp_reset(ctx);

	ret = panel_m84_35_03_0bp_on(ctx);
	if (ret < 0) {
		dev_err(dev, "Failed to initialize panel: %d\n", ret);
		gpiod_set_value_cansleep(ctx->reset_gpio, 1);
		return ret;
	}

	drm_dsc_pps_payload_pack(&pps, &ctx->dsc);

	ret = mipi_dsi_picture_parameter_set(ctx->dsi, &pps);
	if (ret < 0) {
		dev_err(panel->dev, "failed to transmit PPS: %d\n", ret);
		return ret;
	}

	ret = mipi_dsi_compression_mode(ctx->dsi, true);
	if (ret < 0) {
		dev_err(dev, "failed to enable compression mode: %d\n", ret);
		return ret;
	}

	msleep(28); /* TODO: Is this panel-dependent? */

	return 0;
}

static int panel_m84_35_03_0bp_unprepare(struct drm_panel *panel)
{
	struct panel_m84_35_03_0bp *ctx = to_panel_m84_35_03_0bp(panel);
	struct device *dev = &ctx->dsi->dev;
	int ret;

	ret = panel_m84_35_03_0bp_off(ctx);
	if (ret < 0)
		dev_err(dev, "Failed to un-initialize panel: %d\n", ret);

	gpiod_set_value_cansleep(ctx->reset_gpio, 1);

	return 0;
}

static const struct drm_display_mode panel_m84_35_03_0bp_mode = {
	.clock = (1200 + 50 + 20 + 36) * (1920 + 410 + 6 + 10) * 90 / 1000,
	.hdisplay = 1200,
	.hsync_start = 1200 + 50,
	.hsync_end = 1200 + 50 + 20,
	.htotal = 1200 + 50 + 20 + 36,
	.vdisplay = 1920,
	.vsync_start = 1920 + 410,
	.vsync_end = 1920 + 410 + 6,
	.vtotal = 1920 + 410 + 6 + 10,
	.width_mm = 147,
	.height_mm = 236,
	.type = DRM_MODE_TYPE_DRIVER,
};

static int panel_m84_35_03_0bp_get_modes(struct drm_panel *panel,
					 struct drm_connector *connector)
{
	return drm_connector_helper_get_modes_fixed(connector, &panel_m84_35_03_0bp_mode);
}

static const struct drm_panel_funcs panel_m84_35_03_0bp_panel_funcs = {
	.prepare = panel_m84_35_03_0bp_prepare,
	.unprepare = panel_m84_35_03_0bp_unprepare,
	.get_modes = panel_m84_35_03_0bp_get_modes,
};

static int panel_m84_35_03_0bp_bl_update_status(struct backlight_device *bl)
{
	struct mipi_dsi_device *dsi = bl_get_data(bl);
	u16 brightness = backlight_get_brightness(bl);

	/* The init sequence sends DCS commands in LP mode.  Toggling
	 * MIPI_DSI_MODE_LPM off for a single command while video is
	 * running produces FIFO/timeout errors and can wedge the DSI
	 * link, so keep the panel in its normal LP mode.
	 */
	return mipi_dsi_dcs_set_display_brightness_large(dsi, brightness);
}

static const struct backlight_ops panel_m84_35_03_0bp_bl_ops = {
	.update_status = panel_m84_35_03_0bp_bl_update_status,
};


static struct backlight_device *
panel_m84_35_03_0bp_create_backlight(struct mipi_dsi_device *dsi)
{
	struct device *dev = &dsi->dev;
	const struct backlight_properties props = {
		.type = BACKLIGHT_RAW,
		.brightness = 2047,
		.max_brightness = 4095,
	};

	return devm_backlight_device_register(dev, dev_name(dev), dev, dsi,
					      &panel_m84_35_03_0bp_bl_ops, &props);
}

static int panel_m84_35_03_0bp_probe(struct mipi_dsi_device *dsi)
{
	struct device *dev = &dsi->dev;
	struct panel_m84_35_03_0bp *ctx;
	int ret;

	ctx = devm_drm_panel_alloc(dev, struct panel_m84_35_03_0bp, panel,
				   &panel_m84_35_03_0bp_panel_funcs,
				   DRM_MODE_CONNECTOR_DSI);
	if (IS_ERR(ctx))
		return PTR_ERR(ctx);

	ctx->reset_gpio = devm_gpiod_get(dev, "reset", GPIOD_OUT_HIGH);
	if (IS_ERR(ctx->reset_gpio))
		return dev_err_probe(dev, PTR_ERR(ctx->reset_gpio),
				     "Failed to get reset-gpios\n");

	ctx->dsi = dsi;
	mipi_dsi_set_drvdata(dsi, ctx);

	dsi->lanes = 4;
	dsi->format = MIPI_DSI_FMT_RGB888;
	dsi->mode_flags = MIPI_DSI_MODE_VIDEO | MIPI_DSI_CLOCK_NON_CONTINUOUS |
			  MIPI_DSI_MODE_LPM;

	ctx->panel.prepare_prev_first = true;

	ctx->panel.backlight = panel_m84_35_03_0bp_create_backlight(dsi);
	if (IS_ERR(ctx->panel.backlight))
		return dev_err_probe(dev, PTR_ERR(ctx->panel.backlight),
				     "Failed to create backlight\n");

	ret = devm_drm_panel_add(dev, &ctx->panel);
	if (ret)
		return ret;

	/* This panel only supports DSC; unconditionally enable it */
	dsi->dsc = &ctx->dsc;

	ctx->dsc.dsc_version_major = 1;
	ctx->dsc.dsc_version_minor = 1;

	/* TODO: Pass slice_per_pkt = 2 */
	ctx->dsc.slice_height = 40;
	ctx->dsc.slice_width = 600;
	/*
	 * TODO: hdisplay should be read from the selected mode once
	 * it is passed back to drm_panel (in prepare?)
	 */
	WARN_ON(1200 % ctx->dsc.slice_width);
	ctx->dsc.slice_count = 1200 / ctx->dsc.slice_width;
	ctx->dsc.bits_per_component = 8;
	ctx->dsc.bits_per_pixel = 8 << 4; /* 4 fractional bits */
	ctx->dsc.block_pred_enable = true;

	return devm_mipi_dsi_attach(dev, dsi);
}

static const struct of_device_id panel_m84_35_03_0bp_of_match[] = {
	{ .compatible = "xiaomi,m84-35-03-0b" }, // FIXME
	{ /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, panel_m84_35_03_0bp_of_match);

static struct mipi_dsi_driver panel_m84_35_03_0bp_driver = {
	.probe = panel_m84_35_03_0bp_probe,
	.driver = {
		.name = "panel-panel-m84-35-03-0bp",
		.of_match_table = panel_m84_35_03_0bp_of_match,
	},
};
module_mipi_dsi_driver(panel_m84_35_03_0bp_driver);

MODULE_AUTHOR("linux-mdss-dsi-panel-driver-generator <fix@me>"); // FIXME
MODULE_DESCRIPTION("DRM driver for m84 35 03 0b video mode fhdp dsi panel");
MODULE_LICENSE("GPL");
