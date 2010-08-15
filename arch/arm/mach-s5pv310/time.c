/* linux/arch/arm/mach-s5pv310/time.c
 *
 * Copyright (c) 2010 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com
 *
 * S5PV310 (and compatible) HRT support
 * System timer is used for this feature
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/err.h>
#include <linux/clk.h>
#include <linux/clockchips.h>

#include <asm/smp_twd.h>

#include <mach/regs-systimer.h>
#include <asm/mach/time.h>

enum systimer_type {
	SYS_FRC,
	SYS_TICK,
};

static unsigned long clock_count_per_tick;

static struct clk *timerclk;

static void s5pv310_systimer_write(unsigned int value,	void *reg_offset)
{
	unsigned int temp_regs;

	__raw_writel(value, reg_offset);

	if (reg_offset == S5PV310_TCON) {
		while(!(__raw_readl(S5PV310_INT_CSTAT) & 1<<7));

		temp_regs = __raw_readl(S5PV310_INT_CSTAT);
		temp_regs |= 1<<7;
		__raw_writel(temp_regs, S5PV310_INT_CSTAT);

	} else if (reg_offset == S5PV310_TICNTB) {
		while(!(__raw_readl(S5PV310_INT_CSTAT) & 1<<3));

		temp_regs = __raw_readl(S5PV310_INT_CSTAT);
		temp_regs |= 1<<3;
		__raw_writel(temp_regs, S5PV310_INT_CSTAT);

	} else if (reg_offset == S5PV310_FRCNTB) {
		while(!(__raw_readl(S5PV310_INT_CSTAT) & 1<<6));

		temp_regs = __raw_readl(S5PV310_INT_CSTAT);
		temp_regs |= 1<<6;
		__raw_writel(temp_regs, S5PV310_INT_CSTAT);
	}
}

static void s5pv310_systimer_stop(enum systimer_type type)
{
	unsigned long tcon;

	tcon = __raw_readl(S5PV310_TCON);

	switch (type) {
	case SYS_FRC:
		tcon &= ~S5PV310_TCON_FRC_START;
		break;

	case SYS_TICK:
		tcon &= ~(S5PV310_TCON_TICK_START | S5PV310_TCON_TICK_INT_START);
		break;

	default:
		break;
	}

	s5pv310_systimer_write(tcon, S5PV310_TCON);
}

static void s5pv310_systimer_init(enum systimer_type type, unsigned long tcnt)
{
	/* timers reload after counting zero, so reduce the count by 1 */
	tcnt--;

	s5pv310_systimer_stop(type);

	switch (type) {
	case SYS_FRC:
		s5pv310_systimer_write(tcnt, S5PV310_FRCNTB);
		break;

	case SYS_TICK:
		s5pv310_systimer_write(tcnt, S5PV310_TICNTB);
		break;

	default:
		break;
	}
}

static inline void s5pv310_systimer_start(enum systimer_type type)
{
	unsigned long tcon;
	unsigned long tcfg;

	tcon = __raw_readl(S5PV310_TCON);

	switch (type) {
	case SYS_FRC:
		tcon |= S5PV310_TCON_FRC_START;
		break;
	case SYS_TICK:
		tcfg = __raw_readl(S5PV310_TCFG);
		tcfg |= S5PV310_TCFG_TICK_SWRST;
		s5pv310_systimer_write(tcfg, S5PV310_TCFG);

		tcon |= S5PV310_TCON_TICK_START | S5PV310_TCON_TICK_INT_START;
		break;
	default:
		break;
	}
	s5pv310_systimer_write(tcon, S5PV310_TCON);

}

static int s5pv310_systimer_set_next_event(unsigned long cycles,
					struct clock_event_device *evt)
{
	s5pv310_systimer_init(SYS_TICK, cycles);
	s5pv310_systimer_start(SYS_TICK);
	return 0;
}

static void s5pv310_systimer_set_mode(enum clock_event_mode mode,
				struct clock_event_device *evt)
{
	s5pv310_systimer_stop(SYS_TICK);

	switch (mode) {
	case CLOCK_EVT_MODE_PERIODIC:
		s5pv310_systimer_init(SYS_TICK, clock_count_per_tick);
		s5pv310_systimer_start(SYS_TICK);
		break;
	case CLOCK_EVT_MODE_ONESHOT:
	case CLOCK_EVT_MODE_UNUSED:
	case CLOCK_EVT_MODE_SHUTDOWN:
	case CLOCK_EVT_MODE_RESUME:
		break;
	}
}

