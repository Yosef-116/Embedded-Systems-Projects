#define F_CPU 16000000UL              // Tell <util/delay.h> the CPU clock is 16 MHz so _delay_ms/_delay_us are accurate

#include <avr/io.h>                   // Register/port definitions (DDRx, PORTx, PINx, etc.) for the target AVR chip
#include <util/delay.h>               // Provides _delay_ms() / _delay_us() busy-wait functions
#include <avr/pgmspace.h>             // Lets us store constant data (like the font table) in flash instead of RAM
#include <avr/interrupt.h>            // Interrupt-related macros: ISR(), sei() (enable global interrupts), etc.

// Define I2C Pins on Port C
#define SDA_PIN PC4                   // Data line for bit-banged I2C is Port C, pin 4
#define SCL_PIN PC5                   // Clock line for bit-banged I2C is Port C, pin 5

// Define I2C Slave Address for SSD1306 OLED
#define OLED_ADDR 0x78                // 8-bit I2C write address of the OLED controller

// Nokia OS States
typedef enum {                        // Enumeration listing every screen/mode the "phone" can be in
  STATE_HOME_MENU,                    // Main menu screen (choose Dial-up / SMS / File manager)
  STATE_DIAL_UP,                      // Screen for entering a phone number to "dial"
  STATE_DIAL_CALLING,                 // Screen shown while a fake call is in progress
  STATE_SMS_RECIPIENT,                // Entering "To:" phone number
  STATE_SMS_OPTIONS,                  // Selecting preset text
  STATE_SMS_ANIMATION,                // Watching envelope transmit
  STATE_FILE_MANAGER                  // Placeholder "file manager" screen (always empty)
} PhoneState;                         // Type name used to declare state variables

volatile PhoneState current_state = STATE_HOME_MENU; // Tracks which screen we're on; volatile because it could be touched by an ISR so can change at any time.

// Dynamic Clock & Battery variables
volatile uint8_t hours = 12;          // Current clock hour, updated by the Timer1 ISR; volatile since it's shared with an ISR
volatile uint8_t minutes = 0;         // Current clock minute, updated by the Timer1 ISR
volatile uint8_t seconds = 0;         // Current clock second, updated by the Timer1 ISR
volatile uint8_t battery = 99;        // Simulated battery percentage, decremented periodically by the Timer1 ISR
volatile uint8_t update_status_bar = 1; // Flag set by the ISR (once per second) telling main() to redraw the status bar
uint8_t cursor_col = 0;               // Tracks the current text-drawing column on the OLED (not used across ISR, so not volatile)

// Dial-up & SMS Buffers
char dial_buffer[16];                 // Stores the digits typed for a "phone number" while dialing
uint8_t dial_length = 0;              // Number of valid digits currently stored in dial_buffer

char sms_buffer[16];                  // Stores the digits typed for the SMS "To:" recipient
uint8_t sms_length = 0;               // Number of valid digits currently stored in sms_buffer

// 1. Bit-Banged I2C Functions
void I2C_Init(void) {                 // Configure the SDA/SCL pins for software (bit-banged) I2C
  DDRC |= (1 << SDA_PIN) | (1 << SCL_PIN);   // Set SDA and SCL pins as outputs
  PORTC |= (1 << SDA_PIN) | (1 << SCL_PIN);  // Drive both lines high (idle state for I2C)
}

void I2C_Start(void) {                // Generate an I2C START condition
  PORTC |= (1 << SDA_PIN) | (1 << SCL_PIN);  // Ensure both SDA and SCL start high
  _delay_us(4);                              // Short delay to satisfy I2C timing
  PORTC &= ~(1 << SDA_PIN);                  // Pull SDA low while SCL is still high which is the START condition
  _delay_us(4);                              // Hold for required setup time to validate transfer
  PORTC &= ~(1 << SCL_PIN);                  // Pull SCL low to lock the bus and prepare for data transfer
}

void I2C_Stop(void) {                 // Generate an I2C STOP condition
  PORTC &= ~(1 << SDA_PIN);                  // Make sure SDA is low first
  PORTC |= (1 << SCL_PIN);                   // Raise SCL high while SDA is still held low
  _delay_us(4);                              // Hold for required setup time
  PORTC |= (1 << SDA_PIN);                   // Raise SDA while SCL is high which is the STOP condition
  _delay_us(4);                              // Hold to complete the STOP condition before starting next transmission
}

void I2C_WriteByte(uint8_t byte) {    // Send a single byte (8 bits) across the I2C bus and clear ACK clock
  for (uint8_t i = 0; i < 8; i++) {          // Loop over all 8 bits of the byte
    if (byte & 0x80)                         // Test the most-significant bit
      PORTC |= (1 << SDA_PIN);               // Bit is 1 -> drive SDA high
    else
      PORTC &= ~(1 << SDA_PIN);              // Bit is 0 -> drive SDA low
    _delay_us(2);                            // Let SDA settle before clocking
    PORTC |= (1 << SCL_PIN);                 // Raise SCL high to signal the receiver to capture tha data bit
    _delay_us(2);                            // Hold clock high to satisfy the clock high period timing limit
    PORTC &= ~(1 << SCL_PIN);                // Pull SCL line back low to finalize the pulse and change data state
    byte <<= 1;                              // Shift left so the next bit becomes the MSB
  }
  PORTC |= (1 << SCL_PIN);                   // Raise SCL high for the 9th clock pulse (the ACK bit from the slave)
  _delay_us(2);                              // Hold clock high to let the slave assert ACK
  PORTC &= ~(1 << SCL_PIN);                  // Pull SCL line low to complete the transmission block
}

