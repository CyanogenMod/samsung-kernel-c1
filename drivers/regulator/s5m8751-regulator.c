/*
 * s5m8751-regulator.c  --  S5M8751 Advanced PMIC with AUDIO DAC
 *
 * Copyright 2009 Samsung Electronics.
 *
 *  This program is free software; you can redistribute  it and/or modify it
 *  under  the terms of  the GNU General  Public License as published by the
 *  Free Software Foundation;  either version 2 of the  License, or (at your
 *  option) any later version.
 */

#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/init.h>
#include <linux/bitops.h>
#include <linux/err.h>
#include <linux/i2c.h>
#include <linux/fs.h>
#include <linux/platform_device.h>
#include <linux/regulator/driver.h>
#include <linux/regulator/machine.h>
#include <linux/mfd/s5m8751/core.h>

static inline int s5m8751_ldo_val_to_mvolts(unsigned int val)
{
	return (val * 100) + 1800;
}

static inline u8 s5m8751_ldo_mvolts_to_val(int mV)
{
	return (mV - 1800) / 100;
}

static inline int s5m8751_ldo3_4_val_to_mvolts(unsigned int val)
{
	return (val * 100) + 800;
}

static inline u8 s5m8751_ldo3_4_mvolts_to_val(int mV)
{
	return (mV - 800) / 100;
}

static inline int s5m8751_buck1_val_to_mvolts(unsigned int val)
{
	return (val * 25) + 1800;
}

static inline int s5m8751_buck2_val_to_mvolts(unsigned int val)
{
	return (val * 25) + 500;
}

static inline u8 s5m8751_buck1_mvolts_to_val(int mV)
{
	return (mV - 1800) / 25;
}

static inline u8 s5m8751_buck2_mvolts_to_val(int mV)
{
	return (mV - 500) / 25;
}

static int s5m8751_ldo_set_voltage(struct regulator_dev *rdev, int min_uV,
	int max_uV)
{
	struct s5m8751 *s5m8751 = rdev_get_drvdata(rdev);
	int ret;
	int volt_reg, ldo = rdev_get_id(rdev), mV, min_mV = min_uV / 1000,
		max_mV = max_uV / 1000;
	u8 mask, val;

	if (min_mV < 1800 || min_mV > 3300)
		return -EINVAL;
	if (max_mV < 1800 || max_mV > 3300)
		return -EINVAL;

	mV = ((min_mV - 1701) / 100);
	if (s5m8751_ldo_val_to_mvolts(mV) > max_mV)
		return -EINVAL;
	BUG_ON(s5m8751_ldo_val_to_mvolts(mV) < min_mV);

	switch (ldo) {
	case S5M8751_LDO1:
		volt_reg = S5M8751_LDO1_VSET;
		mask = S5M8751_LDO1_VSET_MASK;
		break;
	case S5M8751_LDO2:
		volt_reg = S5M8751_LDO2_VSET;
		mask = S5M8751_LDO2_VSET_MASK;
		break;
	case S5M8751_LDO5:
		volt_reg = S5M8751_LDO5_VSET;
		mask = S5M8751_LDO5_VSET_MASK;
		break;
	case S5M8751_LDO6:
		volt_reg = S5M8751_LDO6_VSET;
		mask = S5M8751_LDO6_VSET_MASK;
		break;
	default:
		return -EINVAL;
	}
	ret = s5m8751_reg_read(s5m8751, volt_reg, &val);
	if (ret)
		goto out;

	val &= ~mask;
	ret = s5m8751_reg_write(s5m8751, volt_reg, val | mV);
out:
	return ret;
}

static int s5m8751_ldo_get_voltage(struct regulator_dev *rdev)
{
	struct s5m8751 *s5m8751 = rdev_get_drvdata(rdev);
	int ret;
	int volt_reg, ldo = rdev_get_id(rdev);
	u8 mask, val;

	switch (ldo) {
	case S5M8751_LDO1:
		volt_reg = S5M8751_LDO1_VSET;
		mask = S5M8751_LDO1_VSET_MASK;
		break;
	case S5M8751_LDO2:
		volt_reg = S5M8751_LDO2_VSET;
		mask = S5M8751_LDO2_VSET_MASK;
		break;
	case S5M8751_LDO5:
		volt_reg = S5M8751_LDO5_VSET;
		mask = S5M8751_LDO5_VSET_MASK;
		break;
	case S5M8751_LDO6:
		volt_reg = S5M8751_LDO6_VSET;
		mask = S5M8751_LDO6_VSET_MASK;
		break;
	default:
		return -EINVAL;
	}
	ret = s5m8751_reg_read(s5m8751, volt_reg, &val);
	if (ret)
		goto out;

	val &= mask;
	ret = s5m8751_ldo_val_to_mvolts(val) * 1000;
out:
	return ret;
}

