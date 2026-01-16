// SPDX-License-Identifier: GPL-2.0
/*
 * Novatek NT36672C DSI panel driver for Xiaomi 36 02 0a video mode DSC DSI panel
 * Based on panel-novatek-nt36672a.c and panel-novatek-nt36672e.c from mainline Linux
 */

#include <linux/backlight.h>
#include <linux/delay.h>
#include <linux/gpio/consumer.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/regulator/consumer.h>

#include <video/mipi_display.h>

#include <drm/drm_mipi_dsi.h>
#include <drm/drm_modes.h>
#include <drm/drm_panel.h>

struct nt36672c {
	struct drm_panel panel;
	struct mipi_dsi_device *dsi;
	struct gpio_desc *reset_gpio;
	struct regulator_bulk_data supplies[2]; /* vddio, vdd - adjust based on your hardware */
	bool prepared;
};

static inline struct nt36672c *to_nt36672c(struct drm_panel *panel)
{
	return container_of(panel, struct nt36672c, panel);
}


#define dcs_switch_page(ctx, page) \
	mipi_dsi_dcs_write_seq_multi(ctx,0xff , (page))

static void nt36672c_reload_cmds(struct mipi_dsi_multi_context *ctx)
{
	mipi_dsi_dcs_write_seq_multi(ctx, 0xFB, 0x01);
}

