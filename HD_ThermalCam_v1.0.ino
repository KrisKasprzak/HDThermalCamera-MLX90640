/*

  example program in using the mlx90640 32 x 24 thermal camera

  Compile 600 mhz, faster option

  requires Teensy 4.0,        https://www.pjrc.com/store/teensy40.html
  ILI9341 320 x 240 display (use the ones with black headers) https://www.amazon.com/gp/product/B017FZTIO6/ref=ppx_yo_dt_b_search_asin_title?ie=UTF8&psc=1
  32 x 24 Thermal camera,     https://www.digikey.com/en/product-highlight/m/melexis/mlx90640-fir-sensor

  Revision
  rev   data      author    Description
  1.0   4/4/2021  Kasprzak  Initial code
  
*/

#include <Wire.h>
#include "MLX90640_API.h"           // https://github.com/sparkfun/SparkFun_MLX90640_Arduino_Example
#include "MLX90640_I2C_Driver.h"    // https://github.com/sparkfun/SparkFun_MLX90640_Arduino_Example
#include <font_Arial.h>             //https://github.com/PaulStoffregen/ILI9341_t3
#include <font_ArialBold.h>         //https://github.com/PaulStoffregen/ILI9341_t3
#include <ILI9341_t3.h>             //https://github.com/PaulStoffregen/ILI9341_t3
#include "ILI9341_t3_Controls.h"    //https://github.com/KrisKasprzak/ILI9341_t3_controls
#include "EEPROM.h"
#include <SPI.h>
#include <XPT2046_Touchscreen.h>        // https://github.com/PaulStoffregen/XPT2046_Touchscreen
#include <ILI9341_t3_PrintScreen_SD.h>  //https://github.com/KrisKasprzak/ILI9341_t3_PrintScreen

#define BOX_SIZE 10
#define TA_SHIFT 8

#define ROW1 20
#define ROW2 50
#define ROW3 80
#define ROW4 110
#define ROW5 140
#define ROW6 170
#define ROW7 200

#define COL1 10
#define COL2 70

int BtnX, BtnY;  // holders for screen coordinate drawing
byte byteWidth, j, i, sbyte;
const uint8_t settings_icon[] = {
  // create icon in some paintint program or import icon into some painting program
  // resize to your desired pixel size
  // reduce colors to 2 (inverted though)
  // uploade and get the c file https://lvgl.io/tools/imageconverter
  // 20 x 20 used here
  0x00, 0xf0, 0x00, 0x00, 0x90, 0x00,
  0x1d, 0x9b, 0x80, 0x37, 0x0e, 0xc0,
  0x20, 0x00, 0x40, 0x30, 0x00, 0xc0,
  0x11, 0xf8, 0x80, 0x33, 0x0c, 0xc0,
  0xe2, 0x04, 0x70, 0x82, 0x04, 0x10,
  0x82, 0x04, 0x10, 0xe2, 0x04, 0x70,
  0x33, 0x0c, 0xc0, 0x11, 0xf8, 0x80,
  0x30, 0x00, 0xc0, 0x20, 0x00, 0x40,
  0x37, 0x0e, 0xc0, 0x1d, 0x9b, 0x80,
  0x00, 0x90, 0x00, 0x00, 0xf0, 0x00,
};

// note guessing at FPS....while sensor may measure in hz, interpolate and display
// can cut that in half
const char *FPS[] = { "0 FPS", "0 FPS", "1 FPS", "2 FPS", "4 FPS", "8 FPS" };
const char *TR[] = { "+/- 2 C", "+/- 3 C", "+/- 4 C", "+/- 5 C", "+/- 6 C", "+/- 7 C"};
int TRVal[] = { 2, 3, 4, 5, 6, 7};

char Name[15] = "HDCAM000.bmp";

