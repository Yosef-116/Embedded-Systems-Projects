#define F_CPU 8000000UL // 8 MHz internal clock
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>

volatile uint8_t hours = 0;
volatile uint8_t minutes = 0;
volatile uint8_t seconds = 0;

const uint8_t digit_map[10] = {
    0b00111111, // 0 
    0b00000110, // 1 
    0b01011011, // 2 
    0b01001111, // 3 
    0b01100110, // 4 
    0b01101101, // 5 
    0b01111101, // 6 
    0b00000111, // 7 
    0b01111111, // 8 
    0b01101111  // 9 
};

void sync_time_with_pc() {
    const char time_str[] = __TIME__; 
    
    hours   = (time_str[0] - '0') * 10 + (time_str[1] - '0');
    minutes = (time_str[3] - '0') * 10 + (time_str[4] - '0');
    seconds = (time_str[6] - '0') * 10 + (time_str[7] - '0');
}

void Timer1_Setup() {
    TCCR1A = 0; 
    TCCR1B = (1 << WGM12) | (1 << CS12); 
    OCR1A = 31249;
    TIMSK1 = (1 << OCIE1A); 
}

// Triggers every 1 second sfa
ISR(TIMER1_COMPA_vect) {
    seconds++;
    if (seconds >= 60) {
        seconds = 0;
        minutes++;
        if (minutes >= 60) {
            minutes = 0; 
            hours++;
            if (hours >= 24) {
                hours = 0; 
            }
        }
    }
}

void display_time() {
    
    uint8_t display_data[4] = {
        hours / 10, 
        hours % 10, 
        minutes / 10, 
        minutes % 10
    };

    for (uint8_t i = 0; i < 4; i++) {
        PORTC &= 0xF0; 
        uint8_t seg_data = digit_map[display_data[i]];
        if (i == 1 && (seconds % 2 == 0)) {
            seg_data |= 0x80; 
        }
        PORTB = ~seg_data; 
        PORTC |= (1 << i); 
        _delay_ms(2);
    }
}

int main(void) {
   
    DDRB = 0xFF; 
    DDRC = 0x0F; 
    
    sync_time_with_pc(); 
    Timer1_Setup(); 
    sei(); 
    while (1) {
        display_time(); 
    }
}
