#include <Arduino.h>
#include <Wire.h>
#include "DFRobot_RGBLCD1602.h"

#define SDA_PIN 4
#define SCL_PIN 5

// MCP23017 Register Addresses
const int mcpAddress = 0x20; // I2C address for MCP23017 (A0, A1, A2 tied to GND)
const int IODIRA = 0x00; // I/O Direction Register A
const int GPIOA = 0x12;  // Port A GPIO Data Register
const int IODIRB = 0x01; // I/O Direction Register B
const int GPIOB = 0x13;  // Port B GPIO Data Register

DFRobot_RGBLCD1602 lcd(0x6B, 16, 2);  //lcd address, 16 characters, 2 lines

String message = "";
String message2 = "";
bool common_test_point = false;
bool main_menu = true;
bool keypress = false;
bool lcdbacklight = true;

void SetRelay(String status1, String status2, byte data);
void writetodisplay(String input1, String input2);
void invalidSelection();
void selftest();

// ***************************** CHARACTERS ******************************
// 1 = 119, 2 = 123, 3 = 125, 4 = 183, 5 = 187, 6 = 189, 7 = 215, 8 = 219
// 9 = 221, 0 = 235, A = 126, B = 190, C = 222, D = 238, * = 231, # = 237
//
// ******************************* OUTPUTS *******************************
// Relays (floating)               Relays (common)
// 0 and 0 = 0x00 --> 0000 0000    0 and 0 = 0x00 --> 0000 0000
// 1 and 8 = 0x81 --> 1000 0001    1 and 1 = 0x11 --> 0001 0001
// 2 and 8 = 0x82 --> 1000 0010    2 and 2 = 0x22 --> 0010 0010
// 3 and 8 = 0x84 --> 1000 0100    3 and 3 = 0x44 --> 0100 0100
// 4 and 8 = 0x88 --> 1000 1000    4 and 4 = 0x88 --> 1000 1000

void setup() {
   Wire.begin(SDA_PIN, SCL_PIN); // Join I2C bus as master
   //Serial.begin(115200);
   lcd.init();
   lcd.setPWM(255, 100);
   // Print Initialization message
   writetodisplay("Functional Test", "Start up...     ");
   delay(2000);
   // 1. Set A4-A7 as Inputs and A0-A3 as outputs (binary 11110000 = 0xF0)
   Wire.beginTransmission(mcpAddress);
   Wire.write(IODIRA); // IODIRA register
   Wire.write(0xF0); // Pins 4-7 = 1 (Input), Pins 0-3 = 0 (Output)
   Wire.endTransmission();
   // 2. Enable internal pull-ups for pins A4-A7 (binary 11110000 = 0xF0)
   Wire.beginTransmission(mcpAddress);
   Wire.write(0x0C); // GPPUA register
   Wire.write(0xF0); // Pins 4-7 = 1 (Pull-up Enabled)
   Wire.endTransmission();
   // 3. Set Port B (IODIRB) as outputs (0 = Output, 1 = Input)
   Wire.beginTransmission(mcpAddress);
   Wire.write(IODIRB);
   Wire.write(0x00); // 0x00 sets all 8 pins of Port B to OUTPUT (0 = Output, 1 = Input)
   Wire.endTransmission();
   // Print a message to LCD.
   writetodisplay("Functional Test", "Ready ...       ");
   delay(2000);
   // Print a message to the LCD.
   SetRelay("A = Floating TP", "B = Common TP", 0x0);
}

