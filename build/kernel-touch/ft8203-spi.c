// SPDX-License-Identifier: GPL-2.0-only
/*
 * FocalTech FT8203 touchscreen on SPI.
 *
 * The controller boots into a ROM loader; the application firmware has to be
 * uploaded over SPI at probe time (the stock driver does the same). Frames are
 * then read from register 1 on the interrupt line. Every transaction is a
 * full-duplex SPI transfer: the status byte is at offset 3 and reads carry a
 * CRC-16/X-25 of the payload.
 */

#include <linux/delay.h>
#include <linux/firmware.h>
#include <linux/gpio/consumer.h>
#include <linux/input.h>
#include <linux/input/mt.h>
#include <linux/module.h>
#include <linux/spi/spi.h>

#define FT8203_MAX_TOUCH		10
#define FT8203_FRAME_BYTES		90
#define FT8203_CHUNK			4096

/* The controller reports in a 1600x2560 grid, the panel is 1200x1920 */
#define FT8203_RAW_WIDTH		1536
#define FT8203_RAW_HEIGHT		2400

#define FT8203_FW_HUAXING		"focaltech_ts_fw_huaxing_ft8203.bin"
#define FT8203_FW_BOE			"focaltech_ts_fw_boe_ft8203.bin"

struct ft8203 {
	struct spi_device *spi;
	struct input_dev *input;
	struct gpio_desc *reset;
	struct mutex lock;		/* serializes transactions */
	u8 *tx;
	u8 *rx;
};

static u16 ft8203_crc(const u8 *data, size_t length)
{
	u16 crc = 0xffff;
	size_t i;
	unsigned int bit;

	for (i = 0; i < length; i++) {
		crc ^= data[i];
		for (bit = 0; bit < 8; bit++)
			crc = (crc >> 1) ^ ((crc & 1) ? 0x8408 : 0);
	}
	return crc;
}

static int ft8203_exchange(struct ft8203 *ts, size_t length, bool read,
			   size_t payload)
{
	struct spi_transfer xfer = {
		.tx_buf = ts->tx,
		.rx_buf = ts->rx,
		.len = length,
	};
	int ret = -EIO;
	unsigned int attempt;

	for (attempt = 0; attempt < 3; attempt++) {
		memset(ts->rx, 0, length);
		ret = spi_sync_transfer(ts->spi, &xfer, 1);
		usleep_range(150, 250);
		if (ret)
			continue;
		if (ts->rx[3] & 0xa0) {
			ret = -EIO;
			continue;
		}
		if (read) {
			u16 received = ts->rx[7 + payload] |
				       (ts->rx[8 + payload] << 8);

			if (ft8203_crc(ts->rx + 7, payload) != received) {
				ret = -EBADMSG;
				continue;
			}
		}
		return 0;
	}
	return ret;
}

static int ft8203_read(struct ft8203 *ts, u8 command, u8 *data, size_t length)
{
	int ret;

	memset(ts->tx, 0, length + 9);
	ts->tx[0] = command;
	ts->tx[1] = 0xa0;
	ts->tx[2] = length >> 8;
	ts->tx[3] = length;

	ret = ft8203_exchange(ts, length + 9, true, length);
	if (!ret)
		memcpy(data, ts->rx + 7, length);
	return ret;
}

static int ft8203_write(struct ft8203 *ts, u8 command, const u8 *data,
			size_t length)
{
	size_t total = length ? 7 + length : 4;

	if (length > FT8203_CHUNK)
		return -EINVAL;

	memset(ts->tx, 0, total);
	ts->tx[0] = command;
	ts->tx[2] = length >> 8;
	ts->tx[3] = length;
	if (length)
		memcpy(ts->tx + 7, data, length);

	return ft8203_exchange(ts, total, false, length);
}

static void ft8203_reset(struct ft8203 *ts)
{
	if (!ts->reset)
		return;

	gpiod_set_value_cansleep(ts->reset, 1);
	msleep(20);
	gpiod_set_value_cansleep(ts->reset, 0);
}

static u16 ft8203_firmware_ecc(const u8 *data, size_t length)
{
	u16 ecc = 0;
	size_t i;
	unsigned int bit;

	for (i = 0; i + 1 < length; i += 2) {
		ecc ^= ((u16)data[i] << 8) | data[i + 1];
		for (bit = 0; bit < 16; bit++)
			ecc = (ecc >> 1) ^ ((ecc & 1) ? 0x8408 : 0);
	}
	return ecc;
}

