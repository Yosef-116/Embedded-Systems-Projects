#define F_CPU 16000000UL
#include <avr/io.h>
#include <avr/interrupt.h>

volatile uint32_t milliseconds = 0;

// Interrupt Service Routine for Timer 0 Compare Match A
// Fires once every 1 millisecond
ISR(TIMER0_COMPA_vect) {
    milliseconds++;
}

// Configure Timer 0 to generate an interrupt every 1ms
void timer0_init(void) {
    // Set CTC (Clear Timer on Compare Match) mode
    TCCR0A |= (1 << WGM01);
    
    // Set prescaler to 64
    TCCR0B |= (1 << CS01) | (1 << CS00);
    
    // Set compare match register for 1ms interval:
    // (16,000,000 Hz / 64 prescaler) / 1000 Hz (1ms) - 1 = 249
    OCR0A = 249; 
    
    // Enable Timer 0 Compare Match A Interrupt
    TIMSK0 |= (1 << OCIE0A);
}

// Safely read the millisecond counter (handling 32-bit atomic read)
uint32_t get_millis(void) {
    uint32_t ms;
    cli(); // Disable interrupts
    ms = milliseconds;
    sei(); // Re-enable interrupts
    return ms;
}

int main(void) {
    // Configure PB5 and PC3 as outputs
    DDRB |= (1 << PB5);
    DDRC |= (1 << PC3);

    timer0_init();
    sei(); // Enable global interrupts

    // Variables to track the last time each LED changed state
    uint32_t last_time_B = 0;
    uint32_t last_time_C = 0;

    // Track the current state of each LED (0 = OFF, 1 = ON)
    uint8_t state_B = 0;
    uint8_t state_C = 0;

    while(1) {
        uint32_t current_time = get_millis();

        // --- LED B State Machine ---
        if (state_B == 0) {
            // If OFF, check if 10ms have passed to turn ON
            if (current_time - last_time_B >= 10) {
                PORTB |= (1 << PB5);
                state_B = 1;
                last_time_B = current_time;
            }
        } else {
            // If ON, check if 50ms have passed to turn OFF
            if (current_time - last_time_B >= 50) {
                PORTB &= ~(1 << PB5);
                state_B = 0;
                last_time_B = current_time;
            }
        }

        // --- LED C State Machine ---
        if (state_C == 0) {
            // If OFF, check if 10ms have passed to turn ON
            if (current_time - last_time_C >= 10) {
                PORTC |= (1 << PC3);
                state_C = 1;
                last_time_C = current_time;
            }
        } else {
            // If ON, check if 25ms have passed to turn OFF
            if (current_time - last_time_C >= 25) {
                PORTC &= ~(1 << PC3);
                state_C = 0;
                last_time_C = current_time;
            }
        }
    }
    
    return 0; // Standard main return (though the loop is infinite)
}