byte SensorAddress = 0x33;
byte red, green, blue;
byte Histogram[50];
int MaxPoints;
int MINTEMP = 22;
int MAXTEMP = 28;
int x, y, val, stat_val, MaxCol;
float Temps[320][24]; // well cache the columns (x) but draw the rows (y) as we interpolate
float SensorTemps[768];
float a, b, c, High, Low, Delta, TempC, vdd, Ta, tr, emissivity;
byte MenuOption0ID, MenuOption1ID, MenuOption2ID;
byte MenuOptionVal = 1;
int InterpolateVal = 1;
int TemperatureVal = 22, RangeVal = 3, RefreshVal = 3;
uint16_t SensorFrame[834];
int SensorStat ;
int kk, jj;
float inc;
int FileInc = 0;
uint16_t SensorParameters[832];

// if you omit the RESET pin and you get white screens, try adding a resistor in series to 3V3 and RESET and
// a cap (maybe 1000 ohm and 10 uF) basically slow the charge to display RESET pin
ILI9341_t3 Display(10, 9, 8);

SliderH Temperature(&Display);
SliderH Range(&Display);
SliderH Refresh(&Display);

OptionButton MenuDisplay(&Display);
CheckBox Interpolate(&Display);

Button SettingsBTN(&Display);
Button DoneBTN(&Display);

paramsMLX90640 Sensor;

// create the // Touch screen object
XPT2046_Touchscreen Touch(0, 1);

TS_Point p;

void setup() {

  Serial.begin(38400);
  // while (!Serial);
  Wire.begin();

  //Increase I2C clock speed to 1 mhz (must be using teensy 4.0 or higher), otherwise use something like 400000
  Wire.setClock(1000000);
  Serial.println("Starting");

  Display.begin();
  Display.setRotation(3);

  Touch.begin();
  Touch.setRotation(1);

  GetParameters();

  GetTempRange();

  SettingsBTN.init(200, 20, 70, 35, ILI9341_BLUE, ILI9341_WHITE, ILI9341_BLACK, ILI9341_BLACK, "Option", -10, -8, Arial_14 ) ;
  DoneBTN.init(    270, 200, 70, 35, ILI9341_BLUE, ILI9341_WHITE, ILI9341_BLACK, ILI9341_BLACK, "Done", -10, -8, Arial_14 ) ;

  Temperature.init (140, ROW1, 155, 0, 100, 5, 1, ILI9341_WHITE, ILI9341_BLACK, ILI9341_BLUE);
  Range.init       (140, ROW2, 155, 0, 5, 1, 1, ILI9341_WHITE, ILI9341_BLACK, ILI9341_BLUE);
  Refresh.init     (140, ROW3, 155, 2, 5, 1, 1, ILI9341_WHITE, ILI9341_BLACK, ILI9341_BLUE);
  MenuDisplay.init(ILI9341_WHITE, ILI9341_BLUE , ILI9341_WHITE , ILI9341_WHITE, ILI9341_BLACK, 20, -2, Arial_14 );
  MenuOption0ID = MenuDisplay.add(30, ROW4, "Hide panel");
  MenuOption1ID = MenuDisplay.add(30, ROW5, "Gradient panel");
  MenuOption2ID = MenuDisplay.add(30, ROW6, "Histogram panel");

  MenuDisplay.select(MenuOptionVal);

  Interpolate.init(10, ROW7, ILI9341_WHITE, ILI9341_BLUE , ILI9341_WHITE, ILI9341_WHITE, ILI9341_BLACK, 30, 2, "Smooth image", Arial_14 );

  Display.fillScreen(ILI9341_BLACK);

  Display.fillRect(0, 0, 319, 5, ILI9341_WHITE);
  Display.fillRect(0, 234, 319, 5, ILI9341_WHITE);
  Display.fillRect(0, 0, 5, 239, ILI9341_WHITE);
  Display.fillRect(314, 0, 5, 239, ILI9341_WHITE);

  Display.setFont(Arial_24);

  Display.setTextColor(ILI9341_BLUE);
  Display.setCursor(12 , 32 );
  Display.print(F("HD Thermal Camera"));

  Display.setTextColor(ILI9341_GREEN);
  Display.setCursor(11 , 31 );
  Display.print(F("HD Thermal Camera"));

  Display.setTextColor(ILI9341_RED);
  Display.setCursor(10 , 30 );
  Display.print(F("HD Thermal Camera"));

  Display.setTextColor(ILI9341_WHITE);
  Display.setFont(Arial_18);
  Display.setCursor(10 , 110 - 20);
  Display.print(F("Sensor: "));

  Wire.beginTransmission((uint8_t)SensorAddress);
  stat_val = Wire.endTransmission();
  if (stat_val != 0)   {
    Display.setCursor(160 , 110 - 20 );
    Display.print(F("NOT FOUND"));
    while (1);
  }

  Display.setCursor(160 , 110 - 20 );
  Display.print(F("FOUND"));
  //Serial.println("MLX90640 online!");

  stat_val = MLX90640_DumpEE(SensorAddress, SensorParameters);
  Display.setCursor(10 , 135 - 20 );
  Display.print(F("Parameters: "));

  if (stat_val != 0) {
    Display.setCursor(160 , 135 - 20);
    Display.print(F("NOT FOUND"));
  }
  Display.setCursor(160 , 135 - 20 );
  Display.print(F("FOUND"));

  Display.setCursor(10 , 160 - 20);
  Display.print(F("Settings: "));
  stat_val = MLX90640_ExtractParameters(SensorParameters, &Sensor);
  if (stat_val != 0) {
    Display.setCursor(160 , 160 - 20 );
    Display.print(F("NOT FOUND"));
  }
  Display.setCursor(160 , 160 - 20 );
  Display.print(F("FOUND"));

  MLX90640_SetResolution(SensorAddress, 2);

  Display.setCursor(10 , 210 - 20 );
  Display.print(F("Refresh rate:"));
  Display.setCursor(160 , 210 - 20 );
  Display.print(FPS[RefreshVal]);
  MLX90640_SetRefreshRate(SensorAddress, RefreshVal);

  // not sure what this really does
  // Serial.print("Chess mode "); Serial.println(MLX90640_SetChessMode(SensorAddress));

  delay(2000);

  DrawPanel();

}