// 2. OLED Writing Helpers
void OLED_WriteCmd(uint8_t cmd) {     // Send a single command byte to the OLED display
  I2C_Start();                               // Begin I2C transaction
  I2C_WriteByte(OLED_ADDR);                  // Send the OLED's I2C address (write mode)
  I2C_WriteByte(0x00);                       // Control byte 0x00 tells the OLED "the next byte is a command"
  I2C_WriteByte(cmd);                        // Send the actual command byte
  I2C_Stop();                                // End I2C transaction
}

void OLED_WriteData(uint8_t data) {   // Send a single data (pixel/GRAM) byte to the OLED display
  I2C_Start();                               // Begin I2C transaction
  I2C_WriteByte(OLED_ADDR);                  // Send the OLED's I2C address (write mode)
  I2C_WriteByte(0x40);                       // Control byte 0x40 tells the OLED "the next byte is display data"
  I2C_WriteByte(data);                       // Send the actual data byte (a vertical 8-pixel column)
  I2C_Stop();                                // End I2C transaction
}

void OLED_Init(void) {                // Power-on / configuration sequence for the SSD1306
  _delay_ms(100);                            // Give the OLED time to power up before talking to it
  OLED_WriteCmd(0xAE);                       // Display OFF (sleep) while we configure it
  OLED_WriteCmd(0x20);                       // Command parameter to enter memory addressing mode setup
  OLED_WriteCmd(0x02);                       // Set memory addressing configuration to Page Addressing Mode
  OLED_WriteCmd(0xA8);                       // Command parameter to adjust display Multiplex Ratio (height setting)
  OLED_WriteCmd(0x3F);                       // Configure multiplex ratio to 64 lines (representing 128x64 display size)
  OLED_WriteCmd(0x8D);                       // Command parameter to initiate Charge Pump voltage configuration
  OLED_WriteCmd(0x14);                       // Enable internal charge pump power supply to generate active panel voltages
  OLED_WriteCmd(0xAF);                       // Display ON
  _delay_ms(10);                             // Small settling delay after turning the display on
}

void OLED_Clear(void) {               // Blank the entire 128x64 screen (all 8 pages)
  for (uint8_t page = 0; page < 8; page++) { // Iterate over each of the 8 horizontal 8-pixel-tall pages
    OLED_WriteCmd(0xB0 + page);              // Set current page (0xB0 | page number)
    OLED_WriteCmd(0x00);                     // Set column address low nibble = 0
    OLED_WriteCmd(0x10);                     // Set column address high nibble = 0 (column 0 total)
    for (uint8_t col = 0; col < 128; col++) { // Iterate across all 128 columns of this page
      OLED_WriteData(0x00);                  // Write 0x00 (all pixels off) to this column
    }
  }
}

// Clears only typing/menu areas (Pages 1-7) leaving status bar on Page 0 intact
void OLED_ClearMainArea(void) {       // Same as OLED_Clear but skips page 0 so the status bar isn't erased
  for (uint8_t page = 1; page < 8; page++) { // Start at page 1 instead of 0
    OLED_WriteCmd(0xB0 + page);              // Select this page
    OLED_WriteCmd(0x00);                     // Column low nibble = 0
    OLED_WriteCmd(0x10);                     // Column high nibble = 0
    for (uint8_t col = 0; col < 128; col++) { // Sweep all 128 columns
      OLED_WriteData(0x00);                  // Clear this column to blank
    }
  }
}

void OLED_ResetTypeCursor(void) {     // Move the drawing cursor to the start of the "typing" line (page 2)
  OLED_WriteCmd(0xB2);                       // Start typing on Page 2
  OLED_WriteCmd(0x00);                       // Lower col pointer to index 0
  OLED_WriteCmd(0x10);                       // Upper col pointer to index 0
  cursor_col = 0;                            // Reset the tracked cursor column back to 0
}

