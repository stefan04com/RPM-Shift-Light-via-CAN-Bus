#include "LiquidCrystal_I2C.h"
#include <Arduino.h>

LiquidCrystal_I2C::LiquidCrystal_I2C(uint8_t lcd_addr, uint8_t lcd_cols, uint8_t lcd_rows) {
  _addr = lcd_addr;
  _cols = lcd_cols;
  _rows = lcd_rows;
  _numlines = lcd_rows;
  _backlightval = LCD_BACKLIGHT;
  _displayfunction = LCD_4BITMODE | LCD_2LINE | LCD_5x8DOTS;
}

void LiquidCrystal_I2C::begin() {
  init();
}

void LiquidCrystal_I2C::init() {
  Wire.begin();
  _displaycontrol = LCD_DISPLAYON | LCD_CURSOROFF | LCD_BLINKOFF;
  _displaymode = LCD_ENTRYLEFT | LCD_ENTRYSHIFTDECREMENT;
  expanderWrite(_backlightval);
  delay(50);

  write4bits(0x03 << 4);
  delayMicroseconds(4500);
  write4bits(0x03 << 4);
  delayMicroseconds(4500);
  write4bits(0x03 << 4);
  delayMicroseconds(150);
  write4bits(0x02 << 4);

  command(LCD_FUNCTIONSET | _displayfunction);
  display();
  clear();
  home();
  command(LCD_ENTRYMODESET | _displaymode);
}

void LiquidCrystal_I2C::clear() {
  command(LCD_CLEARDISPLAY);
  delayMicroseconds(2000);
}

void LiquidCrystal_I2C::home() {
  command(LCD_RETURNHOME);
  delayMicroseconds(2000);
}

void LiquidCrystal_I2C::noDisplay() {
  _displaycontrol &= ~LCD_DISPLAYON;
  command(LCD_DISPLAYCONTROL | _displaycontrol);
}

void LiquidCrystal_I2C::display() {
  _displaycontrol |= LCD_DISPLAYON;
  command(LCD_DISPLAYCONTROL | _displaycontrol);
}

void LiquidCrystal_I2C::noBlink() {
  _displaycontrol &= ~LCD_BLINKON;
  command(LCD_DISPLAYCONTROL | _displaycontrol);
}

void LiquidCrystal_I2C::blink() {
  _displaycontrol |= LCD_BLINKON;
  command(LCD_DISPLAYCONTROL | _displaycontrol);
}

void LiquidCrystal_I2C::noCursor() {
  _displaycontrol &= ~LCD_CURSORON;
  command(LCD_DISPLAYCONTROL | _displaycontrol);
}

void LiquidCrystal_I2C::cursor() {
  _displaycontrol |= LCD_CURSORON;
  command(LCD_DISPLAYCONTROL | _displaycontrol);
}

void LiquidCrystal_I2C::scrollDisplayLeft() {
  command(0x18);
}

void LiquidCrystal_I2C::scrollDisplayRight() {
  command(0x1C);
}

void LiquidCrystal_I2C::leftToRight() {
  _displaymode |= LCD_ENTRYLEFT;
  command(LCD_ENTRYMODESET | _displaymode);
}

void LiquidCrystal_I2C::rightToLeft() {
  _displaymode &= ~LCD_ENTRYLEFT;
  command(LCD_ENTRYMODESET | _displaymode);
}

void LiquidCrystal_I2C::autoscroll() {
  _displaymode |= 0x01;
  command(LCD_ENTRYMODESET | _displaymode);
}

void LiquidCrystal_I2C::noAutoscroll() {
  _displaymode &= ~0x01;
  command(LCD_ENTRYMODESET | _displaymode);
}

void LiquidCrystal_I2C::createChar(uint8_t location, uint8_t charmap[]) {
  location &= 0x7;
  command(0x40 | (location << 3));
  for (int i = 0; i < 8; i++) {
    write(charmap[i]);
  }
}

void LiquidCrystal_I2C::setCursor(uint8_t col, uint8_t row) {
  static uint8_t row_offsets[] = {0x00, 0x40, 0x14, 0x54};
  if (row >= _rows) {
    row = _rows - 1;
  }
  command(LCD_SETDDRAMADDR | (col + row_offsets[row]));
}

void LiquidCrystal_I2C::backlight() {
  _backlightval = LCD_BACKLIGHT;
  expanderWrite(0);
}

void LiquidCrystal_I2C::noBacklight() {
  _backlightval = LCD_NOBACKLIGHT;
  expanderWrite(0);
}

void LiquidCrystal_I2C::command(uint8_t value) {
  send(value, 0);
}

void LiquidCrystal_I2C::send(uint8_t value, uint8_t mode) {
  uint8_t highnib = value & 0xF0;
  uint8_t lownib = (value << 4) & 0xF0;
  write4bits(highnib | mode);
  write4bits(lownib | mode);
}

void LiquidCrystal_I2C::write4bits(uint8_t value) {
  expanderWrite(value);
  pulseEnable(value);
}

void LiquidCrystal_I2C::expanderWrite(uint8_t value) {
  Wire.beginTransmission(_addr);
  Wire.write(value | _backlightval);
  Wire.endTransmission();
}

void LiquidCrystal_I2C::pulseEnable(uint8_t value) {
  expanderWrite(value | En);
  delayMicroseconds(1);
  expanderWrite(value & ~En);
  delayMicroseconds(50);
}

size_t LiquidCrystal_I2C::write(uint8_t value) {
  send(value, Rs);
  return 1;
}
