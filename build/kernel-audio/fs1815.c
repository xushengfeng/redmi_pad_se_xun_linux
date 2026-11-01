// SPDX-License-Identifier: GPL-2.0-only
/*
 * Foursemi FS1815 smart speaker amplifier (I2C).
 *
 * The Redmi Pad SE has four of them: two on I2C1 behind a shared reset line
 * and two more on I2C2, driving the four speakers.  Linux does not control
 * them through ASoC here; the driver brings each amplifier up with the
 * register preset of the board (fsm preset "M84/MI", scene "music") and
 * leaves it running, so the WCD937x AUX output is always amplified.
 *
 * Register access: one byte of address, a 16-bit big-endian value; read is
 * an address write followed by two bytes.  The register tables below are the
 * ones the vendor driver reads from fs1815.fsm for the addresses 0x34..0x37
 * (only 0x19, the channel select, differs); see build/kernel-audio/fsm-preset.py.
 */

#include <linux/delay.h>
#include <linux/gpio/consumer.h>
#include <linux/i2c.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/string.h>

#define FS1815_REG_DEVID	0x01
#define FS1815_REG_REVID	0x02
#define FS1815_REG_CHIPINI	0x0e
#define FS1815_REG_PWRCTRL	0x10
#define FS1815_REG_SYSCTRL	0x11
#define FS1815_REG_SPKCOEF	0x14

#define FS1815_DEV_ID		0x12
#define FS1815_CHIPINI_DONE	0x0003

struct fs1815_reg {
	u8 reg;
	u8 pos;
	u8 len;
	u16 val;
};

/* Preset common table. */
static const struct fs1815_reg fs1815_common[] = {
	{ 0x11, 0, 15, 0x0001 },
	{ 0x3e, 0, 15, 0x8500 },
	{ 0x3f, 0, 15, 0x8110 },
	{ 0x4c, 0, 15, 0x7008 },
	{ 0x2a, 0, 15, 0x0030 },
	{ 0x58, 0, 15, 0x827a },
	{ 0x59, 0, 15, 0xd821 },
	{ 0x5a, 0, 15, 0xd841 },
	{ 0x5b, 0, 15, 0xd07b },
	{ 0x5c, 0, 15, 0xd800 },
	{ 0x88, 0, 15, 0xa000 },
	{ 0x89, 0, 15, 0x0032 },
	{ 0xbc, 4, 4, 0x0002 },
	{ 0x4c, 3, 0, 0x0000 },
	{ 0x22, 0, 15, 0x0028 },
	{ 0x23, 0, 15, 0x0027 },
	{ 0xb8, 0, 15, 0x0002 },
	{ 0x5d, 0, 15, 0xa001 },
};

/* Preset "music" scene; 0x19 is filled in from the amplifier position. */
static const struct fs1815_reg fs1815_music[] = {
	{ 0x20, 0, 15, 0x07c2 },
	{ 0x18, 0, 15, 0x000a },
	{ 0x3f, 15, 0, 0x0000 },
	{ 0xbc, 15, 0, 0x0000 },
	{ 0x1f, 0, 15, 0xca91 },
	{ 0x58, 0, 15, 0x821a },
	{ 0x59, 12, 3, 0x0000 },
	{ 0x19, 0, 4, 0x0009 },
	{ 0x5a, 0, 15, 0xf8e0 },
	{ 0x1f, 0, 15, 0x0000 },
};

struct fs1815 {
	struct i2c_client *client;
	struct gpio_desc *reset;
	u8 position;
};

static int fs1815_read(struct i2c_client *client, u8 reg, u16 *val)
{
	u8 buf[2];
	struct i2c_msg msgs[2] = {
		{ .addr = client->addr, .flags = 0, .len = 1, .buf = &reg },
		{ .addr = client->addr, .flags = I2C_M_RD, .len = 2, .buf = buf },
	};
	int ret;

	ret = i2c_transfer(client->adapter, msgs, ARRAY_SIZE(msgs));
	if (ret != ARRAY_SIZE(msgs))
		return ret < 0 ? ret : -EIO;

	*val = (buf[0] << 8) | buf[1];
	return 0;
}

static int fs1815_write(struct i2c_client *client, u8 reg, u16 val)
{
	u8 buf[3] = { reg, val >> 8, val & 0xff };
	struct i2c_msg msg = {
		.addr = client->addr, .flags = 0, .len = 3, .buf = buf,
	};
	int ret;

	ret = i2c_transfer(client->adapter, &msg, 1);
	return ret == 1 ? 0 : (ret < 0 ? ret : -EIO);
}

static int fs1815_update(struct i2c_client *client,
			 const struct fs1815_reg *r, u16 val)
{
	u16 mask, cur;
	int ret;

	if (r->pos == 0 && r->len == 15)
		return fs1815_write(client, r->reg, val);

	ret = fs1815_read(client, r->reg, &cur);
	if (ret)
		return ret;

	mask = ((1u << (r->len + 1)) - 1) << r->pos;
	cur = (cur & ~mask) | ((val << r->pos) & mask);
	return fs1815_write(client, r->reg, cur);
}

static int fs1815_write_table(struct i2c_client *client,
			      const struct fs1815_reg *tbl, size_t n)
{
	int ret;
	size_t i;

	for (i = 0; i < n; i++) {
		ret = fs1815_update(client, &tbl[i], tbl[i].val);
		if (ret)
			return ret;
	}
	return 0;
}

