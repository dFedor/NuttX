/****************************************************************************
 * arch/arm/src/lpc31xx/lpc31_timerisr.c
 *
 *   Copyright (C) 2009 Gregory Nutt. All rights reserved.
 *   Author: Gregory Nutt <gnutt@nuttx.org>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 * 3. Neither the name NuttX nor the names of its contributors may be
 *    used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <stdint.h>
#include <time.h>
#include <debug.h>

#include <nuttx/arch.h>
#include <arch/board/board.h>

#include "clock/clock.h"
#include "up_internal.h"
#include "up_arch.h"

#include "lpc31_timer.h"
#include "lpc31_internal.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/****************************************************************************
 * Private Types
 ****************************************************************************/

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

/****************************************************************************
 * Global Functions
 ****************************************************************************/

/****************************************************************************
 * Function:  up_timerisr
 *
 * Description:
 *   The timer ISR will perform a variety of services for various portions
 *   of the systems.
 *
 ****************************************************************************/

int up_timerisr(int irq, uint32_t *regs)
{
  /* Clear the lattched timer interrupt (Writing any value to the CLEAR register
   * clears the interrupt generated by the counter timer
   */

  putreg32(1, LPC31_TIMER0_CLEAR);

  /* Process timer interrupt */

  sched_process_timer();
  return 0;
}

/****************************************************************************
 * Function:  up_timer_initialize
 *
 * Description:
 *   This function is called during start-up to initialize
 *   the timer interrupt.
 *
 ****************************************************************************/

void up_timer_initialize(void)
{
  uint32_t regval;
  uint64_t load;
  uint64_t freq;

  /* Enable the timer0 system clock */

  lpc31_enableclock(CLKID_TIMER0PCLK);

  /* Soft reset the timer0 module so that we start in a known state */

  lpc31_softreset(RESETID_TIMER0RST);

  /* Set timer load register to 10mS (100Hz).  First, get the frequency
   * of the timer0 module clock (in the AHB0APB1_BASE domain (2)).
   */

  freq = (uint64_t)lpc31_clkfreq(CLKID_TIMER0PCLK, DOMAINID_AHB0APB1);

  /* If the clock is >1MHz, use pre-dividers */

  regval = getreg32(LPC31_TIMER0_CTRL);
  if (freq > 1000000)
  {
    /* Use the divide by 16 pre-divider */

    regval &= ~TIMER_CTRL_PRESCALE_MASK;
    regval |=  TIMER_CTRL_PRESCALE_DIV16;
    freq   >>= 4;
  }

  load =((freq * (uint64_t)10000) / 1000000);
  putreg32((uint32_t)load, LPC31_TIMER0_LOAD);

  /* Set periodic mode */

  regval |= TIMER_CTRL_PERIODIC;
  putreg32(regval, LPC31_TIMER0_CTRL);

  /* Attach the timer interrupt vector */

  (void)irq_attach(LPC31_IRQ_TMR0, (xcpt_t)up_timerisr);

   /* Clear any latched timer interrupt (Writing any value to the CLEAR register
   * clears the latched interrupt generated by the counter timer)
   */

  putreg32(1, LPC31_TIMER0_CLEAR);

  /* Enable timers (starts counting) */

  regval |= TIMER_CTRL_ENABLE;
  putreg32(regval, LPC31_TIMER0_CTRL);

  /* Enable timer match interrupts in the interrupt controller */

  up_enable_irq(LPC31_IRQ_TMR0);
}