// 3. ASCII 5x7 Font Definition (ASCII 32 'Space' up to 95 '_')
// Stored in Flash Memory (Total: 64 chars * 5 bytes = 320 bytes)
const uint8_t font5x7[64][5] PROGMEM = {     // 64 characters, each represented by 5 columns of 7/8-pixel-tall glyph data, stored in flash (PROGMEM) to save RAM
  {0x00, 0x00, 0x00, 0x00, 0x00}, // 32: (space)       
  {0x00, 0x00, 0x5F, 0x00, 0x00}, // 33: !              
  {0x00, 0x07, 0x00, 0x07, 0x00}, // 34: "              
  {0x14, 0x7F, 0x14, 0x7F, 0x14}, // 35: #              
  {0x24, 0x2A, 0x7F, 0x2A, 0x12}, // 36: $              
  {0x23, 0x13, 0x08, 0x64, 0x62}, // 37: %              
  {0x36, 0x49, 0x55, 0x22, 0x50}, // 38: &              
  {0x00, 0x05, 0x03, 0x00, 0x00}, // 39: '              
  {0x00, 0x1C, 0x22, 0x41, 0x00}, // 40: (              
  {0x00, 0x41, 0x22, 0x1C, 0x00}, // 41: )              
  {0x08, 0x2A, 0x1C, 0x2A, 0x08}, // 42: *              
  {0x08, 0x08, 0x3E, 0x08, 0x08}, // 43: +              
  {0x00, 0x50, 0x30, 0x00, 0x00}, // 44: ,              
  {0x08, 0x08, 0x08, 0x08, 0x08}, // 45: -              
  {0x00, 0x60, 0x60, 0x00, 0x00}, // 46: .              
  {0x20, 0x10, 0x08, 0x04, 0x02}, // 47: /              
  {0x3E, 0x51, 0x49, 0x45, 0x3E}, // 48: 0              
  {0x00, 0x42, 0x7F, 0x40, 0x00}, // 49: 1              
  {0x42, 0x61, 0x51, 0x49, 0x46}, // 50: 2              
  {0x21, 0x41, 0x45, 0x4B, 0x31}, // 51: 3              
  {0x18, 0x14, 0x12, 0x7F, 0x10}, // 52: 4              
  {0x27, 0x45, 0x45, 0x45, 0x39}, // 53: 5              
  {0x3C, 0x4A, 0x49, 0x49, 0x30}, // 54: 6              
  {0x01, 0x71, 0x09, 0x05, 0x03}, // 55: 7              
  {0x36, 0x49, 0x49, 0x49, 0x36}, // 56: 8              
  {0x06, 0x49, 0x49, 0x29, 0x1E}, // 57: 9              
  {0x00, 0x36, 0x36, 0x00, 0x00}, // 58: :              
  {0x00, 0x56, 0x36, 0x00, 0x00}, // 59: ;              
  {0x00, 0x08, 0x14, 0x22, 0x41}, // 60: <              
  {0x14, 0x14, 0x14, 0x14, 0x14}, // 61: =              
  {0x41, 0x22, 0x14, 0x08, 0x00}, // 62: >              
  {0x02, 0x01, 0x51, 0x09, 0x06}, // 63: ?              
  {0x32, 0x49, 0x79, 0x41, 0x3E}, // 64: @              
  {0x7E, 0x11, 0x11, 0x11, 0x7E}, // 65: A              
  {0x7F, 0x49, 0x49, 0x49, 0x36}, // 66: B              
  {0x3E, 0x41, 0x41, 0x41, 0x22}, // 67: C              
  {0x7F, 0x41, 0x41, 0x22, 0x1C}, // 68: D              
  {0x7F, 0x49, 0x49, 0x49, 0x41}, // 69: E              
  {0x7F, 0x09, 0x09, 0x01, 0x01}, // 70: F              
  {0x3E, 0x41, 0x41, 0x51, 0x32}, // 71: G              
  {0x7F, 0x08, 0x08, 0x08, 0x7F}, // 72: H              
  {0x00, 0x41, 0x7F, 0x41, 0x00}, // 73: I              
  {0x20, 0x40, 0x41, 0x3F, 0x01}, // 74: J              
  {0x7F, 0x08, 0x14, 0x22, 0x41}, // 75: K              
  {0x7F, 0x40, 0x40, 0x40, 0x40}, // 76: L             
  {0x7F, 0x02, 0x0C, 0x02, 0x7F}, // 77: M              
  {0x7F, 0x04, 0x08, 0x10, 0x7F}, // 78: N              
  {0x3E, 0x41, 0x41, 0x41, 0x3E}, // 79: O              
  {0x7F, 0x09, 0x09, 0x09, 0x06}, // 80: P              
  {0x3E, 0x41, 0x51, 0x21, 0x5E}, // 81: Q              
  {0x7F, 0x09, 0x19, 0x29, 0x46}, // 82: R              
  {0x46, 0x49, 0x49, 0x49, 0x31}, // 83: S              
  {0x01, 0x01, 0x7F, 0x01, 0x01}, // 84: T              
  {0x3F, 0x40, 0x40, 0x40, 0x3F}, // 85: U              
  {0x1F, 0x20, 0x40, 0x20, 0x1F}, // 86: V              
  {0x7F, 0x20, 0x18, 0x20, 0x7F}, // 87: W              
  {0x63, 0x14, 0x08, 0x14, 0x63}, // 88: X              
  {0x03, 0x04, 0x78, 0x04, 0x03}, // 89: Y              
  {0x61, 0x51, 0x49, 0x45, 0x43}, // 90: Z              
  {0x00, 0x7F, 0x41, 0x41, 0x00}, // 91: [              
  {0x02, 0x04, 0x08, 0x10, 0x20}, // 92: \ (backslash) 
  {0x00, 0x41, 0x41, 0x7F, 0x00}, // 93: ]              
  {0x04, 0x02, 0x01, 0x02, 0x04}, // 94: ^              
  {0x40, 0x40, 0x40, 0x40, 0x40}  // 95: _              
};                                     