static int ft8203_upload(struct ft8203 *ts, const u8 *fw, size_t size)
{
	u8 rom[2];
	u8 status;
	u8 bytes[2];
	u8 request[6];
	unsigned int words, offset;
	size_t length;
	u16 expected, device;
	int ret;
	unsigned int attempt, i;

	if (size < 0x104)
		return -EINVAL;

	words = ((unsigned int)fw[0x100] << 8) | fw[0x101];
	if ((u16)(words + (((unsigned int)fw[0x102] << 8) | fw[0x103])) != 0xffff)
		return -EINVAL;
	length = (size_t)words * 2;
	if (length < 0x104 || length > 96 * 1024 || length > size)
		return -EINVAL;

	/* Get the ROM loader running */
	for (attempt = 0; attempt < 6; attempt++) {
		ft8203_reset(ts);
		msleep(150);
		ret = ft8203_write(ts, 0x55, NULL, 0);
		if (ret) {
			msleep(50);
			continue;
		}
		msleep(20);
		ret = ft8203_read(ts, 0x90, rom, sizeof(rom));
		dev_dbg(&ts->spi->dev, "rom probe %u: ret=%d id=%02x%02x\n",
			attempt, ret, rom[0], rom[1]);
		if (!ret && rom[0] == 0x82 && rom[1] == 0x01)
			break;
		ret = -ENODEV;
	}
	if (ret)
		return ret;

	for (offset = 0; offset < length; offset += FT8203_CHUNK) {
		size_t count = min_t(size_t, length - offset, FT8203_CHUNK);
		u8 address[3] = { offset >> 16, offset >> 8, offset };

		ret = ft8203_write(ts, 0xad, address, sizeof(address));
		if (ret)
			return ret;
		ret = ft8203_write(ts, 0xae, fw + offset, count);
		if (ret)
			return ret;
	}

	memset(request, 0, sizeof(request));
	request[3] = length >> 16;
	request[4] = length >> 8;
	request[5] = length;
	ret = ft8203_write(ts, 0xcc, request, sizeof(request));
	if (ret)
		return ret;

	msleep(20);
	status = 0;
	for (attempt = 0; attempt < 100; attempt++) {
		ret = ft8203_read(ts, 0xce, &status, 1);
		if (ret)
			return ret;
		if (status == 0xa5)
			break;
		usleep_range(1000, 2000);
	}
	if (status != 0xa5)
		return -ETIMEDOUT;

	ret = ft8203_read(ts, 0xcd, bytes, sizeof(bytes));
	if (ret)
		return ret;
	expected = ft8203_firmware_ecc(fw, length);
	device = ((u16)bytes[0] << 8) | bytes[1];
	if (expected != device)
		return -EBADMSG;

	ret = ft8203_write(ts, 0x08, NULL, 0);
	if (ret)
		return ret;
	msleep(210);

	return 0;
}

static int ft8203_load_firmware(struct ft8203 *ts)
{
	const struct firmware *fw = NULL;
	const char *name;
	int ret, i;

	if (!device_property_read_string(&ts->spi->dev, "firmware-name", &name))
		ret = request_firmware(&fw, name, &ts->spi->dev);
	else
		ret = request_firmware(&fw, FT8203_FW_HUAXING, &ts->spi->dev);
	if (ret && (!name || strcmp(name, FT8203_FW_BOE)))
		ret = request_firmware(&fw, FT8203_FW_BOE, &ts->spi->dev);
	if (ret)
		return dev_err_probe(&ts->spi->dev, ret,
				     "touch firmware not found\n");

	/* the ROM loader is happier at a low clock */
	ts->spi->max_speed_hz = 1000000;
	spi_setup(ts->spi);

	for (ret = 0, i = 0; i < 3; i++) {
		mutex_lock(&ts->lock);
		ret = ft8203_upload(ts, fw->data, fw->size);
		mutex_unlock(&ts->lock);
		if (!ret)
			break;
		dev_info(&ts->spi->dev, "upload attempt %d failed: %d\n",
			 i, ret);
		msleep(100);
	}
	release_firmware(fw);

	ts->spi->max_speed_hz = 10000000;
	spi_setup(ts->spi);

	if (ret) {
		dev_err(&ts->spi->dev, "firmware upload failed: %d\n", ret);
		return ret;
	}
	return 0;
}

static void ft8203_report(struct ft8203 *ts, const u8 *buf)
{
	struct input_dev *input = ts->input;
	unsigned int points = buf[1] & 0x0f;
	bool touch = false;
	unsigned int i;

	if (points > FT8203_MAX_TOUCH)
		return;

	for (i = 0; i < FT8203_MAX_TOUCH; i++) {
		const u8 *p = &buf[2 + i * 6];
		unsigned int event = p[0] >> 6;
		unsigned int id = p[2] >> 4;
		unsigned int x, y;

		if (id >= FT8203_MAX_TOUCH)
			break;

		input_mt_slot(input, id);

		if (event == 1 || (p[0] & 0x10)) {
			input_mt_report_slot_state(input, MT_TOOL_FINGER, false);
			continue;
		}
		if (event != 0 && event != 2)
			continue;

		x = ((p[0] & 0x0f) << 8) | p[1];
		y = ((p[2] & 0x0f) << 8) | p[3];
		if (x >= FT8203_RAW_WIDTH || y >= FT8203_RAW_HEIGHT)
			continue;

		touch = true;
		input_mt_report_slot_state(input, MT_TOOL_FINGER, true);
		input_report_abs(input, ABS_MT_POSITION_X,
				 1199 - x * 1200 / FT8203_RAW_WIDTH);
		input_report_abs(input, ABS_MT_POSITION_Y,
				 1919 - y * 1920 / FT8203_RAW_HEIGHT);
	}

	input_report_key(input, BTN_TOUCH, touch);
	input_mt_sync_frame(input);
	input_sync(input);
}

