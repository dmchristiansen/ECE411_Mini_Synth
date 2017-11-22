/*
 * synth.h
 *
 * Includes struct and function definitions for Takpad synth code
 *
 */

enum note_state {
	OFF = 0,
	ON = 1,
	DONE = 2
};

struct note_t {
	enum note_state state;
	uint8_t velocity;
	uint8_t phase;
	uint8_t step;
	uint16_t env_phase;
	uint8_t env_step;
};

// Function prototypes
uint8_t read_ADC(uint8_t ADC_Channel);
void adc_init();
void io_init();
void tc_init();
void init_notes();
void start_note(struct note_t* note);
void stop_note(struct note_t* note);
void update_note(struct note_t* note);