static int s5m8751_ldo3_4_set_voltage(struct regulator_dev *rdev, int min_uV,
	int max_uV)
{
	struct s5m8751 *s5m8751 = rdev_get_drvdata(rdev);
	int ret;
	int volt_reg, ldo = rdev_get_id(rdev), mV, min_mV = min_uV / 1000,
		max_mV = max_uV / 1000;
	u8 mask, val;

	if (min_mV < 800 || min_mV > 2300)
		return -EINVAL;
	if (max_mV < 800 || max_mV > 2300)
		return -EINVAL;

	/* step size is 100mV */
	mV = ((min_mV - 701) / 100);
	if (s5m8751_ldo3_4_val_to_mvolts(mV) > max_mV)
		return -EINVAL;
	BUG_ON(s5m8751_ldo3_4_val_to_mvolts(mV) < min_mV);

	switch (ldo) {
	case S5M8751_LDO3:
		volt_reg = S5M8751_LDO3_VSET;
		mask = S5M8751_LDO3_VSET_MASK;
		break;
	case S5M8751_LDO4:
		volt_reg = S5M8751_LDO4_VSET;
		mask = S5M8751_LDO4_VSET_MASK;
		break;
	default:
		return -EINVAL;
	}
	ret = s5m8751_reg_read(s5m8751, volt_reg, &val);
	if (ret)
		goto out;

	val &= ~mask;
	ret = s5m8751_reg_write(s5m8751, volt_reg, val | mV);
out:
	return ret;
}

static int s5m8751_ldo3_4_get_voltage(struct regulator_dev *rdev)
{
	struct s5m8751 *s5m8751 = rdev_get_drvdata(rdev);
	int ret;
	int volt_reg, ldo = rdev_get_id(rdev);
	u8 mask, val;

	switch (ldo) {
	case S5M8751_LDO3:
		volt_reg = S5M8751_LDO3_VSET;
		mask = S5M8751_LDO3_VSET_MASK;
		break;
	case S5M8751_LDO4:
		volt_reg = S5M8751_LDO4_VSET;
		mask = S5M8751_LDO4_VSET_MASK;
		break;
	default:
		return -EINVAL;
	}
	ret = s5m8751_reg_read(s5m8751, volt_reg, &val);
	if (ret)
		goto out;

	val &= mask;
	ret = s5m8751_ldo3_4_val_to_mvolts(val) * 1000;
out:
	return ret;
}

static int s5m8751_ldo_enable(struct regulator_dev *rdev)
{
	struct s5m8751 *s5m8751 = rdev_get_drvdata(rdev);
	int ret;
	int ldo = rdev_get_id(rdev);
	u8 enable_reg, shift;

	if (ldo < S5M8751_LDO1 || ldo > S5M8751_LDO6)
		return -EINVAL;

	switch (ldo) {
	case S5M8751_LDO1:
		enable_reg = S5M8751_ONOFF2;
		shift = S5M8751_LDO1_SHIFT;
		break;
	case S5M8751_LDO2:
		enable_reg = S5M8751_ONOFF2;
		shift = S5M8751_LDO2_SHIFT;
		break;
	case S5M8751_LDO3:
		enable_reg = S5M8751_ONOFF2;
		shift = S5M8751_LDO3_SHIFT;
		break;
	case S5M8751_LDO4:
		enable_reg = S5M8751_ONOFF2;
		shift = S5M8751_LDO4_SHIFT;
		break;
	case S5M8751_LDO5:
		enable_reg = S5M8751_ONOFF2;
		shift = S5M8751_LDO5_SHIFT;
		break;
	case S5M8751_LDO6:
		enable_reg = S5M8751_ONOFF3;
		shift = S5M8751_LDO6_SHIFT;
		break;
	default:
		return -EINVAL;
	}
	ret = s5m8751_set_bits(s5m8751, enable_reg, 1 << shift);

	return ret;
}

