#define F_CPU 16000000UL

#include <avr/io.h>
#include <avr/interrupt.h>

volatile uint32_t milliseconds = 0;

ISR(TIMER0_COMPA_vect){
  milliseconds++;
}

void timer0_init(void){
  TCCR0A |= (1 << WGM01);

  TCCR0B |= (1 << CS02);

  OCR0A = 62;

  TIMSK0 |= (1 << OCIE0A);
}

uint32_t getmillis(void) {
  uint32_t ms;
  cli();
  ms = milliseconds;
  sei();
  return ms;
}

void timer1_pwm_init(void) {
    // 1. Set PB1 (OC1A) as an output pin
    DDRB |= (1 << PB1);

    // 2. Set TCCR1A Register:
    // COM1A1 = 1, COM1A0 = 0 -> Non-inverting Fast PWM mode (Clear OC1A on match, set at BOTTOM)
    // WGM10 = 1 -> Combine with WGM12 for 8-bit Fast PWM mode (Mode 5)
    TCCR1A |= (1 << COM1A1) | (1 << WGM10);

    // 3. Set TCCR1B Register:
    // WGM12 = 1 -> Completes WGM bits for Mode 5 (Fast PWM, 8-bit)
    // CS11 = 1, CS10 = 1 -> Select prescaler of 64
    TCCR1B |= (1 << WGM12) | (1 << CS11) | (1 << CS10);

    // At 16MHz CPU clock, a 64 prescaler and an 8-bit (256 step) timer yields:
    // 16,000,000 / (64 * 256) = 976.5 Hz PWM frequency (perfectly flicker-free)

    OCR1A = 0; // Initialize PWM duty cycle to 0 (LED off)
}

