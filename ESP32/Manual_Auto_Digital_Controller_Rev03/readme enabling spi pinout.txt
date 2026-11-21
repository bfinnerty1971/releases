------------------------------------------------------------------------------------

User_Setup.h
Step 1. Enable HSPI_PORT
	Enable: Add #define USE_HSPI_PORT

Step 2. Enable ILI9488_DRIVER
	Disable: //#define ILI9341_DRIVER
	Enable:  #define ILI9488_DRIVER

Step 3. Disable pinouts
	Disable: Add // in front of the pin descriptions in which you need to disable

Step 4. Enable Touch Chip Select
	#define TOUCH_CS 14     // Chip select pin (T_CS) of touch screen

------------------------------------------------------------------------------------

User_Setup_Select.h
Step 1. Include the ILI9488_DRIVER
	#include <User_Setups/Setup21_ILI9488.h>           // Setup file for ESP32 and ILI9488 SPI bus TFT

------------------------------------------------------------------------------------

User_Setups\Setup21_ILI9488.h
Step 1. Add the correct pinout for SPI bus interface
	#define TFT_MISO 13
	#define TFT_MOSI 11
	#define TFT_SCLK 12
	#define TFT_CS   10  // Chip select control pin
	#define TFT_DC    6  // Data Command control pin
	#define TFT_RST   7  // Reset pin (could connect to RST pin)
	#define TOUCH_CS 14     // Chip select pin (T_CS) of touch screen

------------------------------------------------------------------------------------
