/*
 * ssd_1306.c
 *
 *  Created on: Apr 29, 2023
 *      Author: vl
 */
//#include <math.h>
//#include <stdlib.h>
//#include <string.h>
#include "ssd_1306.h"
// Screen object
static SSD1306_t SSD1306;
// Screenbuffer
static uint8_t SSD1306_Buffer[SSD1306_BUFFER_SIZE];


void ssd1306_Init(void) {
    // Reset OLED
    ssd1306_Reset();
    // Wait for the screen to boot
    Delay_Ms(100);//100ms
    // Init OLED
    ssd1306_SetDisplayOn(0); //display off
    ssd1306_WriteCommand(0x20); //Set Memory Addressing Mode
    ssd1306_WriteCommand(0x00); // 00b,Horizontal Addressing Mode; 01b,Vertical Addressing Mode;
                                // 10b,Page Addressing Mode (RESET); 11b,Invalid
    ssd1306_WriteCommand(0xB0); //Set Page Start Address for Page Addressing Mode,0-7

    //SSD1306_MIRROR_VERT
    //ssd1306_WriteCommand(0xC0); // Mirror vertically

    ssd1306_WriteCommand(0xC8); //Set COM Output Scan Direction


    ssd1306_WriteCommand(0x00); //---set low column address
    ssd1306_WriteCommand(0x10); //---set high column address

    ssd1306_WriteCommand(0x40); //--set start line address - CHECK

    ssd1306_SetContrast(0xFF);

// SSD1306_MIRROR_HORIZ
  //  ssd1306_WriteCommand(0xA0); // Mirror horizontally

    ssd1306_WriteCommand(0xA1); //--set segment re-map 0 to 127 - CHECK


//#ifdef SSD1306_INVERSE_COLOR
  //  ssd1306_WriteCommand(0xA7); //--set inverse color
    ssd1306_WriteCommand(0xA6); //--set normal color
  // Set multiplex ratio.

    ssd1306_WriteCommand(0xA8); //--set multiplex ratio(1 to 64) - CHECK
    ssd1306_WriteCommand(0x3F); //H=64


    ssd1306_WriteCommand(0xA4); //0xa4,Output follows RAM content;0xa5,Output ignores RAM content

    ssd1306_WriteCommand(0xD3); //-set display offset - CHECK
    ssd1306_WriteCommand(0x00); //-not offset

    ssd1306_WriteCommand(0xD5); //--set display clock divide ratio/oscillator frequency
    ssd1306_WriteCommand(0xF0); //--set divide ratio

    ssd1306_WriteCommand(0xD9); //--set pre-charge period
    ssd1306_WriteCommand(0x22); //

    ssd1306_WriteCommand(0xDA); //--set com pins hardware configuration - CHECK
    ssd1306_WriteCommand(0x12);
    ssd1306_WriteCommand(0xDB); //--set vcomh
    ssd1306_WriteCommand(0x20); //0x20,0.77xVcc
    ssd1306_WriteCommand(0x8D); //--set DC-DC enable
    ssd1306_WriteCommand(0x14); //
    ssd1306_SetDisplayOn(1); //--turn on SSD1306 panel

    // Clear screen
    ssd1306_Fill(Black);

    // Flush buffer to screen
    ssd1306_UpdateScreen();

 // Set default values for screen object
    SSD1306.CurrentX = 0;
    SSD1306.CurrentY = 0;

    SSD1306.Initialized = 1;
}

// Fill the whole screen with the given color
void ssd1306_Fill(SSD1306_COLOR color) {
    /* Set memory */
    uint32_t y;
   // ssd1306_SetDisplayOn(0);
    for(y = 0; y < sizeof(SSD1306_Buffer); y++) {
        SSD1306_Buffer[y] = (color == Black) ? 0x00 : 0xff;
    }
}
void ssd1306_DrawPixel(uint8_t x, uint8_t y, SSD1306_COLOR color) {
    if(x >= SSD1306_WIDTH || y >= SSD1306_HEIGHT) {
        // Don't write outside the buffer
        return;
    }
    // Draw in the right color
    if(color == White) {
        SSD1306_Buffer[x + (y / 8) * SSD1306_WIDTH] |= 1 << (y % 8);
    } else {
        SSD1306_Buffer[x + (y / 8) * SSD1306_WIDTH] &= ~(1 << (y % 8));
    }
}


// Draw filled rectangle
void ssd1306_FillRectangle(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2, SSD1306_COLOR color) {
    for (uint8_t y = y2; y >= y1; y--) {
        for (uint8_t x = x2; x >= x1; x--) {
            ssd1306_DrawPixel(x, y, color);
        }
    }
    return;
}
// Draw 1 char to the screen buffer
// ch       => char om weg te schrijven
// Font     => Font waarmee we gaan schrijven
// color    => Black or White
void SSD1306_GotoXY(uint16_t x, uint16_t y) {
	/* Set write pointers */
	SSD1306.CurrentX = x;
	SSD1306.CurrentY = y;
}

char SSD1306_Putc(char ch, FontDef_t* Font, SSD1306_COLOR color) {
	uint32_t i, b, j;

	/* Check available space in LCD */
	if (
		SSD1306_WIDTH <= (SSD1306.CurrentX + Font->FontWidth) ||
		SSD1306_HEIGHT <= (SSD1306.CurrentY + Font->FontHeight)
	) {
		/* Error */
		return 0;
	}

	/* Go through font */
	for (i = 0; i < Font->FontHeight; i++) {
		b = Font->data[(ch - 32) * Font->FontHeight + i];
		for (j = 0; j < Font->FontWidth; j++) {
			if ((b << j) & 0x8000) {
				ssd1306_DrawPixel(SSD1306.CurrentX + j, (SSD1306.CurrentY + i), (SSD1306_COLOR) color);
			} else {
				ssd1306_DrawPixel(SSD1306.CurrentX + j, (SSD1306.CurrentY + i), (SSD1306_COLOR)!color);
			}
		}
	}

	/* Increase pointer */
	SSD1306.CurrentX += Font->FontWidth;

	/* Return character written */
	return ch;
}

