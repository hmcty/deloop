#include "daisy_seed.h"

#define COLOR_BLACK ((uint16_t)0x0000)
#define COLOR_GREEN ((uint16_t)0x07E0)
#define COLOR_RED ((uint16_t)0xF800)
#define COLOR_BLUE ((uint16_t)0x001F)
#define COLOR_WHITE ((uint16_t)0xFFFF)

class GC9A01 {
public:
  GC9A01(daisy::SpiHandle spi, daisy::GPIO dc, daisy::GPIO rst)
      : spi_(spi), dc_(dc), rst_(rst) {}

  void Init(void);
  void DrawRectangle(int x, int y, int w, int h, uint16_t color);
  void FillScreen(uint16_t color);

private:
  void WriteCommand(uint8_t cmd) {
    dc_.Write(false);
    spi_.BlockingTransmit(&cmd, 1);
    dc_.Write(true);
  }

  void WriteData8(uint8_t data) { spi_.BlockingTransmit(&data, 1); }

  void WriteData16(uint16_t data) {
    uint8_t bytes[2] = {static_cast<uint8_t>(data >> 8),
                        static_cast<uint8_t>(data & 0xFF)};
    spi_.BlockingTransmit(bytes, 2);
  }

  void WriteData32C(uint16_t data1, uint16_t data2) {
    uint8_t bytes[4] = {
        static_cast<uint8_t>(data1 >> 8), static_cast<uint8_t>(data1 & 0xFF),
        static_cast<uint8_t>(data2 >> 8), static_cast<uint8_t>(data2 & 0xFF)};

    spi_.BlockingTransmit(bytes, 4);
  }

  daisy::SpiHandle spi_;
  daisy::GPIO dc_, rst_;
};
