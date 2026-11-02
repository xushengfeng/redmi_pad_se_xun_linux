// SPDX-License-Identifier: GPL-2.0-only
// Copyright (c) 2026 FIXME
// Generated with linux-mdss-dsi-panel-driver-generator from vendor device tree:
//   Copyright (c) 2013, The Linux Foundation. All rights reserved. (FIXME)

#include <linux/backlight.h>
#include <linux/workqueue.h>
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

struct panel_m84_36_02_0ap {
	struct drm_panel panel;
	struct mipi_dsi_device *dsi;
	struct drm_dsc_config dsc;
	struct gpio_desc *reset_gpio;
	struct delayed_work bl_work;
	u16 bl_value;
};

static inline
struct panel_m84_36_02_0ap *to_panel_m84_36_02_0ap(struct drm_panel *panel)
{
	return container_of_const(panel, struct panel_m84_36_02_0ap, panel);
}

static void panel_m84_36_02_0ap_reset(struct panel_m84_36_02_0ap *ctx)
{
	gpiod_set_value_cansleep(ctx->reset_gpio, 0);
	usleep_range(10000, 11000);
	gpiod_set_value_cansleep(ctx->reset_gpio, 1);
	usleep_range(10000, 11000);
	gpiod_set_value_cansleep(ctx->reset_gpio, 0);
	usleep_range(10000, 11000);
}

static int panel_m84_36_02_0ap_on(struct panel_m84_36_02_0ap *ctx)
{
	struct mipi_dsi_multi_context dsi_ctx = { .dsi = ctx->dsi };

	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xff, 0x23);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xfb, 0x01);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0x00, 0x60);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0x07, 0x20);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0x08, 0x01);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0x09, 0x80);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xff, 0x23);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xfb, 0x01);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0x03, 0x00);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0x0a, 0x1d);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0x0b, 0x1d);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0x0c, 0xa8);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0x0d, 0x05);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0x10, 0x00);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0x11, 0x01);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0x12, 0x68);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0x15, 0xea);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0x16, 0x15);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, MIPI_DCS_SET_PARTIAL_ROWS, 0xff);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, MIPI_DCS_SET_PARTIAL_COLUMNS,
				     0xfe);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0x32, 0xfd);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0x33, 0xfc);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0x34, 0xfb);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0x35, 0xfa);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, MIPI_DCS_SET_ADDRESS_MODE, 0xf8);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0x37, 0xf6);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0x38, 0xf3);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0x39, 0xf2);
	mipi_dsi_dcs_set_pixel_format_multi(&dsi_ctx, 0xf0);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0x3b, 0xee);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, MIPI_DCS_SET_3D_CONTROL, 0xec);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0x3f, 0xea);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, MIPI_DCS_SET_VSYNC_TIMING, 0xe8);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0x41, 0xe6);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, MIPI_DCS_GET_SCANLINE, 0xff);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0x46, 0xfc);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0x47, 0xf8);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0x48, 0xf3);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0x49, 0xed);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0x4a, 0xe7);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0x4b, 0xe1);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0x4c, 0xda);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0x4d, 0xd2);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0x4e, 0xc9);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0x4f, 0xbf);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0x50, 0xb5);
	mipi_dsi_dcs_set_display_brightness_multi(&dsi_ctx, 0x00ab);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0x52, 0xa1);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, MIPI_DCS_WRITE_CONTROL_DISPLAY,
				     0x93);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0x54, 0x83);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0x58, 0xff);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0x59, 0xfd);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0x5a, 0xfa);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0x5b, 0xf8);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0x5c, 0xf5);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0x5d, 0xf0);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, MIPI_DCS_SET_CABC_MIN_BRIGHTNESS,
				     0xea);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0x5f, 0xe2);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0x60, 0xdc);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0x61, 0xd7);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0x62, 0xd2);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0x63, 0xcd);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0x64, 0xc8);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0x65, 0xc2);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0x66, 0xb0);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0x67, 0xae);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0x19, 0x00);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0x1a, 0x04);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0x1b, 0x08);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0x1c, 0x0c);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0x1d, 0x10);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0x1e, 0x14);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0x1f, 0x18);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0x20, 0x1c);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0x21, 0x20);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0x22, 0x24);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0x23, 0x28);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0x24, 0x2c);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0x25, 0x30);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, MIPI_DCS_SET_GAMMA_CURVE, 0x34);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0x27, 0x38);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0x28, 0x3c);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0x29, 0x10);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0x2a, 0x20);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0x2b, 0x20);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xff, 0x27);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xfb, 0x01);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0x13, 0x00);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0x14, 0x22);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xd0, 0x31);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xd1, 0x28);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xd2, 0x38);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xd4, 0x08);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xde, 0x43);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xdf, 0x02);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xff, 0x20);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xfb, 0x01);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0x01, 0x22);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0x02, 0x22);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xff, 0x10);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xfb, 0x01);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0x3b,
				     0x03, 0x14, 0x36, 0x0a, 0x0a, 0x00);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0x90, 0x13);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0x91,
				     0x89, 0xa8, 0x00, 0x08, 0xd2, 0x00, 0x00,
				     0x00, 0x00, 0xf0, 0x00, 0x10, 0x0e, 0xdc,
				     0x05, 0xb9);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0x92, 0x10, 0xf0);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0x68, 0x02, 0x01);
	mipi_dsi_dcs_set_display_brightness_multi(&dsi_ctx, 0x3207);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, MIPI_DCS_WRITE_CONTROL_DISPLAY,
				     0x2c);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0x11, 0x00);
	mipi_dsi_msleep(&dsi_ctx, 100);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0x29, 0x00);
	mipi_dsi_msleep(&dsi_ctx, 20);

	return dsi_ctx.accum_err;
}

