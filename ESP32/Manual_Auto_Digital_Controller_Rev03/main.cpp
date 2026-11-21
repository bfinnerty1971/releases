#include <arduino.h>
#include <SPI.h>
#include <TFT_eSPI.h>
#include <Wire.h>

#define SDA_PIN 4
#define SCL_PIN 5
#define RXD2 18
#define TXD2 17
#define SERIAL_ON 16

// Rev 3.0 Manual and Computer Interface for the Digital Controller
// Last update: November 20, 2026 @ 7:39am
// Last update to Microcontroller under test: November 20, 2026 @ 7:39am

// MCP23017 Register Addresses
const int mcpAddress = 0x20; // I2C address for MCP23017 (A0, A1, A2 tied to GND)
const int IODIRA = 0x00; // I/O Direction Register A
const int GPIOA = 0x12;  // Port A GPIO Data Register
const int IODIRB = 0x01; // I/O Direction Register B
const int GPIOB = 0x13;  // Port B GPIO Data Register

/*
* ******************************** OUTPUTS *********************************
* Relays (common Negative)        Relays (floating Negative lead)
* 0 and 0 = 0x00 --> 0000 0000    0 and 0 = 0x00 --> 0000 0000
* 1 and 8 = 0x81 --> 1000 0001    1 and 1 = 0x11 --> 0001 0001
* 2 and 8 = 0x82 --> 1000 0010    2 and 2 = 0x22 --> 0010 0010
* 3 and 8 = 0x84 --> 1000 0100    3 and 3 = 0x44 --> 0100 0100
* 4 and 8 = 0x88 --> 1000 1000    4 and 4 = 0x88 --> 1000 1000
* ************************** Interaction Commands **************************
* TP Off, TP1A, TP2A, TP3A, TP4A, TP1B, TP2B, TP3B, TP4B, Exit, MainMenu
* **************************************************************************
*/

void drawButton(String name, int32_t BTN_X, int32_t BTN_Y, int32_t BTN_W, int32_t BTN_H, int32_t radius, bool ON_OFF);
void buttonmenuA(int button);
void buttonmenu1(int button);
void stillpressed(int num_x1, int num_x2, int num_y1, int num_y2);
void SetRelay(byte data);
void SetBacklight(bool on_off);
bool checkSerial();
void serialmenu1(int button);
void removeButton(String name, int32_t BTN_X, int32_t BTN_Y, int32_t BTN_W, int32_t BTN_H, int32_t radius);

TFT_eSPI tft = TFT_eSPI();

bool ModeA = false;
bool ModeB = false;
bool ModeSerial = false;
bool SerialON = false;
int num_x1 = 0;
int num_x2 = 0;
int num_y1 = 0;
int num_y2 = 0;
uint16_t x = 0, y = 0; // Variables to store pixel coordinates

void setup(void) {
  Serial.begin(115200);
  // Initialize the TFT display
  tft.init();
  tft.setRotation(1);
  tft.fillScreen(TFT_BLACK);
  tft.setTextDatum(MC_DATUM); // Middle center text alignment
  // Initialize the I2C bus and configure the MCP23017 GPIO expander
  Wire.begin(SDA_PIN, SCL_PIN); // Join I2C bus as master
  // 1. Set Port A as outputs (0 = Output, 1 = Input)
  Wire.beginTransmission(mcpAddress);
  Wire.write(IODIRA); // IODIRA register
  Wire.write(0x00);  // 0x00 sets all 8 pins of Port B to OUTPUT (0 = Output, 1 = Input)
  Wire.endTransmission();
  // 2. Set Port B (IODIRB) as outputs (0 = Output, 1 = Input)
  Wire.beginTransmission(mcpAddress);
  Wire.write(IODIRB);
  Wire.write(0x00); // 0x00 sets all 8 pins of Port B to OUTPUT (0 = Output, 1 = Input)
  Wire.endTransmission();
  // Set the SERIAL_ON device pin as an input to detect if the computer is connected to the microcontroller
  pinMode(SERIAL_ON, INPUT);
  // Initialize Serial2 for UART communication with the computer
  Serial2.begin(9600, SERIAL_8N1, RXD2, TXD2);
  while(!Serial2) { delay(10); }
  Serial2.flush();
  Serial2.end();
  // Turn Back Light ON
  SetBacklight(true);
  SerialON = checkSerial();
  // Load Main Menu
  buttonmenu1(0);
}