static void nt36672c_init_sequence(struct mipi_dsi_multi_context *ctx)
{
	// Page 0x24
	dcs_switch_page(ctx, 0x24);
	nt36672c_reload_cmds(ctx);
	mipi_dsi_dcs_write_seq_multi(ctx, 0x4D, 0x02);
	mipi_dsi_dcs_write_seq_multi(ctx, 0x4E, 0x30);
	mipi_dsi_dcs_write_seq_multi(ctx, 0x4F, 0x30);
	mipi_dsi_dcs_write_seq_multi(ctx, 0x53, 0x30);
	mipi_dsi_dcs_write_seq_multi(ctx, 0x7A, 0x01);
	mipi_dsi_dcs_write_seq_multi(ctx, 0x7B, 0x8C);
	mipi_dsi_dcs_write_seq_multi(ctx, 0x7D, 0x05);
	mipi_dsi_dcs_write_seq_multi(ctx, 0x80, 0x05);
	mipi_dsi_dcs_write_seq_multi(ctx, 0x81, 0x05);
	mipi_dsi_dcs_write_seq_multi(ctx, 0xA0, 0x0C);
	mipi_dsi_dcs_write_seq_multi(ctx, 0xA2, 0x0C);
	mipi_dsi_dcs_write_seq_multi(ctx, 0xA3, 0x01);
	mipi_dsi_dcs_write_seq_multi(ctx, 0xA4, 0x05);
	mipi_dsi_dcs_write_seq_multi(ctx, 0xA5, 0x05);
	mipi_dsi_dcs_write_seq_multi(ctx, 0xC4, 0x80);
	mipi_dsi_dcs_write_seq_multi(ctx, 0xC6, 0xC0);
	mipi_dsi_msleep(ctx, 1);  // From 15 01 ... e9 02
	mipi_dsi_dcs_write_seq_multi(ctx, 0xE9, 0x02);

	// Page 0x25
	dcs_switch_page(ctx, 0x25);
	nt36672c_reload_cmds(ctx);
	mipi_dsi_dcs_write_seq_multi(ctx, 0xDA, 0x00);
	mipi_dsi_dcs_write_seq_multi(ctx, 0xE0, 0x00);
	mipi_dsi_msleep(ctx, 1);
	mipi_dsi_dcs_write_seq_multi(ctx, 0xF1, 0x04);

	// Page 0x2B
	dcs_switch_page(ctx, 0x2B);
	nt36672c_reload_cmds(ctx);
	mipi_dsi_dcs_write_seq_multi(ctx, 0xB7, 0x08);
	mipi_dsi_dcs_write_seq_multi(ctx, 0xB8, 0x1A);
	mipi_dsi_msleep(ctx, 1);
	mipi_dsi_dcs_write_seq_multi(ctx, 0xC0, 0x04);

	// Page 0xF0
	dcs_switch_page(ctx, 0xF0);
	nt36672c_reload_cmds(ctx);
	mipi_dsi_dcs_write_seq_multi(ctx, 0x1C, 0x01);
	mipi_dsi_msleep(ctx, 1);
	mipi_dsi_dcs_write_seq_multi(ctx, 0x33, 0x01);

	// Page 0x23
	dcs_switch_page(ctx, 0x23);
	nt36672c_reload_cmds(ctx);
	mipi_dsi_dcs_write_seq_multi(ctx, 0x00, 0x80);
	mipi_dsi_dcs_write_seq_multi(ctx, 0x01, 0x84);
	mipi_dsi_dcs_write_seq_multi(ctx, 0x05, 0x2D);
	mipi_dsi_dcs_write_seq_multi(ctx, 0x06, 0x00);
	mipi_dsi_dcs_write_seq_multi(ctx, 0x07, 0x00);
	mipi_dsi_dcs_write_seq_multi(ctx, 0x08, 0x01);
	mipi_dsi_dcs_write_seq_multi(ctx, 0x09, 0x45);
	mipi_dsi_dcs_write_seq_multi(ctx, 0x11, 0x01);
	mipi_dsi_dcs_write_seq_multi(ctx, 0x12, 0x95);
	mipi_dsi_dcs_write_seq_multi(ctx, 0x15, 0x68);
	mipi_dsi_dcs_write_seq_multi(ctx, 0x16, 0x0B);
	mipi_dsi_dcs_write_seq_multi(ctx, 0x29, 0x0A);
	mipi_dsi_dcs_write_seq_multi(ctx, 0x30, 0xFF);
	mipi_dsi_dcs_write_seq_multi(ctx, 0x31, 0xFE);
	mipi_dsi_dcs_write_seq_multi(ctx, 0x32, 0xFD);
	mipi_dsi_dcs_write_seq_multi(ctx, 0x33, 0xFB);
	mipi_dsi_dcs_write_seq_multi(ctx, 0x34, 0xF8);
	mipi_dsi_dcs_write_seq_multi(ctx, 0x35, 0xF5);
	mipi_dsi_dcs_write_seq_multi(ctx, 0x36, 0xF3);
	mipi_dsi_dcs_write_seq_multi(ctx, 0x37, 0xF2);
	mipi_dsi_dcs_write_seq_multi(ctx, 0x38, 0xF2);
	mipi_dsi_dcs_write_seq_multi(ctx, 0x39, 0xF2);
	mipi_dsi_dcs_write_seq_multi(ctx, 0x3A, 0xEF);
	mipi_dsi_dcs_write_seq_multi(ctx, 0x3B, 0xEC);
	mipi_dsi_dcs_write_seq_multi(ctx, 0x3D, 0xE9);
	mipi_dsi_dcs_write_seq_multi(ctx, 0x3F, 0xE5);
	mipi_dsi_dcs_write_seq_multi(ctx, 0x40, 0xE5);
	mipi_dsi_dcs_write_seq_multi(ctx, 0x41, 0xE5);
	mipi_dsi_dcs_write_seq_multi(ctx, 0x2A, 0x13);
	mipi_dsi_dcs_write_seq_multi(ctx, 0x45, 0xFF);
	mipi_dsi_dcs_write_seq_multi(ctx, 0x46, 0xF4);
	mipi_dsi_dcs_write_seq_multi(ctx, 0x47, 0xE7);
	mipi_dsi_dcs_write_seq_multi(ctx, 0x48, 0xDA);
	mipi_dsi_dcs_write_seq_multi(ctx, 0x49, 0xCD);
	mipi_dsi_dcs_write_seq_multi(ctx, 0x4A, 0xC0);
	mipi_dsi_dcs_write_seq_multi(ctx, 0x4B, 0xB3);
	mipi_dsi_dcs_write_seq_multi(ctx, 0x4C, 0xB2);
	mipi_dsi_dcs_write_seq_multi(ctx, 0x4D, 0xB2);
	mipi_dsi_dcs_write_seq_multi(ctx, 0x4E, 0xB2);
	mipi_dsi_dcs_write_seq_multi(ctx, 0x4F, 0x99);
	mipi_dsi_dcs_write_seq_multi(ctx, 0x50, 0x80);
	mipi_dsi_dcs_write_seq_multi(ctx, 0x51, 0x68);
	mipi_dsi_dcs_write_seq_multi(ctx, 0x52, 0x66);
	mipi_dsi_dcs_write_seq_multi(ctx, 0x53, 0x66);
	mipi_dsi_dcs_write_seq_multi(ctx, 0x54, 0x66);
	mipi_dsi_dcs_write_seq_multi(ctx, 0x2B, 0x0E);
	mipi_dsi_dcs_write_seq_multi(ctx, 0x58, 0xFF);
	mipi_dsi_dcs_write_seq_multi(ctx, 0x59, 0xFB);
	mipi_dsi_dcs_write_seq_multi(ctx, 0x5A, 0xF7);
	mipi_dsi_dcs_write_seq_multi(ctx, 0x5B, 0xF3);
	mipi_dsi_dcs_write_seq_multi(ctx, 0x5C, 0xEF);
	mipi_dsi_dcs_write_seq_multi(ctx, 0x5D, 0xE3);
	mipi_dsi_dcs_write_seq_multi(ctx, 0x5E, 0xDA);
	mipi_dsi_dcs_write_seq_multi(ctx, 0x5F, 0xD8);
	mipi_dsi_dcs_write_seq_multi(ctx, 0x60, 0xD8);
	mipi_dsi_dcs_write_seq_multi(ctx, 0x61, 0xD8);
	mipi_dsi_dcs_write_seq_multi(ctx, 0x62, 0xCB);
	mipi_dsi_dcs_write_seq_multi(ctx, 0x63, 0xBF);
	mipi_dsi_dcs_write_seq_multi(ctx, 0x64, 0xB3);
	mipi_dsi_dcs_write_seq_multi(ctx, 0x65, 0xB2);
	mipi_dsi_dcs_write_seq_multi(ctx, 0x66, 0xB2);
	mipi_dsi_msleep(ctx, 1);
	mipi_dsi_dcs_write_seq_multi(ctx, 0x67, 0xB2);

	// Page 0x27
	dcs_switch_page(ctx, 0x27);
	nt36672c_reload_cmds(ctx);
	mipi_dsi_msleep(ctx, 1);
	mipi_dsi_dcs_write_seq_multi(ctx, 0x40, 0x20);

	// Page 0x10
	dcs_switch_page(ctx, 0x10);
	nt36672c_reload_cmds(ctx);
	mipi_dsi_dcs_write_seq_multi(ctx, 0xC0, 0x03);  // Long write as short, adjust if needed
	mipi_dsi_dcs_write_seq_multi(ctx, 0x51, 0x0C, 0x27);  // Long write
	mipi_dsi_msleep(ctx, 1);
	mipi_dsi_dcs_write_seq_multi(ctx, 0x53, 0x24);

	// Page 0x10 again
	dcs_switch_page(ctx, 0x10);

	// Sleep Out with delay
	mipi_dsi_msleep(ctx, 70);  // From 46 00 for 11
	mipi_dsi_dcs_exit_sleep_mode_multi(ctx);

	// Display On with delay
	mipi_dsi_msleep(ctx, 1);
	mipi_dsi_dcs_set_display_on_multi(ctx);

	// Final page 0x27
	dcs_switch_page(ctx, 0x27);
	nt36672c_reload_cmds(ctx);
	mipi_dsi_dcs_write_seq_multi(ctx, 0x3F, 0x01);
	mipi_dsi_dcs_write_seq_multi(ctx, 0x43, 0x08);
	mipi_dsi_msleep(ctx, 1);
	mipi_dsi_dcs_write_seq_multi(ctx, 0x40, 0x25);

	// Final page 0x10
	dcs_switch_page(ctx, 0x10);
}