void loop() {

  if (Touch.touched()) {
    ProcessTouch();

    if ((BtnX > 290) & (BtnY < 50)) {
      DrawMenu();
      DrawPanel();
      delay(1000); // attempt to debounce button
    }
    else if (BtnX < 200) {
      FileInc++;
      Name[5] =  (int) (FileInc / 100) + '0';
      Name[6] =  (int) ((FileInc / 10) % 10) + '0';
      Name[7] =  (int) (FileInc  % 10) + '0';

      delay(500);
      Serial.print("Saving file to : ");
      Serial.println(Name);
      bool worked = SaveBMP24(&Display, A0, Name);
      if (worked) {
        Display.fillRect((320 - 200) / 2, (240 - 50) / 2, 200, 50, ILI9341_WHITE);
        Display.setFont(Arial_12);
        Display.setTextColor(ILI9341_BLACK);
        Display.setCursor((320 - 200) / 2 + 10 , (240 - 50) / 2 + 15);
        Display.print(Name);
        Display.print(F(" saved."));

        Serial.println("Save a success.");
        delay(100);
        Display.fillRect((320 - 200) / 2, (240 - 50) / 2, 200, 50, ILI9341_WHITE);
        Display.setFont(Arial_12);
        Display.setTextColor(ILI9341_BLACK);
        Display.setCursor((320 - 200) / 2 + 10 , (240 - 50) / 2 + 15);
        Display.print(F("Displaying."));
        delay(100);

        Serial.println("Drawing BMP file: ");
        Serial.print(Name);
        Display.fillScreen(ILI9341_BLACK);
        DrawBMP24(&Display, A0, Name);
        Serial.print("Drawing complete.");
        delay(1000);
        Display.fillRect((320 - 150) / 2, (240 - 25) / 2, 150, 25, ILI9341_WHITE);
        Display.setFont(Arial_12);
        Display.setTextColor(ILI9341_BLACK);
        Display.setCursor((320 - 150) / 2 + 5 , (240 - 30) / 2 + 7);
        Display.print(F("Touch to continue."));
        while (!Touch.touched()) {}

        Display.fillScreen(ILI9341_BLACK);
        DrawPanel();
      }
      else {
        Display.fillRect((320 - 200) / 2, (240 - 50) / 2, 200, 50, ILI9341_WHITE);
        Display.setFont(Arial_18);
        Display.setTextColor(ILI9341_RED);
        Display.setCursor((320 - 200) / 2 + 20 , (240 - 50) / 2 + 15);
        Display.print(F("Save failed."));
        Serial.println("Save fail.");
      }
    }
  }

  if (MenuOptionVal == 0) {
    drawBitmap(295, 5, settings_icon, 20, 20, ILI9341_BLACK); //draw SD card icon
  }

  for (byte x = 0 ; x < 2 ; x++)   {
    SensorStat = MLX90640_GetFrameData(SensorAddress, SensorFrame);

    if (SensorStat < 0)     {
      Display.fillScreen(ILI9341_BLACK);
      Display.setFont(Arial_24);
      Display.setTextColor(ILI9341_WHITE);
      Display.setCursor(12 , 32 );
      Display.print(F("Camera ERROR: "));
      Display.print(SensorStat);
    }

    //float vdd = MLX90640_GetVdd(SensorFrame, &mlx90640);
    Ta = MLX90640_GetTa(SensorFrame, &Sensor);
    tr = Ta - TA_SHIFT; //Reflected temperature based on the sensor ambient temperature
    emissivity = 0.95;
    MLX90640_CalculateTo(SensorFrame, &Sensor, emissivity, tr, SensorTemps);
  }

  // clear histogram array for next pass
  MaxPoints = 0;
  for (jj = 0; jj < 50; jj++) {
    Histogram[jj] = 0;
  }

  for (kk = 0; kk < 768; kk++) {
    for (jj = 0; jj < 50; jj++) {
      if (abs(SensorTemps[kk] - (MINTEMP + (inc * jj))) < inc) {

        Histogram[jj]++;
        if (Histogram[jj] > MaxPoints) {
          MaxPoints = Histogram[jj];
        }
        break;
      }
    }
  }

  // interpolate x row first
  if (InterpolateVal == 1) {
    if (MenuOptionVal == 0) {
      MaxCol = 310;
    }
    else {
      MaxCol = 285;
    }

    for (y = 0; y < 24; y++) {
      for (x = 0 ; x < MaxCol ; x++)   {

        Low = SensorTemps[(x / 10) + (32 * y)];
        High = SensorTemps[(x / 10) + (32 * y) + 1];

        Delta = (High - Low) / 10.0;
        // Serial.print("Delta ");Serial.println(Delta);
        TempC = Low +  (Delta * (x % 10) );
        Temps[x][y] = TempC;
      }
    }

    // interpolate y and draw
    for (x = 0 ; x < MaxCol ; x++)   {
      for (y = 0; y < 230; y++) {

        Low = Temps[x][y / 10];
        High = Temps[x][(y / 10) + 1];

        Delta = (High - Low) / 10.0;
        TempC = Low + (Delta * (y % 10));

        red = constrain(255.0 / (c - b) * TempC - ((b * 255.0) / (c - b)), 0, 255);
        if (TempC < a) {
          green = constrain(255.0 / (a - MINTEMP) * TempC - (255.0 * MINTEMP) / (a - MINTEMP), 0, 255);
        }
        else if (TempC > c) {
          green = constrain(255.0 / (c - MAXTEMP) * TempC - (MAXTEMP * 255.0) / (c - MAXTEMP), 0, 255);
        }
        else {
          green = 255;
        }
        if (TempC > (MAXTEMP )) {
          blue = constrain((TempC - MAXTEMP) * 55.0, 0, 255);
        }
        else {
          blue = constrain(255.0 / (a - b) * TempC - ((a * 255.0) / (a - b)), 0, 255);
        }

        Display.drawPixel(x + 5, y + 5, Display.color565(red, green, blue));

      }

    }

    if (MenuOptionVal == 0) {
      UpdateBlank();
    }
    else if (MenuOptionVal == 1) {
      UpdateGradient();
    }
    else if (MenuOptionVal == 2) {
      UpdateHistogram();
    }

  }
  else {
    val = 0;

    for (int y = 0; y < 24; y++) {
      for (int x = 0 ; x < 32 ; x++)   {

        // heck just increment a counter to avoid doing the math to convert a 1 dim array to a 2 dim
        TempC = SensorTemps[val];

        red = constrain(255.0 / (c - b) * TempC - ((b * 255.0) / (c - b)), 0, 255);
        if (TempC < a) {
          green = constrain(255.0 / (a - MINTEMP) * TempC - (255.0 * MINTEMP) / (a - MINTEMP), 0, 255);
        }
        else if (TempC > c) {
          green = constrain(255.0 / (c - MAXTEMP) * TempC - (MAXTEMP * 255.0) / (c - MAXTEMP), 0, 255);
        }
        else {
          green = 255;
        }
        if (TempC > (MAXTEMP )) {
          blue = constrain((TempC - MAXTEMP) * 55.0, 0, 255);
        }
        else {
          blue = constrain(255.0 / (a - b) * TempC - ((a * 255.0) / (a - b)), 0, 255);
        }

        Display.fillRect((x * BOX_SIZE), (y * BOX_SIZE), BOX_SIZE, BOX_SIZE, Display.color565(red, green, blue));

        val++;

      }
    }
    UpdateBlank();
  }

}

