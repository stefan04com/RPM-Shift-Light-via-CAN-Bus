#ifndef LiquidCrystal_I2C_h
#define LiquidCrystal_I2C_h

#include <Wire.h>
#include <Print.h>

#define En 0x04
#define Rw 0x02
#define Rs 0x01

#define LCD_BACKLIGHT 0x08
#define LCD_NOBACKLIGHT 0x00

#define LCD_CLEARDISPLAY 0x01
#define LCD_RETURNHOME 0x02
#define LCD_ENTRYMODESET 0x04
#define LCD_DISPLAYCONTROL 0x08
#define LCD_FUNCTIONSET 0x20
#define LCD_SETDDRAMADDR 0x80

#define LCD_ENTRYLEFT 0x02
#define LCD_ENTRYSHIFTDECREMENT 0x00
#define LCD_DISPLAYON 0x04
#define LCD_CURSORON 0x02
#define LCD_CURSOROFF 0x00
#define LCD_BLINKON 0x01
#define LCD_BLINKOFF 0x00
#define LCD_4BITMODE 0x00
#define LCD_2LINE 0x08
#define LCD_5x8DOTS 0x00

class LiquidCrystal_I2C : public Print {
public:
  LiquidCrystal_I2C(uint8_t lcd_addr, uint8_t lcd_cols, uint8_t lcd_rows);
  void init();
  void begin();
  void clear();
  void home();
  void noDisplay();
  void display();
  void noBlink();
  void blink();
  void noCursor();
  void cursor();
  void scrollDisplayLeft();
  void scrollDisplayRight();
  void leftToRight();
  void rightToLeft();
  void autoscroll();
  void noAutoscroll();
  void createChar(uint8_t location, uint8_t charmap[]);
  void setCursor(uint8_t col, uint8_t row);
  void backlight();
  void noBacklight();
  virtual size_t write(uint8_t value);
  using Print::write;

protected:
  void command(uint8_t value);
  void send(uint8_t value, uint8_t mode);
  void write4bits(uint8_t value);
  void expanderWrite(uint8_t value);
  void pulseEnable(uint8_t value);

  uint8_t _addr;
  uint8_t _cols;
  uint8_t _rows;
  uint8_t _backlightval;
  uint8_t _displayfunction;
  uint8_t _displaycontrol;
  uint8_t _displaymode;
  uint8_t _numlines;
};

#endif