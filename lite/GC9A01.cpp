#include "daisy_seed.h"

#include "GC9A01.hpp"

void GC9A01::Init(void) {
  dc_.Write(true); // Leave in data mode by default

  // WriteCommand(0x00); // Put SPI into known state

  // Toggle to perform reset
  rst_.Write(true);
  daisy::System::Delay(5);
  rst_.Write(false);
  daisy::System::Delay(20);
  rst_.Write(true);

  WriteCommand(0xEF);
  WriteCommand(0xEB);
  WriteData8(0x14);

  WriteCommand(0xFE);
  WriteCommand(0xEF);

  WriteCommand(0xEB);
  WriteData8(0x14);

  WriteCommand(0x84);
  WriteData8(0x40);

  WriteCommand(0x85);
  WriteData8(0xFF);

  WriteCommand(0x86);
  WriteData8(0xFF);

  WriteCommand(0x87);
  WriteData8(0xFF);

  WriteCommand(0x88);
  WriteData8(0x0A);

  WriteCommand(0x89);
  WriteData8(0x21);

  WriteCommand(0x8A);
  WriteData8(0x00);

  WriteCommand(0x8B);
  WriteData8(0x80);

  WriteCommand(0x8C);
  WriteData8(0x01);

  WriteCommand(0x8D);
  WriteData8(0x01);

  WriteCommand(0x8E);
  WriteData8(0xFF);

  WriteCommand(0x8F);
  WriteData8(0xFF);

  WriteCommand(0xB6);
  WriteData8(0x00);
  WriteData8(0x20);

  WriteCommand(0x3A);
  WriteData8(0x05);

  WriteCommand(0x90);
  WriteData8(0x08);
  WriteData8(0x08);
  WriteData8(0x08);
  WriteData8(0x08);

  WriteCommand(0xBD);
  WriteData8(0x06);

  WriteCommand(0xBC);
  WriteData8(0x00);

  WriteCommand(0xFF);
  WriteData8(0x60);
  WriteData8(0x01);
  WriteData8(0x04);

  WriteCommand(0xC3);
  WriteData8(0x13);
  WriteCommand(0xC4);
  WriteData8(0x13);

  WriteCommand(0xC9);
  WriteData8(0x22);

  WriteCommand(0xBE);
  WriteData8(0x11);

  WriteCommand(0xE1); // Uncocumented
  WriteData8(0x10);
  WriteData8(0x0E);

  WriteCommand(0xDF); // Undocumented
  WriteData8(0x21);
  WriteData8(0x0c);
  WriteData8(0x02);

  WriteCommand(0xF0); // Set Gamma 1
  WriteData8(0x45);
  WriteData8(0x09);
  WriteData8(0x08);
  WriteData8(0x08);
  WriteData8(0x26);
  WriteData8(0x2A);

  WriteCommand(0xF1); // Set Gamma 2
  WriteData8(0x43);
  WriteData8(0x70);
  WriteData8(0x72);
  WriteData8(0x36);
  WriteData8(0x37);
  WriteData8(0x6F);

  WriteCommand(0xF2); // Set Gamma 3
  WriteData8(0x45);
  WriteData8(0x09);
  WriteData8(0x08);
  WriteData8(0x08);
  WriteData8(0x26);
  WriteData8(0x2A);

  WriteCommand(0xF3); // Set Gamma 4
  WriteData8(0x43);
  WriteData8(0x70);
  WriteData8(0x72);
  WriteData8(0x36);
  WriteData8(0x37);
  WriteData8(0x6F);

  WriteCommand(0xED); // Undocumented
  WriteData8(0x1B);
  WriteData8(0x0B);

  WriteCommand(0xAE); // Undocumented
  WriteData8(0x77);

  WriteCommand(0xCD); // Undocumented
  WriteData8(0x63);

  WriteCommand(0x70); // Undocumented
  WriteData8(0x07);
  WriteData8(0x07);
  WriteData8(0x04);
  WriteData8(0x0E);
  WriteData8(0x0F);
  WriteData8(0x09);
  WriteData8(0x07);
  WriteData8(0x08);
  WriteData8(0x03);

  WriteCommand(0xE8); // Frame rate
  WriteData8(0x34);

  WriteCommand(0x62); // Undocumented
  WriteData8(0x18);
  WriteData8(0x0D);
  WriteData8(0x71);
  WriteData8(0xED);
  WriteData8(0x70);
  WriteData8(0x70);
  WriteData8(0x18);
  WriteData8(0x0F);
  WriteData8(0x71);
  WriteData8(0xEF);
  WriteData8(0x70);
  WriteData8(0x70);

  WriteCommand(0x63); // Undocumented
  WriteData8(0x18);
  WriteData8(0x11);
  WriteData8(0x71);
  WriteData8(0xF1);
  WriteData8(0x70);
  WriteData8(0x70);
  WriteData8(0x18);
  WriteData8(0x13);
  WriteData8(0x71);
  WriteData8(0xF3);
  WriteData8(0x70);
  WriteData8(0x70);

  WriteCommand(0x64); // Undocumented
  WriteData8(0x28);
  WriteData8(0x29);
  WriteData8(0xF1);
  WriteData8(0x01);
  WriteData8(0xF1);
  WriteData8(0x00);
  WriteData8(0x07);

  WriteCommand(0x66); // Undocumented
  WriteData8(0x3C);
  WriteData8(0x00);
  WriteData8(0xCD);
  WriteData8(0x67);
  WriteData8(0x45);
  WriteData8(0x45);
  WriteData8(0x10);
  WriteData8(0x00);
  WriteData8(0x00);
  WriteData8(0x00);

  WriteCommand(0x67); // Undocumented
  WriteData8(0x00);
  WriteData8(0x3C);
  WriteData8(0x00);
  WriteData8(0x00);
  WriteData8(0x00);
  WriteData8(0x01);
  WriteData8(0x54);
  WriteData8(0x10);
  WriteData8(0x32);
  WriteData8(0x98);

  WriteCommand(0x74); // Undocumented
  WriteData8(0x10);
  WriteData8(0x85);
  WriteData8(0x80);
  WriteData8(0x00);
  WriteData8(0x00);
  WriteData8(0x4E);
  WriteData8(0x00);

  WriteCommand(0x98); // SET_GAMMA2
  WriteData8(0x3e);
  WriteData8(0x07);

  WriteCommand(0x35); // Tearing Effect Line ON
  WriteCommand(0x21); // Display Inversion ON

  WriteCommand(0x36);   // Memory Access Control
  WriteData8((0 << 7) | // Row Address Order
             (0 << 6) | // Column Address Order
             (0 << 5) | // Row/Column Exchange
             (1 << 4) | // Vertical Refresh Order
             (1 << 3) | // RGB/BGR Order
             (0 << 2)); // Horizontal Refresh ORDER

  // WriteCommand(0xE9);
  // WriteData8(0x00); // Enable 2-data line mode

  WriteCommand(0x51); // Write Brightness
  WriteData8(0x0F);

  WriteCommand(0x29); // Display ON

  WriteCommand(0x11); // Sleep OUT
  daisy::System::Delay(120);
  WriteCommand(0x29); // Display on
  daisy::System::Delay(20);
}

void GC9A01::DrawRectangle(int x, int y, int w, int h, uint16_t color) {
  WriteCommand(0x2A); // CASET?
  WriteData32C(x, x + w - 1);
  WriteCommand(0x2B); // PASET?
  WriteData32C(y, y + h - 1);
  WriteCommand(0x2C); // RAMWR

  const int chunk_size = 128;
  int num_pixels = w * h;
  uint8_t buf[chunk_size * 2];
  while (num_pixels > 0) {
    int msg_size = std::min(num_pixels, chunk_size);
    for (int i = 0; i < msg_size * 2; i += 2) {
      buf[i] = color >> 8;
      buf[i + 1] = color & 0xFF;
    }
    spi_.BlockingTransmit(buf, msg_size * 2);
    num_pixels -= msg_size;
  }
}

void GC9A01::FillScreen(uint16_t color) {
  DrawRectangle(0, 0, 240, 240, color);
}
