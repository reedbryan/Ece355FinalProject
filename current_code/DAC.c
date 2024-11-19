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
#include <unistd.h>
#include "diag/Trace.h"
#include "cmsis/cmsis_device.h"
#include "stm32f051x8.h"
#include "stm32f0xx_hal.h"
#include "led_display.c"

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

void myGPIOA_Init(void);
void EXTI0_1_Init(void);
void myGPIOC_Init(void);
void myTIM2_Init(void);
void ADC_Init(void);
void ADC_Enable(void);
void DAC_Init(void);
uint16_t Convert_Voltage_To_DAC(float voltage);
float Read_ADC_Voltage(void);
float Calculate_Potentiomter_Resistance(float voltage);
void waveformgen_Init(void);

int x = 0;
int x_count = 0;
int x_last = 0;


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
*/

/*****************************************************************/


int
main(int argc, char* argv[])
{

	SystemClock48MHz();

	    //...

	// for led display
	myGPIOB_Init();
	oled_config();



	// By customizing __initialize_args() it is possible to pass arguments,
	// for example when running tests with semihosting you can pass various
	// options to the test.
	// trace_dump_args(argc, argv);

	// Send a greeting to the trace device (skipped on Release).
	//trace_puts("Hello World!");

	// The standard output and the standard error should be forwarded to
	// the trace device. For this to work, a redirection in _write.c is
	// required.
	//puts("Standard output message.");
	//fprintf(stderr, "Standard error message.\n");

	// At this stage the system clock should have already been configured
	// at high speed.
	trace_printf("System clock: %u Hz\n", SystemCoreClock);

	EXTI0_1_Init();		// Initialize I/O port PA
	myGPIOC_Init();		// Initialize I/O port PC
	myTIM2_Init();		// Initialize timer TIM2

	ADC_Init();
	DAC_Init();

	//waveformgen_Init();


	while (1)
	{
		refresh_OLED();

		float voltage = Read_ADC_Voltage();

		float resistance = Calculate_Potentiomter_Resistance(voltage);

		int res = (int)resistance;

		//trace_printf("ADC Resistance: %d\n", res);

		uint16_t DAC_output = Convert_Voltage_To_DAC(voltage);

		//trace_printf("DAC Voltage: %u\n", DAC_output);
		if (x_count % 2 == 1){
			trace_printf("ADC Resistance: %d\n", res);
			trace_printf("DAC Voltage: %u\n", DAC_output);
		}
	}

	return 0;

}


void myGPIOA_Init(void){
	// Enable clock for GPIOA peripheral
	RCC->AHBENR |= RCC_AHBENR_GPIOAEN;

	// Configure PA0 as input
	GPIOA->MODER &= ~(GPIO_MODER_MODER0);
	// Ensure no pull-up/pull-down for PA0
	GPIOA->PUPDR &= ~(GPIO_PUPDR_PUPDR0);
}

void EXTI0_1_Init(void)
{
	// Configure EXTI for user button
	SYSCFG->EXTICR[0] |= SYSCFG_EXTICR1_EXTI0_PA;
	EXTI->IMR |= EXTI_IMR_MR0;
	EXTI->FTSR |= EXTI_FTSR_TR0;
	NVIC_SetPriority(EXTI0_1_IRQn, 0);
	NVIC_EnableIRQ(EXTI0_1_IRQn);
}
void EXTI0_1_IRQHandler(){

	/*
	if (__HAL_GPIO_EXTI_GET_IT(GPIO_PIN_0))  // Check if interrupt is from PA0
	{
		__HAL_GPIO_EXTI_CLEAR_IT(GPIO_PIN_0);  // Clear the interrupt flag
		trace_printf("button clicked\n");
	}
	*/
	EXTI->PR |= EXTI_PR_PR0; // clear interrupt flag
	// get the current time in milliseconds
	uint32_t current_timestamp = TIM2->CNT;
	//trace_printf("current_timestamp = %u\n", current_timestamp);

	if (x > x_last){
		trace_printf("Button Click\n");
		x_last = x;
		x_count++;
	}



	/*
	if (EXTI->PR & EXTI_PR_PR0){
		EXTI->PR |= EXTI_PR_PR0; // clear interupt flag

		// get the current time in milliseconds
		uint32_t currentTime = HAL_GetTick();

		if ((currentTime - lastDebounceTime) > debounceDelay){
			trace_printf("BRIO button clicked\n");

			lastDebounceTime = currentTime;
		}

	}
	*/

}

