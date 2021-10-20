/*
 * DJBR driver
 *
 * Copyright 2016 Analog Devices Inc.
 *  Author: Lars-Peter Clausen <lars@metafoo.de>
 *
 * Licensed under the GPL-2.
 */

#ifndef SOUND_SOC_CODECS_DJBR_H
#define SOUND_SOC_CODECS_DJBR_H

#include <linux/regmap.h>

struct device;

int djbr_probe(struct device *dev, struct regmap *regmap,
	void (*switch_mode)(struct device *dev));

extern const struct regmap_config djbr_regmap_config;

#endif
