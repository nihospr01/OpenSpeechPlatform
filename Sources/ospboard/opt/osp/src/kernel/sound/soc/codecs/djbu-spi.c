/*
 * Digital JellyBeans Unified driver: SPI driver
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

#include <linux/mod_devicetable.h>
#include <linux/module.h>
#include <linux/regmap.h>
#include <linux/spi/spi.h>
#include <sound/soc.h>

#include "djbu.h"

static void djbu_spi_switch_mode(struct device *dev)
{
  struct spi_device *spi = to_spi_device(dev);
  /*
  //No harm in telling codecs to switch to SPI mode more than once, and
  //having this check in place means the codec only gets one chance to get
  //properly initialized, rather than having that chance every time.
  if(djbu.num_spi_switch_modes >= 2){
    printk(KERN_INFO "Not switching to SPI mode\n");
    return;
  }
  ++djbu.num_spi_switch_modes;
  */
  printk(KERN_INFO "djbu_spi_switch_mode\n");
  /*
   * To get the device into SPI mode CLATCH has to be pulled low three
   * times.  Do this by issuing three dummy reads.
   */
  spi_w8r8(spi, 0x00);
  spi_w8r8(spi, 0x00);
  spi_w8r8(spi, 0x00);
}

static int djbul_spi_probe(struct spi_device *spi)
{
  struct regmap_config config;
  printk(KERN_INFO "djbul_spi_probe, setting max speed to 50 kHz\n");
  spi->max_speed_hz = 48000;

  config = djbu_lr_regmap_config;
  config.read_flag_mask = 0x1;
  config.reg_bits = 24; //There is one byte which is 0 for writes or 1 for reads,
  //then the two-byte address

  return djbu_probe(&spi->dev,
    devm_regmap_init_spi(spi, &config), djbu_spi_switch_mode, false);
}
static int djbur_spi_probe(struct spi_device *spi)
{
  struct regmap_config config;
  printk(KERN_INFO "djbur_spi_probe, setting max speed to 50 kHz\n");
  spi->max_speed_hz = 48000;

  config = djbu_lr_regmap_config;
  config.read_flag_mask = 0x1;
  config.reg_bits = 24;

  return djbu_probe(&spi->dev,
    devm_regmap_init_spi(spi, &config), djbu_spi_switch_mode, true);
}

static int djbul_spi_remove(struct spi_device *spi)
{
  return djbu_remove(&spi->dev, false);
}
static int djbur_spi_remove(struct spi_device *spi)
{
  return djbu_remove(&spi->dev, true);
}

static const struct spi_device_id djbul_spi_id[] = {
  { "djbul", 0 },
  { }
};
static const struct spi_device_id djbur_spi_id[] = {
  { "djbur", 0 },
  { }
};
MODULE_DEVICE_TABLE(spi, djbul_spi_id);
MODULE_DEVICE_TABLE(spi, djbur_spi_id);

static struct spi_driver djbul_spi_driver = {
  .driver = {
    .name = "djbul",
  },
  .probe = djbul_spi_probe,
  .remove = djbul_spi_remove,
  .id_table = djbul_spi_id,
};
static struct spi_driver djbur_spi_driver = {
  .driver = {
    .name = "djbur",
  },
  .probe = djbur_spi_probe,
  .remove = djbur_spi_remove,
  .id_table = djbur_spi_id,
};
module_spi_driver(djbul_spi_driver);
module_spi_driver(djbur_spi_driver);

MODULE_DESCRIPTION("ASoC DJBU CODEC SPI driver");
MODULE_AUTHOR("Louis Pisha <lpisha@eng.ucsd.edu>");
MODULE_LICENSE("GPL v3");