void OLED_WriteChar(char c) {         // Draw a single ASCII character at the current OLED cursor position
  if (c >= 'a' && c <= 'z') {                // If the character is lowercase...
    c -= 32;                                 // ...convert it to uppercase byadjusting the ASCII value offset (font table only has uppercase glyphs)
  }

  if (c < 32 || c > 95) return;              // Ignore any character outside the supported range (space..underscore)

  uint8_t idx = c - 32;                      // Convert ASCII code to a 0-based index into font5x7[]
  for (uint8_t i = 0; i < 5; i++) {          // Loop over the 5 glyph columns for this character
    uint8_t col_data = pgm_read_byte(&(font5x7[idx][i])); // Read one column byte out of flash memory
    OLED_WriteData(col_data);                // Send that column to the OLED's GRAM
  }
  OLED_WriteData(0x00);                      // Write one blank column afterward as inter-character spacing
}

void OLED_WriteString(const char *str) { // Draw a null-terminated string starting at the current cursor
  while (*str) {                             // Loop until we hit the string's null terminator
    OLED_WriteChar(*str);                    // Draw the current character
    str++;                                   // Advance to the next character in the string
  }
}

// 4. Custom status bar layout with larger, thicker battery cells
void OLED_DrawBatteryIcon(uint8_t percentage) { // Draw a battery gauge icon on Page 0, near column 110, based on charge percentage
  // Set cursor to Page 0, Column 110 (0x6E)
  OLED_WriteCmd(0xB0);                       // Select Page 0 (top row, where the status bar lives)
  OLED_WriteCmd(0x0E);                       // Lower Column 110
  OLED_WriteCmd(0x16);                       // Upper Column 110

  uint8_t segments = 0;                      // Number of filled battery "bars" to draw (0-3)
  if (percentage > 75) segments = 3;         // Above 75% -> full 3 bars
  else if (percentage > 45) segments = 2;    // Above 45% -> 2 bars
  else if (percentage > 15)  segments = 1;   // Above 15% -> 1 bar (otherwise 0 bars, near empty)

  OLED_WriteData(0x00);                      // Spacing gap before the icon starts
  OLED_WriteData(0x7E);                      // Thick left outer frame border [0111 1110]

  // Draw 3 thick internal columns, separated by distinct gaps
  for (uint8_t i = 0; i < 3; i++) {          // Loop for each of the 3 battery bar slots
    OLED_WriteData(0x42);                    // Internal gap [0100 0010] (Top and bottom frame only) between segments
    if (i < segments) {                      // If this segment index is within the "filled" count...
      OLED_WriteData(0x7E);                  // Thick, tall solid segment (filled bar), drawn twice for extra width
      OLED_WriteData(0x7E);
    } else {                                 // Otherwise this segment is empty
      OLED_WriteData(0x42);                  // Empty segment outline (just top/bottom border), drawn twice for width
      OLED_WriteData(0x42);
    }
  }
  OLED_WriteData(0x42);                      // Internal gap after the last segment
  OLED_WriteData(0x7E);                      // Right outer frame wall
  OLED_WriteData(0x3C);                      // Thicker centered battery tip [0011 1100]
  OLED_WriteData(0x3C);                      // Second column of the battery tip nub
}

void OLED_UpdateStatusBar(void) {     // Redraw the time (HH:MM) and battery icon on Page 0
  OLED_WriteCmd(0xB0);                       // Select Page 0
  OLED_WriteCmd(0x00);                       // Column low nibble = 0
  OLED_WriteCmd(0x10);                       // Column high nibble = 0 (start at column 0)

  OLED_WriteChar('0' + (hours / 10));        // Draw the tens digit of the hour
  OLED_WriteChar('0' + (hours % 10));        // Draw the ones digit of the hour
  OLED_WriteChar(':');                       // Draw a colon separator
  OLED_WriteChar('0' + (minutes / 10));      // Draw the tens digit of the minute
  OLED_WriteChar('0' + (minutes % 10));      // Draw the ones digit of the minute

  OLED_DrawBatteryIcon(battery);             // Draw the battery gauge using the current battery level
}

// Draws navigation key helper labels along Page 7 footer
void OLED_DrawFooter(void) {          // Show "* BACK" and "OK #" hint labels at the bottom of the screen
  // Left Footer: Column 0 on Page 7
  OLED_WriteCmd(0xB7);                       // Select Page 7 (bottom row)
  OLED_WriteCmd(0x00);                       // Column low nibble = 0
  OLED_WriteCmd(0x10);                       // Column high nibble = 0 (start at column 0)
  OLED_WriteString("* BACK");                // Draw the left-side hint text

  // Right Footer: Column 96 on Page 7 (0x60 -> Lower: 0, Upper: 6)
  OLED_WriteCmd(0xB7);                       // Select Page 7 again
  OLED_WriteCmd(0x00);                       // Lower column nibble = 0
  OLED_WriteCmd(0x16);                       // Upper column nibble = 6 (column 96 total)
  OLED_WriteString("OK #");                  // Draw the right-side hint text
}

// 5. Operating System Screens
void Draw_HomeMenu(void) {            // Render the main menu with the 3 options
  OLED_ClearMainArea();                      // Wipe everything except the status bar

  // Note: Divider line loop on Page 1 is removed for a cleaner body layout

  OLED_WriteCmd(0xB2); OLED_WriteCmd(0x00); OLED_WriteCmd(0x10); // Move cursor to Page 2, column 0
  OLED_WriteString("[1] DIAL-UP");           // Draw the first menu option

  OLED_WriteCmd(0xB4); OLED_WriteCmd(0x00); OLED_WriteCmd(0x10); // Move cursor to Page 4, column 0
  OLED_WriteString("[2] SMS MESSAGE");       // Draw the second menu option

  OLED_WriteCmd(0xB6); OLED_WriteCmd(0x00); OLED_WriteCmd(0x10); // Move cursor to Page 6, column 0
  OLED_WriteString("[3] FILE MANAGER");      // Draw the third menu option
}