unsigned int GetColor(float TempC) {

  red = constrain(255.0 / (c - b) * TempC - ((b * 255.0) / (c - b)), 0, 255);
  if (TempC < a) {
    green = constrain(255.0 / (a - MINTEMP) * TempC - (255.0 * MINTEMP) / (a - MINTEMP), 0, 255);
  }
  else if (TempC > c) {
    green = constrain(255.0 / (c - MAXTEMP) * TempC - (MAXTEMP * 255.0) / (c - MAXTEMP), 0, 255);
  }
  else {
    green = 255;
  }
  if (TempC > (MAXTEMP )) {
    blue = constrain((TempC - MAXTEMP) * 55.0, 0, 255);
  }
  else {
    blue = constrain(255.0 / (a - b) * TempC - ((a * 255.0) / (a - b)), 0, 255);
  }

  return Display.color565(red, green, blue);
}

void drawBitmap(int16_t x, int16_t y, const uint8_t *bitmap, int16_t w, int16_t h, uint16_t color) {

  byteWidth = (w + 7) / 8;

  for (j = 0; j < h; j++) {
    for (i = 0; i < w; i++) {
      if (i & 7)  sbyte <<= 1;
      else sbyte   = pgm_read_byte(bitmap + j * byteWidth + i / 8);
      if (sbyte & 0x80) Display.drawPixel(x + i, y + j, color);
    }
  }
}