String commands[] = { "TP Off", "TP1A", "TP2A", "TP3A", "TP4A", "TP1B", "TP2B", "TP3B", "TP4B", "Exit", "MainMenu" };
bool flagSerial = false;
String DisplayMessage[] = { "               All Inputs OFF               ",
                                "  Mode A: TP1 (+) > TP1 (-)  ",
                                "  Mode A: TP2 (+) > TP2 (-)  ",
                                "  Mode A: TP3 (+) > TP3 (-)  ",
                                "  Mode A: TP4 (+) > TP4 (-)  ",
                                "  Mode B: TP1 (+) > TP4 (-)  ",
                                "  Mode B: TP2 (+) > TP4 (-)  ",
                                "  Mode B: TP3 (+) > TP4 (-)  ",
                                "  Mode B: TP4 (+) > TP4 (-)  " };

void loop() {
  SerialON = checkSerial();
  if (flagSerial && !SerialON && ModeSerial && !ModeA && !ModeB)
  {
    flagSerial = false;
    ModeSerial = false;
    tft.fillScreen(TFT_BLACK);
    SetRelay(0x00);
    Serial2.end();
    buttonmenu1(0);
  }
  else if (SerialON && !flagSerial && !ModeA && !ModeB && !ModeSerial)
  { 
    flagSerial = true;
    Serial2.begin(9600, SERIAL_8N1, RXD2, TXD2);
    buttonmenu1(0);
  }
  else if (!SerialON && flagSerial && !ModeA && !ModeB && !ModeSerial)
  { 
    flagSerial = false;
    Serial2.end();
    buttonmenu1(4);
  }
  // Check if the screen is being touched
  String cmd = "";
  if ( Serial2.available() && SerialON && !ModeA && !ModeB && ModeSerial) {
	  cmd = Serial2.readStringUntil('\n');
    cmd.trim();
    String STRstart = cmd.c_str();
    Serial2.println(cmd);
    Serial2.println("hello, the command sent is: " + cmd);
    
    if(cmd == "TP Off") { SetRelay(0x00),  tft.drawString(DisplayMessage[0], 240, 80, 4); }
    else if(cmd == "TP1A") { SetRelay(0x11), tft.drawString(DisplayMessage[1], 240, 80, 4); }
    else if(cmd == "TP2A") { SetRelay(0x22), tft.drawString(DisplayMessage[2], 240, 80, 4); }
    else if(cmd == "TP3A") { SetRelay(0x44), tft.drawString(DisplayMessage[3], 240, 80, 4); }
    else if(cmd == "TP4A") { SetRelay(0x88), tft.drawString(DisplayMessage[4], 240, 80, 4); }
    else if(cmd == "TP1B") { SetRelay(0x81), tft.drawString(DisplayMessage[5], 240, 80, 4); }
    else if(cmd == "TP2B") { SetRelay(0x82), tft.drawString(DisplayMessage[6], 240, 80, 4); }
    else if(cmd == "TP3B") { SetRelay(0x84), tft.drawString(DisplayMessage[7], 240, 80, 4); }
    else if(cmd == "TP4B") { SetRelay(0x88), tft.drawString(DisplayMessage[8], 240, 80, 4); }
    //else if(cmd == "MainMenu") { SetRelay(0x00), tft.fillScreen(TFT_BLACK), buttonmenu1(0); }
    else if(cmd == "Exit") 
    { 
      tft.fillScreen(TFT_BLACK);
      SetRelay(0x00), buttonmenu1(0);
      ModeA = false, ModeB = false, ModeSerial = false;
      Serial2.end(); }
  }
  else if (tft.getTouch(&x, &y)) {
    
    int numArray[][4] = {{ 45, 170, 180, 235 },   // input 1 button parameters
                         { 45, 170, 90, 150 },    // input 2 button parameters
                         { 200, 325, 180, 235 },  // input 3 button parameters
                         { 200, 325, 90, 150 },   // input 4 button parameters
                         { 100, 255, 20, 70 },    // inputs OFF button parameters
                         { 330, 475, 20, 70 },    // Main Menu button parameters
                         { 45, 270, 180, 230 },   // MODE A button parameters
                         { 45, 270, 80, 130 },    // MODE B button parameters
                         { 300, 470, 10, 60 },    // Connect to Serial Mode button parameters
                         { 90, 390, 115, 170 }};  // Serial Mode Exit button parameters
    // Touch Screen Check for Mode A
    if (x >= numArray[6][0] && x <= numArray[6][1] && y >= numArray[6][2] && y <= numArray[6][3] && !ModeA && !ModeB && !ModeSerial)
    {
      buttonmenu1(1);
      stillpressed(numArray[6][0], numArray[6][1],  numArray[6][2], numArray[6][3]);
      tft.fillScreen(TFT_BLACK);
      delay(100);
      ModeA = true, ModeB = false;
      tft.fillScreen(TFT_BLACK);
      buttonmenuA(0);
      x = 0;
      y = 0;
    }
    // Touch Screen Check for Mode B
    if (x >= numArray[7][0] && x <= numArray[7][1] && y >= numArray[7][2] && y <= numArray[7][3] && !ModeA && !ModeB && !ModeSerial)
    {
      buttonmenu1(2);
      stillpressed(numArray[7][0], numArray[7][1],  numArray[7][2], numArray[7][3]);
      tft.fillScreen(TFT_BLACK);
      delay(100);
      ModeA = false, ModeB = true;
      tft.fillScreen(TFT_BLACK);
      buttonmenuA(0);
      x = 0;
      y = 0;
    }
    // Touch Screen Check for Connect to Serial Mode
    if (x >= numArray[8][0] && x <= numArray[8][1] && y >= numArray[8][2] && y <= numArray[8][3] && !ModeA && !ModeB && SerialON && !ModeSerial)
    {
      buttonmenu1(3);
      stillpressed(numArray[8][0], numArray[8][1],  numArray[8][2], numArray[8][3]);
      tft.fillScreen(TFT_BLACK);
      delay(100);
      ModeA = false, ModeB = false, ModeSerial = true, SerialON = true;
      Serial2.begin(9600, SERIAL_8N1, RXD2, TXD2);
      tft.fillScreen(TFT_BLACK);
      serialmenu1(0);
      x = 0;
      y = 0;
    }
    // Touch Screen Check for Serial Mode Exit
    if (x >= numArray[9][0] && x <= numArray[9][1] && y >= numArray[9][2] && y <= numArray[9][3] && !ModeA && !ModeB && SerialON && ModeSerial)
    {
      serialmenu1(1);
      stillpressed(numArray[9][0], numArray[9][1],  numArray[9][2], numArray[9][3]);
      tft.fillScreen(TFT_BLACK);
      delay(100);
      ModeA = false, ModeB = false, ModeSerial = false, SerialON = true;
      SetRelay(0x00);
      Serial2.end();
      tft.fillScreen(TFT_BLACK);
      buttonmenu1(0);
      x = 0;
      y = 0;
    }
    if ((ModeA || ModeB) && !ModeSerial) // ********************* SCREEN A - Menu *********************
    {
      // Touch Screen Check for Mode A and Mode B - Input 1
      if (x >= numArray[0][0] && x <= numArray[0][1] && y >= numArray[0][2] && y <= numArray[0][3])
      {
        buttonmenuA(1);
        stillpressed(numArray[0][0], numArray[0][1],  numArray[0][2], numArray[0][3]);
        if (ModeA) { SetRelay(0x11), tft.drawString(DisplayMessage[1], 240, 50, 4); }       
        else { SetRelay(0x81), tft.drawString(DisplayMessage[5], 240, 50, 4); }
      }
      // Touch Screen Check for Mode A and Mode B - Input 2
      if (x >= numArray[1][0] && x <= numArray[1][1] && y >= numArray[1][2] && y <= numArray[1][3]) 
      {
        buttonmenuA(2);
        stillpressed(numArray[1][0], numArray[1][1],  numArray[1][2], numArray[1][3]);
        if (ModeA) { SetRelay(0x22), tft.drawString(DisplayMessage[2], 240, 50, 4); }
        else { SetRelay(0x82), tft.drawString(DisplayMessage[6], 240, 50, 4); }
      }
      // Touch Screen Check for Mode A and Mode B - Input 3
      if (x >= numArray[2][0] && x <= numArray[2][1] && y >= numArray[2][2] && y <= numArray[2][3])
      {
        buttonmenuA(3);
        stillpressed(numArray[2][0], numArray[2][1],  numArray[2][2], numArray[2][3]);
        if (ModeA) { SetRelay(0x44), tft.drawString(DisplayMessage[3], 240, 50, 4); }
        else { SetRelay(0x84), tft.drawString(DisplayMessage[7], 240, 50, 4); }
      }
      // Touch Screen Check for Mode A and Mode B - Input 4
      if (x >= numArray[3][0] && x <= numArray[3][1] && y >= numArray[3][2] && y <= numArray[3][3])
      {
        buttonmenuA(4);
        stillpressed(numArray[3][0], numArray[3][1],  numArray[3][2], numArray[3][3]);
        if (ModeA) { SetRelay(0x88), tft.drawString(DisplayMessage[4], 240, 50, 4); }
        else { SetRelay(0x88), tft.drawString(DisplayMessage[8], 240, 50, 4);}
      }
      // Touch Screen Check for Mode A and Mode B - Inputs OFF
      if (x >= numArray[4][0] && x <= numArray[4][1] && y >= numArray[4][2] && y <= numArray[4][3])
      {
        buttonmenuA(5);
        stillpressed(numArray[4][0], numArray[4][1],  numArray[4][2], numArray[4][3]);
        SetRelay(0x0), tft.drawString(DisplayMessage[0], 240, 50, 4);
      }
      // Touch Screen Check for Mode A and Mode B - Main Menu
      if (x >= numArray[5][0] && x <= numArray[5][1] && y >= numArray[5][2] && y <= numArray[5][3])
      {
        buttonmenuA(6);
        stillpressed(numArray[5][0], numArray[5][1],  numArray[5][2], numArray[5][3]);
        SetRelay(0x0);
        ModeA = false;
        ModeB = false;
        tft.fillScreen(TFT_BLACK);
        buttonmenu1(0);
      }
    } // ***************** END SCREEN A - Main Menu *********************
    delay(30);
  }
}