void Draw_DialUpScreen(void) {        // Render the "enter phone number" screen
  OLED_ClearMainArea();                      // Wipe everything except the status bar
  OLED_WriteCmd(0xB2); OLED_WriteCmd(0x00); OLED_WriteCmd(0x10); // Move cursor to Page 2, column 0
  OLED_WriteString("ENTER NUMBER:");         // Draw the prompt text
  OLED_DrawFooter();                         // Draw the "* BACK" / "OK #" hints
}

void OLED_RedrawDialBuffer(void) {    // Repaint the line showing the digits typed so far for dialing
  OLED_WriteCmd(0xB4);                       // Select Page 4 (the line used to show entered digits)
  OLED_WriteCmd(0x00);                       // Column low nibble = 0
  OLED_WriteCmd(0x10);                       // Column high nibble = 0
  for (uint8_t i = 0; i < 128; i++) {        // Sweep the whole width of the page
    OLED_WriteData(0x00);                    // Clear it first to erase any previous digits
  }

  OLED_WriteCmd(0xB4);                       // Re-select Page 4
  OLED_WriteCmd(0x00);                       // Column low nibble = 0
  OLED_WriteCmd(0x10);                       // Column high nibble = 0 (back to column 0)

  for (uint8_t i = 0; i < dial_length; i++) { // Loop over every digit currently stored
    OLED_WriteChar(dial_buffer[i]);          // Draw that digit
  }
}

// 6. SMS Routing Screens
void Draw_SMSRecipientScreen(void) {  // Render the "To:" recipient entry screen for SMS
  OLED_ClearMainArea();                      // Wipe everything except the status bar

  OLED_WriteCmd(0xB2); OLED_WriteCmd(0x00); OLED_WriteCmd(0x10); // Move cursor to Page 2, column 0
  OLED_WriteString("TO:");                   // Draw the "To:" label
  OLED_DrawFooter();                         // Draw the navigation hint labels
}

void OLED_RedrawSMSBuffer(void) {     // Repaint the line showing the recipient digits typed so far
  OLED_WriteCmd(0xB4);                       // Select Page 4 (the entry line)
  OLED_WriteCmd(0x00);                       // Column low nibble = 0
  OLED_WriteCmd(0x10);                       // Column high nibble = 0
  for (uint8_t i = 0; i < 128; i++) {        // Sweep the whole page width
    OLED_WriteData(0x00);                    // Clear previous content first
  }

  OLED_WriteCmd(0xB4);                       // Re-select Page 4
  OLED_WriteCmd(0x00);                       // Column low nibble = 0
  OLED_WriteCmd(0x10);                       // Column high nibble = 0 (back to column 0)

  for (uint8_t i = 0; i < sms_length; i++) { // Loop over every digit currently in the SMS buffer
    OLED_WriteChar(sms_buffer[i]);           // Draw that digit
  }
}

void Draw_SMSOptionsScreen(void) {    // Render the preset-message selection screen for SMS
  OLED_ClearMainArea();                      // Wipe everything except the status bar

  OLED_WriteCmd(0xB1); OLED_WriteCmd(0x00); OLED_WriteCmd(0x10); // Move cursor to Page 1, column 0
  OLED_WriteString("[1] HELLO");             // Draw preset option 1

  OLED_WriteCmd(0xB3); OLED_WriteCmd(0x00); OLED_WriteCmd(0x10); // Move cursor to Page 3, column 0
  OLED_WriteString("[2] OK");                // Draw preset option 2

  OLED_WriteCmd(0xB5); OLED_WriteCmd(0x00); OLED_WriteCmd(0x10); // Move cursor to Page 5, column 0
  OLED_WriteString("[3] ON MY WAY");         // Draw preset option 3

  // Left Footer on Page 7 to allow the user to back out
  OLED_WriteCmd(0xB7);                       // Select Page 7 (bottom row)
  OLED_WriteCmd(0x00);                       // Column low nibble = 0
  OLED_WriteCmd(0x10);                       // Column high nibble = 0
  OLED_WriteString("* BACK");                // Draw the back-navigation hint
}