bool ProcessButtonPress(Button TheButton) {

  if (TheButton.press(BtnX, BtnY)) {
    TheButton.draw(B_PRESSED);
    while (Touch.touched()) {
      if (TheButton.press(BtnX, BtnY)) {
        TheButton.draw(B_PRESSED);
      }
      else {
        TheButton.draw(B_RELEASED);
        return false;
      }
      ProcessTouch();
    }

    TheButton.draw(B_RELEASED);
    return true;
  }
  return false;

}

void ProcessTouch() {

  p = Touch.getPoint();

  BtnX = p.x;
  BtnY = p.y;

  // consistency between displays is a mess...
  // this is some debug code to help show
  // where you pressed and the resulting map

  //Serial.print("real: ");
  //Serial.print(BtnX);
  //Serial.print(",");
  //Serial.print(BtnY);
  //Display.drawPixel(BtnX, BtnY, C_RED);

  //different values depending on where Touch happened

  // x  = map(x, real left, real right, 0, width-1);
  // y  = map(y, real bottom, real top, 0, height-1);

  // tft with black headers, yellow headers will be different
  BtnX  = map(BtnX, 3700, 300, 0, 319);
  BtnY  = map(BtnY, 3800, 280, 0, 239);

  //Serial.print(", Mapped: ");
  //Serial.print(BtnX);
  //Serial.print(",");
  //Serial.println(BtnY);
  //Display.drawPixel(BtnX, BtnY, C_GREEN);
  delay(5);

}

