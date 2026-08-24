#ifndef __SSD1306_CONF_H__
#define __SSD1306_CONF_H__

/* Choose a microcontroller family */
#define STM32F4

/* Choose fonts */
#define SSD1306_INCLUDE_FONT_6x8
#define SSD1306_INCLUDE_FONT_7x10
#define SSD1306_INCLUDE_FONT_11x18
#define SSD1306_INCLUDE_FONT_16x26

/* Mirror the display horizontally if needed */
#define SSD1306_MIRROR_VERT
#define SSD1306_MIRROR_HORIZ

/* Set inverse color if needed */
// #define SSD1306_INVERSE_COLOR

/* Include only needed I2C */
#define SSD1306_USE_I2C

/* I2C address */
#define SSD1306_I2C_ADDR       (0x3C << 1)

/* SSD1306 width in pixels */
#define SSD1306_WIDTH           128

/* SSD1306 height in pixels */
#define SSD1306_HEIGHT          64

#define SSD1306_I2C_PORT        hi2c1

#define SSD1306_X_OFFSET_LOWER  0
#define SSD1306_X_OFFSET_UPPER  0

#endif /* __SSD1306_CONF_H__ */
