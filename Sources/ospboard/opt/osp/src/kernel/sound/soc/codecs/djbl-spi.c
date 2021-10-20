/*
 * Driver for DJBL codec
 *
 * Copyright 2016 Analog Devices Inc.
 *  Author: Lars-Peter Clausen <lars@metafoo.de>
 *
 * Licensed under the GPL-2.
 */

#include <linux/mod_devicetable.h>
#include <linux/module.h>
#include <linux/regmap.h>
#include <linux/spi/spi.h>
#include <sound/soc.h>

#include "djbl.h"

static void djbl_spi_switch_mode(struct device *dev)
{
	struct spi_device *spi = to_spi_device(dev);

	/*
	 * To get the device into SPI mode CLATCH has to be pulled low three
	 * times.  Do this by issuing three dummy reads.
	 */
	spi_w8r8(spi, 0x00);
	spi_w8r8(spi, 0x00);
	spi_w8r8(spi, 0x00);
}

static int djbl_spi_probe(struct spi_device *spi)
{
	struct regmap_config config;

	config = djbl_regmap_config;
	config.read_flag_mask = 0x1;

	return djbl_probe(&spi->dev,
		devm_regmap_init_spi(spi, &config), djbl_spi_switch_mode);
}

static int djbl_spi_remove(struct spi_device *spi)
{
	snd_soc_unregister_codec(&spi->dev);
	return 0;
}

static const struct spi_device_id djbl_spi_id[] = {
	{ "djbl", 0 },
	{ }
};
MODULE_DEVICE_TABLE(spi, djbl_spi_id);

static struct spi_driver djbl_spi_driver = {
	.driver = {
		.name = "djbl",
	},
	.probe = djbl_spi_probe,
	.remove = djbl_spi_remove,
	.id_table = djbl_spi_id,
};
module_spi_driver(djbl_spi_driver);

MODULE_DESCRIPTION("ASoC DJBL CODEC SPI driver");
MODULE_AUTHOR("Lars-Peter Clausen <lars@metafoo.de>");
MODULE_LICENSE("GPL v2");