char SSD1306_Puts(char* str, FontDef_t* Font, SSD1306_COLOR color) {
	/* Write characters */
	while (*str) {
		/* Write character by character */
		if (SSD1306_Putc(*str, Font, color) != *str) {
			/* Return error */
			return *str;
		}

		/* Increase string pointer */
		str++;
	}

	/* Everything OK, zero should be returned */
	return *str;
}
void SSD1306_DrawBitmap(int16_t x, int16_t y, const unsigned char* bitmap, int16_t w, int16_t h, uint16_t color)
{

    int16_t byteWidth = (w + 7) / 8; // Bitmap scanline pad = whole byte
    uint8_t byte = 0;

    for(int16_t j=0; j<h; j++, y++)
    {
        for(int16_t i=0; i<w; i++)
        {
            if(i & 7)
            {
               byte <<= 1;
            }
            else
            {
               byte = (*(const unsigned char *)(&bitmap[j * byteWidth + i / 8]));
            }
            if(byte & 0x80) ssd1306_DrawPixel(x+i, y, color);
        }
    }
}



// Position the cursor
void ssd1306_SetCursor(uint8_t x, uint8_t y) {
    SSD1306.CurrentX = x;
    SSD1306.CurrentY = y;
}

// Draw line by Bresenhem's algorithm
void ssd1306_Line(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2, SSD1306_COLOR color) {
  int32_t deltaX = abs(x2 - x1);
  int32_t deltaY = abs(y2 - y1);
  int32_t signX = ((x1 < x2) ? 1 : -1);
  int32_t signY = ((y1 < y2) ? 1 : -1);
  int32_t error = deltaX - deltaY;
  int32_t error2;

  ssd1306_DrawPixel(x2, y2, color);
    while((x1 != x2) || (y1 != y2))
    {
    ssd1306_DrawPixel(x1, y1, color);
    error2 = error * 2;
    if(error2 > -deltaY)
    {
      error -= deltaY;
      x1 += signX;
    }
    else
    {
    /*nothing to do*/
    }

    if(error2 < deltaX)
    {
      error += deltaX;
      y1 += signY;
    }
    else
    {
    /*nothing to do*/
    }
  }
  return;
}

void ssd1306_SetContrast(const uint8_t value) {
    const uint8_t kSetContrastControlRegister = 0x81;
    ssd1306_WriteCommand(kSetContrastControlRegister);
    ssd1306_WriteCommand(value);
}

void ssd1306_SetDisplayOn(const uint8_t on) {
    uint8_t value;
    if (on) {
        value = 0xAF;   // Display on
        SSD1306.DisplayOn = 1;
    } else {
        value = 0xAE;   // Display off
        SSD1306.DisplayOn = 0;
    }
    ssd1306_WriteCommand(value);
}

uint8_t ssd1306_GetDisplayOn() {
    return SSD1306.DisplayOn;
}
void ssd1306_UpdateScreen(void) {
    // Write data to each page of RAM. Number of pages
    // depends on the screen height:
    //
    //  * 32px   ==  4 pages
    //  * 64px   ==  8 pages
    //  * 128px  ==  16 pages
    for(uint8_t i = 0; i < 8; i++) {
       ssd1306_WriteCommand(0xB0 + i); // Set the current RAM page address.
       ssd1306_WriteCommand(0x00 );
       ssd1306_WriteCommand(0x10 );
       ssd1306_WriteData(&SSD1306_Buffer[SSD1306_WIDTH*i],SSD1306_WIDTH);

    }
}
void ssd1306_Reset(void)
{
    // CS = High (not selected)
	GPIO_WriteBit(GPIOA, GPIO_Pin_1, Bit_SET);
    // Reset the OLED
    GPIO_WriteBit(GPIOA, GPIO_Pin_3, Bit_RESET);
    Delay_Ms(10);
    GPIO_WriteBit(GPIOA, GPIO_Pin_3, Bit_SET);
    Delay_Ms(10);

}
// Send a byte to the command register
void ssd1306_WriteCommand(u8 byte)
{
	GPIO_WriteBit(GPIOA, GPIO_Pin_1, Bit_RESET); // CS select OLED
	GPIO_WriteBit(GPIOA, GPIO_Pin_2, Bit_RESET); //CD command
    while(SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_BSY)){}
	SPI_I2S_SendData(SPI1, byte);
	GPIO_WriteBit(GPIOA, GPIO_Pin_1, Bit_SET); // un-select OLED
}
void ssd1306_WriteData(uint8_t buffer[], size_t buff_size)
{
	u16 k=0;
	GPIO_WriteBit(GPIOA, GPIO_Pin_1, Bit_RESET); // select OLED
	GPIO_WriteBit(GPIOA, GPIO_Pin_2, Bit_SET); //CD data
	for(k=0; k <= buff_size; k++)
	{
    while(SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_BSY)){}
	SPI_I2S_SendData(SPI1, buffer[k]);
	}
    GPIO_WriteBit(GPIOA, GPIO_Pin_1, Bit_SET);  // un-select OLED
}