void GetParameters() {

  // if your teensy locks up it may be due to EEPROM has no values
  // and an array has an out of bound element
  // force the puts here for the first time using

  if (EEPROM.get(0, TemperatureVal) == 255) {
    // new Teensy populate the eeprom
    EEPROM.put(0, TemperatureVal);
    EEPROM.put(5, RangeVal);
    EEPROM.put(10, RefreshVal);
    EEPROM.put(15, MenuOptionVal);
    EEPROM.put(20, InterpolateVal);
  }

  EEPROM.get(0, TemperatureVal);
  EEPROM.get(5, RangeVal);
  EEPROM.get(10, RefreshVal);
  EEPROM.get(15, MenuOptionVal);
  EEPROM.get(20, InterpolateVal);

}


void DrawMenu() {

  bool KeepIn = true;
  Display.fillScreen(ILI9341_BLACK);
  Display.fillRect(0, 0, 319, 5, ILI9341_WHITE);
  Display.fillRect(0, 234, 319, 5, ILI9341_WHITE);
  Display.fillRect(0, 0, 5, 239, ILI9341_WHITE);
  Display.fillRect(314, 0, 5, 239, ILI9341_WHITE);

  Display.setFont(Arial_14);
  Display.setTextColor(ILI9341_WHITE);

  Display.setCursor(COL1 , ROW1 - 7 ); Display.print(F("Temp."));
  Display.setCursor(COL2 , ROW1 - 7 ); Display.print(TemperatureVal);
  Display.setCursor(COL1 , ROW2 - 7 ); Display.print(F("Range"));
  Display.setCursor(COL2 , ROW2 - 7 ); Display.print(TR[RangeVal]);
  Display.setCursor(COL1 , ROW3 - 7); Display.print(F("Rate"));
  Display.setCursor(COL2 , ROW3 - 7); Display.print(FPS[RefreshVal]);

  Temperature.draw(TemperatureVal);
  Range.draw(RangeVal);
  Refresh.draw(RefreshVal);

  MenuDisplay.draw(MenuOptionVal);
  Interpolate.draw(InterpolateVal);

  DoneBTN.draw();
  delay(1000); // attempt to debounce button

  while (KeepIn) {
    if (Touch.touched()) {

      ProcessTouch();

      Temperature.slide(BtnX, BtnY);
      Range.slide(BtnX, BtnY);
      Refresh.slide(BtnX, BtnY);

      Interpolate.press(BtnX, BtnY);
      MenuDisplay.press(BtnX, BtnY);

      TemperatureVal = Temperature.value;
      RangeVal = Range.value;
      RefreshVal = Refresh.value;

      Display.setCursor(COL1 , ROW1 - 7 ); Display.print(F("Temp."));
      Display.fillRect(COL2 , ROW1 - 7, 60, 20, ILI9341_BLACK);
      Display.setCursor(COL2 , ROW1 - 7 ); Display.print(TemperatureVal);

      Display.setCursor(COL1 , ROW2 - 7 ); Display.print(F("Range"));
      Display.fillRect(COL2 , ROW2 - 7, 60, 20, ILI9341_BLACK);
      Display.setCursor(COL2 , ROW2 - 7 ); Display.print(TR[RangeVal]);

      Display.setCursor(COL1 , ROW3 - 7 ); Display.print(F("Rate"));
      Display.fillRect(COL2 , ROW3 - 7, 60, 20, ILI9341_BLACK);
      Display.setCursor(COL2 , ROW3 - 7); Display.print(FPS[RefreshVal]);

      if (ProcessButtonPress(DoneBTN)) {
        KeepIn = false;
      }
    }
  }


  TemperatureVal = Temperature.value;
  RangeVal = Range.value;
  RefreshVal = Refresh.value;

  MenuOptionVal = MenuDisplay.value;
  InterpolateVal = Interpolate.value;

  MLX90640_SetRefreshRate(SensorAddress, RefreshVal);

  GetTempRange();

  EEPROM.put(0, TemperatureVal);
  EEPROM.put(5, RangeVal);
  EEPROM.put(10, RefreshVal);
  EEPROM.put(15, MenuOptionVal);
  EEPROM.put(20, InterpolateVal);

  Display.fillScreen(ILI9341_BLACK);

}

