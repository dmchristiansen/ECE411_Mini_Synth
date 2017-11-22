/*
 * Takpad_328P.c
 * Version 0.0.6
 * Last updated: 11/21/2017
 * Author:  ECE411 Group 11, Fall 2017
 */

/*
 * Fuses: 0xFD, 0xD6, 0xFF
 * 
 * TO DO:
 *
 *  Make larger tables?  512 might not suck
 *
 *  Put tables in PROGMEM!
 *
 *  Add ADSR amplitude modulation
 *   Create ADSR tables
 *   Add frequency modulation based on ADSR tables
 *   Add amplitude modulation based on ADSR tables
 *
 *  Create more complex wave tables
 *   
 *  Waveshaping!
 *
 *  Set up SD Card interface
 *   Probably just import a library
 *   Load tables at start?
 */ 

/*
 * Notes on frequencies:  THIS IS WRONG NOW
 *  At 16MHz, the TC1 PWM is 62.5kHz
 *  With a 256 element sine wave table, size 1 steps give ~244 Hz
 *
 *  For an LFO, TC2 with a pre-scaler of 128 gives a TC clock of 125kHz
 *  If the counter counted to 64 before resetting, it would overflow at ~1953Hz
 *  Using a 256 element wave table, it would have a frequency of ~8Hz
 *
 */

#define F_CPU 16000000UL // 16 MHz
#include <util/delay.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include "wavetable.h"
#include "synth.h"

// Global variables
struct note_t note[4];
struct envelope env;
uint8_t LFO_phase = 0;
uint16_t timer_val = 363;

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
	init_notes();
	
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
			
			// Move to next phase of envelope
			if((note[i].env_phase & 0x80) && (note[i].state != OFF))
				update_note(&note[i]);
			
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
				if ((prev_reading[i] > reading[i]) & trigger[i])
				{
					note[i].velocity = peak_reading[i];
					start_note(&note[i]);
					peak_reading[i] = 0;
				}
				trigger[i] = 0;
			}
		}
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
	
	// Increment LFO phase on compare match
	
	if(TIFR2 & OCF2A)
	{
		TIFR2 |= (1 << OCF2A);
		LFO_phase++;
	}
	ICR1 = timer_val + (sine[LFO_phase] >> 3);
	
	// Sum the wave table values of  all four notes (inactive notes should be 0)
	int duty_cycle = 127 +
	((((sine[note[0].phase] + saw[note[0].phase >> 1]) >> 1)
	+((sine[note[1].phase] + saw[note[1].phase >> 1]) >> 1)
	+((sine[note[2].phase] + saw[note[2].phase >> 1]) >> 1)
	+((sine[note[3].phase] + saw[note[3].phase >> 1]) >> 1)) >> 2);
	
	// Update duty cycle register
	OCR1AL = duty_cycle;
	
	for(int i = 0; i < 4; i++)
	{
		// Increment phase accumulator
		if(note[i].state != OFF)
		{
			note[i].phase += note[i].step; // plus envelope freq shift
			note[i].env_phase += note[i].env_step;
		}
	}
	
	// GTCCR |= 0x8001; // This holds the clock prescaler in reset, halting the counter
}

// Set up note values to begin playing or restart
void start_note(struct note_t* note)
{
	if(note->state == OFF)
	{
		note->env_phase = 0;
		note->env_step = env.a_step;
	}
	else
	{
		if(note->state == DECAY)
			note->env_phase = 128 - (note->env_phase & 0x7F);
		else
			note->env_phase = 0;
	}
	note->state = ATTACK;
	note->env_step = env.a_step; // times some velocity modifier
}

// Reset note to known state
void stop_note(struct note_t* note)
{
	note->state = OFF;
	note->velocity = 0;
	note->phase = 0;
}

// Update note state
void update_note(struct note_t* note)
{
	note->env_phase = 0;
	if(note->state == ATTACK)
	{
		note->state = DECAY;
		note->env_step = env.d_step; // times some velocity modifier
	}
	else if(note->state == DECAY)
	{
		note->state = SUSTAIN;
		note->env_step = env.s_step; // times some velocity modifier
	}
	else if(note->state == SUSTAIN)
	{
		note->state = RELEASE;
		note->env_step = env.r_step; // times some velocity modifier
	}
	else if(note->state == RELEASE)
	{
		stop_note(note);
	}
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
	
	// Set up TC2 for CTC mode, 128 pre-scalar, 64 count
	TCCR2A |= (1 << WGM21);
	TCCR2B |= (1 << CS22) | (1 << CS20);
	OCR2A = 64;
	
	// Set up TC1
	// Set COM1A output behavior, set fast PWM mode, TOP = ICR1
	TCCR1A |= (1 << COM1A1) | (1 << WGM11);
	// Set fast PWM mode, set counter clock to sys_clk
	TCCR1B |= (1 << WGM13) | (1 << WGM12) | (1 << CS10);
	// Enable timer overflow interrupts
	TIMSK1 |= 1;
	
	ICR1 = timer_val;
	OCR1A = 0xFF;
	
	// Globally enable interrupts
	sei();
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
	
	// Set Pin 13 (PortB1 / OC1A) as output for PWM output
	DDRB |= (1 << 1);
}

void init_notes()
{
	int i;
	
	// Initialize note structs
	for(i = 0; i < 4; i++)
	{
		note[i].phase = 0;
		note[i].state = OFF;
		note[i].step = (2 * i) + 1;
		note[i].velocity = 0;
		note[i].env_phase = 0;
		note[i].env_step = 0;
	}
	
	// Initialize envelope values
	env.a_step = 10;
	env.d_step = 10;
	env.s_step = 5;
	env.r_step = 2;
}