// 7. Transmission Animation Sequence (Envelope moving to tower)
void Run_SMSTransmissionAnimation(void) { // Play an animation of an envelope icon sliding toward a tower icon, then show "SENT!"
  OLED_ClearMainArea();                      // Clear the screen (except status bar) before animating

  // Redesigned upscale 13x8 envelope (with realistic inner diagonals and thick borders)
  const uint8_t envelope[13] = {0xFF, 0x83, 0x85, 0x89, 0x91, 0xA1, 0xC1, 0xA1, 0x91, 0x89, 0x85, 0x83, 0xFF}; // 13 columns of bitmap data forming the envelope icon

  // Redesigned Eiffel-style 11-column cellular transmission tower with diagonal lattice girders
  const uint8_t tower[11] = {0x80, 0x40, 0x23, 0x14, 0x0E, 0x7F, 0x0E, 0x14, 0x23, 0x40, 0x80}; // 11 columns of bitmap data forming the tower icon

  // Draw the cell tower static on the right (Column 110) of Page 4
  OLED_WriteCmd(0xB4);                       // Select Page 4 (the animation row)
  OLED_WriteCmd(0x0E); // Lower Column 110 (0x6E)   -- lower nibble of column address 110
  OLED_WriteCmd(0x16); // Upper Column 110          -- upper nibble of column address 110
  for (uint8_t i = 0; i < 11; i++) {         // Loop over each column of the tower bitmap
    OLED_WriteData(tower[i]);                // Draw that column (tower stays fixed on the right side)
  }

  // Animate the envelope sliding horizontally on Page 4 (stops adjacent to the tower at Column 96)
  for (uint8_t col = 0; col < 96; col += 2) { // Step the envelope's position across the screen, 2 columns at a time
    // Clear tail space to prevent ghost streaks
    if (col >= 2) {                          // Once we've moved at least 2 columns...
      uint8_t clear_col = col - 2;           // ...compute the position just behind the envelope's current spot
      OLED_WriteCmd(0xB4);                   // Select Page 4
      OLED_WriteCmd(clear_col & 0x0F);       // Set column low nibble for the trailing position
      OLED_WriteCmd(0x10 | ((clear_col >> 4) & 0x0F)); // Set column high nibble for the trailing position
      OLED_WriteData(0x00);                  // Erase the leftover pixel column (part 1)
      OLED_WriteData(0x00);                  // Erase the leftover pixel column (part 2, to fully clear the gap)
    }

    // Draw envelope
    OLED_WriteCmd(0xB4);                     // Select Page 4
    OLED_WriteCmd(col & 0x0F);               // Set column low nibble for the envelope's current position
    OLED_WriteCmd(0x10 | ((col >> 4) & 0x0F)); // Set column high nibble for the envelope's current position
    for (uint8_t i = 0; i < 13; i++) {       // Loop over all 13 columns of the envelope bitmap
      OLED_WriteData(envelope[i]);           // Draw each column of the envelope at this position
    }

    _delay_ms(35);                           // Pause briefly so the sliding motion is visible to the eye
  }

  _delay_ms(300); // Hold at tower impact          -- pause once the envelope reaches the tower

  // Wipe screen and display success prompt
  OLED_ClearMainArea();                      // Clear the animation area
  OLED_WriteCmd(0xB4); // Page 4                   -- select page 4 for the success message
  OLED_WriteCmd(0x08); // Column 40 (Lower: 8, Upper: 2)  -- column low nibble for column 40
  OLED_WriteCmd(0x12);                       // Column high nibble for column 40 (roughly centers "SENT!")
  OLED_WriteString("SENT!");                 // Draw the success message

  _delay_ms(1500); // 1.5 seconds success pause    -- lets the user read "SENT!" before returning

  // Route back to home menu
  current_state = STATE_HOME_MENU;           // Update the state machine back to the home menu
  Draw_HomeMenu();                           // Redraw the home menu screen
}

// 8. 3x4 Keypad Configuration & Scan Functions (Using PD0, PD1, PD2)
const char keys[4][3] = {             // Lookup table mapping (row, column) to the physical key label
  {'1', '2', '3'},                           // Row 0
  {'4', '5', '6'},                           // Row 1
  {'7', '8', '9'},                           // Row 2
  {'*', '0', '#'}                            // Row 3
};

void Keypad_Init(void) {              // Configure the GPIO pins used for keypad scanning
  DDRB &= ~((1 << PB0) | (1 << PB1) | (1 << PB2) | (1 << PB3)); // Set PB0-PB3 (row inputs) as inputs
  PORTB |= (1 << PB0) | (1 << PB1) | (1 << PB2) | (1 << PB3);   // Enable internal pull-up resistors on the row pins
  DDRD |= (1 << PD0) | (1 << PD1) | (1 << PD2);                 // Set PD0-PD2 (column outputs) as outputs
  PORTD |= (1 << PD0) | (1 << PD1) | (1 << PD2);                // Drive all column outputs high (inactive) by default
}

char Keypad_Scan(void) {              // Scan the keypad matrix and return the pressed key, or 0 if none
  for (uint8_t col = 0; col < 3; col++) {    // Loop over each of the 3 columns
    if (col == 0) PORTD &= ~(1 << PD0);      // Drive column 0 low to test it (active-low scanning)
    else if (col == 1) PORTD &= ~(1 << PD1); // Drive column 1 low to test it
    else if (col == 2) PORTD &= ~(1 << PD2); // Drive column 2 low to test it

    _delay_us(5);                            // Brief delay to let the signal settle before reading rows

    for (uint8_t row = 0; row < 4; row++) {  // Loop over each of the 4 rows
      if (!(PINB & (1 << row))) {            // If this row reads low, a key in this row/column is pressed
        _delay_ms(20);                       // Debounce delay: wait, then re-check

        if (!(PINB & (1 << row))) {          // Confirm the key is still pressed after debounce delay
          while (!(PINB & (1 << row)));      // Wait here until the key is released (blocks until release)
          _delay_ms(20);                     // Debounce delay after release too

          PORTD |= (1 << PD0) | (1 << PD1) | (1 << PD2); // Restore all column outputs to high (idle) before returning
          return keys[row][col];             // Return the character corresponding to this row/column
        }
      }
    }
    PORTD |= (1 << PD0) | (1 << PD1) | (1 << PD2); // No key found in this column; restore it high before moving to next column
  }
  return 0;                                  // No key was pressed in any column/row
}