static const struct drm_display_mode nt36672c_mode = {
	.clock = 157500,  // Calculated from resolution, porches, and framerate (adjust if needed)
	.hdisplay = 1080,
	.hsync_start = 1080 + 40,  // h-back-porch = 0x28 = 40
	.hsync_end = 1080 + 40 + 12,  // h-pulse-width = 0x0c = 12
	.htotal = 1080 + 40 + 12 + 50,  // h-front-porch = 0x32 = 50
	.vdisplay = 2400,  // Approximate from physical dimensions and typical FHD+ aspect
	.vsync_start = 2400 + 30,  // v-back-porch = 0x1e = 30
	.vsync_end = 2400 + 30 + 2,  // v-pulse-width = 0x02 = 2
	.vtotal = 2400 + 30 + 2 + 33,  // v-front-porch = 0x21 = 33
	.flags = DRM_MODE_FLAG_PHSYNC | DRM_MODE_FLAG_PVSYNC,
};

static int nt36672c_get_modes(struct drm_panel *panel,
			      struct drm_connector *connector)
{
	struct drm_display_mode *mode;

	mode = drm_mode_duplicate(connector->dev, &nt36672c_mode);
	if (!mode)
		return -ENOMEM;

	drm_mode_set_name(mode);
	mode->type = DRM_MODE_TYPE_DRIVER | DRM_MODE_TYPE_PREFERRED;
	connector->display_info.width_mm = 69;  // From DTS pan-physical-width-dimension / 100
	connector->display_info.height_mm = 154;  // From height
	drm_mode_probed_add(connector, mode);

	return 1;
}

