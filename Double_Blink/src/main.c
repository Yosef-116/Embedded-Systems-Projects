#define F_CPU 16000000UL

#include <avr/interrupt.h>
#include <avr/io.h>

volatile uint32_t milliseconds= 0;

ISR(TIMER0_COPMA_vect) {
    milliseconds++;
}

void timer0_init(void) {
    TCCR0A |= (1 << WGM01); // CTC

    TCCR0B |= (1 << CS02); // 256 pre-scaler

    OCR0A = 62;

    TIMSK0 |= (1 << OCIE0A);
}

uint32_t get_millis(void) {
    uint32_t ms;
    cli();
    ms = milliseconds;
    sei();
    return ms;
}

int main(void) {
    DDRD |= (1 << PD3);
    DDRD |= (1 << PD5);
    
    timer0_init();
    sei();

    uint32_t last_time_3 = 0;
    uint32_t last_time = 0;

    uint8_t state_3 = 0;
    uint8_t state_5 = 0;

    while(1) {
        uint32_t current_time = get_millis();

        if (state_3 == 0) {
            if ( current_time - last_time >= 10 ) {
                PORTD |= (1 << PD3);
                state_3 = 1;
                last_time = current_time;
            }
        } else {
            if ( current_time - last_time >= 50 ) {
                PORTD &= ~(1 << PD3);
                state_3 = 0;
                last_time = current_time;
            }
        }
     
        if (state_5 == 0) {
            if ( current_time - last_time >= 15 ) {
                PORTD |= (1 << PD5);
                state_5 = 1;
                last_time = current_time;
            }
        } else {
            if ( current_time - last_time >= 50 ) {
                PORTD &= (1 << PD5);
                state_5 = 0;
                last_time = current_time;
            }
        }

    return 0;
    }
}