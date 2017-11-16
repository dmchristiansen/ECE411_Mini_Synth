/*
 * Takpad_328P.c
 * Version 0.0.2
 * Last updated: 11/14/2017
 * Author:  ECE411 Group 11, Fall 2017
 */

/*
 * Fuses: 0xFD, 0xD6, 0xFF
 * 
 * TO DO:
 *
 *  Implement wave tables
 *   Create tables
 *   Set up interrupt routine
 *   
 *  Set up SD Card interface
 *   Probably just import a library
 *  
 *  Make this not one giant file ffs
 */ 

#define F_CPU 16000000UL // 16 MHz
#include <util/delay.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include "wavetable.h"

// Function prototypes
uint8_t read_ADC(uint8_t ADC_Channel);
void adc_init();
void io_init();
void tc_init();


// Global variables
// static uint8_t note_state[4];
static uint8_t note_velocity[4];

int main(void)
{
	// We're only using high byte of sensor readings
	// sensor_threshold should be (threshold >> 2)
	uint8_t sensor_threshold = 0x32; 
	
	// Storage for ADC readings
	uint8_t reading[4] = {0}, prev_reading[4] = {0}, peak_reading[4] = {0};
	uint8_t i;
	uint8_t trigger[4] = {0};
	
	// Initial setup functions
	adc_init();
	io_init();
	tc_init();
	
	//PORTD |= 0x0F;
	
	
	// Polling loop
	while (1)
	{
		
		for	(i = 0; i < 4; i++)
		{
			// Turn on LED when sensor reading is > threshold
			if (trigger[i])
				PORTD |= (1 << i);
			else
				PORTD &= ~(1 << i);
			
			// update sensor reading
			prev_reading[i] = reading[i];
			reading[i] = read_ADC(i);
			
			// update note state
			if (reading[i] >= sensor_threshold)
			{
				peak_reading[i] = reading[i];
				trigger[i] = 1;
			}
			else
			{
				if (prev_reading[i] > reading[i])
				{
					note_velocity[i] = peak_reading[i];
				}
				trigger[i] = 0;
			}
		}
		
		//_delay_ms(50);
	}
	


	
}

// PWM interrupt routine
ISR(TIMER1_OVF_vect)
{
	// TCNT1 is constantly compared with OCR1x
	// The OCF1x flag will be set upon a match
	// If OCIE1x is enabled, an interrupt will be generated as well
	// The OCF1x flag will be cleared when the interrupt is serviced
	
	// Set PWM duty cycle by altering OCR1AL
	
	// GTCCR |= 0x8001; // This holds the clock prescaler in reset, halting the counter
	
	
}

// Read ADC, blocking read
uint8_t read_ADC(uint8_t ADC_Channel)
{
	// Set channel mux in ADMUX
	ADMUX = (ADMUX & 0xF0) | (ADC_Channel & 0x0F);
	// Set start conversion bit
	ADCSRA |= (1 << ADSC);
	// Loop until conversion is complete
	while (ADCSRA & (1 << ADSC));
	// Return high byte of results
	return ADCH;
}

// Initialize Timer/Counter 1
void tc_init(void)
{
	// The timer/counter should be in Fast PWM mode,
	// with a PWM frequency of ~20KHz.
	// T/C will set OC1A to 1 at 0x00, count to OCR1A,
	// set OC1A low, count to 0xFF, reset to 0x00.
	
	// Set COM1A output behavior, set fast PWM mode
	TCCR1A |= (1 << COM1A1) | (1 < WGM11);
	
	// Set fast PWM mode, set counter clock to sys_clk / 8
	TCCR1B |= (1 << WGM12) | (1 << CS11);
}

// Initialize ADC
void adc_init(void)
{
	// Note: Set ADIE bit in ADCSRA and I-bit in SREG to
	// enable conversion complete interrupt.
	
	// Set Aref to AVcc, left-adjust results
	ADMUX = (1 << REFS0) | (1 << ADLAR);
	// Set prescalar to 128 (for now), enable ADC
	ADCSRA |= (1 << ADPS2)  | (1 << ADPS1) | (1 << ADPS0) | (1 << ADEN);
	
	// Start first conversion to initialize ADC
	ADCSRA |= (1 << ADSC);
	// ADSC bit will go low once
	while (ADCSRA & (1 << ADSC));	
}

// Initialize I/O pins
void io_init(void)
{
	// There might be a need to set ADC pins as input...though that should be default
	// There are resistor settings that might need to be tweaked?
	
	// Set Pins 30-32, 1 (Port D 0-3) as output for LEDs
	DDRD |= 0x0F;
	
	// Set Pin 13 (Port B 1) as output for PWM output
	DDRB |= 0x02;
}