// 9. PC Compiler Clock Sync & Background Timers (Timer 1)
void InitializeTimeFromCompiler(void) { // Seed the clock variables using the PC's build time at compile time
  const char *comp_time = __TIME__; // Formatted as "HH:MM:SS"   -- compiler-provided string literal of build time
  hours   = (comp_time[0] - '0') * 10 + (comp_time[1] - '0'); // Parse the two hour digits into a number
  minutes = (comp_time[3] - '0') * 10 + (comp_time[4] - '0'); // Parse the two minute digits into a number
  seconds = (comp_time[6] - '0') * 10 + (comp_time[7] - '0'); // Parse the two second digits into a number
}

void Timer1_Init(void) {              // Configure hardware Timer1 to interrupt once per second
  TCCR1B |= (1 << WGM12);  // CTC mode                -- Clear Timer on Compare match mode, so the timer resets at OCR1A
  TIMSK1 |= (1 << OCIE1A); // Match A interrupt        -- enable the "compare match A" interrupt
  OCR1A = 62499;           // Match at 1 second (16MHz / 256 prescaler) -- compare value chosen so overflow happens exactly once per second
  TCCR1B |= (1 << CS12);   // Prescaler 256, start timer -- select clock/256 prescaler, which also starts the timer running
  sei();                                     // Globally enable interrupts so the ISR can fire
}

ISR(TIMER1_COMPA_vect) {              // Interrupt Service Routine: runs once per second (Timer1 compare match)
  seconds++;                                 // Advance the seconds counter
  if (seconds >= 60) {                       // If a full minute has elapsed...
    seconds = 0;                             // ...reset seconds
    minutes++;                               // ...and advance minutes

    if (minutes >= 60) {                     // If a full hour has elapsed...
      minutes = 0;                           // ...reset minutes
      hours++;                               // ...and advance hours
      if (hours >= 24) {                     // If a full day has elapsed...
        hours = 0;                           // ...wrap hours back to 0
      }
    }
  }

  // Fast Battery simulation: drops 25% (1 bar) every 20 seconds for simulation display
  static uint8_t battery_tick = 0;           // Counts seconds elapsed since the last battery drop (persists between ISR calls)
  battery_tick++;                            // Increment the tick counter each second
  if (battery_tick >= 20) {                  // Every 20 seconds...
    battery_tick = 0;                        // ...reset the counter
    if (battery > 25) {                      // If there's more than one "bar" of charge left...
      battery -= 25;                         // ...drain one bar (25%)
    } else {
      battery = 99; // Re-charge                    -- otherwise simulate a recharge back to ~full
    }
  }

  update_status_bar = 1;                     // Signal main() that the status bar needs to be redrawn
}

