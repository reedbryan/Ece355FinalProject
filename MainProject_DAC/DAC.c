#include <stdio.h>
#include <unistd.h>
#include "diag/Trace.h"
#include "cmsis/cmsis_device.h"
#include "stm32f051x8.h"
#include "stm32f0xx_hal.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wmissing-declarations"
#pragma GCC diagnostic ignored "-Wreturn-type"

/* Definitions of registers and their bits are
   given in system/include/cmsis/stm32f051x8.h */

/* ADC defs */
#define V_REF 3.3
#define POT_MAX_RESISTANCE 5000.0

void myGPIOC_Init(void);
void ADC_Init(void);
void DAC_Init(void);
uint16_t Convert_Voltage_To_DAC(float voltage);
float Read_ADC_Voltage(void);
float Calculate_Potentiomter_Resistance(float voltage);


/* Global variable indicating which LED is blinking */
volatile uint16_t blinkingLED = ((uint16_t)0x0100);


/*** Call this function to boost the STM32F0xx clock to 48 MHz ***/
void SystemClock48MHz( void )
{
// Disable the PLL
   RCC->CR &= ~(RCC_CR_PLLON);


// Wait for the PLL to unlock
   while (( RCC->CR & RCC_CR_PLLRDY ) != 0 );


// Configure the PLL for a 48MHz system clock
   RCC->CFGR = 0x00280000;


// Enable the PLL
   RCC->CR |= RCC_CR_PLLON;


// Wait for the PLL to lock
   while (( RCC->CR & RCC_CR_PLLRDY ) != RCC_CR_PLLRDY );


// Switch the processor to the PLL clock source
   RCC->CFGR = ( RCC->CFGR & (~RCC_CFGR_SW_Msk)) | RCC_CFGR_SW_PLL;


// Update the system with the new clock frequency
   SystemCoreClockUpdate();
}
/*****************************************************************/

int main(int argc, char* argv[])
{
	SystemClock48MHz();

	myGPIOC_Init();		/* Initialize I/O port PC */
	ADC_Init();		/* Initialize ADC */
	DAC_Init();		/* Initialize DAC */
	
	while (1)
	{
		float voltage = Read_ADC_Voltage(); // Calculate voltage
		
		uint16_t DAC_output = Convert_Voltage_To_DAC(voltage); 	// Sent voltage to DAC
		trace_printf("Value sent to DAC: %u\n", DAC_output);	// Print value sent for dubugging purposes
	}
	return 0;
}


void Button_Init(void)
{
	// Enable clock for GPIOA peripheral
	RCC->AHBENR |= RCC_AHBENR_GPIOAEN;

	// Configure PA0 as input
	GPIOA->MODER &= ~(GPIO_MODER_MODER0);
	// Ensure no pull-up/pull-down for PA0
	GPIOA->PUPDR &= ~(GPIO_PUPDR_PUPDR0);

	// Configure EXTI for user button
	SYSCFG->EXTICR[0] |= SYSCFG_EXTICR1_EXTI0_PA;
	EXTI->IMR |= EXTI_IMR_MR0;
	EXTI->FTSR |= EXTI_FTSR_TR0;
	NVIC_EnableIRQ(EXTI0_1_IRQn);
}


void ADC_Init(void){
	// Enable ADC clock
	RCC->APB2ENR |= RCC_APB2ENR_ADC1EN;

	// Configure the ADC
	ADC1->CR |= ADC_CR_ADEN;
	// wait for ADRDY to be set
	while(!(ADC1->ISR & ADC_ISR_ADRDY)){}

	ADC1->CHSELR = ADC_CHSELR_CHSEL5;	// select channel 5 for PA5
	ADC1->CFGR1 = ADC_CFGR1_RES_0;		// set 12-bit resolution
}
float Read_ADC_Voltage(void){
	ADC1->CR |= ADC_CR_ADSTART;	// start ADC conversion
	while (!(ADC1->ISR & ADC_ISR_EOC)); //  wait for conversion to complete

	uint32_t adcValue = ADC1->DR;
	return (adcValue / 4095.0) * V_REF; // divide by 2^12-1 because its 12-bit
}

void DAC_Init(void){

	// Enable DAC clock
	RCC->APB1ENR |= RCC_APB1ENR_DACEN;

	// Enable DAC channel 1
	DAC->CR |=DAC_CR_EN1;
}
uint16_t Convert_Voltage_To_DAC(float voltage){
	// Convert potentiometer voltage to DAC output, multiply by 2^12-1 because DAC it 12-bit
	uint16_t DAC_output = (uint16_t)((voltage / V_REF) * 4095);
	// Load to DAC data register
	DAC->DHR12R1 = DAC_output;
	return DAC_output;
}
#pragma GCC diagnostic pop

// ----------------------------------------------------------------------------
