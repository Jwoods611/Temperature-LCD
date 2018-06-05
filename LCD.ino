#include <Wire.h> //Necessary to include Wire library for I2C comm
#define ADDR 0x27 //Device address
#define TEMP_ADDR 0x48 //Temperature sensor device address
#define CMD_WRITE 0x00 //Constant used to set RS control signal low
#define DATA_WRITE 0x01 //Constant used to set RS control signal high
#define CMD_MOVE 0x80 //Constant to specify the "move cursor" command
#define BL 0x08 //Constant for blacklight control signal high
#define EN 0x04 //Constant for EN control signal high
#define RS 0x01 //Constant for RS control signal high


//array to hold first column index for each line in the LCD
byte glbLineAddr[4] = {0x00, 0x40, 0x14, 0x54}; 
int temp; //temperature variable
String tempString; //string to hold the temperature
/*formatStrings 1-3 are used for formatting the LCD display in order    
 *to achieve desired output
 */
String formatString1 = "";
String formatString2 = "";
String formatString3 = "";
byte line = 0x00; //Init line index to 0
byte column = 0x00; //Init column index to 0

void setup() {
  Wire.begin(); //Init the Wire library for I2C comm
  Serial.begin(9600); //Init Serial connection with 9600 baud
  clearLCD(); //Start by clearing the LCD
  /*
   *Again, formatStrings 1-3 are used for formatting the LCD display
   */
  formatString1 = "    Temperature     ";
  formatString2 = "                    ";
  formatString3 = "    Fahrenheit     ";
  /*
   *Print the formatted strings to the LCD. The temperature will be 
   *added byreplacing formatString2 with tempString once we
   *retrieve the temperature reading from the sensor.
   */
  printStrLCD(formatString1 + formatString2 + formatString3);
}

void loop() {
  temp = 32 + GetTemp()*(9/5); //Retrieve temp., convert to fahrenheit
  tempString = temp; //parse int to string to be displayed
  /*
   *The correct positioning for the temperature on the display is
   *the 10th column of the second line down, which corresponds to the
   *element of index 1 in the glbLineAddr array defined globally,
   *After appending the display with tempString, we move the LCD 
   *cursor to line
   *0, column 0, then also print the temperature to the serial 
   *monitor. Finally, we delay by one second and repeat indefinitely.
   */
  line = 0x01;
  column = 0x09;
  moveCursorLCD(line, column);
  printStrLCD(tempString);
  homeLCD();
  Serial.println(tempString);
  delay(1000);
}

/*
 *This function retrieves the temperature reading from the temperature 
 *sensor by sending the sensor's device address via I2C, then 
 *requesting and reading 1 byte from the sensor. Finally, a sign  
 *extension is added to the byte. This is necessary because 'temp' is 
 *stored in an int which is 2 bytes, but the temperature we get back  
 *from the sensor is only 1 byte. It also allows for accurate reading  
 *of the temperature with regard to sign.
 */
int GetTemp () {
  int value = 0;
  Wire.beginTransmission(TEMP_ADDR);
  Wire.write(0);
  Wire.endTransmission();
  Wire.requestFrom(TEMP_ADDR,1);
  byte byteIn = Wire.read();
  
  if (byteIn >= 0x80)
    value = 0xFF00 | int(byteIn);
  else
    value = int(byteIn);
  return value;
}

/*
 * This function writes a byte of data to the LCD by sending the LCD's 
 *device address via I2C, then doing 4 I2C writes. The upper nibble is 
 *sent, first with EN = 1, then with EN = 0. Next, the lower nibble is  
 *sent with EN = 1, then again with EN = 0. Finally, the RS control  
 *signal needs to be high because RS = 1 selects the data register in  
 *the LCD.
 */
void dataWriteLCD (byte data) {
  Wire.beginTransmission(ADDR);
  Wire.write(createUpperNibble(data)|BL|EN|RS);
  Wire.write(createUpperNibble(data)|BL|RS);
  Wire.write(createLowerNibble(data)|BL|EN|RS);
  Wire.write(createLowerNibble(data)|BL|RS);
  Wire.endTransmission();
}

/*
 *This function is similar to dataWriteLCD, except it's used to write  
 *commands to the LCD instead of data. For this reason, RS will always 
 *be low because RS = 0 selects the instruction register of the LCD.
 */
void cmdWriteLCD (byte data) {
  Wire.beginTransmission(ADDR);
  Wire.write(createUpperNibble(data)|BL|EN);
  Wire.write(createUpperNibble(data)|BL);
  Wire.write(createLowerNibble(data)|BL|EN);
  Wire.write(createLowerNibble(data)|BL);
  Wire.endTransmission();
}

/*
 * This function moves the LCD cursor to a desired line and column
 *Index, which are both passed as parameters to the function. This  
 *function then computes
 *a "place" on the LCD display based on the input line and column 
 *index, and bitwise ORs that place with the constant specifying the  
 *MOVE command.
 * Finally, this function writes the resulting byte to the LCD as a command.
 */
void moveCursorLCD(byte line, byte column) {
  byte place;
  place = glbLineAddr[line]+column;  
  place |= CMD_MOVE;
  cmdWriteLCD(place);
}
/*
 *This function clears the LCD display. First, it specifies the
 *"clear display" command (from the data sheet) with a 0x01, then   
 *passes this command to the LCD using cmdWriteLCD. Finally, this 
 *function implements a necessary delay of 10ms.
 */
void clearLCD () {
  cmdWriteLCD(0x01);
  delay(10);
}

/*
 *This function moves the LCD cursor to line 0, column 0 of the LCD  
 *display by passing 0x00 as both the line and column index as  
 *arguments to the moveCursorLCD function. Then, it implements a 10ms 
 *delay to ensure enough time has passed to move the cursor.
 */
void homeLCD() {
  moveCursorLCD(0x00, 0x00);
  delay(10);
}

/*
 *This function prints a string (passed as a parameter to the 
 *function)to the LCD. First, this function converts the String object  
 *to an array of chars. Then, it moves the cursor to the desired 
 *location. For our purposes, this will be line 1, column 10 (0x09 due 
 *to zero-indexing) because that's the position where the temperature 
 *will be printed. Then, it writes the data to the LCD, char by char, 
 *incrementing column. Finally, if the column index surpasses the 
 *number of columns in the LCD display, it moves the data to
 *the next line.
 */
void printStrLCD (String s) {
  char strang[s.length()];
  s.toCharArray(strang, s.length()+1);

  for (int i=0;i<s.length();i++) {
    moveCursorLCD(line, column);
    dataWriteLCD(strang[i]);
    column++;
    if (column > 0x13) {
      column = 0x00;
      line++;
    }
  }
}

/*
 *Since we need to send one nibble at a time via I2C, this function
 *extracts the upper nibble from a byte of data by bitwise ANDing it 
 *with 1111 0000 (0xF0).
 */
byte createUpperNibble(byte data) {
  return data & 0xF0;  
}

/*
 *Similar to createUpperNibble, this function extracts the 
 *lower nibble of a byte of data to send via I2C by 
 *left-shifting the byte by 4 bits. This works because the bits
 *are not carried around to the other side. Rather, bitwise shifting
 *fills these with 0s, giving us the lower nibble.
 */
byte createLowerNibble(byte data) {
  return data<<4;  
}