// 10. Main Loop and State Machine Routing
int main(void) {                      // Program entry point
  I2C_Init();                                // Set up the bit-banged I2C bus pins
  OLED_Init();                               // Initialize the SSD1306 OLED controller
  OLED_Clear();                              // Blank the entire screen
  Keypad_Init();                             // Set up the keypad GPIO pins
  InitializeTimeFromCompiler();              // Seed the clock from the compiler's build timestamp
  Timer1_Init();                             // Start the 1-second hardware timer/interrupt

  Draw_HomeMenu(); // Start on Nokia Menu          -- render the initial home screen

  while (1) {                                // Main superloop; runs forever
    if (update_status_bar) {                 // If the ISR flagged that a second has passed...
      OLED_UpdateStatusBar();                // ...redraw the clock and battery icon
      update_status_bar = 0;                 // ...and clear the flag until next time
    }

    char key = Keypad_Scan();                // Poll the keypad for a (debounced) key press

    if (key != 0) {                          // Only act if a key was actually pressed
      switch (current_state) {               // Dispatch behavior based on which screen we're currently on

        case STATE_HOME_MENU:                        // We're on the home menu
          if (key == '1') {                          // Key '1' selects Dial-Up
            current_state = STATE_DIAL_UP;            // Switch state to dial-up entry
            dial_length = 0;                          // Reset the dial buffer
            Draw_DialUpScreen();                      // Render the dial-up screen
          }
          else if (key == '2') {                      // Key '2' selects SMS
            current_state = STATE_SMS_RECIPIENT;      // Switch state to SMS recipient entry
            sms_length = 0;                           // Reset the SMS buffer
            Draw_SMSRecipientScreen();                // Render the SMS recipient screen
          }
          else if (key == '3') {                      // Key '3' selects File Manager
            current_state = STATE_FILE_MANAGER;       // Switch state to the file manager placeholder
            OLED_ClearMainArea();                     // Clear the body of the screen
            OLED_WriteCmd(0xB2); OLED_WriteCmd(0x00); OLED_WriteCmd(0x10); // Move cursor to Page 2, column 0
            OLED_WriteString("FILE MANAGER");         // Draw the header text
            OLED_WriteCmd(0xB4); OLED_WriteCmd(0x00); OLED_WriteCmd(0x10); // Move cursor to Page 4, column 0
            OLED_WriteString("EMPTY CARD");            // Indicate there are no files
            OLED_WriteCmd(0xB6); OLED_WriteCmd(0x00); OLED_WriteCmd(0x10); // Move cursor to Page 6, column 0
            OLED_WriteString("PRESS # TO EXIT");        // Show the exit hint
          }
          break;                                       // Done handling this state

        case STATE_DIAL_UP:                           // We're entering a phone number
          if (key >= '0' && key <= '9') {              // Digit keys append to the number
            if (dial_length < 15) {                    // Only if there's still room in the buffer
              dial_buffer[dial_length++] = key;        // Store the digit and advance the length
              OLED_RedrawDialBuffer();                 // Repaint the buffer on screen
            }
          }
          else if (key == '*') {                       // '*' is Backspace/Back
            if (dial_length > 0) {                      // If there's something typed...
              dial_length--;                            // ...remove the last digit
              OLED_RedrawDialBuffer();                  // ...and repaint
            } else {
              // Navigation: Back Key when buffer is empty returns to Home Menu
              current_state = STATE_HOME_MENU;          // Otherwise, go back to the home menu
              Draw_HomeMenu();                          // Render the home menu
            }
          }
          else if (key == '#') { // '#' dials the number
            if (dial_length > 0) {                      // Only "dial" if a number was actually entered
              current_state = STATE_DIAL_CALLING;       // Switch to the calling state
              OLED_ClearMainArea();                     // Clear the screen body

              OLED_WriteCmd(0xB2); OLED_WriteCmd(0x00); OLED_WriteCmd(0x10); // Move cursor to Page 2, column 0
              OLED_WriteString("CALLING...");           // Show the "calling" message

              OLED_WriteCmd(0xB4); OLED_WriteCmd(0x00); OLED_WriteCmd(0x10); // Move cursor to Page 4, column 0
              for (uint8_t i = 0; i < dial_length; i++) { // Loop over the dialed digits
                OLED_WriteChar(dial_buffer[i]);          // Draw each digit of the number being called
              }

              OLED_WriteCmd(0xB6); OLED_WriteCmd(0x00); OLED_WriteCmd(0x10); // Move cursor to Page 6, column 0
              OLED_WriteString("ANY KEY TO HANGUP");     // Show the hangup hint
            }
          }
          break;                                        // Done handling this state

        case STATE_DIAL_CALLING:                       // We're in the "calling" screen
          current_state = STATE_HOME_MENU;              // Any key press hangs up and returns home
          Draw_HomeMenu();                              // Render the home menu
          break;                                        // Done handling this state

        case STATE_SMS_RECIPIENT:                      // We're entering the SMS "To:" number
          if (key >= '0' && key <= '9') {               // Digit keys append to the recipient number
            if (sms_length < 15) {                      // Only if there's still room
              sms_buffer[sms_length++] = key;           // Store the digit and advance length
              OLED_RedrawSMSBuffer();                   // Repaint the buffer
            }
          }
          else if (key == '*') {                        // '*' is Backspace/Back
            if (sms_length > 0) {                        // If there's something typed...
              sms_length--;                              // ...remove the last digit
              OLED_RedrawSMSBuffer();                    // ...and repaint
            } else {
              // Navigation: Back Key when buffer is empty returns to Home Menu
              current_state = STATE_HOME_MENU;           // Otherwise go back to home menu
              Draw_HomeMenu();                           // Render the home menu
            }
          }
          else if (key == '#') { // '#' moves to message option selection
            if (sms_length > 0) {                        // Only proceed if a recipient number was entered
              current_state = STATE_SMS_OPTIONS;         // Move to the preset-message selection state
              Draw_SMSOptionsScreen();                   // Render that screen
            }
          }
          break;                                         // Done handling this state

        case STATE_SMS_OPTIONS:                         // We're choosing a preset message
          // Selecting keys 1 to 3 plays transmission animation and exits
          if (key >= '1' && key <= '3') {                // Any of the 3 preset options...
            current_state = STATE_SMS_ANIMATION;         // ...moves us into the sending-animation state
            Run_SMSTransmissionAnimation();               // ...and plays/handles the whole animation + return to home
          }
          // Pressing '*' acts as Back button and goes back to Recipient screen
          else if (key == '*') {                         // '*' backs out to the recipient entry screen
            current_state = STATE_SMS_RECIPIENT;          // Switch state back
            Draw_SMSRecipientScreen();                    // Redraw the recipient screen
            OLED_RedrawSMSBuffer();                       // Restore the previously entered digits on screen
          }
          break;                                          // Done handling this state

        case STATE_FILE_MANAGER:                        // We're on the placeholder file manager screen
          if (key == '#') {                              // '#' exits back to the home menu
            current_state = STATE_HOME_MENU;              // Switch state back
            Draw_HomeMenu();                              // Render the home menu
          }
          break;                                          // Done handling this state

        default:                                         // Should never happen (unknown state)
          break;                                          // No-op safety fallback
      }
    }
  }
}                                        // End of main() — the while(1) loop above never actually exits