static int panel_m84_36_02_0ap_off(struct panel_m84_36_02_0ap *ctx)
{
	struct mipi_dsi_multi_context dsi_ctx = { .dsi = ctx->dsi };

	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xff, 0x23);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xfb, 0x01);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0x06, 0x00);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xff, 0x10);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xfb, 0x01);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0x28, 0x00);
	mipi_dsi_msleep(&dsi_ctx, 50);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0x10, 0x00);
	mipi_dsi_msleep(&dsi_ctx, 150);

	return dsi_ctx.accum_err;
}

static int panel_m84_36_02_0ap_prepare(struct drm_panel *panel)
{
	struct panel_m84_36_02_0ap *ctx = to_panel_m84_36_02_0ap(panel);

	struct device *dev = &ctx->dsi->dev;
	struct drm_dsc_picture_parameter_set pps;
	int ret;

	panel_m84_36_02_0ap_reset(ctx);

	ret = panel_m84_36_02_0ap_on(ctx);
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

static int panel_m84_36_02_0ap_unprepare(struct drm_panel *panel)
{
	struct panel_m84_36_02_0ap *ctx = to_panel_m84_36_02_0ap(panel);

	cancel_delayed_work_sync(&ctx->bl_work);
	struct device *dev = &ctx->dsi->dev;
	int ret;

	ret = panel_m84_36_02_0ap_off(ctx);
	if (ret < 0)
		dev_err(dev, "Failed to un-initialize panel: %d\n", ret);

	gpiod_set_value_cansleep(ctx->reset_gpio, 1);

	return 0;
}

static const struct drm_display_mode panel_m84_36_02_0ap_mode = {
	.clock = (1200 + 90 + 4 + 92) * (1920 + 54 + 4 + 16) * 90 / 1000,
	.hdisplay = 1200,
	.hsync_start = 1200 + 90,
	.hsync_end = 1200 + 90 + 4,
	.htotal = 1200 + 90 + 4 + 92,
	.vdisplay = 1920,
	.vsync_start = 1920 + 54,
	.vsync_end = 1920 + 54 + 4,
	.vtotal = 1920 + 54 + 4 + 16,
	.width_mm = 147,
	.height_mm = 236,
	.type = DRM_MODE_TYPE_DRIVER,
};

static int panel_m84_36_02_0ap_get_modes(struct drm_panel *panel,
					 struct drm_connector *connector)
{
	return drm_connector_helper_get_modes_fixed(connector, &panel_m84_36_02_0ap_mode);
}

static const struct drm_panel_funcs panel_m84_36_02_0ap_panel_funcs = {
	.prepare = panel_m84_36_02_0ap_prepare,
	.unprepare = panel_m84_36_02_0ap_unprepare,
	.get_modes = panel_m84_36_02_0ap_get_modes,
};

static void panel_m84_36_02_0ap_bl_work(struct work_struct *work)
{
	struct panel_m84_36_02_0ap *ctx = container_of(work, struct panel_m84_36_02_0ap,
						    bl_work.work);

	mipi_dsi_dcs_set_display_brightness_large(ctx->dsi, ctx->bl_value);
}

static int panel_m84_36_02_0ap_bl_update_status(struct backlight_device *bl)
{
	struct mipi_dsi_device *dsi = bl_get_data(bl);
	struct panel_m84_36_02_0ap *ctx = mipi_dsi_get_drvdata(dsi);
	u16 brightness = backlight_get_brightness(bl);
	u16 level = brightness << 1;

	/* Coalesce the stream of values a brightness slider produces.  Each
	 * DCS write briefly disturbs the video stream, so send only the value
	 * the slider settles on instead of one command per step.
	 */
	ctx->bl_value = ((level >> 4) << 8) | (level & 0xf);
	schedule_delayed_work(&ctx->bl_work, msecs_to_jiffies(100));
	return 0;
}

static const struct backlight_ops panel_m84_36_02_0ap_bl_ops = {
	.update_status = panel_m84_36_02_0ap_bl_update_status,
};


static struct backlight_device *
panel_m84_36_02_0ap_create_backlight(struct mipi_dsi_device *dsi)
{
	struct device *dev = &dsi->dev;
	const struct backlight_properties props = {
		.type = BACKLIGHT_RAW,
		.brightness = 1024,
		.max_brightness = 2047,
	};

	return devm_backlight_device_register(dev, dev_name(dev), dev, dsi,
					      &panel_m84_36_02_0ap_bl_ops, &props);
}

static int panel_m84_36_02_0ap_probe(struct mipi_dsi_device *dsi)
{
	struct device *dev = &dsi->dev;
	struct panel_m84_36_02_0ap *ctx;
	int ret;

	ctx = devm_drm_panel_alloc(dev, struct panel_m84_36_02_0ap, panel,
				   &panel_m84_36_02_0ap_panel_funcs,
				   DRM_MODE_CONNECTOR_DSI);
	if (IS_ERR(ctx))
		return PTR_ERR(ctx);

	ctx->reset_gpio = devm_gpiod_get(dev, "reset", GPIOD_OUT_HIGH);
	if (IS_ERR(ctx->reset_gpio))
		return dev_err_probe(dev, PTR_ERR(ctx->reset_gpio),
				     "Failed to get reset-gpios\n");

	ctx->dsi = dsi;
	mipi_dsi_set_drvdata(dsi, ctx);
	INIT_DELAYED_WORK(&ctx->bl_work, panel_m84_36_02_0ap_bl_work);

	dsi->lanes = 4;
	dsi->format = MIPI_DSI_FMT_RGB888;
	dsi->mode_flags = MIPI_DSI_MODE_VIDEO | MIPI_DSI_CLOCK_NON_CONTINUOUS |
			  MIPI_DSI_MODE_LPM;

	ctx->panel.prepare_prev_first = true;

	ctx->panel.backlight = panel_m84_36_02_0ap_create_backlight(dsi);
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
	ctx->dsc.slice_height = 8;
	ctx->dsc.slice_width = 1200;
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

static const struct of_device_id panel_m84_36_02_0ap_of_match[] = {
	{ .compatible = "xiaomi,m84-36-02-0a" }, // FIXME
	{ /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, panel_m84_36_02_0ap_of_match);

static struct mipi_dsi_driver panel_m84_36_02_0ap_driver = {
	.probe = panel_m84_36_02_0ap_probe,
	.driver = {
		.name = "panel-panel-m84-36-02-0ap",
		.of_match_table = panel_m84_36_02_0ap_of_match,
	},
};
module_mipi_dsi_driver(panel_m84_36_02_0ap_driver);

MODULE_AUTHOR("linux-mdss-dsi-panel-driver-generator <fix@me>"); // FIXME
MODULE_DESCRIPTION("DRM driver for m84 36 02 0a video mode fhdp dsi panel");
MODULE_LICENSE("GPL");
