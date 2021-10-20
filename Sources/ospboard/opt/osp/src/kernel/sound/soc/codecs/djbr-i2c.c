/*
 * Driver for DJBR codec
 *
 * Copyright 2016 Analog Devices Inc.
 *  Author: Lars-Peter Clausen <lars@metafoo.de>
 *
 * Licensed under the GPL-2.
 */

#include <linux/i2c.h>
#include <linux/mod_devicetable.h>
#include <linux/module.h>
#include <linux/regmap.h>
#include <sound/soc.h>

#include "djbr.h"

static int djbr_i2c_probe(struct i2c_client *client,
	const struct i2c_device_id *id)
{
	return djbr_probe(&client->dev,
		devm_regmap_init_i2c(client, &djbr_regmap_config), NULL);
}

static int djbr_i2c_remove(struct i2c_client *client)
{
	snd_soc_unregister_codec(&client->dev);
	return 0;
}

static const struct i2c_device_id djbr_i2c_ids[] = {
	{ "djbr", 0 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, djbr_i2c_ids);

static struct i2c_driver djbr_i2c_driver = {
	.driver = {
		.name = "djbr",
	},
	.probe = djbr_i2c_probe,
	.remove = djbr_i2c_remove,
	.id_table = djbr_i2c_ids,
};
module_i2c_driver(djbr_i2c_driver);

MODULE_DESCRIPTION("ASoC DJBR CODEC I2C driver");
MODULE_AUTHOR("Lars-Peter Clausen <lars@metafoo.de>");
MODULE_LICENSE("GPL v2");
