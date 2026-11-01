// SPDX-License-Identifier: GPL-2.0-only
/*
 * SinoWealth SH366101 battery fuel gauge (SBS over SMBus).
 *
 * Minimal mainline driver: the gauge tracks the battery by itself, so this
 * only polls the standard SBS registers and exposes them through power_supply
 * for upower/GNOME.
 */

#include <linux/delay.h>
#include <linux/i2c.h>
#include <linux/module.h>
#include <linux/power_supply.h>
#include <linux/workqueue.h>

#define SH366101_REG_STATUS	0x06
#define SH366101_REG_VOLTAGE	0x04
#define SH366101_REG_CURRENT	0x10
#define SH366101_REG_SOC	0x1c
#define SH366101_REG_TEMP	0x1e
#define SH366101_REG_RMC	0x0c
#define SH366101_REG_FCC	0x0e
#define SH366101_REG_CYCLES	0x1a
#define SH366101_REG_DEVICE_ID	0x01

#define SH366101_POLL_MS	10000
#define SH366101_STATUS_FC	0x4000	/* fully charged */

struct sh366101 {
	struct i2c_client *client;
	struct power_supply *psy;
	struct delayed_work work;
	struct mutex lock;

	bool charger_online;
	int status;
	int capacity;
	int voltage_uv;
	int current_ua;
	int temp_dc;
	int charge_now_uah;
	int charge_full_uah;
	int cycles;
	int device_id;
};

static int sh366101_read_word(struct sh366101 *fg, u8 reg, u16 *val)
{
	int ret, i;

	for (i = 0; i < 3; i++) {
		ret = i2c_smbus_read_word_data(fg->client, reg);
		if (ret >= 0) {
			*val = (u16)ret;
			return 0;
		}
		usleep_range(200, 400);
	}
	return ret;
}

static int sh366101_read_signed(struct sh366101 *fg, u8 reg, s16 *val)
{
	u16 raw;
	int ret = sh366101_read_word(fg, reg, &raw);

	if (!ret)
		*val = (s16)raw;
	return ret;
}

static void sh366101_update(struct sh366101 *fg)
{
	union power_supply_propval online = { };
	struct power_supply *charger;
	u16 u;
	s16 s;

	/* With a full battery and the charger connected the gauge reports the
	 * small load current as a discharge; follow the charger's AC state. */
	charger = power_supply_get_by_name("bq25890-charger-0");
	if (charger) {
		if (!power_supply_get_property(charger,
					       POWER_SUPPLY_PROP_ONLINE, &online))
			fg->charger_online = online.intval;
		power_supply_put(charger);
	}

	mutex_lock(&fg->lock);
	if (!sh366101_read_word(fg, SH366101_REG_STATUS, &u))
		fg->status = u;
	if (!sh366101_read_word(fg, SH366101_REG_VOLTAGE, &u))
		fg->voltage_uv = u * 1000;
	if (!sh366101_read_signed(fg, SH366101_REG_CURRENT, &s))
		fg->current_ua = s * 1000;
	if (!sh366101_read_word(fg, SH366101_REG_SOC, &u) && u <= 100)
		fg->capacity = u;
	if (!sh366101_read_word(fg, SH366101_REG_TEMP, &u))
		fg->temp_dc = (int)u - 2731;	/* 0.1 K -> 0.1 C */
	if (!sh366101_read_word(fg, SH366101_REG_RMC, &u))
		fg->charge_now_uah = u * 1000;
	if (!sh366101_read_word(fg, SH366101_REG_FCC, &u))
		fg->charge_full_uah = u * 1000;
	if (!sh366101_read_word(fg, SH366101_REG_CYCLES, &u))
		fg->cycles = u;
	if (fg->device_id < 0 && !sh366101_read_word(fg, SH366101_REG_DEVICE_ID, &u))
		fg->device_id = u;
	mutex_unlock(&fg->lock);
}

static void sh366101_work(struct work_struct *work)
{
	struct sh366101 *fg = container_of(work, struct sh366101, work.work);

	sh366101_update(fg);
	power_supply_changed(fg->psy);
	schedule_delayed_work(&fg->work,
			      msecs_to_jiffies(SH366101_POLL_MS));
}

static int sh366101_get_property(struct power_supply *psy,
				 enum power_supply_property psp,
				 union power_supply_propval *val)
{
	struct sh366101 *fg = power_supply_get_drvdata(psy);

