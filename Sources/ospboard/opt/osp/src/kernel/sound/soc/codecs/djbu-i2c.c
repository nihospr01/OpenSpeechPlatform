/*
 * Digital JellyBeans Unified driver: I2C driver
 *
 * Copyright 2018 UCSD / THE_Lab
 *  Author: Louis Pisha <lpisha@eng.ucsd.edu>
 *
 * Based on ADAU1372 Driver
 * Copyright 2016 Analog Devices Inc.
 *  Author: Lars-Peter Clausen <lars@metafoo.de>
 *
 * Licensed under the GPL3
 */

#include <linux/i2c.h>
#include <linux/mod_devicetable.h>
#include <linux/module.h>
#include <linux/regmap.h>
#include <sound/soc.h>

#include "djbu.h"

static int djbul_i2c_probe(struct i2c_client *client,
  const struct i2c_device_id *id)
{
  return djbu_probe(&client->dev,
    devm_regmap_init_i2c(client, &djbu_lr_regmap_config), NULL, false);
}
static int djbur_i2c_probe(struct i2c_client *client,
  const struct i2c_device_id *id)
{
  return djbu_probe(&client->dev,
    devm_regmap_init_i2c(client, &djbu_lr_regmap_config), NULL, true);
}

static int djbul_i2c_remove(struct i2c_client *client)
{
  return djbu_remove(&client->dev, false);
}
static int djbur_i2c_remove(struct i2c_client *client)
{
  return djbu_remove(&client->dev, true);
}

static const struct i2c_device_id djbul_i2c_ids[] = {
  { "djbul", 0 },
  { }
};
static const struct i2c_device_id djbur_i2c_ids[] = {
  { "djbur", 0 },
  { }
};
MODULE_DEVICE_TABLE(i2c, djbul_i2c_ids);
MODULE_DEVICE_TABLE(i2c, djbur_i2c_ids);

static struct i2c_driver djbul_i2c_driver = {
  .driver = {
    .name = "djbul",
  },
  .probe = djbul_i2c_probe,
  .remove = djbul_i2c_remove,
  .id_table = djbul_i2c_ids,
};
static struct i2c_driver djbur_i2c_driver = {
  .driver = {
    .name = "djbur",
  },
  .probe = djbur_i2c_probe,
  .remove = djbur_i2c_remove,
  .id_table = djbur_i2c_ids,
};
module_i2c_driver(djbul_i2c_driver);
module_i2c_driver(djbur_i2c_driver);

MODULE_DESCRIPTION("ASoC DJBU CODEC I2C driver");
MODULE_AUTHOR("Louis Pisha <lpisha@eng.ucsd.edu>");
MODULE_LICENSE("GPL v3");