void loop(){
   byte data[] = { 0x07, 0x0B, 0x0D, 0x0E };
   for (int i = 0; i < 4; i++)
   {
      Wire.beginTransmission(mcpAddress);
      Wire.write(GPIOA);   // Select Port A
      Wire.write(data[i]);    // Send byte (Write LOW)
      Wire.endTransmission();

      // Read Port A
      Wire.beginTransmission(mcpAddress);
      Wire.write(GPIOA); // GPIOA register address
      Wire.endTransmission();
      Wire.requestFrom(mcpAddress, 1);
      if (Wire.available())
      {
         byte portAState = Wire.read();
         if (portAState == 126 && main_menu == true) // Menu Selection 'A' = Mode A
         {
            SetRelay("Mode A:", "1, 2, 3, 4 or 0", 0x0);
            common_test_point = false;
            main_menu = false;
         }
         else if (portAState == 190 && main_menu == true) // Menu Selection 'B' = Mode B
         {
            SetRelay("Mode B:", "1, 2, 3, 4 or 0", 0x0);
            common_test_point = true;
            main_menu = false;
         }
         else if (portAState == 238 && main_menu == true) // Menu Selection 'D' = Self test
         {
            selftest();
         }
         else if (portAState == 119 && main_menu == false) // Menu Selection '1' = Output 1 - ON
         {
            if (!common_test_point)
            { SetRelay("output 1, 5 = on", "main menu = *", 0x11); }
            else { SetRelay("output 1, 8 = on", "main menu = *", 0x81); }
         }
         else if (portAState == 123 && main_menu == false) // Menu Selection '2' = Output 2 - ON
         {
            if (!common_test_point) { SetRelay("output 2, 6 = on", "main menu = *", 0x22); }
            else { SetRelay("output 2, 8 = on", "main menu = *", 0x82); }
         }
         else if (portAState == 125 && main_menu == false) // Menu Selection '3' = Output 3 - ON
         {
            if (!common_test_point) { SetRelay("output 3, 7 = on", "main menu = *", 0x44); }
            else { SetRelay("output 3, 8 = on", "main menu = *", 0x84); }
         }
         else if (portAState == 183 && main_menu == false) // Menu Selection '4' = Output 4 - ON
         {
            if (!common_test_point) { SetRelay("output 4, 8 = on", "main menu = *", 0x88); }
            else { SetRelay("output 4, 8 = on", "main menu = *", 0x88); }
         }
         else if (portAState == 235 && main_menu == false) // Menu Selection '0' = All Outputs - OFF
         {
            SetRelay("Outputs = off", "main menu = *", 0x00);
         }
         else if (portAState == 231 && main_menu == false) // Menu Selection '*' = Go to Main Menu
         {
            SetRelay("A = Floating TP", "B = Common TP", 0x00);
            main_menu = true;
            common_test_point = false;
         }
         else if (portAState == 187 || portAState == 189 || portAState == 215 || portAState == 219 || portAState == 221 || portAState == 222 || portAState == 237)
         {
            // 5 = 187, 6 = 189, 7 = 215, 8 = 219, 9 = 221, C = 222
            invalidSelection();
         }
         else if (main_menu == true && portAState == 119 || portAState == 123 || portAState == 125 || portAState == 183 || portAState == 231 || portAState == 235)
         {
            // 1 = 119, 2 = 123, 3 = 125, 4 = 183, 0 = 235
            invalidSelection();
         }
         else if (main_menu == false && portAState == 126 || portAState == 190 || portAState == 238)
         {
            // A = 126, B = 190, D = 238
            invalidSelection();
         }
         delay(70);
      }
   }
}

void SetRelay(String status1, String status2, byte data)
{
   writetodisplay(status1, status2);
   Wire.beginTransmission(mcpAddress); // expansion board address
   Wire.write(GPIOB); // GPIOB is for the relay outputs of the expansion board
   Wire.write(0x0);
   delay(50);
   Wire.write(GPIOB); // GPIOB is for the relay outputs of the expansion board
   Wire.write(data);
   Wire.endTransmission();
}

void invalidSelection()
{
   String temp1 = message; // store previous message to display temporary message
   String temp2 = message2; // store previous message2 to display temporary message2
   writetodisplay("     invalid    ", "    selection   ");
   delay(500);
   writetodisplay(temp1, temp2);
}

void writetodisplay(String input1, String input2)
{
  message = input1;
  message2 = input2;
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(input1);
  lcd.setCursor(0, 1);
  lcd.print(input2);
}

void selftest()
{
   String var1;
   byte relays[] = { 0x0, 0x1, 0x2, 0x4, 0x8, 0x10, 0x20, 0x40, 0x80 };
   for (int i = 1; i < 9; i++)
   {
      var1 = "Relay: ";
      var1 = var1 + i;
      SetRelay("Self Test", var1, relays[i]);
      delay(500);
   }
   SetRelay("Self Test", "Relays = OFF", relays[0]);
   delay(2000);
   SetRelay("A = Floating TP", "B = Common TP", 0x0);
}