static int s5m8751_ldo_disable(struct regulator_dev *rdev)
{
	struct s5m8751 *s5m8751 = rdev_get_drvdata(rdev);
	int ldo = rdev_get_id(rdev);
	int ret;
	u8 enable_reg, shift;

	if (ldo < S5M8751_LDO1 || ldo > S5M8751_LDO6)
		return -EINVAL;

	switch (ldo) {
	case S5M8751_LDO1:
		enable_reg = S5M8751_ONOFF2;
		shift = S5M8751_LDO1_SHIFT;
		break;
	case S5M8751_LDO2:
		enable_reg = S5M8751_ONOFF2;
		shift = S5M8751_LDO2_SHIFT;
		break;
	case S5M8751_LDO3:
		enable_reg = S5M8751_ONOFF2;
		shift = S5M8751_LDO3_SHIFT;
		break;
	case S5M8751_LDO4:
		enable_reg = S5M8751_ONOFF2;
		shift = S5M8751_LDO4_SHIFT;
		break;
	case S5M8751_LDO5:
		enable_reg = S5M8751_ONOFF2;
		shift = S5M8751_LDO5_SHIFT;
		break;
	case S5M8751_LDO6:
		enable_reg = S5M8751_ONOFF3;
		shift = S5M8751_LDO6_SHIFT;
		break;
	default:
		return -EINVAL;
	}
	ret = s5m8751_clear_bits(s5m8751, enable_reg, 1 << shift);

	return ret;
}

static int s5m8751_ldo_set_suspend_enable(struct regulator_dev *rdev)
{
	struct s5m8751 *s5m8751 = rdev_get_drvdata(rdev);
	int ret;
	int ldo = rdev_get_id(rdev);
	u8 enable_reg, shift;

	if (ldo < S5M8751_LDO1 || ldo > S5M8751_LDO6)
		return -EINVAL;

	switch (ldo) {
	case S5M8751_LDO1:
		enable_reg = S5M8751_SLEEP_CNTL1;
		shift = S5M8751_LDO1_SHIFT;
		break;
	case S5M8751_LDO2:
		enable_reg = S5M8751_SLEEP_CNTL1;
		shift = S5M8751_LDO2_SHIFT;
		break;
	case S5M8751_LDO3:
		enable_reg = S5M8751_SLEEP_CNTL1;
		shift = S5M8751_LDO3_SHIFT;
		break;
	case S5M8751_LDO4:
		enable_reg = S5M8751_SLEEP_CNTL1;
		shift = S5M8751_LDO4_SHIFT;
		break;
	case S5M8751_LDO5:
		enable_reg = S5M8751_SLEEP_CNTL1;
		shift = S5M8751_LDO5_SHIFT;
		break;
	case S5M8751_LDO6:
		enable_reg = S5M8751_SLEEP_CNTL2;
		shift = S5M8751_LDO6_SHIFT;
		break;
	default:
		return -EINVAL;
	}
	ret = s5m8751_set_bits(s5m8751, enable_reg, 1 << shift);

	return ret;
}

static int s5m8751_ldo_set_suspend_disable(struct regulator_dev *rdev)
{
	struct s5m8751 *s5m8751 = rdev_get_drvdata(rdev);
	int ret;
	int ldo = rdev_get_id(rdev);
	u8 enable_reg, shift;

	if (ldo < S5M8751_LDO1 || ldo > S5M8751_LDO6)
		return -EINVAL;

	switch (ldo) {
	case S5M8751_LDO1:
		enable_reg = S5M8751_SLEEP_CNTL1;
		shift = S5M8751_LDO1_SHIFT;
		break;
	case S5M8751_LDO2:
		enable_reg = S5M8751_SLEEP_CNTL1;
		shift = S5M8751_LDO2_SHIFT;
		break;
	case S5M8751_LDO3:
		enable_reg = S5M8751_SLEEP_CNTL1;
		shift = S5M8751_LDO3_SHIFT;
		break;
	case S5M8751_LDO4:
		enable_reg = S5M8751_SLEEP_CNTL1;
		shift = S5M8751_LDO4_SHIFT;
		break;
	case S5M8751_LDO5:
		enable_reg = S5M8751_SLEEP_CNTL1;
		shift = S5M8751_LDO5_SHIFT;
		break;
	case S5M8751_LDO6:
		enable_reg = S5M8751_SLEEP_CNTL2;
		shift = S5M8751_LDO6_SHIFT;
		break;
	default:
		return -EINVAL;
	}
	ret = s5m8751_clear_bits(s5m8751, enable_reg, 1 << shift);

	return ret;
}

