/*
 * Driver for DJBL codec
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

#include "djbl.h"

static int djbl_i2c_probe(struct i2c_client *client,
	const struct i2c_device_id *id)
{
	return djbl_probe(&client->dev,
		devm_regmap_init_i2c(client, &djbl_regmap_config), NULL);
}

static int djbl_i2c_remove(struct i2c_client *client)
{
	snd_soc_unregister_codec(&client->dev);
	return 0;
}

static const struct i2c_device_id djbl_i2c_ids[] = {
	{ "djbl", 0 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, djbl_i2c_ids);

static struct i2c_driver djbl_i2c_driver = {
	.driver = {
		.name = "djbl",
	},
	.probe = djbl_i2c_probe,
	.remove = djbl_i2c_remove,
	.id_table = djbl_i2c_ids,
};
module_i2c_driver(djbl_i2c_driver);

MODULE_DESCRIPTION("ASoC DJBL CODEC I2C driver");
MODULE_AUTHOR("Lars-Peter Clausen <lars@metafoo.de>");
MODULE_LICENSE("GPL v2");