	switch (psp) {
	case POWER_SUPPLY_PROP_STATUS:
		if ((fg->status & SH366101_STATUS_FC) ||
		    (fg->charger_online && fg->capacity >= 99))
			val->intval = POWER_SUPPLY_STATUS_FULL;
		else if (fg->current_ua > 0)
			val->intval = POWER_SUPPLY_STATUS_CHARGING;
		else if (fg->current_ua < 0)
			val->intval = POWER_SUPPLY_STATUS_DISCHARGING;
		else
			val->intval = POWER_SUPPLY_STATUS_NOT_CHARGING;
		break;
	case POWER_SUPPLY_PROP_CAPACITY:
		val->intval = fg->capacity;
		break;
	case POWER_SUPPLY_PROP_VOLTAGE_NOW:
		val->intval = fg->voltage_uv;
		break;
	case POWER_SUPPLY_PROP_CURRENT_NOW:
		val->intval = fg->current_ua;
		break;
	case POWER_SUPPLY_PROP_TEMP:
		val->intval = fg->temp_dc;
		break;
	case POWER_SUPPLY_PROP_CHARGE_NOW:
		val->intval = fg->charge_now_uah;
		break;
	case POWER_SUPPLY_PROP_CHARGE_FULL:
	case POWER_SUPPLY_PROP_CHARGE_FULL_DESIGN:
		val->intval = fg->charge_full_uah;
		break;
	case POWER_SUPPLY_PROP_CYCLE_COUNT:
		val->intval = fg->cycles;
		break;
	case POWER_SUPPLY_PROP_HEALTH:
		val->intval = POWER_SUPPLY_HEALTH_GOOD;
		break;
	case POWER_SUPPLY_PROP_TECHNOLOGY:
		val->intval = POWER_SUPPLY_TECHNOLOGY_LION;
		break;
	default:
		return -EINVAL;
	}
	return 0;
}

static enum power_supply_property sh366101_props[] = {
	POWER_SUPPLY_PROP_STATUS,
	POWER_SUPPLY_PROP_CAPACITY,
	POWER_SUPPLY_PROP_VOLTAGE_NOW,
	POWER_SUPPLY_PROP_CURRENT_NOW,
	POWER_SUPPLY_PROP_TEMP,
	POWER_SUPPLY_PROP_CHARGE_NOW,
	POWER_SUPPLY_PROP_CHARGE_FULL,
	POWER_SUPPLY_PROP_CHARGE_FULL_DESIGN,
	POWER_SUPPLY_PROP_CYCLE_COUNT,
	POWER_SUPPLY_PROP_HEALTH,
	POWER_SUPPLY_PROP_TECHNOLOGY,
};

static const struct power_supply_desc sh366101_desc = {
	.name = "sh366101",
	.type = POWER_SUPPLY_TYPE_BATTERY,
	.properties = sh366101_props,
	.num_properties = ARRAY_SIZE(sh366101_props),
	.get_property = sh366101_get_property,
};

static int sh366101_probe(struct i2c_client *client)
{
	struct power_supply_config cfg = { };
	struct sh366101 *fg;
	int ret;

	fg = devm_kzalloc(&client->dev, sizeof(*fg), GFP_KERNEL);
	if (!fg)
		return -ENOMEM;

	fg->client = client;
	fg->device_id = -1;
	fg->capacity = 0;
	mutex_init(&fg->lock);
	i2c_set_clientdata(client, fg);

	cfg.drv_data = fg;
	cfg.fwnode = client->dev.fwnode;
	fg->psy = devm_power_supply_register(&client->dev, &sh366101_desc,
					     &cfg);
	if (IS_ERR(fg->psy))
		return dev_err_probe(&client->dev, PTR_ERR(fg->psy),
				     "failed to register power supply\n");

	sh366101_update(fg);
	dev_info(&client->dev, "SH366101 fuel gauge (id %04x, %d%%)\n",
		 fg->device_id, fg->capacity);

	INIT_DELAYED_WORK(&fg->work, sh366101_work);
	schedule_delayed_work(&fg->work,
			      msecs_to_jiffies(SH366101_POLL_MS));
	return 0;
}

static void sh366101_remove(struct i2c_client *client)
{
	struct sh366101 *fg = i2c_get_clientdata(client);

	cancel_delayed_work_sync(&fg->work);
}

static const struct of_device_id sh366101_of_match[] = {
	{ .compatible = "sinowealth,sh366101" },
	{ .compatible = "sh,sh366101" },
	{ /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, sh366101_of_match);

static const struct i2c_device_id sh366101_id[] = {
	{ "sh366101", 0 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, sh366101_id);

static struct i2c_driver sh366101_driver = {
	.driver = {
		.name = "sh366101",
		.of_match_table = sh366101_of_match,
	},
	.probe = sh366101_probe,
	.remove = sh366101_remove,
	.id_table = sh366101_id,
};
module_i2c_driver(sh366101_driver);

MODULE_AUTHOR("Redmi Pad SE bring-up");
MODULE_DESCRIPTION("SinoWealth SH366101 battery fuel gauge");
MODULE_LICENSE("GPL");
