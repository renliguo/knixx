/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2018 Bart Van Der Meerssche <bart@flukso.net>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:

 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.

 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include <libopencmsis/core_cm3.h>
#include <libopencm3/cm3/nvic.h>
#include <libopencm3/cm3/systick.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/flash.h>
#include <libopencm3/stm32/gpio.h>
#include "clock.h"
#include "usb.h"
#include "cli.h"
#include "button.h"
#include "ncn.h"
#include "config.h"
#include "pwm.h"
#include "led.h"

static void rcc_wait_for_osc_not_ready(enum rcc_osc osc)
{
	while (rcc_is_osc_ready(osc));
}

static void rcc_set_pll_source(enum rcc_osc osc)
{
	switch (osc) {
	case RCC_HSI:
		RCC_CFGR &= ~RCC_CFGR_PLLSRC;
		break;
	case RCC_HSE:
		RCC_CFGR |= RCC_CFGR_PLLSRC;;
		break;
	case RCC_LSE:
	case RCC_HSI48:
	case RCC_HSI14:
	case RCC_LSI:
	case RCC_PLL:
		/* Do nothing */
		break;
	}
}

static void clk_tree_setup(void)
{
	/* cf. rcc_clock_setup_in_hsi_out_48mhz */
	rcc_osc_on(RCC_HSI);
	rcc_wait_for_osc_ready(RCC_HSI);
	rcc_set_sysclk_source(RCC_HSI);
	rcc_osc_on(RCC_HSE);
	rcc_wait_for_osc_ready(RCC_HSE);
	rcc_osc_off(RCC_PLL);
	rcc_wait_for_osc_not_ready(RCC_PLL);
	flash_set_ws(FLASH_ACR_LATENCY_024_048MHZ);
#ifdef NUCLEO
	rcc_set_pll_multiplication_factor(RCC_CFGR_PLLMUL_MUL6);
#else
	/* 16MHz * 3 = 48MHz */
	rcc_set_pll_multiplication_factor(RCC_CFGR_PLLMUL_MUL3);
#endif
	rcc_set_pll_source(RCC_HSE);
	rcc_osc_on(RCC_PLL);
	rcc_wait_for_osc_ready(RCC_PLL);
	rcc_set_sysclk_source(RCC_PLL);
	rcc_set_hpre(RCC_CFGR_HPRE_NODIV);
	rcc_set_ppre(RCC_CFGR_PPRE_NODIV);
	rcc_apb1_frequency = 48000000UL;
	rcc_ahb_frequency = 48000000UL;
	rcc_set_usbclk_source(RCC_PLL);
}

static void mco_setup(void)
{
	rcc_periph_clock_enable(RCC_GPIOA);
	gpio_mode_setup(GPIOA, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO8);
	gpio_set_output_options(GPIOA, GPIO_OTYPE_PP, GPIO_OSPEED_100MHZ, GPIO8);
	gpio_set_af(GPIOA, GPIO_AF0, GPIO8);
	rcc_set_mco(RCC_CFGR_MCO_HSE);
}

int main(void)
{
	cm_disable_interrupts();
	clk_tree_setup();
	mco_setup();
	clock_setup();
	usb_setup();
	cli_setup();
	pwm_init();
	led_setup();
	button_setup();
	ncn_setup();
	cm_enable_interrupts();

	while (1) {
		__WFI();
	}

	return 0;
}
