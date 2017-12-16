/*
See rights and use declaration in License.h
This library has been modified for the Maple Mini.
Includes DMA transfers on DMA1 CH2 and CH3.

-----------
modified by IOXhop (www.ioxhop.com)
 - Edit SPI to standard SPI library.
-----------
*/
#include <Adafruit_ILI9481_ESP32_SPI.h>

/*****************************************************************************/
Adafruit_ILI9481_ESP32_SPI::Adafruit_ILI9481_ESP32_SPI(void) : Adafruit_GFX(TFTWIDTH, TFTHEIGHT){}
/*****************************************************************************/
void writebus(uint8_t d) {
	GPIO.out_w1tc = ((uint32_t)1 << TFT_SPI_CS); // Cls
	SPI.transfer(d);
	GPIO.out_w1ts = ((uint32_t)1 << TFT_SPI_CS); // Set
	
	CS_ACTIVE;
	CS_IDLE;
}
/*****************************************************************************/
void writedata16(uint16_t c)
{
	CD_DATA;
	// CS_ACTIVE;
	/*uint16_t tmpR = (c&0xF80>>7)&0x1F;
	uint16_t tmpG = c&0x7E0;
	uint16_t tmpB = (c&0x1F<<11)&0xF8;
	c = tmpR|tmpG|tmpB;*/
	writebus(c>>8);
	writebus(c&0xFF);
	// CS_IDLE;
}
/*****************************************************************************/
void writedata16(uint16_t color, uint32_t num)
{
	CD_DATA;
	// CS_ACTIVE;
	for (int n=0;n<num;n++) {
		writebus(color>>8);
		writebus(color&0xFF);
	}
	// CS_IDLE;
}
/*****************************************************************************/
void writecommand(uint8_t c)
{
	CD_COMMAND;
	// CS_ACTIVE;
    writebus(c);
	// CS_IDLE;
}
/*****************************************************************************/
void writedata(uint8_t c)
{
	CD_DATA;
	// CS_ACTIVE;
    writebus(c);
	// CS_IDLE;
}
/*****************************************************************************/
// https://github.com/notro/fbtft/blob/master/fb_ili9481.c
#define DELAY 0x80
/*****************************************************************************/
const uint8_t default_init_sequence[] =
{
	/* SLP_OUT - Sleep out */
	1, 0x11,
	DELAY, 50,
	/* Power setting */
	4, 0xD0, 0x07, 0x42, 0x18,
	/* VCOM */
	4, 0xD1, 0x00, 0x07, 0x10,
	/* Power setting for norm. mode */
	3, 0xD2, 0x01, 0x02,
	/* Panel driving setting */
	6, 0xC0, 0x10, 0x3B, 0x00, 0x02, 0x11,
	/* Frame rate & inv. */
	2, 0xC5, 0x03,
	
	2, 0x36, 0x0A,
	/* Pixel format */
	2, 0x3A, 0x55,
	
	5, 0x2A, 0x00, 0x00, 0x01, 0x3F,
	5, 0x2B, 0x00, 0x00, 0x01, 0xE0,
	/* Gamma */
	13, 0xC8, 0x00, 0x32, 0x36, 0x45, 0x06, 0x16,
		  0x37, 0x75, 0x77, 0x54, 0x0C, 0x00,
	/* DISP_ON */
	1, 0x29,
	// 1, 0x2C,
	0
};
/*****************************************************************************/
// Companion code to the above tables.  Reads and issues
// a series of LCD commands stored in PROGMEM byte array.
/*****************************************************************************/
void commandList(const uint8_t *addr)
{
	uint8_t  numBytes, tmp;

	while ( (numBytes=(*addr++))>0 ) { // end marker == 0
		if ( numBytes&DELAY ) {
			//Serial.print("delay ");
			tmp = *addr++;
			//Serial.println(tmp);
			delay(tmp); // up to 255 millis
		} else {
			//Serial.print(numBytes); Serial.print("byte(s): ");
			tmp = *addr++;
			//Serial.write('<'); Serial.print(tmp, HEX); Serial.write('>');
			writecommand(tmp); // first byte is command
			while (--numBytes) { //   For each argument...
				tmp = *addr++;
				//Serial.print(tmp, HEX); Serial.write('.');
				writedata(tmp); // all consecutive bytes are data
			}
			//Serial.write('\n');
		}
	}
}
/*****************************************************************************/
void Adafruit_ILI9481_ESP32_SPI::begin(void)
{
	pinMode(TFT_RST, OUTPUT);
	pinMode(TFT_RS, OUTPUT);
	pinMode(TFT_CS, OUTPUT);
	pinMode(TFT_SPI_CS, OUTPUT);
  
	digitalWrite(TFT_RST, HIGH);
	digitalWrite(TFT_CS, HIGH);
	digitalWrite(TFT_SPI_CS, HIGH);

	// toggle RST low to reset
	if (TFT_RST > 0) {
		//Serial.println("resetting display...");
		digitalWrite(TFT_RST, HIGH);
		delay(5);
		digitalWrite(TFT_RST, LOW);
		delay(15);
		digitalWrite(TFT_RST, HIGH);
		delay(15);
	}
	
	SPI.begin(TFT_SPI_SCK, 12, TFT_SPI_MOSI, TFT_SPI_CS);
	//SPI.begin();
	SPI.beginTransaction(SPISettings(240000000, MSBFIRST, SPI_MODE0));
	
	commandList(default_init_sequence);
	invertDisplay(true);
}
/*****************************************************************************/
void Adafruit_ILI9481_ESP32_SPI::setAddrWindow(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1)
{
	writecommand(ILI9481_CASET); // Column addr set
	writedata(x0 >> 8);
	writedata(x0 & 0xFF);     // XSTART
	writedata(x1 >> 8);
	writedata(x1 & 0xFF);     // XEND

	writecommand(ILI9481_PASET); // Row addr set
	writedata(y0 >> 8);
	writedata(y0);     // YSTART
	writedata(y1 >> 8);
	writedata(y1);     // YEND

	writecommand(ILI9481_RAMWR); // write to RAM
}
/*****************************************************************************/
void Adafruit_ILI9481_ESP32_SPI::pushColor(uint16_t color)
{
	writedata16(color);
}
/*****************************************************************************/
void Adafruit_ILI9481_ESP32_SPI::drawPixel(int16_t x, int16_t y, uint16_t color)
{
	if ((x < 0) || (x >= _width) || (y < 0) || (y >= _height)) return;

	setAddrWindow(x, y, x + 1, y + 1);
	pushColor(color);
}
/*****************************************************************************/
void Adafruit_ILI9481_ESP32_SPI::drawFastVLine(int16_t x, int16_t y, int16_t h, uint16_t color)
{
	// Rudimentary clipping
	if ((x >= _width) || (y >= _height || h < 1)) return;
	if ((y + h - 1) >= _height)	{ h = _height - y; }
	if (h < 2 ) { drawPixel(x, y, color); return; }

	setAddrWindow(x, y, x, y + h - 1);
	writedata16(color, h);
}
/*****************************************************************************/
void Adafruit_ILI9481_ESP32_SPI::drawFastHLine(int16_t x, int16_t y, int16_t w, uint16_t color)
{
	// Rudimentary clipping
	if ((x >= _width) || (y >= _height || w < 1)) return;
	if ((x + w - 1) >= _width) { w = _width - x; }
	if (w < 2 ) { drawPixel(x, y, color); return; }

	setAddrWindow(x, y, x + w - 1, y);
	writedata16(color, w);
}