static irqreturn_t ft8203_irq(int irq, void *dev_id)
{
	struct ft8203 *ts = dev_id;
	u8 buf[FT8203_FRAME_BYTES];
	int ret;

	mutex_lock(&ts->lock);
	ret = ft8203_read(ts, 0x01, buf, sizeof(buf));
	mutex_unlock(&ts->lock);

	if (ret) {
		dev_dbg_ratelimited(&ts->spi->dev, "frame read failed: %d\n",
				    ret);
		return IRQ_HANDLED;
	}

	if (buf[1] != 0xff)
		dev_dbg_ratelimited(&ts->spi->dev,
				    "frame %*ph points=%u\n", 12, buf,
				    buf[1] & 0x0f);

	if (buf[1] == 0xff && buf[2] == 0xff && buf[3] == 0xff &&
	    buf[4] == 0xff && buf[5] == 0xff) {
		input_mt_sync_frame(ts->input);
		input_report_key(ts->input, BTN_TOUCH, false);
		input_sync(ts->input);
		return IRQ_HANDLED;
	}

	ft8203_report(ts, buf);
	return IRQ_HANDLED;
}

static int ft8203_probe(struct spi_device *spi)
{
	struct ft8203 *ts;
	struct input_dev *input;
	u8 high, low;
	int ret;

	ts = devm_kzalloc(&spi->dev, sizeof(*ts), GFP_KERNEL);
	if (!ts)
		return -ENOMEM;

	ts->spi = spi;
	mutex_init(&ts->lock);

	spi->mode = SPI_MODE_0;
	spi->bits_per_word = 8;
	spi->max_speed_hz = 10000000;
	ret = spi_setup(spi);
	if (ret)
		return dev_err_probe(&spi->dev, ret, "spi_setup failed\n");

	ts->tx = devm_kzalloc(&spi->dev, FT8203_CHUNK + 9, GFP_KERNEL);
	ts->rx = devm_kzalloc(&spi->dev, FT8203_CHUNK + 9, GFP_KERNEL);
	if (!ts->tx || !ts->rx)
		return -ENOMEM;

	ts->reset = devm_gpiod_get_optional(&spi->dev, "reset",
					    GPIOD_OUT_LOW);
	if (IS_ERR(ts->reset))
		return dev_err_probe(&spi->dev, PTR_ERR(ts->reset),
				     "failed to get reset gpio\n");

	ret = ft8203_load_firmware(ts);
	if (ret)
		return ret;

	mutex_lock(&ts->lock);
	ret = ft8203_read(ts, 0xa3, &high, 1);
	if (!ret)
		ret = ft8203_read(ts, 0x9f, &low, 1);
	mutex_unlock(&ts->lock);
	if (ret) {
		dev_err(&spi->dev, "cannot read chip id: %d\n", ret);
		return ret;
	}
	dev_info(&spi->dev, "FT%02x%02x touchscreen\n", high, low);

	input = devm_input_allocate_device(&spi->dev);
	if (!input)
		return -ENOMEM;
	ts->input = input;
	input->name = "Focaltech FT8203 Touchscreen";
	input->id.bustype = BUS_SPI;
	input->dev.parent = &spi->dev;

	input_set_abs_params(input, ABS_MT_POSITION_X, 0, 1199, 0, 0);
	input_set_abs_params(input, ABS_MT_POSITION_Y, 0, 1919, 0, 0);
	input_set_capability(input, EV_KEY, BTN_TOUCH);

	ret = input_mt_init_slots(input, FT8203_MAX_TOUCH,
				  INPUT_MT_DIRECT | INPUT_MT_DROP_UNUSED);
	if (ret)
		return dev_err_probe(&spi->dev, ret,
				     "failed to init MT slots\n");

	ret = input_register_device(input);
	if (ret)
		return dev_err_probe(&spi->dev, ret,
				     "failed to register input\n");

	ret = devm_request_threaded_irq(&spi->dev, spi->irq, NULL,
					ft8203_irq,
					IRQF_ONESHOT | IRQF_TRIGGER_FALLING,
					"ft8203", ts);
	if (ret)
		return dev_err_probe(&spi->dev, ret,
				     "failed to request irq\n");

	return 0;
}

static const struct of_device_id ft8203_of_match[] = {
	{ .compatible = "focaltech,ft8203" },
	{ /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, ft8203_of_match);

static struct spi_driver ft8203_driver = {
	.probe = ft8203_probe,
	.driver = {
		.name = "ft8203-spi",
		.of_match_table = ft8203_of_match,
	},
};
module_spi_driver(ft8203_driver);

MODULE_AUTHOR("Redmi Pad SE bring-up");
MODULE_DESCRIPTION("FocalTech FT8203 SPI touchscreen");
MODULE_LICENSE("GPL");
