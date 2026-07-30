#define F_CPU 16000000UL // 16 MHz clock speed

#include <avr/io.h>
#include <util/delay.h>
#include <string.h>

#define PASSWORD "1234"

// ==========================================
// 1. SERVO MOTOR FUNCTIONS (Timer 1)
// ==========================================
void servo_init() {
    DDRB |= (1 << PB1); // Set PB1 (Pin 9) as output
    
    // Fast PWM Mode 14, Top = ICR1, Prescaler = 8, Non-inverted OC1A
    TCCR1A = (1 << COM1A1) | (1 << WGM11);
    TCCR1B = (1 << WGM13) | (1 << WGM12) | (1 << CS11);
    
    ICR1 = 39999; // 20ms period (50Hz)
}

void servo_set_angle(uint16_t pulse) {
    // 2000 = ~0 degrees (Locked), 4000 = ~180 degrees (Unlocked)
    OCR1A = pulse; 
}

// ==========================================
// 2. LCD DISPLAY FUNCTIONS (4-Bit Mode)
// ==========================================
void lcd_pulse_en() {
    PORTD |= (1 << PD3);  // EN High
    _delay_us(1);
    PORTD &= ~(1 << PD3); // EN Low
    _delay_us(100);
}

void lcd_write(uint8_t data, uint8_t is_char) {
    if (is_char) PORTD |= (1 << PD2);  // RS High for characters
    else PORTD &= ~(1 << PD2);         // RS Low for commands

    // Send upper 4 bits to PC0-PC3
    PORTC = (PORTC & 0xF0) | (data >> 4);
    lcd_pulse_en();

    // Send lower 4 bits to PC0-PC3
    PORTC = (PORTC & 0xF0) | (data & 0x0F);
    lcd_pulse_en();
    
    if(!is_char) _delay_ms(2); // Commands need more time
}

void lcd_init() {
    DDRD |= (1 << PD2) | (1 << PD3); // RS and EN as output
    DDRC |= 0x0F;                    // D4-D7 (PC0-PC3) as output

    _delay_ms(20); // Wait for LCD to power up

    // Special 4-bit initialization sequence
    PORTC = (PORTC & 0xF0) | 0x03; lcd_pulse_en(); _delay_ms(5);
    PORTC = (PORTC & 0xF0) | 0x03; lcd_pulse_en(); _delay_us(150);
    PORTC = (PORTC & 0xF0) | 0x03; lcd_pulse_en(); 
    PORTC = (PORTC & 0xF0) | 0x02; lcd_pulse_en(); // Switch to 4-bit

    lcd_write(0x28, 0); // 4-bit mode, 2 lines, 5x8 font
    lcd_write(0x0C, 0); // Display ON, Cursor OFF
    lcd_write(0x01, 0); // Clear display
    lcd_write(0x06, 0); // Increment cursor
}

void lcd_print(const char* str) {
    while (*str) {
        lcd_write(*str++, 1);
    }
}

void lcd_clear() {
    lcd_write(0x01, 0);
}

// ==========================================
// 3. KEYPAD FUNCTIONS
// ==========================================
void keypad_init() {
    DDRD |= 0xF0;   // PD4-PD7 (Cols) as outputs
    DDRB &= ~0x3C;  // PB2-PB5 (Rows) as inputs (0x3C is 00111100 in binary)
    PORTB |= 0x3C;  // Enable internal pull-up resistors on PB2-PB5
}

char keypad_scan() {
    // Standard 4x4 keypad mapping
    char keys[4][4] = {
        {'1','2','3','A'},
        {'4','5','6','B'},
        {'7','8','9','C'},
        {'*','0','#','D'}
    };

    for (int col = 0; col < 4; col++) {
        // Set all columns HIGH
        PORTD |= 0xF0;
        // Set the current column LOW
        PORTD &= ~(1 << (col + 4));
        _delay_ms(2); // Voltage stabilization delay

        for (int row = 0; row < 4; row++) {
            // Check if the specific row pin is LOW (PB2 is row 0, PB3 is row 1, etc.)
            if (!(PINB & (1 << (row + 2)))) { 
                _delay_ms(20); // Debounce delay
                // Wait for the user to let go of the button
                while (!(PINB & (1 << (row + 2)))); 
                return keys[row][col];
            }
        }
    }
    return 0; // No key pressed
}

// ==========================================
// 4. MAIN PROGRAM LOGIC
// ==========================================
int main(void) {
    // Initialize all hardware
    servo_init();
    lcd_init();
    keypad_init();

    char input_buffer[5]; 
    uint8_t index = 0;

    servo_set_angle(2000); // Start with door locked
    
    lcd_clear();
    lcd_print("Enter PIN:");
    lcd_write(0xC0, 0); // Move cursor to second line

    while (1) {
        char key = keypad_scan();

        if (key != 0) { // If a button was pressed
            
            lcd_write('*', 1); // Print a star for security
            input_buffer[index] = key;
            index++;


            if (index == 4) { // Once 4 keys are entered
                input_buffer[4] = '\0'; // End the string

                lcd_clear();

                if (strcmp(input_buffer, PASSWORD) == 0) {
                    lcd_print("Access Granted!");
                    servo_set_angle(4000); // Unlock (180 deg)
                    _delay_ms(4000);       // Keep open for 4 seconds
                    servo_set_angle(2000); // Lock again
                } else {
                    lcd_print("Wrong PIN!");
                    _delay_ms(2000);
                }
                
                // Reset for the next entry
                index = 0;
                lcd_clear();
                lcd_print("Enter PIN:");
                lcd_write(0xC0, 0); // Move cursor to second line
            }
        }
    }
}