static int s5m8751_ldo_is_enabled(struct regulator_dev *rdev)
{
	struct s5m8751 *s5m8751 = rdev_get_drvdata(rdev);
	int ret;
	int ldo = rdev_get_id(rdev), shift;
	u8 enable_reg, val;

	if (ldo < S5M8751_LDO1 || ldo > S5M8751_LDO6)
		return -EINVAL;

	switch (ldo) {
	case S5M8751_LDO1:
		enable_reg = S5M8751_ONOFF2;
		shift = S5M8751_LDO1_SHIFT;
		break;
	case S5M8751_LDO2:
		enable_reg = S5M8751_ONOFF2;
		shift = S5M8751_LDO2_SHIFT;
		break;
	case S5M8751_LDO3:
		enable_reg = S5M8751_ONOFF2;
		shift = S5M8751_LDO3_SHIFT;
		break;
	case S5M8751_LDO4:
		enable_reg = S5M8751_ONOFF2;
		shift = S5M8751_LDO4_SHIFT;
		break;
	case S5M8751_LDO5:
		enable_reg = S5M8751_ONOFF2;
		shift = S5M8751_LDO5_SHIFT;
		break;
	case S5M8751_LDO6:
		enable_reg = S5M8751_ONOFF3;
		shift = S5M8751_LDO6_SHIFT;
		break;
	default:
		return -EINVAL;
	}
	ret = s5m8751_reg_read(s5m8751, enable_reg, &val);
	if (ret)
		goto out;

	ret = val & (1 << shift);
out:
	return ret;
}

int s5m8751_dcdc_set_voltage(struct regulator_dev *rdev, int min_uV,
	int max_uV)
{
	struct s5m8751 *s5m8751 = rdev_get_drvdata(rdev);
	int ret;
	int volt_reg, dcdc = rdev_get_id(rdev), mV, min_mV = min_uV / 1000,
		max_mV = max_uV / 1000;
	u8 mask, val;

	if ((dcdc == S5M8751_BUCK1_1) || (dcdc == S5M8751_BUCK1_2)) {
		if (min_mV < 1800 || min_mV > 3300)
			return -EINVAL;
		if (max_mV < 1800 || max_mV > 3300)
			return -EINVAL;
	} else {
		if (min_mV < 500 || min_mV > 1650)
			return -EINVAL;
		if (max_mV < 500 || max_mV > 1650)
			return -EINVAL;
	}

	/* step size is 25mV */
	if ((dcdc == S5M8751_BUCK1_1) || (dcdc == S5M8751_BUCK1_2)) {
		mV = ((min_mV - 1776) / 25);
		if (s5m8751_buck1_val_to_mvolts(mV) > max_mV)
			return -EINVAL;
		BUG_ON(s5m8751_buck1_val_to_mvolts(mV) < min_mV);
	} else {
		mV = ((min_mV - 476) / 25);
		if (s5m8751_buck2_val_to_mvolts(mV) > max_mV)
			return -EINVAL;
		BUG_ON(s5m8751_buck2_val_to_mvolts(mV) < min_mV);
	}

	switch (dcdc) {
	case S5M8751_BUCK1_1:
		volt_reg = S5M8751_BUCK1_V1_SET;
		mask = S5M8751_BUCK1_V1_SET_MASK;
		break;
	case S5M8751_BUCK1_2:
		volt_reg = S5M8751_BUCK1_V2_SET;
		mask = S5M8751_BUCK1_V2_SET_MASK;
		break;
	case S5M8751_BUCK2_1:
		volt_reg = S5M8751_BUCK2_V1_SET;
		mask = S5M8751_BUCK2_V1_SET_MASK;
		break;
	case S5M8751_BUCK2_2:
		volt_reg = S5M8751_BUCK2_V2_SET;
		mask = S5M8751_BUCK2_V2_SET_MASK;
		break;
	default:
		return -EINVAL;
	}
	ret = s5m8751_reg_read(s5m8751, volt_reg, &val);
	if (ret)
		goto out;

	val &= ~mask;
	ret = s5m8751_reg_write(s5m8751, volt_reg, val | mV);

out:
	return ret;
}