/*****************************************************************************/
void Adafruit_ILI9481_ESP32_SPI::fillScreen(uint16_t color) {
	setAddrWindow(0, 0,  _width - 1, _height - 1);
	writedata16(color, (_width * _height));
}

/*****************************************************************************/
void Adafruit_ILI9481_ESP32_SPI::fillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color)
{
	// rudimentary clipping (drawChar w/big text requires this)
	if ((x >= _width) || (y >= _height || h < 1 || w < 1)) return;
	if ((x + w - 1) >= _width) { w = _width  - x; }
	if ((y + h - 1) >= _height) { h = _height - y; }
	if (w == 1 && h == 1) {
		drawPixel(x, y, color);
		return;
	}

	setAddrWindow(x, y, x + w - 1, y + h - 1);
	writedata16(color, (w*h));
}

/*
* Draw lines faster by calculating straight sections and drawing them with fastVline and fastHline.
*/
/*****************************************************************************/
void Adafruit_ILI9481_ESP32_SPI::drawLine(int16_t x0, int16_t y0,int16_t x1, int16_t y1, uint16_t color)
{
	if ((y0 < 0 && y1 <0) || (y0 > _height && y1 > _height)) return;
	if ((x0 < 0 && x1 <0) || (x0 > _width && x1 > _width)) return;
	if (x0 < 0) x0 = 0;
	if (x1 < 0) x1 = 0;
	if (y0 < 0) y0 = 0;
	if (y1 < 0) y1 = 0;

	if (y0 == y1) {
		if (x1 > x0) {
			drawFastHLine(x0, y0, x1 - x0 + 1, color);
		}
		else if (x1 < x0) {
			drawFastHLine(x1, y0, x0 - x1 + 1, color);
		}
		else {
			drawPixel(x0, y0, color);
		}
		return;
	}
	else if (x0 == x1) {
		if (y1 > y0) {
			drawFastVLine(x0, y0, y1 - y0 + 1, color);
		}
		else {
			drawFastVLine(x0, y1, y0 - y1 + 1, color);
		}
		return;
	}

	bool steep = abs(y1 - y0) > abs(x1 - x0);
	if (steep) {
		swap(x0, y0);
		swap(x1, y1);
	}
	if (x0 > x1) {
		swap(x0, x1);
		swap(y0, y1);
	}

	int16_t dx, dy;
	dx = x1 - x0;
	dy = abs(y1 - y0);

	int16_t err = dx / 2;
	int16_t ystep;

	if (y0 < y1) {
		ystep = 1;
	}
	else {
		ystep = -1;
	}

	int16_t xbegin = x0;
	if (steep) {
		for (; x0 <= x1; x0++) {
			err -= dy;
			if (err < 0) {
				int16_t len = x0 - xbegin;
				if (len) {
					drawFastVLine (y0, xbegin, len + 1, color);
					//writeVLine_cont_noCS_noFill(y0, xbegin, len + 1);
				}
				else {
					drawPixel(y0, x0, color);
					//writePixel_cont_noCS(y0, x0, color);
				}
				xbegin = x0 + 1;
				y0 += ystep;
				err += dx;
			}
		}
		if (x0 > xbegin + 1) {
			//writeVLine_cont_noCS_noFill(y0, xbegin, x0 - xbegin);
			drawFastVLine(y0, xbegin, x0 - xbegin, color);
		}

	}
	else {
		for (; x0 <= x1; x0++) {
			err -= dy;
			if (err < 0) {
				int16_t len = x0 - xbegin;
				if (len) {
					drawFastHLine(xbegin, y0, len + 1, color);
					//writeHLine_cont_noCS_noFill(xbegin, y0, len + 1);
				}
				else {
					drawPixel(x0, y0, color);
					//writePixel_cont_noCS(x0, y0, color);
				}
				xbegin = x0 + 1;
				y0 += ystep;
				err += dx;
			}
		}
		if (x0 > xbegin + 1) {
			//writeHLine_cont_noCS_noFill(xbegin, y0, x0 - xbegin);
			drawFastHLine(xbegin, y0, x0 - xbegin, color);
		}
	}
}
/*****************************************************************************/
// Pass 8-bit (each) R,G,B, get back 16-bit packed color
/*****************************************************************************/
uint16_t Adafruit_ILI9481_ESP32_SPI::color565(uint8_t r, uint8_t g, uint8_t b)
{
  return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
}
/*****************************************************************************/
void Adafruit_ILI9481_ESP32_SPI::setRotation(uint8_t m)
{
	writecommand(ILI9481_MADCTL);
	rotation = m & 3; // can't be higher than 3
	switch (rotation) {
		case 0:
			//writedata(MADCTL_MX |MADCTL_BGR);
			writedata(0x0A);
			_width  = TFTWIDTH;
			_height = TFTHEIGHT;
			break;
		case 1:
			//writedata(MADCTL_MV | MADCTL_BGR);
			writedata(0x28);
			_width  = TFTHEIGHT;
			_height = TFTWIDTH;
			break;
		case 2:
			//writedata(MADCTL_MY | MADCTL_BGR);
			writedata(0x09);
			_width  = TFTWIDTH;
			_height = TFTHEIGHT;
			break;
		case 3:
			//writedata(MADCTL_MX | MADCTL_MY | MADCTL_MV | MADCTL_BGR);
			writedata(0x2B);
			_width  = TFTHEIGHT;
			_height = TFTWIDTH;
			break;
	}
}
/*****************************************************************************/
void Adafruit_ILI9481_ESP32_SPI::invertDisplay(boolean i)
{
	writecommand(i ? ILI9481_INVON : ILI9481_INVOFF);
}

void Adafruit_ILI9481_ESP32_SPI::enable(bool enable) {
	writecommand(enable ? 0x29 : 0x28);
}