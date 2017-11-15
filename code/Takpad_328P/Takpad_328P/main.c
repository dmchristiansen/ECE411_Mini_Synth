/*
 * Takpad_328P.c
 *
 * Author:  ECE411 Group 11, Fall 2017
 * 
 * 
 */

/*
 * TO DO:
 *  Implement wave tables
 *  Set up ADC interrupts
 *
 */ 

#include <avr/io.h>
#include <avr/interrupt.h>

// Function prototypes
uint8_t read_ADC(uint8_t ADC_Channel);
void adc_init();
void io_init();


// Global variables
// static uint8_t note_state[4];
static uint8_t note_velocity[4];

int main(void)
{
	// We're only using high byte of sensor readings
	// sensor_threshold should be (threshold >> 2)
	uint8_t sensor_threshold = 0x32; // 200
	
	// Storage for ADC readings
	uint8_t reading[4] = {0}, prev_reading[4] = {0}, peak_reading[4] = {0};
	uint8_t i;
	uint8_t trigger[4] = {0};
	
	// Initial setup functions
	adc_init();
	io_init();
	
	// Polling loop
	while (1)
	{
		
		// Turn on LED when sensor reading is > threshold
		for (i = 0; i < 4; i++)
		{
			if (trigger[i])
				PORTD |= (1 << i);
			else
				PORTD &= ~(1 << i);
		}
		
		for	(i = 0; i < 4; i++)
		{
			prev_reading[i] = reading[i];
			reading[i] = read_ADC(i);
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
	}


	
}

ISR

SIGNAL()

// Read ADC, blocking read
uint8_t read_ADC(uint8_t ADC_Channel)
{
	// Set channel mux in ADMUX
	ADMUX |= (ADMUX & 0xF0) | (ADC_Channel & 0x0F);
	// Set start conversion bit
	ADCSRA |= (1 << ADSC);
	// Loop until conversion is complete
	while (ADCSRA & (1 << ADSC));
	// Return high byte of results
	return ADCH;
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
	
}

