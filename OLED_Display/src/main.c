#define F_CPU 16000000UL 

#include <avr/io.h> 
#include <util/delay.h> 

// Define I2C Pins on Port C 
#define SDA_PIN PC4 
#define SCL_PIN PC5 

// 1. Minimal Bit-Banged I2C Functions
void I2C_Init(void) { 
  // Set SDA and SCL as outputs 
  DDRC |= (1 << SDA_PIN) | (1 << SCL_PIN); 
  // Bring bus lines HIGH (Idle state) 
  PORTC |= (1 << SDA_PIN) | (1 << SCL_PIN); 
} 

void I2C_Start(void) { 
  PORTC |= (1 << SDA_PIN) | (1 << SCL_PIN); 
  _delay_us(4); 
  PORTC &= ~(1 << SDA_PIN); // Pull SDA LOW first 
  _delay_us(4); 
  PORTC &= ~(1 << SCL_PIN); // Pull SCL LOW 
} 

void I2C_Stop(void) { 
  PORTC &= ~(1 << SDA_PIN); 
  PORTC |= (1 << SCL_PIN); // Pull SCL HIGH first 
  _delay_us(4); 
  PORTC |= (1 << SDA_PIN); // Pull SDA HIGH 
  _delay_us(4); 
} 

void I2C_WriteByte(uint8_t byte) { 
  for (uint8_t i = 0; i < 8; i++) { // Setup data bit on SDA line (MSB first) 
    if (byte & 0x80) 
      PORTC |= (1 << SDA_PIN); 
    else 
      PORTC &= ~(1 << SDA_PIN); 
    _delay_us(2); 
    PORTC |= (1 << SCL_PIN); // Clock pulse HIGH 
    _delay_us(2); 
    PORTC &= ~(1 << SCL_PIN); // Clock pulse LOW 
    byte <<= 1; // Shift to next bit 
  } 
  // Clock pulse for Ack/Nack (ignoring actual read check for brevity) 
  PORTC |= (1 << SCL_PIN); 
  _delay_us(2);
  PORTC &= ~(1 << SCL_PIN); 
} 

// 2. OLED Specific Communication Functions 
#define OLED_ADDR 0x78 // Standard 7-bit I2C Address 0x3C shifted left for Write (0x3C << 1) 

void OLED_WriteCmd(uint8_t cmd) { 
  I2C_Start(); 
  I2C_WriteByte(OLED_ADDR); 
  I2C_WriteByte(0x00); // Co = 0, D/C = 0 -> Control byte tells screen: "Commands Incoming" 
  I2C_WriteByte(cmd); 
  I2C_Stop(); 
}  

void OLED_WriteData(uint8_t data) { 
  I2C_Start(); 
  I2C_WriteByte(OLED_ADDR); 
  I2C_WriteByte(0x40); // Co = 0, D/C = 1 -> Control byte tells screen: "Visual Data Incoming" 
  I2C_WriteByte(data); 
  I2C_Stop(); 
} 

// 3. Hardware Initialization Sequence 
void OLED_Init(void) { 
  _delay_ms(100); // Wait for screen internal power to stabilize 
  OLED_WriteCmd(0xAE); // Display OFF (Sleep Mode) 
  OLED_WriteCmd(0x20); // Set Memory Addressing Mode 
  OLED_WriteCmd(0x02); // Page Addressing Mode (Fills lines chunk by chunk) 
  OLED_WriteCmd(0xA8); // Set Multiplex Ratio 
  OLED_WriteCmd(0x3F); // 64MUX (Sets vertical height to 64 pixels) 
  OLED_WriteCmd(0x8D); // Charge Pump Configuration 
  OLED_WriteCmd(0x14); // Enable internal Charge Pump (Required to turn on pixels!) 
  OLED_WriteCmd(0xAF); // Display ON (Wake panel from sleep)
  _delay_ms(10); 
} 

// 4. Clear Screen Sequence 
void OLED_Clear(void) { 
  // Loop through all 8 structural vertical Pages (8 pages * 8 vertical pixels = 64 pixels total height) 
  for (uint8_t page = 0; page < 8; page++) { 
    OLED_WriteCmd(0xB0 + page); // Set target Page start address 
    OLED_WriteCmd(0x00); // Set lower column start address to 0 
    OLED_WriteCmd(0x10); // Set higher column start address to 0 
    // Wipe all 128 horizontal columns in this page layout 
    for (uint8_t col = 0; col < 128; col++) { 
      OLED_WriteData(0x00); // Send 0x00 to turn off all 8 pixels in this stripe segment 
    } 
  } 
} 

int main(void) { 
  I2C_Init(); 
  OLED_Init(); 
  OLED_Clear(); // Draw an alternating stripe pattern across the first page 
  OLED_WriteCmd(0xB0); // Select Page 0 
  OLED_WriteCmd(0x00); // Column Start 0 
  OLED_WriteCmd(0x10); 
  for (uint8_t col = 0; col < 128; col++) { 
    if (col % 2 == 0) { 
      OLED_WriteData(0x55); // Binary 01010101 (Every other pixel turns on vertically) 
    } 
    else { 
      OLED_WriteData(0xAA); // Binary 10101010 (Inverted stripes) 
    }
  } 
  while (1) { 
    // Main loop empty; display remains static via internal screen RAM 
  } 
}