/*
 * synth.h
 *
 * Includes struct and function definitions for Takpad synth code
 *
 */

enum note_state {
	OFF		= 0,
	ATTACK	= 1,
	DECAY	= 2,
	SUSTAIN = 3,
	RELEASE = 4,
};

struct note_t {
	enum note_state state;
	uint8_t velocity;
	uint8_t phase;
	uint8_t step;
	uint16_t duration;
};

struct envelope {
	int attack;
	int decay;
	int sustain;
	int release;
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