void DrawPanel() {

  if (MenuOptionVal == 0) {
    Display.fillRect(0, 0, 319, 5, ILI9341_WHITE);
    Display.fillRect(0, 234, 319, 5, ILI9341_WHITE);
    Display.fillRect(0, 0, 5, 239, ILI9341_WHITE);
    Display.fillRect(314, 0, 5, 239, ILI9341_WHITE);
  }
  else if (MenuOptionVal != 0) {
    Display.fillRect(0, 0, 319, 5, ILI9341_WHITE);
    Display.fillRect(0, 234, 319, 5, ILI9341_WHITE);
    Display.fillRect(0, 0, 5, 239, ILI9341_WHITE);
    Display.fillRect(314, 0, 5, 239, ILI9341_WHITE);
    Display.fillRect(290, 0, 29, 239, ILI9341_WHITE);
  }

  drawBitmap(295, 5, settings_icon, 20, 20, ILI9341_BLACK); //draw SD card icon

}

void UpdateBlank() {

  drawBitmap(295, 5, settings_icon, 20, 20, ILI9341_BLACK); //draw SD card icon

}

void UpdateGradient() {

  int j;
  float ii;

  inc = (MAXTEMP - MINTEMP ) / 160.0;
  j = 0;
  for (ii = MINTEMP; ii < MAXTEMP; ii += inc) {
    Display.drawFastHLine(295, 210 - j++, 20, GetColor(ii));
  }

  Display.setFont(Arial_12);
  Display.setTextColor(ILI9341_BLACK);
  Display.setCursor(295 , 30 );
  Display.print(MAXTEMP);
  Display.setCursor(295 , 215 );
  Display.print(MINTEMP);

}

void UpdateHistogram() {

  int j;
  byte Length;

  for (j = 0; j < 50; j++) {
    Length = map(Histogram[j], 0, MaxPoints, 0, 24);
    if (Length < 1) {
      Length = 1;
    }
    if (Length > 24) {
      Length = 24;
    }
    
    // draw histogram bars in color 
    Display.fillRect(293, 200 - (j * 3), Length, 2, GetColor( (MINTEMP + (inc * j))   ));
    
    // or draw black
    // Display.fillRect(293, 200 - (j * 3), Length, 2, ILI9341_BLACK);
    
    // regarless paint the remainder of the old bar. 
    Display.fillRect(293 + Length, 200 - (j * 3), 24 - Length, 2, ILI9341_WHITE);
    
  }

  Display.setFont(Arial_12);
  Display.setTextColor(ILI9341_BLACK);
  Display.setCursor(295 , 30 );
  Display.print(MAXTEMP);
  Display.setCursor(295 , 215 );
  Display.print(MINTEMP);

}

void GetTempRange() {

  MINTEMP = TemperatureVal - TRVal[RangeVal];
  MAXTEMP = TemperatureVal + TRVal[RangeVal];

  inc = (MAXTEMP - MINTEMP) / 50.0;


  b = (MAXTEMP + MINTEMP) / 2.0;
  a = b - (MAXTEMP - MINTEMP) * .08 ;  // 0.08
  c = b + (MAXTEMP - MINTEMP) * .15 ; // 0.15

}
