/*
 * FreeRTOS Kernel V10.0.0
 * Copyright (C) 2017 Amazon.com, Inc. or its affiliates.  All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software. If you wish to use our Amazon
 * FreeRTOS name, please do so in a fair use way that does not cause confusion.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * http://www.FreeRTOS.org
 * http://aws.amazon.com/freertos
 *
 * 1 tab == 4 spaces!
 */

/*
 * Normally the interrupt nesting test would use [at least] two separate timers.
 * In this case, there was difficulty generating interrupts from TC1, so only
 * TC0 is used.  Nested interrupts are instead generated by manually pending the
 * TC1 interrupt from inside the TC0 interrupt handler.  This means TC1 must be
 * assigned an interrupt priority above TC0.  [Note this arrangement does not
 * really fulfil the purpose of the test as the nesting always occurs at the
 * same point in the code, whereas the test is designed to test nesting
 * occurring within the queue API functions]
 */

/* Scheduler includes. */
#include "FreeRTOS.h"

/* Demo includes. */
#include "IntQueueTimer.h"
#include "IntQueue.h"

/* Library includes. */
#include "board.h"
#include "asf.h"

/* The frequency at which the int queue timer executes. */
#define tmrTIMER_0_FREQUENCY	( 2030UL )

/* The genuine timer interrupt executes at the lower priority.  The manually
pended timer interrupt executes at the higher priority to ensure it always nests
with the lower priority (which in turn will occasionally nest with the tick
interrupt - creating a maximum interrupt nesting depth of 3). */
#define tmrLOWER_PRIORITY		( configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY + 1 )
#define tmrHIGHER_PRIORITY		( configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY )

/*-----------------------------------------------------------*/

void vInitialiseTimerForIntQueueTest( void )
{
uint32_t ulDivider, ulTCCLKS;

	/* Configure PMC for TC0. */
	pmc_enable_periph_clk( ID_TC0 );

	/* Configure TC0 channel 0 for interrupts at tmrTIMER_0_FREQUENCY. */
	tc_find_mck_divisor( tmrTIMER_0_FREQUENCY, configCPU_CLOCK_HZ, &ulDivider, &ulTCCLKS, configCPU_CLOCK_HZ );
	tc_init( TC0, 0, ulTCCLKS | TC_CMR_CPCTRG );
	ulDivider <<= 1UL;
	tc_write_rc( TC0, 0, ( configCPU_CLOCK_HZ / ulDivider ) / tmrTIMER_0_FREQUENCY );
	tc_enable_interrupt( TC0, 0, TC_IER_CPCS );

	/* Configure and enable interrupts for both TC0 and TC1, as TC1 interrupts
	are manually pended from within the TC0 interrupt handler (see the notes at
	the top of this file). */
	NVIC_ClearPendingIRQ( TC0_IRQn );
	NVIC_ClearPendingIRQ( TC1_IRQn );
	NVIC_SetPriority( TC0_IRQn, tmrLOWER_PRIORITY );
	NVIC_SetPriority( TC1_IRQn, tmrHIGHER_PRIORITY );
	NVIC_EnableIRQ( TC0_IRQn );
	NVIC_EnableIRQ( TC1_IRQn );

	/* Start the timer last of all. */
	tc_start( TC0, 0 );
}
/*-----------------------------------------------------------*/

void TC0_Handler( void )
{
static uint32_t ulISRCount = 0;

	/* Clear the interrupt. */
	if( tc_get_status( TC0, 0 ) != 0 )
	{
		/* As noted in the comments above, manually pend the TC1 interrupt from
		the TC0 interrupt handler.  This is not done on each occurrence of the
		TC0 interrupt though, to make sure interrupts don't nest in every single
		case. */
		ulISRCount++;
		if( ( ulISRCount & 0x07 ) != 0x07 )
		{
			/* Pend an interrupt that will nest with this interrupt. */
			NVIC_SetPendingIRQ( TC1_IRQn );
		}

		/* Call the IntQ test function for this channel. */
		portYIELD_FROM_ISR( xFirstTimerHandler() );
	}
}
/*-----------------------------------------------------------*/

void TC1_Handler( void )
{
	/* Call the IntQ test function that would normally get called from a second
	and independent timer. */
	portYIELD_FROM_ISR( xSecondTimerHandler() );
}

