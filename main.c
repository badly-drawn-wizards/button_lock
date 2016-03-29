#include <avr/io.h>
#include <avr/cpufunc.h>
#include <avr/interrupt.h>

const uint8_t LED_PORT = PB4;
const uint8_t BUTTON_CHANNEL = 3;

const uint8_t DEBOUNCE_THRESHOLD = 2;

const uint8_t PASSWORD_LENGTH = 8;
const uint8_t PASSWORD[8] = {0,2,0,1,1,0,1,2};

const uint8_t NUM_VOLTAGES = 4;
const uint8_t VOLTAGES[] = {0, 85, 127, 255};

int main();
void setup_pins();

struct password_state {
  uint8_t counter;
};
uint8_t read_password(uint8_t button, const uint8_t password[], uint8_t password_length, struct password_state* state);

struct debounce_state {
  uint8_t last_button;
  uint8_t counter;
};
uint8_t debounce(uint8_t button, uint8_t threshold, struct debounce_state *state);
uint8_t button_of_reading (uint8_t reading);

void set_led(uint8_t port, uint8_t state);
uint8_t read_analog(uint8_t ch);



int main() {
  setup_pins();

  struct debounce_state debounce_state;
  struct password_state password_state;
  while(1) {
    uint8_t result = read_password(debounce(button_of_reading(read_analog(BUTTON_CHANNEL)), DEBOUNCE_THRESHOLD, &debounce_state), PASSWORD, PASSWORD_LENGTH, &password_state);

    if(result) {
      for(uint16_t i=0; i<30000; i++) {
  	set_led(LED_PORT, 1);
      }
      set_led(LED_PORT, 0);
    }
  }
}

void setup_pins() {
  DDRB |= (1 << LED_PORT); // LED_PORT is output

  // ADC enable and prescale is 1/128
  ADCSRA = (1 << ADEN) | (0b111 << ADPS0);
}


uint8_t read_password(uint8_t button, const uint8_t password[], uint8_t password_length, struct password_state *state) {
  if(state->counter & 1) {
    if(button == 0) {
      state->counter++;
    }
  } else {
    if(button == 0) return 0;
    if(password[state->counter>>1] == button - 1) {
      state->counter++;
    } else {
      state->counter=0;
    }
  }

  if(state->counter == password_length<<1) {
    state->counter = 0;
    return 1;
  } else {
    return 0;
  }
}

uint8_t debounce(uint8_t button, uint8_t threshold, struct debounce_state *state) {
  if (button == state->last_button) {
    state->counter++;
  } else {
    state->counter = 0;
  }
  state->last_button = button;

  if (state->counter > threshold) {
    return button;
  } else {
    return 0;
  }
}

uint8_t button_of_reading(uint8_t reading) {
  uint8_t result = NUM_VOLTAGES - 1;
  for(uint8_t i=0; i<NUM_VOLTAGES - 1; i++) {
    if(reading < (VOLTAGES[i] + VOLTAGES[i+1]) >> 1) {
      result = i;
      break;
    }
  }
  return result;
}

void set_led(uint8_t port, uint8_t state) {
  state = state & 1;
  PORTB = (PORTB & ~(1 << port)) | (state << port); 
}

uint8_t read_analog(uint8_t ch) {
  ch &= 0b111; // Clamp channel 
  ADMUX &= ~0b111 & ~(1 << ADLAR); // Clear channel bits and ADLAR
  ADMUX |= ch | (1 << ADLAR); // Set channel bits and ADLAR


  ADCSRA |= (1 << ADSC); // Start conversion
  while(ADCSRA & (1 << ADSC)); // Busy loop until conversion is complete

  return ADCH;
}