static struct clock_event_device sys_tick_event_device = {
	.name		= "sys_tick",
	.features       = CLOCK_EVT_FEAT_PERIODIC | CLOCK_EVT_FEAT_ONESHOT,
	.rating		= 360,
	.shift		= 32,
	.set_next_event	= s5pv310_systimer_set_next_event,
	.set_mode	= s5pv310_systimer_set_mode,
};

irqreturn_t s5pv310_clock_event_isr(int irq, void *dev_id)
{
	struct clock_event_device *evt = &sys_tick_event_device;
	unsigned long int_status;

	if (evt->mode != CLOCK_EVT_MODE_PERIODIC)
		s5pv310_systimer_stop(SYS_TICK);

	/* Clear the system tick interrupt */
	int_status = __raw_readl(S5PV310_INT_CSTAT);
	int_status |= S5PV310_INT_TICK_STATUS;
	s5pv310_systimer_write(int_status, S5PV310_INT_CSTAT);

	evt->event_handler(evt);

	return IRQ_HANDLED;
}

static struct irqaction s5pv310_clock_event_irq = {
	.name		= "sys_tick_irq",
	.flags		= IRQF_DISABLED | IRQF_TIMER | IRQF_IRQPOLL,
	.handler	= s5pv310_clock_event_isr,
};

static void __init s5pv310_clockevent_init(void)
{
	unsigned long clock_rate;

	clock_rate = clk_get_rate(timerclk);

	clock_count_per_tick = clock_rate / HZ;

	sys_tick_event_device.mult =
		div_sc(clock_rate, NSEC_PER_SEC, sys_tick_event_device.shift);
	sys_tick_event_device.max_delta_ns =
		clockevent_delta2ns(-1, &sys_tick_event_device);
	sys_tick_event_device.min_delta_ns =
		clockevent_delta2ns(0x1, &sys_tick_event_device);

	sys_tick_event_device.cpumask = cpumask_of(0);
	clockevents_register_device(&sys_tick_event_device);

	s5pv310_systimer_write(S5PV310_INT_TICK_EN | S5PV310_INT_EN, S5PV310_INT_CSTAT);
	setup_irq(IRQ_SYSTEM_TIMER, &s5pv310_clock_event_irq);
}

static cycle_t s5pv310_frc_read(struct clocksource *cs)
{
	return (cycle_t) ~__raw_readl(S5PV310_FRCNTO);
}

struct clocksource sys_frc_clksrc = {
	.name		= "sys_frc",
	.rating		= 250,
	.read		= s5pv310_frc_read,
	.mask		= CLOCKSOURCE_MASK(32),
	.shift		= 20,
	.flags		= CLOCK_SOURCE_IS_CONTINUOUS ,
};

static void __init s5pv310_clocksource_init(void)
{
	unsigned long clock_rate;

	clock_rate = clk_get_rate(timerclk);

	sys_frc_clksrc.mult =
		clocksource_khz2mult(clock_rate/1000, sys_frc_clksrc.shift);

	s5pv310_systimer_init(SYS_FRC, 0);
	s5pv310_systimer_start(SYS_FRC);

	if (clocksource_register(&sys_frc_clksrc))
		panic("%s: can't register clocksource\n", sys_frc_clksrc.name);
}

static void __init s5pv310_timer_resources(void)
{
#ifndef CONFIG_S5PV310_FPGA
	unsigned long tcfg;

	tcfg = __raw_readl(S5PV310_TCFG);
	tcfg &= ~S5PV310_TCFG_CLKBASE_MASK;
	tcfg |= S5PV310_TCFG_CLKBASE_SYS_MAIN;
	s5pv310_systimer_write(tcfg, S5PV310_TCFG);

	timerclk = clk_get(NULL, "ext_xtal");

	if (IS_ERR(timerclk))
		panic("failed to get ext_xtal clock for system timer");

	clk_enable(timerclk);
#else
	timerclk = clk_get(NULL, "xtal");
#endif
}

static void __init s5pv310_timer_init(void)
{
#ifdef CONFIG_LOCAL_TIMERS
	twd_base = S5P_VA_TWD;
#endif

	s5pv310_timer_resources();
	s5pv310_clockevent_init();
	s5pv310_clocksource_init();
}

struct sys_timer s5pv310_timer = {
	.init		= s5pv310_timer_init,
};