static int s5m8751_dcdc_get_voltage(struct regulator_dev *rdev)
{
	struct s5m8751 *s5m8751 = rdev_get_drvdata(rdev);
	int ret;
	int volt_reg, dcdc = rdev_get_id(rdev);
	u8 mask, val;

	switch (dcdc) {
	case S5M8751_BUCK1_1:
		volt_reg = S5M8751_BUCK1_V1_SET;
		mask = S5M8751_BUCK1_V1_SET_MASK;
		break;
	case S5M8751_BUCK1_2:
		volt_reg = S5M8751_BUCK1_V2_SET;
		mask = S5M8751_BUCK1_V2_SET_MASK;
		break;
	case S5M8751_BUCK2_1:
		volt_reg = S5M8751_BUCK2_V1_SET;
		mask = S5M8751_BUCK2_V1_SET_MASK;
		break;
	case S5M8751_BUCK2_2:
		volt_reg = S5M8751_BUCK2_V2_SET;
		mask = S5M8751_BUCK2_V2_SET_MASK;
		break;
	default:
		return -EINVAL;
	}
	ret = s5m8751_reg_read(s5m8751, volt_reg, &val);
	if (ret)
		goto out;

	val &= mask;
	if ((dcdc == S5M8751_BUCK1_1) || (dcdc == S5M8751_BUCK1_2))
		ret = s5m8751_buck1_val_to_mvolts(val) * 1000;
	else
		ret = s5m8751_buck2_val_to_mvolts(val) * 1000;

out:
	return ret;
}

static int s5m8751_dcdc_enable(struct regulator_dev *rdev)
{
	struct s5m8751 *s5m8751 = rdev_get_drvdata(rdev);
	int ret;
	int dcdc = rdev_get_id(rdev);
	u8 enable_reg, shift;

	if (dcdc < S5M8751_BUCK1_1 || dcdc > S5M8751_BUCK2_2)
		return -EINVAL;

	switch (dcdc) {
	case S5M8751_BUCK1_1:
	case S5M8751_BUCK1_2:
		enable_reg = S5M8751_ONOFF3;
		shift = S5M8751_BUCK1_SHIFT;
		break;
	case S5M8751_BUCK2_1:
	case S5M8751_BUCK2_2:
		enable_reg = S5M8751_ONOFF2;
		shift = S5M8751_BUCK2_SHIFT;
		break;
	default:
		return -EINVAL;
	}
	ret = s5m8751_set_bits(s5m8751, enable_reg, 1 << shift);

	return ret;
}

static int s5m8751_dcdc_disable(struct regulator_dev *rdev)
{
	struct s5m8751 *s5m8751 = rdev_get_drvdata(rdev);
	int ret;
	int dcdc = rdev_get_id(rdev);
	u8 enable_reg, shift;

	if (dcdc < S5M8751_BUCK1_1 || dcdc > S5M8751_BUCK2_2)
		return -EINVAL;

	switch (dcdc) {
	case S5M8751_BUCK1_1:
	case S5M8751_BUCK1_2:
		enable_reg = S5M8751_ONOFF3;
		shift = S5M8751_BUCK1_SHIFT;
		break;
	case S5M8751_BUCK2_1:
	case S5M8751_BUCK2_2:
		enable_reg = S5M8751_ONOFF2;
		shift = S5M8751_BUCK2_SHIFT;
		break;
	default:
		return -EINVAL;
	}
	ret = s5m8751_clear_bits(s5m8751, enable_reg, 1 << shift);

	return ret;
}

static int s5m8751_dcdc_set_suspend_enable(struct regulator_dev *rdev)
{
	struct s5m8751 *s5m8751 = rdev_get_drvdata(rdev);
	int ret;
	int dcdc = rdev_get_id(rdev);
	u8 enable_reg, shift;

	if (dcdc < S5M8751_BUCK1_1 || dcdc > S5M8751_BUCK2_2)
		return -EINVAL;

	switch (dcdc) {
	case S5M8751_BUCK1_1:
	case S5M8751_BUCK1_2:
		enable_reg = S5M8751_SLEEP_CNTL2;
		shift = S5M8751_BUCK1_SHIFT;
		break;
	case S5M8751_BUCK2_1:
	case S5M8751_BUCK2_2:
		enable_reg = S5M8751_SLEEP_CNTL2;
		shift = S5M8751_BUCK2_SHIFT;
		break;
	default:
		return -EINVAL;
	}
	ret = s5m8751_set_bits(s5m8751, enable_reg, 1 << shift);

	return ret;
}

