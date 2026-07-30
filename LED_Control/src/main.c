#define F_CPU 16000000UL

#include <avr/io.h>
#include <util/delay.h>

#define LED_PIN PB3 
#define POT_PIN PC0 

void adc_init(void) { 
    // 1. Set Voltage Reference to AVCC (5V) and select Channel 0 (PC0) 
    // REFS1 = 0, REFS0 = 1, ADLAR = 0, MUX3:0 = 0000 
    ADMUX = (1 << REFS0); 
    // 2. Enable ADC and set Clock Prescaler to 128 (16 MHz / 128 = 125 kHz) 
    ADCSRA = (1 << ADEN) | (1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0); 
} 
uint16_t adc_read(void) { 
    // Start the conversion 
    ADCSRA |= (1 << ADSC); 
    // Wait until conversion completes (ADSC becomes 0) 
    while (ADCSRA & (1 << ADSC)) ; 
    // Return the combined 10-bit value from ADCL and ADCH automatically return
}

ISR(ADC_vect) {
    adc_read = ADC;
}

int main(void) {
    DDRC &= ~(1 << POT_PIN);
    
    pwm_init();
    adc_init();
    sei();

    while (1) {

        uint16_t adc_value = adc_read();

        uint8_t pwm_duty = (uint8_t)(adc_value >> 2);

        OCR2A = pwm_duty;

        _delay_ms(15);
    }
}