static int nt36672c_prepare(struct drm_panel *panel)
{
	struct nt36672c *ctx = to_nt36672c(panel);
	struct mipi_dsi_device *dsi = ctx->dsi;
	struct mipi_dsi_multi_context dsi_ctx = { .dsi = dsi };
	int ret;

	if (ctx->prepared)
		return 0;

	ret = regulator_bulk_enable(ARRAY_SIZE(ctx->supplies), ctx->supplies);
	if (ret < 0)
		return ret;

	gpiod_set_value_cansleep(ctx->reset_gpio, 1);
	usleep_range(10000, 11000);
	gpiod_set_value_cansleep(ctx->reset_gpio, 0);
	usleep_range(10000, 11000);

	// Enable DSC compression mode (based on DTS config)
	ret = mipi_dsi_compression_mode(dsi, true);
	if (ret < 0)
		goto fail;

	nt36672c_init_sequence(&dsi_ctx);

	if (dsi_ctx.accum_err) {
		ret = dsi_ctx.accum_err;
		goto fail;
	}

	ctx->prepared = true;
	return 0;

fail:
	regulator_bulk_disable(ARRAY_SIZE(ctx->supplies), ctx->supplies);
	return ret;
}

static int nt36672c_unprepare(struct drm_panel *panel)
{
	struct nt36672c *ctx = to_nt36672c(panel);
	struct mipi_dsi_device *dsi = ctx->dsi;

	if (!ctx->prepared)
		return 0;

	mipi_dsi_dcs_set_display_off(dsi);
	mipi_dsi_dcs_enter_sleep_mode(dsi);
	msleep(120);

	// Disable DSC if needed (usually not, as host handles)
	mipi_dsi_compression_mode(dsi, false);

	gpiod_set_value_cansleep(ctx->reset_gpio, 1);
	regulator_bulk_disable(ARRAY_SIZE(ctx->supplies), ctx->supplies);

	ctx->prepared = false;
	return 0;
}

static const struct drm_panel_funcs nt36672c_funcs = {
	.prepare = nt36672c_prepare,
	.unprepare = nt36672c_unprepare,
	.get_modes = nt36672c_get_modes,
};

static int nt36672c_probe(struct mipi_dsi_device *dsi)
{
	struct device *dev = &dsi->dev;
	struct nt36672c *ctx;
	int ret;

	ctx = devm_kzalloc(dev, sizeof(*ctx), GFP_KERNEL);
	if (!ctx)
		return -ENOMEM;

	ctx->reset_gpio = devm_gpiod_get(dev, "reset", GPIOD_OUT_HIGH);
	if (IS_ERR(ctx->reset_gpio))
		return dev_err_probe(dev, PTR_ERR(ctx->reset_gpio), "Failed to get reset-gpios\n");

	ctx->supplies[0].supply = "vddio";
	ctx->supplies[1].supply = "vdd";
	ret = devm_regulator_bulk_get(dev, ARRAY_SIZE(ctx->supplies), ctx->supplies);
	if (ret < 0)
		return dev_err_probe(dev, ret, "Failed to get regulators\n");

	ctx->dsi = dsi;
	mipi_dsi_set_drvdata(dsi, ctx);

	dsi->lanes = 4;
	dsi->format = MIPI_DSI_FMT_RGB888;
	dsi->mode_flags = MIPI_DSI_MODE_VIDEO | MIPI_DSI_MODE_VIDEO_BURST |
			  MIPI_DSI_MODE_LPM | MIPI_DSI_MODE_NO_EOT_PACKET |
			  MIPI_DSI_CLOCK_NON_CONTINUOUS;

	drm_panel_init(&ctx->panel, dev, &nt36672c_funcs, DRM_MODE_CONNECTOR_DSI);

	ret = drm_panel_of_backlight(&ctx->panel);
	if (ret)
		return dev_err_probe(dev, ret, "Failed to get backlight handle\n");

	drm_panel_add(&ctx->panel);

	ret = mipi_dsi_attach(dsi);
	if (ret < 0) {
		drm_panel_remove(&ctx->panel);
		return dev_err_probe(dev, ret, "Failed to attach to DSI host\n");
	}

	return 0;
}

static void nt36672c_remove(struct mipi_dsi_device *dsi)
{
	struct nt36672c *ctx = mipi_dsi_get_drvdata(dsi);

	mipi_dsi_detach(dsi);
	drm_panel_remove(&ctx->panel);
}

static const struct of_device_id nt36672c_of_match[] = {
	{ .compatible = "qcom,mdss-dsi-j17-36-02-0a-dsc-video" },  // From your DTS
	{ /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, nt36672c_of_match);

static struct mipi_dsi_driver nt36672c_driver = {
	.probe = nt36672c_probe,
	.remove = nt36672c_remove,
	.driver = {
		.name = "panel-nt36672c",
		.of_match_table = nt36672c_of_match,
	},
};
module_mipi_dsi_driver(nt36672c_driver);

MODULE_AUTHOR("Grok <grok@x.ai>");
MODULE_DESCRIPTION("Novatek NT36672C DSI Video Mode Panel Driver with DSC");
MODULE_LICENSE("GPL");