static int s5m8751_dcdc_set_suspend_disable(struct regulator_dev *rdev)
{
	struct s5m8751 *s5m8751 = rdev_get_drvdata(rdev);
	int ret;
	int dcdc = rdev_get_id(rdev);
	u8 enable_reg, shift;

	if (dcdc < S5M8751_BUCK1_1 || dcdc > S5M8751_BUCK2_2)
		return -EINVAL;

	switch (dcdc) {
	case S5M8751_BUCK1_1:
	case S5M8751_BUCK1_2:
		enable_reg = S5M8751_SLEEP_CNTL2;
		shift = S5M8751_BUCK1_SHIFT;
		break;
	case S5M8751_BUCK2_1:
	case S5M8751_BUCK2_2:
		enable_reg = S5M8751_SLEEP_CNTL2;
		shift = S5M8751_BUCK2_SHIFT;
		break;
	default:
		return -EINVAL;
	}
	ret = s5m8751_clear_bits(s5m8751, enable_reg, 1 << shift);

	return ret;
}

static int s5m8751_dcdc_is_enabled(struct regulator_dev *rdev)
{
	struct s5m8751 *s5m8751 = rdev_get_drvdata(rdev);
	int ret;
	int dcdc = rdev_get_id(rdev), shift;
	u8 enable_reg, val;

	if (dcdc < S5M8751_BUCK1_1 || dcdc > S5M8751_BUCK2_2)
		return -EINVAL;

	switch (dcdc) {
	case S5M8751_BUCK1_1:
	case S5M8751_BUCK1_2:
		enable_reg = S5M8751_ONOFF3;
		shift = S5M8751_BUCK1_SHIFT;
		break;
	case S5M8751_BUCK2_1:
	case S5M8751_BUCK2_2:
		enable_reg = S5M8751_ONOFF3;
		shift = S5M8751_BUCK2_SHIFT;
		break;
	default:
		return -EINVAL;
	}
	ret = s5m8751_reg_read(s5m8751, enable_reg, &val);
	if (ret)
		goto out;

	ret = val & (1 << shift);
out:
	return ret;
}

static struct regulator_ops s5m8751_ldo_ops = {
	.set_voltage = s5m8751_ldo_set_voltage,
	.get_voltage = s5m8751_ldo_get_voltage,
	.enable = s5m8751_ldo_enable,
	.disable = s5m8751_ldo_disable,
	.is_enabled = s5m8751_ldo_is_enabled,
	.set_suspend_enable = s5m8751_ldo_set_suspend_enable,
	.set_suspend_disable = s5m8751_ldo_set_suspend_disable,
};

static struct regulator_ops s5m8751_ldo3_4_ops = {
	.set_voltage = s5m8751_ldo3_4_set_voltage,
	.get_voltage = s5m8751_ldo3_4_get_voltage,
	.enable = s5m8751_ldo_enable,
	.disable = s5m8751_ldo_disable,
	.is_enabled = s5m8751_ldo_is_enabled,
	.set_suspend_enable = s5m8751_ldo_set_suspend_enable,
	.set_suspend_disable = s5m8751_ldo_set_suspend_disable,
};

static struct regulator_ops s5m8751_buck_ops = {
	.set_voltage = s5m8751_dcdc_set_voltage,
	.get_voltage = s5m8751_dcdc_get_voltage,
	.enable = s5m8751_dcdc_enable,
	.disable = s5m8751_dcdc_disable,
	.is_enabled = s5m8751_dcdc_is_enabled,
	.set_suspend_enable = s5m8751_dcdc_set_suspend_enable,
	.set_suspend_disable = s5m8751_dcdc_set_suspend_disable,
};