//=============================================== Supporting Subroutines ===============================================

void stillpressed(int num_x1, int num_x2, int num_y1, int num_y2) { // waits until the button parameters are released
  do{
    x = 0;
    y = 0;
    tft.getTouch(&x, &y);
    delay(50);
  }while(x >= num_x1 && x <= num_x2 && y >= num_y1 && y <= num_y2);
}

bool checkSerial() { // checks if the serial connection is available
  int state = 0;
  state = digitalRead(SERIAL_ON);
  if(state == 1) { return true; }
  else { return false; }
}

void serialmenu1(int button) { // Serial Mode Menu
  tft.setTextColor(TFT_WHITE);
  tft.drawString("Serial Mode Selected", 240, 20, 4);
  drawButton("Press to Exit Serial Mode", 95, 145, 290, 50, 8, false);
  switch(button) {
    case 0:
      break;
    case 1:
      drawButton("Press to Exit Serial Mode", 95, 145, 290, 50, 8, true);
      break;
    default:
      break;
  }
  delay(20);
}

void buttonmenu1(int button) { // Main Menu Options
  tft.setTextColor(TFT_WHITE);
  tft.drawString("Select Mode", 240, 20, 4);
  drawButton("Mode A", 50, 80, 220, 50, 8, false);
  tft.drawString("Floating (-)", 380, 107, 4);
  drawButton("Mode B", 50, 180, 220, 50, 8, false);
  tft.drawString("Common (-)", 380, 207, 4);
  if(SerialON) { drawButton("Serial Mode", 305, 250, 150, 50, 8, false); }
  //else { drawButton("Serial Mode", 305, 250, 150, 50, 8, true); }
  switch(button) {
    case 0:
      break;
    case 1:
      drawButton("Mode A", 50, 80, 220, 50, 8, true);
      break;
    case 2:
      drawButton("Mode B", 50, 180, 220, 50, 8, true);
      break;
    case 3:
      drawButton("Serial Mode", 305, 250, 150, 50, 8, true);
      break;
    case 4:
      removeButton("Serial Mode", 305, 250, 150, 50, 8);
      break;
    default:
      break;
  }
  delay(20);
}

