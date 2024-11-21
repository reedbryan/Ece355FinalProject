//
// This file is part of the GNU ARM Eclipse distribution.
// Copyright (c) 2014 Liviu Ionescu.
//

// ----------------------------------------------------------------------------
// School: University of Victoria, Canada.
// Course: ECE 355 "Microprocessor-Based Systems".
// This is tutorial code for Part 1 of Introductory Lab.
//
// See "system/include/cmsis/stm32f051x8.h" for register/bit definitions.
// See "system/src/cmsis/vectors_stm32f051x8.c" for handler declarations.
// ----------------------------------------------------------------------------

#include <stdio.h>
#include "diag/Trace.h"
#include "cmsis/cmsis_device.h"
#include "stm32f051x8.h"
#include "stm32f0xx_hal.h"

// ----------------------------------------------------------------------------
//
// STM32F0 empty sample (trace via $(trace)).
//
// Trace support is enabled by adding the TRACE macro definition.
// By default the trace messages are forwarded to the $(trace) output,
// but can be rerouted to any device or completely suppressed, by
// changing the definitions required in system/src/diag/trace_impl.c
// (currently OS_USE_TRACE_ITM, OS_USE_TRACE_SEMIHOSTING_DEBUG/_STDOUT).
//

// ----- main() ---------------------------------------------------------------

// Sample pragmas to cope with warnings. Please note the related line at
// the end of this function, used to pop the compiler diagnostics status.
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wmissing-declarations"
#pragma GCC diagnostic ignored "-Wreturn-type"


/* Definitions of registers and their bits are
   given in system/include/cmsis/stm32f051x8.h */

/* ADC defs */
#define V_REF 3.3
#define POT_MAX_RESISTANCE 5000.0

/* Clock prescaler for TIM2 timer: no prescaling */
#define myTIM2_PRESCALER ((uint16_t)0x0000)
/* Delay count for TIM2 timer: 1/4 sec at 48 MHz */
#define myTIM2_PERIOD ((uint32_t)12000000)


void myGPIOC_Init(void);
void ADC_Init(void);
void ADC_Enable(void);
float Read_ADC_Voltage(void);
float Calculate_Potentiomter_Resistance(float voltage);


/* Global variable indicating which LED is blinking */
volatile uint16_t blinkingLED = ((uint16_t)0x0100);


/*** Call this function to boost the STM32F0xx clock to 48 MHz ***/

void SystemClock48MHz( void )
{

//
// Disable the PLL
//
    RCC->CR &= ~(RCC_CR_PLLON);

//
// Wait for the PLL to unlock
//
    while (( RCC->CR & RCC_CR_PLLRDY ) != 0 );

//
// Configure the PLL for a 48MHz system clock
//
    RCC->CFGR = 0x00280000;

//
// Enable the PLL
//
    RCC->CR |= RCC_CR_PLLON;

//
// Wait for the PLL to lock
//
    while (( RCC->CR & RCC_CR_PLLRDY ) != RCC_CR_PLLRDY );

//
// Switch the processor to the PLL clock source
//
    RCC->CFGR = ( RCC->CFGR & (~RCC_CFGR_SW_Msk)) | RCC_CFGR_SW_PLL;

//
// Update the system with the new clock frequency
//
    SystemCoreClockUpdate();

}

/*****************************************************************/


int
main(int argc, char* argv[])
{
	SystemClock48MHz();
	ADC_Init();

	while (1)
	{
		float voltage = Read_ADC_Voltage();

		float resistance = Calculate_Potentiomter_Resistance(voltage);

		int res = (int)resistance;
		trace_printf("ADC Voltage: %dmV\n", (int)(voltage * 1000));
		trace_printf("ADC Resistance: %d\n", res);
	}

	return 0;

}

void ADC_Init(void){
	// Enable ADC clock
	RCC->APB2ENR |= RCC_APB2ENR_ADC1EN;

	// Configure the ADC
	ADC1->CR |= ADC_CR_ADEN;
	// wait for ADRDY to be set
	while(!(ADC1->ISR & ADC_ISR_ADRDY)){}

	ADC1->CHSELR = ADC_CHSELR_CHSEL5;	// select channel 5 for PA5
	ADC1->CFGR1 &= ~( (1UL << 3) | (1UL << 2));		// set 12-bit resolution
}
float Read_ADC_Voltage(void){
	ADC1->CR |= ADC_CR_ADSTART;	// start ADC conversion
	while (!(ADC1->ISR & ADC_ISR_EOC)); //  wait for conversion to complete

	uint32_t adcValue = ADC1->DR;
	return (adcValue / 4095.0) * V_REF; // divide by 2^12-1 because its 12-bit
}
float Calculate_Potentiomter_Resistance(float voltage) {
	// Ohm's Law: R = V / (V_REF - V) * R_total_res
	if (voltage < V_REF){
		return (voltage / (V_REF - voltage) * POT_MAX_RESISTANCE);
	} else{
		return 0;
	}
}

#pragma GCC diagnostic pop

// ----------------------------------------------------------------------------