/*
static uint8_t current_edge_detected = 0;
uint32_t period;
float frequency;
void waveformgen_Init(void){

	// Map EXTI2 line to PA2
	// Relevant register: SYSCFG->EXTICR[0]
	SYSCFG->EXTICR[0] |= SYSCFG_EXTICR1_EXTI2_PA;

	// EXTI2 line interrupts: set rising-edge trigger
	// Relevant register: EXTI->RTSR
	EXTI->RTSR |= EXTI_RTSR_TR2;

	// Unmask interrupts from EXTI2 line
	// Relevant register: EXTI->IMR
	EXTI->IMR |= EXTI_IMR_IM2;

	// Assign EXTI2 interrupt priority = 0 in NVIC
	// Relevant register: NVIC->IP[2], or use NVIC_SetPriority
	//NVIC_SetPriority(EXTI2_3_IRQn, 0);

	// Enable EXTI2 interrupts in NVIC
	// Relevant register: NVIC->ISER[0], or use NVIC_EnableIRQ
	NVIC_EnableIRQ(EXTI2_3_IRQn);
}
void EXTI2_3_IRQHandler()
{
	// Declare/initialize your local variables here...

	// Check if EXTI2 interrupt pending flag is indeed set
	if ((EXTI->PR & EXTI_PR_PR2) != 0)
	{
		TIM2->CR1 &= ~(TIM_CR1_CEN);
		// 1. If this is the first edge:
		//	- Clear count register (TIM2->CNT).
		//	- Start timer (TIM2->CR1).
		if (current_edge_detected == 0){
			    TIM2->CNT = 0;
				TIM2->CR1 |= TIM_CR1_CEN;
				current_edge_detected = 1;
		}
		//    Else (this is the second edge):
		//	- Stop timer (TIM2->CR1).
		//	- Read out count register (TIM2->CNT).
		//	- Calculate signal period and frequency.
		//	- Print calculated values to the console.
		//	  NOTE: Function trace_printf does not work
		//	  with floating-point numbers: you must use
		//	  "unsigned int" type to print your signal
		//	  period and frequency.
		//
		else{
			EXTI->PR |= EXTI_PR_PR2;   // Clear EXTI2 interrupt flag
			EXTI->IMR &= ~(EXTI_IMR_MR2);  // Disable EXTI2 interrupt

			period = TIM2->CNT;
			current_edge_detected = 2;
			frequency = ((float)SystemCoreClock)/((float)period);
			if (x_count % 2 ==0){
				trace_printf("frequency: %f, p = %u\n", frequency, period);
			}
			current_edge_detected = 0;

			EXTI->IMR |= EXTI_IMR_MR2;
		}
		// 2. Clear EXTI2 interrupt pending flag (EXTI->PR).
		// NOTE: A pending register (PR) bit is cleared
		// by writing 1 to it.
		//

		EXTI->PR |= EXTI_PR_PR2;
	}
}
*/



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
float Calculate_Potentiomter_Resistance(float voltage) {
	// Ohm's Law: R = V / (V_REF - V) * R_total_res
	if (voltage < V_REF){
		return (voltage / (V_REF - voltage) * POT_MAX_RESISTANCE);
	} else{
		return 0;
	}
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
	DAC->DHR12R1 = DAC_output;
	return DAC_output;
}


void myGPIOC_Init()
{
	/* Enable clock for GPIOC peripheral */
	RCC->AHBENR |= RCC_AHBENR_GPIOCEN;

	/* Configure PC8 and PC9 as outputs */
	GPIOC->MODER |= (GPIO_MODER_MODER8_0 | GPIO_MODER_MODER9_0);
	/* Ensure push-pull mode selected for PC8 and PC9 */
	GPIOC->OTYPER &= ~(GPIO_OTYPER_OT_8 | GPIO_OTYPER_OT_9);
	/* Ensure high-speed mode for PC8 and PC9 */
	GPIOC->OSPEEDR |= (GPIO_OSPEEDER_OSPEEDR8 | GPIO_OSPEEDER_OSPEEDR9);
	/* Ensure no pull-up/pull-down for PC8 and PC9 */
	GPIOC->PUPDR &= ~(GPIO_PUPDR_PUPDR8 | GPIO_PUPDR_PUPDR9);
}


void myTIM2_Init()
{
	/* Enable clock for TIM2 peripheral */
	RCC->APB1ENR |= RCC_APB1ENR_TIM2EN;

	/* Configure TIM2: buffer auto-reload, count up, stop on overflow,
	 * enable update events, interrupt on overflow only */
	TIM2->CR1 = ((uint16_t)0x008C);

	/* Set clock prescaler value */
	TIM2->PSC = myTIM2_PRESCALER;
	/* Set auto-reloaded delay */
	TIM2->ARR = myTIM2_PERIOD;

	/* Update timer registers */
	TIM2->EGR = ((uint16_t)0x0001);

	/* Assign TIM2 interrupt priority = 0 in NVIC */
	//NVIC_SetPriority(TIM2_IRQn, 0);
	// Same as: NVIC->IP[3] = ((uint32_t)0x00FFFFFF);

	/* Enable TIM2 interrupts in NVIC */
	NVIC_EnableIRQ(TIM2_IRQn);
	// Same as: NVIC->ISER[0] = ((uint32_t)0x00008000) */

	/* Enable update interrupt generation */
	TIM2->DIER |= TIM_DIER_UIE;
	/* Start counting timer pulses */
	TIM2->CR1 |= TIM_CR1_CEN;
}


/* This handler is declared in system/src/cmsis/vectors_stm32f051x8.c */
void TIM2_IRQHandler()
{
	uint16_t LEDstate;

	/* Check if update interrupt flag is indeed set */
	if ((TIM2->SR & TIM_SR_UIF) != 0)
	{
		x++;
		TIM2->SR &= ~(TIM_SR_UIF);	/* Clear update interrupt flag */
		TIM2->CR1 |= TIM_CR1_CEN;	/* Restart stopped timer */
	}
}


#pragma GCC diagnostic pop

// ----------------------------------------------------------------------------
