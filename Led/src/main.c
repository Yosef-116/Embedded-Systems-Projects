#define F_CPU 16000000UL     // Assumes a 16 MHz external crystal is attached
#include <avr/io.h>
#include <util/delay.h>

int main(void) {
    DDRB |= (1 << PB5); 
    DDRC |= (1 << PC5); 
    DDRD |= (1 << PD5); 
    
    while(1){
        PORTB |= (1 << PB5);
        _delay_ms(200);
        PORTB &= ~(1 << PB5);
        _delay_ms(10);

        PORTC |= (1 << PC5);
        _delay_ms(100);
        PORTC &= ~(1 << PB5);
        _delay_ms(10);

        PORTD |= (1 << PD5);
        _delay_ms(200);
        PORTD &= ~(1 << PB5);
        _delay_ms(10);
    }
}