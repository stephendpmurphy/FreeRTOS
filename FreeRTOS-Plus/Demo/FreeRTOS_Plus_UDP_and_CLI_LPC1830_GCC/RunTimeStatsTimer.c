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

/* FreeRTOS includes. */
#include "FreeRTOS.h"

/* Utility functions to implement run time stats on Cortex-M CPUs.  The collected
run time data can be viewed through the CLI interface.  See the following URL for
more information on run time stats:
http://www.freertos.org/rtos-run-time-stats.html */

/* Addresses of registers in the Cortex-M debug hardware. */
#define rtsDWT_CYCCNT 			( *( ( unsigned long * ) 0xE0001004 ) )
#define rtsDWT_CONTROL 			( *( ( unsigned long * ) 0xE0001000 ) )
#define rtsSCB_DEMCR 			( *( ( unsigned long * ) 0xE000EDFC ) )
#define rtsTRCENA_BIT			( 0x01000000UL )
#define rtsCOUNTER_ENABLE_BIT	( 0x01UL )

/* Simple shift divide for scaling to avoid an overflow occurring too soon.  The
number of bits to shift depends on the clock speed. */
#define runtimeSLOWER_CLOCK_SPEEDS	( 70000000UL )
#define runtimeSHIFT_13				13
#define runtimeOVERFLOW_BIT_13		( 1UL << ( 32UL - runtimeSHIFT_13 ) )
#define runtimeSHIFT_14				14
#define runtimeOVERFLOW_BIT_14		( 1UL << ( 32UL - runtimeSHIFT_14 ) )

/*-----------------------------------------------------------*/

void vMainConfigureTimerForRunTimeStats( void )
{
	/* Enable TRCENA. */
	rtsSCB_DEMCR = rtsSCB_DEMCR | rtsTRCENA_BIT;

	/* Reset counter. */
	rtsDWT_CYCCNT = 0;

	/* Enable counter. */
	rtsDWT_CONTROL = rtsDWT_CONTROL | rtsCOUNTER_ENABLE_BIT;
}
/*-----------------------------------------------------------*/

uint32_t ulMainGetRunTimeCounterValue( void )
{
static unsigned long ulLastCounterValue = 0UL, ulOverflows = 0;
unsigned long ulValueNow;

	ulValueNow = rtsDWT_CYCCNT;

	/* Has the value overflowed since it was last read. */
	if( ulValueNow < ulLastCounterValue )
	{
		ulOverflows++;
	}
	ulLastCounterValue = ulValueNow;

	/* Cannot use configCPU_CLOCK_HZ directly as it may itself not be a constant
	but instead map to a variable that holds the clock speed. */

	/* There is no prescale on the counter, so simulate in software. */
	if( configCPU_CLOCK_HZ < runtimeSLOWER_CLOCK_SPEEDS )
	{
		ulValueNow >>= runtimeSHIFT_13;
		ulValueNow += ( runtimeOVERFLOW_BIT_13 * ulOverflows );
	}
	else
	{
		ulValueNow >>= runtimeSHIFT_14;
		ulValueNow += ( runtimeOVERFLOW_BIT_14 * ulOverflows );
	}

	return ulValueNow;
}
/*-----------------------------------------------------------*/