static int fs1815_soft_reset(struct i2c_client *client)
{
	u16 val;
	int ret, i;

	ret = fs1815_write(client, FS1815_REG_PWRCTRL, 0x0002);
	fs1815_read(client, FS1815_REG_PWRCTRL, &val);
	msleep(15);
	fs1815_write(client, FS1815_REG_PWRCTRL, 0x0001);

	for (i = 0; i < 20; i++) {
		ret = fs1815_read(client, FS1815_REG_CHIPINI, &val);
		if (!ret && (val == FS1815_CHIPINI_DONE || val == 0x0300))
			return 0;
		usleep_range(1000, 2000);
	}

	dev_err(&client->dev, "chip init did not finish (0x%04x)\n", val);
	return -ETIMEDOUT;
}

/* The reset line is shared by the two amplifiers of one bus. */
static void fs1815_reset_line(struct fs1815 *amp)
{
	if (!amp->reset)
		return;

	gpiod_set_value_cansleep(amp->reset, 1);
	msleep(10);
	gpiod_set_value_cansleep(amp->reset, 0);
	msleep(1);
}

static int fs1815_start(struct fs1815 *amp)
{
	struct i2c_client *client = amp->client;
	struct fs1815_reg music[ARRAY_SIZE(fs1815_music)];
	u16 revid = 0;
	int ret, i;

	ret = fs1815_soft_reset(client);
	if (ret)
		return ret;

	/* Speaker temperature coefficient from the preset. */
	ret = fs1815_write(client, FS1815_REG_SPKCOEF, 229 << 1);
	if (ret)
		return ret;

	ret = fs1815_write_table(client, fs1815_common, ARRAY_SIZE(fs1815_common));
	if (ret)
		return ret;

	memcpy(music, fs1815_music, sizeof(music));
	for (i = 0; i < ARRAY_SIZE(music); i++)
		if (music[i].reg == 0x19)
			music[i].val = amp->position ? 0x000a : 0x0009;
	ret = fs1815_write_table(client, music, ARRAY_SIZE(music));
	if (ret)
		return ret;

	fs1815_read(client, FS1815_REG_REVID, &revid);
	if (revid == 0x0011) {
		ret = fs1815_write(client, 0xbc, 0x9433);
		if (ret)
			return ret;
	}

	ret = fs1815_write(client, FS1815_REG_SYSCTRL, 0x008f);
	if (ret)
		return ret;
	ret = fs1815_write(client, FS1815_REG_PWRCTRL, 0x0000);
	if (ret)
		return ret;

	msleep(10);
	return 0;
}

static int fs1815_probe(struct i2c_client *client)
{
	struct fs1815 *amp;
	u16 id = 0;
	u32 dev_addr;
	int ret;

	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C))
		return -EIO;

	amp = devm_kzalloc(&client->dev, sizeof(*amp), GFP_KERNEL);
	if (!amp)
		return -ENOMEM;
	amp->client = client;
	i2c_set_clientdata(client, amp);

	amp->reset = devm_gpiod_get_optional(&client->dev, "reset",
					     GPIOD_OUT_HIGH);
	if (IS_ERR(amp->reset)) {
		int err = PTR_ERR(amp->reset);

		dev_err(&client->dev, "reset gpio unavailable: %d\n", err);
		/* The other amplifier on this bus owns the shared line. */
		if (err != -EBUSY)
			return err;
		amp->reset = NULL;
	}

	fs1815_reset_line(amp);
	if (fs1815_soft_reset(client))
		dev_info(&client->dev, "no chip init after reset\n");

	ret = fs1815_read(client, FS1815_REG_DEVID, &id);
	if (ret || (id & 0xff) != FS1815_DEV_ID) {
		dev_info(&client->dev, "no FS1815 (id 0x%04x, ret %d)\n",
			 id, ret);
		return -ENODEV;
	}

	ret = of_property_read_u32(client->dev.of_node, "foursemi,dev-addr",
				   &dev_addr);
	if (ret)
		dev_addr = client->addr;
	/* 0x34/0x36 are one channel, 0x35/0x37 the other (preset tables). */
	amp->position = (dev_addr == 0x35 || dev_addr == 0x37);

	ret = fs1815_start(amp);
	if (ret)
		return dev_err_probe(&client->dev, ret, "start failed\n");

	dev_info(&client->dev, "FS1815 at 0x%02x (preset 0x%02x), power on\n",
		 client->addr, dev_addr);
	return 0;
}

static void fs1815_remove(struct i2c_client *client)
{
	struct fs1815 *amp = i2c_get_clientdata(client);

	fs1815_write(client, FS1815_REG_PWRCTRL, 0x0001);
	fs1815_write(client, FS1815_REG_SYSCTRL, 0x0000);
	if (amp->reset)
		gpiod_set_value_cansleep(amp->reset, 1);
}

static const struct of_device_id fs1815_of_match[] = {
	{ .compatible = "foursemi,fs1815" },
	{ .compatible = "foursemi,fs16xx" },
	{ }
};
MODULE_DEVICE_TABLE(of, fs1815_of_match);

static const struct i2c_device_id fs1815_id[] = {
	{ "fs1815", 0 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, fs1815_id);

static struct i2c_driver fs1815_driver = {
	.driver = {
		.name = "fs1815",
		.of_match_table = fs1815_of_match,
	},
	.probe = fs1815_probe,
	.remove = fs1815_remove,
	.id_table = fs1815_id,
};
module_i2c_driver(fs1815_driver);

MODULE_AUTHOR("Redmi Pad SE bring-up");
MODULE_DESCRIPTION("Foursemi FS1815 speaker amplifier");
MODULE_LICENSE("GPL");
