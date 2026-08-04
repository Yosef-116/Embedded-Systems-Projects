

#include <avr/interrupt.h>
#include <avr/io.h>

#define LED_PIN PD5

volatile uint32_t timer_flag = 0;

ISR(TIMER1_OVF_vect){
  TCNT1 = 3036; // Calculation: 65536 - (16,000,000 / 256 / 1 Hz) = 3036

  timer_flag = 1;
}

void timer1_init(void) {
  TCCR1B = 0x00; //set to normal mode

  TCCR1B |= (1 << CS12);

  TCNT1 = 3036; //starts at this preset value till overflows

  TIMSK1 = (1 << TOIE1);
}

int main(void){
  DDRD |= (1 << LED_PIN);

  timer1_init();

  sei();

  while (1) {
    if(timer_flag) {
      timer_flag = 0;

      PORTD ^= (1 << LED_PIN);
    }
  }
  
  return 0;
}