static struct regulator_desc s5m8751_reg[NUM_S5M8751_REGULATORS] = {
	{
		.name = "LDO1",
		.id = S5M8751_LDO1,
		.ops = &s5m8751_ldo_ops,
		.type = REGULATOR_VOLTAGE,
		.owner = THIS_MODULE,
	},
	{
		.name = "LDO2",
		.id = S5M8751_LDO2,
		.ops = &s5m8751_ldo_ops,
		.type = REGULATOR_VOLTAGE,
		.owner = THIS_MODULE,
	},
	{
		.name = "LDO3",
		.id = S5M8751_LDO3,
		.ops = &s5m8751_ldo3_4_ops,
		.type = REGULATOR_VOLTAGE,
		.owner = THIS_MODULE,
	},
	{
		.name = "LDO4",
		.id = S5M8751_LDO4,
		.ops = &s5m8751_ldo3_4_ops,
		.type = REGULATOR_VOLTAGE,
		.owner = THIS_MODULE,
	},
	{
		.name = "LDO5",
		.id = S5M8751_LDO5,
		.ops = &s5m8751_ldo_ops,
		.type = REGULATOR_VOLTAGE,
		.owner = THIS_MODULE,
	},
	{
		.name = "LDO6",
		.id = S5M8751_LDO6,
		.ops = &s5m8751_ldo_ops,
		.type = REGULATOR_VOLTAGE,
		.owner = THIS_MODULE,
	},
	{
		.name = "BUCK1_1",
		.id = S5M8751_BUCK1_1,
		.ops = &s5m8751_buck_ops,
		.type = REGULATOR_VOLTAGE,
		.owner = THIS_MODULE,
	},
	{
		.name = "BUCK1_2",
		.id = S5M8751_BUCK1_2,
		.ops = &s5m8751_buck_ops,
		.type = REGULATOR_VOLTAGE,
		.owner = THIS_MODULE,
	},
	{
		.name = "BUCK2_1",
		.id = S5M8751_BUCK2_1,
		.ops = &s5m8751_buck_ops,
		.type = REGULATOR_VOLTAGE,
		.owner = THIS_MODULE,
	},
	{
		.name = "BUCK2_2",
		.id = S5M8751_BUCK2_2,
		.ops = &s5m8751_buck_ops,
		.type = REGULATOR_VOLTAGE,
		.owner = THIS_MODULE,
	},
};

static int s5m8751_regulator_probe(struct platform_device *pdev)
{
	struct regulator_dev *rdev;

	if (pdev->id < S5M8751_LDO1 || pdev->id > S5M8751_BUCK2_2)
		return -ENODEV;

	/* register regulator */
	rdev = regulator_register(&s5m8751_reg[pdev->id], &pdev->dev,
				pdev->dev.platform_data,
				dev_get_drvdata(&pdev->dev));
	if (IS_ERR(rdev)) {
		dev_err(&pdev->dev, "failed to register %s\n",
			s5m8751_reg[pdev->id].name);
		return PTR_ERR(rdev);
	}

	return 0;
}

static int s5m8751_regulator_remove(struct platform_device *pdev)
{
	struct regulator_dev *rdev = platform_get_drvdata(pdev);

	regulator_unregister(rdev);

	return 0;
}

int s5m8751_register_regulator(struct s5m8751 *s5m8751, int reg,
			      struct regulator_init_data *initdata)
{
	struct platform_device *pdev;
	int ret;

	if (s5m8751->pmic.pdev[reg])
		return -EBUSY;

	pdev = platform_device_alloc("s5m8751-regulator", reg);
	if (!pdev)
		return -ENOMEM;

	s5m8751->pmic.pdev[reg] = pdev;
	initdata->driver_data = s5m8751;

	pdev->dev.platform_data = initdata;
	pdev->dev.parent = s5m8751->dev;
	platform_set_drvdata(pdev, s5m8751);

	ret = platform_device_add(pdev);
	if (ret != 0) {
		dev_err(s5m8751->dev, "Failed to register regulator %d: %d\n",
			reg, ret);
		platform_device_del(pdev);
		s5m8751->pmic.pdev[reg] = NULL;
	}
	return ret;
}
EXPORT_SYMBOL(s5m8751_register_regulator);

static struct platform_driver s5m8751_regulator_driver = {
	.probe = s5m8751_regulator_probe,
	.remove = s5m8751_regulator_remove,
	.driver		= {
		.name	= "s5m8751-regulator",
	},
};

static int __init s5m8751_regulator_init(void)
{
	return platform_driver_register(&s5m8751_regulator_driver);
}
module_init(s5m8751_regulator_init);

static void __exit s5m8751_regulator_exit(void)
{
	platform_driver_unregister(&s5m8751_regulator_driver);
}
module_exit(s5m8751_regulator_exit);

/* Module Information */
MODULE_DESCRIPTION("S5M8751 Regulator driver");
MODULE_AUTHOR("Jongbin, Won <jongbin.won@samsung.com>");
MODULE_LICENSE("GPL");