void buttonmenuA(int button) { // Mode A and Mode B Input Selection Menu Options
  tft.setTextColor(TFT_WHITE);
  String Modeselect = "";
  if (ModeA) { Modeselect = "Mode A - Floating (-)"; }
  else { Modeselect = "Mode B - Common (-)"; }
  tft.drawString(Modeselect, 240, 20, 4);
  drawButton("Input 1", 50, 80, 120, 50, 8, false);
  drawButton("Input 2", 50, 165, 120, 50, 8, false);
  drawButton("Input 3", 205, 80, 120, 50, 8, false);
  drawButton("Input 4", 205, 165, 120, 50, 8, false);
  drawButton("Inputs OFF", 110, 245, 150, 50, 8, false);
  drawButton("Main Menu", 335, 245, 140, 50, 8, false);
  switch(button) {
    case 0:
      break;
    case 1:
      drawButton("Input 1", 50, 80, 120, 50, 8, true);
      break;
    case 2:
      drawButton("Input 2", 50, 165, 120, 50, 8, true);
      break;
    case 3:
      drawButton("Input 3", 205, 80, 120, 50, 8, true);
      break;
    case 4:
      drawButton("Input 4", 205, 165, 120, 50, 8, true);
      break;
    case 5:
      drawButton("Inputs OFF", 110, 245, 150, 50, 8, true);
      break;
    case 6:
      drawButton("Main Menu", 335, 245, 140, 50, 8, true);
      break;
    default:
      break;
  }
  delay(20);
}

