/*
 * Digital JellyBeans Unified driver: Header
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

#ifndef SOUND_SOC_CODECS_DJBU_H
#define SOUND_SOC_CODECS_DJBU_H

#include <linux/regmap.h>

struct device;

struct djbu_data {
    struct device *dev_l;
    struct device *dev_r;

    struct regmap *regmap_l;
    struct regmap *regmap_r;

    bool enabled_m;
    bool enabled_s;
    void (*switch_mode_l)(struct device *dev);
    void (*switch_mode_r)(struct device *dev);

    struct snd_pcm_hw_constraint_list rate_constraints;

    bool running;
    unsigned int num_spi_switch_modes;
    unsigned int reg_cap_state;
    unsigned int reg_ply_state;
};
extern struct djbu_data djbu;

int djbu_probe(struct device *dev, struct regmap *regmap,
  void (*switch_mode)(struct device *dev), bool ear_r);

int djbu_remove(struct device *dev, bool ear_r);

extern const struct regmap_config djbu_lr_regmap_config;

#endif