// Draws buttons on the TFT display with specified parameters
void drawButton(String name, int32_t BTN_X, int32_t BTN_Y, int32_t BTN_W, int32_t BTN_H, int32_t radius, bool ON_OFF) {
  if (!ON_OFF) { tft.fillRoundRect(BTN_X, BTN_Y, BTN_W, BTN_H, 8, TFT_GREEN), tft.setTextColor(TFT_BLACK, TFT_GREEN); }
  else { tft.fillRoundRect(BTN_X, BTN_Y, BTN_W, BTN_H, 8, TFT_LIGHTGREY), tft.setTextColor(TFT_BLACK, TFT_LIGHTGREY); }
  tft.drawString(name, BTN_X + (BTN_W / 2), BTN_Y + (BTN_H / 2), 4);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
}

// Removes a specific button from the TFT display by filling its area with black and setting the text color to black
void removeButton(String name, int32_t BTN_X, int32_t BTN_Y, int32_t BTN_W, int32_t BTN_H, int32_t radius) {
  tft.fillRoundRect(BTN_X, BTN_Y, BTN_W, BTN_H, 8, TFT_BLACK), tft.setTextColor(TFT_BLACK, TFT_BLACK);
  tft.drawString(name, BTN_X + (BTN_W / 2), BTN_Y + (BTN_H / 2), 4);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
}

void SetRelay(byte data) // Sets the desired relay output to ON or OFF
{
    Wire.beginTransmission(mcpAddress); // expansion board address
    Wire.write(GPIOB); // GPIOB is for the relay outputs of the expansion board
    Wire.write(0x0);
    Wire.endTransmission();
    delay(50);
    Wire.beginTransmission(mcpAddress); // expansion board address
    Wire.write(GPIOB); // GPIOB is for the relay outputs of the expansion board
    Wire.write(data);
    Wire.endTransmission();
}

void SetBacklight(bool on_off)// Turns the Backlight of the Display On or Off
{
    Wire.beginTransmission(mcpAddress);
    Wire.write(GPIOA); // GPIOA is for the backlight outputs of the expansion board
    Wire.write(on_off ? 0x02 : 0x00);
    Wire.endTransmission();
}
