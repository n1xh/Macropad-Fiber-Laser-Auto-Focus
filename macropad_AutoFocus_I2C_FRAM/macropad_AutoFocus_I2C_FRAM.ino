#include <Adafruit_SH110X.h>
#include <Adafruit_NeoPixel.h>
#include <RotaryEncoder.h>
#include <Wire.h>
#include <Adafruit_ADS1X15.h>
#include <Adafruit_PCF8575.h>
#include "FRAM.h"
#include "Keyboard.h"

FRAM fram;

//Define Fram Variables
int MotorSpeed = 10;
String LensSize;
float LensFL;
float resistor;
int SensorMin = 100;
int SensorMax = 400; //LE250
//int SensorMax = 1000; //LE550
int LensNum;
String Lens1Size = "70x70x100";
float Lens1FL;
float Resistor1;
String Lens2Size = "110x110x163";
float Lens2FL;
float Resistor2;
String Lens3Size = "150x150x210";
float Lens3FL;
float Resistor3;
String Lens4Size = "200x200x290";
float Lens4FL;
float Resistor4;
String Lens5Size = "400x400x525";
float Lens5FL;
float Resistor5;

// Define PCF8575 Constants
const int enaPin = 9;
const int dirPin = 10;
const int stepPin = 11;
const int sensorDigitalPin = 14;

// Define ADS1115 Constants
const int sensorAnalogPin = 3;

// Create the neopixel strip with the built in definitions NUM_NEOPIXEL and PIN_NEOPIXEL
Adafruit_NeoPixel pixels = Adafruit_NeoPixel(NUM_NEOPIXEL, PIN_NEOPIXEL, NEO_GRB + NEO_KHZ800);

// Create the OLED display
Adafruit_SH1106G display = Adafruit_SH1106G(128, 64, &SPI1, OLED_DC, OLED_RST, OLED_CS);

// Create the rotary encoder
RotaryEncoder encoder(PIN_ROTB, PIN_ROTA, RotaryEncoder::LatchMode::FOUR3);
void checkPosition() {
  encoder.tick();  // just call tick() to check the state.
}

//Define Variables
int encoder_pos = 0;
int stepperDelay;

int adc;
float distance;

// Create the ads1115
Adafruit_ADS1115 ads1115;

// Create the PCF8575
Adafruit_PCF8575 pcf;

void setup() {
  Serial.begin(115200);
  //while (!Serial) { delay(10); }     // wait till serial port is opened
  delay(100);  // RP2040 delay is not a bad idea
  Serial.println("Adafruit Macropad with RP2040");
  //Start Adafruit_PCF8575 pcf
  if (!pcf.begin(0x20, &Wire)) {
    Serial.println("Couldn't find PCF8575");
   // while (1);
  }




  Keyboard.begin();
  // We will use I2C for scanning the Stemma QT port
  Wire.begin();

  Wire.setClock(400000);
  //    LensNum = 0; // reset to first run
  //    fram.writeObject(50, LensNum);// reset to first run

  fram.readObject(50, LensNum);
  fram.readObject(54, Lens1FL);
  fram.readObject(58, Resistor1);
  fram.readObject(62, Lens2FL);
  fram.readObject(66, Resistor2);
  fram.readObject(70, Lens3FL);
  fram.readObject(74, Resistor3);
  fram.readObject(78, Lens4FL);
  fram.readObject(82, Resistor4);
  fram.readObject(86, Lens5FL);
  fram.readObject(90, Resistor5);
  storeDefaultsFirstRun();
  //Set pin modes
  pcf.pinMode(enaPin, OUTPUT);
  pcf.digitalWrite(enaPin, LOW);
  pcf.pinMode(dirPin, OUTPUT);
  pcf.digitalWrite(dirPin, HIGH);
  pcf.pinMode(stepPin, OUTPUT);
  pcf.digitalWrite(stepPin, HIGH);
  pcf.pinMode(sensorDigitalPin, INPUT_PULLUP);

  //start ads1115
  ads1115.begin();

  // start pixels!
  pixels.begin();
  pixels.setBrightness(6);
  pixels.show(); // Initialize all pixels to 'off'

  // Start OLED
  display.begin(0, true); // we dont use the i2c address but we will reset!
  display.display();

  // set all mechanical keys to inputs
  for (uint8_t i = 0; i <= 12; i++) {
    pinMode(i, INPUT_PULLUP);
  }

  ads1115.startADCReading(ADS1X15_REG_CONFIG_MUX_SINGLE_3, /*continuous=*/false);

  if (LensNum == 1) {
    LensSize = Lens1Size;
    LensFL = Lens1FL;
    resistor = Resistor1;
  }
  else if (LensNum == 2) {
    LensSize = Lens2Size;
    LensFL = Lens2FL;
    resistor = Resistor2;
  }
  else if (LensNum == 3) {
    LensSize = Lens3Size;
    LensFL = Lens3FL;
    resistor = Resistor3;
  }
  else if (LensNum == 4) {
    LensSize = Lens4Size;
    LensFL = Lens4FL;
    resistor = Resistor4;
  }
  else if (LensNum == 5) {
    LensSize = Lens5Size;
    LensFL = Lens5FL;
    resistor = Resistor5;
  }

  // set rotary encoder inputs and interrupts
  pinMode(PIN_ROTA, INPUT_PULLUP);
  pinMode(PIN_ROTB, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(PIN_ROTA), checkPosition, CHANGE);
  attachInterrupt(digitalPinToInterrupt(PIN_ROTB), checkPosition, CHANGE);


  // text display tests
  display.setTextSize(1);
  display.setTextWrap(false);
  display.setTextColor(SH110X_WHITE, SH110X_BLACK); // white text, black background
  display.clearDisplay();
  DisplayKeys();

  // Enable speaker
  pinMode(PIN_SPEAKER_ENABLE, OUTPUT);
  digitalWrite(PIN_SPEAKER_ENABLE, HIGH);
  // Play some tones
  pinMode(PIN_SPEAKER, OUTPUT);
  digitalWrite(PIN_SPEAKER, LOW);
  tone(PIN_SPEAKER, 988, 100);  // tone1 - B5
  delay(100);
  tone(PIN_SPEAKER, 1319, 200); // tone2 - E6
  delay(200);
}

uint8_t j = 0;
bool i2c_found[128] = {false};

void loop() {
  ads1115.setDataRate(RATE_ADS1115_8SPS);

  encoder.tick();          // check the encoder
  int newPos = encoder.getPosition();
  if (encoder_pos != newPos) {
    if (MotorSpeed + (newPos - encoder_pos) >= 1 && MotorSpeed + (newPos - encoder_pos) <= 10) {
      MotorSpeed = MotorSpeed + (newPos - encoder_pos);
      encoder_pos = newPos;
    }
    else {
      MotorSpeed = MotorSpeed;
      encoder_pos = newPos;
    }
  }
  //  display.clearDisplay();
  display.setCursor(0, 0);
  display.print(LensSize);
  display.print(" Speed: ");
  display.print(MotorSpeed);
  display.println("  ");
  display.print("Focal Length:");
  display.print(LensFL);
  display.println("mm");

  ReadDISTANCE();
  display.print("Distance: ");
  display.print(distance, 2);
  display.println("mm");

  // check encoder press
  if (!digitalRead(PIN_SWITCH)) {
    if (LensNum < 5) {
      LensNum++;
    }
    else {
      LensNum = 1;
    }
    if (LensNum == 1) {
      LensSize = Lens1Size;
      LensFL = Lens1FL;
      resistor = Resistor1;
    }
    else if (LensNum == 2) {
      LensSize = Lens2Size;
      LensFL = Lens2FL;
      resistor = Resistor2;
    }
    else if (LensNum == 3) {
      LensSize = Lens3Size;
      LensFL = Lens3FL;
      resistor = Resistor3;
    }
    else if (LensNum == 4) {
      LensSize = Lens4Size;
      LensFL = Lens4FL;
      resistor = Resistor4;
    }
    else if (LensNum == 5) {
      LensSize = Lens5Size;
      LensFL = Lens5FL;
      resistor = Resistor5;
    }
    display.setCursor(0, 0);
    display.print("*  Next Lens Size   *");
    display.display();
    pixels.setBrightness(255);     // bright!
    pixels.show();
    while (!digitalRead(PIN_SWITCH)) {
    }
  }
  else {
    pixels.setBrightness(6);
  }

  for (int i = 4; i < pixels.numPixels(); i++) {
    pixels.setPixelColor(i, Wheel(((i * 256 / pixels.numPixels()) + j) & 255));
  }
  
  pixels.setPixelColor(0, 0xFF0000); // make 1st key red
  pixels.setPixelColor(1, 0xFFFF00); // make 2nd key yellow
  pixels.setPixelColor(2, 0x008000); // make 3rd key green
  pixels.setPixelColor(3, 0x0000FF); // make 4th key blue 
   
  stepperDelay = map(MotorSpeed, 1, 10, 10000, 5) + 1000;

  for (int i = 1; i <= 12; i++) {
    if (!digitalRead(i)) { // switch pressed!
      pixels.setPixelColor(i - 1, 0xFFFFFF); // make white
      // show neopixels, incredment swirl
      pixels.show();
      j++;

      while (!digitalRead(1)) {
        display.setCursor(0, 0);
        display.println("*         UP        *");
        display.display();
        pcf.digitalWrite(enaPin, HIGH);
        while (!digitalRead(1)) {
          moveUp();
        }
        pcf.digitalWrite(enaPin, LOW);
        loop();
      }
      while (!digitalRead(2)) {
        display.setCursor(0, 0);
        display.println("*Calibrate Distance *");
        display.display();
        CalResistor();
        while (!digitalRead(2)) {
        }
        loop();
      }
      while (!digitalRead(3)) {
        display.setCursor(0, 0);
        display.println("* Set Focal Length  *");
        display.display();
        CalFocalLength();
        while (!digitalRead(3)) {
        }
        loop();
      }
      while (!digitalRead(4)) {
        display.setCursor(0, 0);
        display.println("*        DOWN       *");
        display.display();
        pcf.digitalWrite(enaPin, HIGH);
        while (!digitalRead(4)) {
          moveDown();
        }
        pcf.digitalWrite(enaPin, LOW);
        loop();
      }
      while (!digitalRead(5)) {
        display.setCursor(0, 0);
        display.println("*   Set to default  *");
        display.display();
        storeDefaults();
        while (!digitalRead(5)) {
        }
        loop();
      }
      while (!digitalRead(6)) {
        display.setCursor(0, 0);
        display.println("* Save Calibration  *");
        display.display();
        storeCal();
        while (!digitalRead(6)) {
        }
        loop();
      }
      while (!digitalRead(7)) {
        display.setCursor(0, 0);
        display.println("*  Auto Focus Hold  *");
        while (!digitalRead(7)) {
          autoFocusHold();
        }
        loop();
      }

      while (!digitalRead(8)) {
        display.setCursor(0, 0);
        display.println("*        COPY       *");
        display.display();
        Keyboard.press(KEY_LEFT_CTRL);  // press and hold Shift
        Keyboard.press('c');          // press and hold F2
        Keyboard.releaseAll();           // release both
        while (!digitalRead(8)) {
        }
        loop();
      }
      while (!digitalRead(9)) {
        display.setCursor(0, 0);
        display.println("*        PASTE       *");
        display.display();
        Keyboard.press(KEY_LEFT_CTRL);  // press and hold Shift
        Keyboard.press('v');          // press and hold F2
        Keyboard.releaseAll();           // release both
        while (!digitalRead(9)) {
        }
        loop();
      }
      while (!digitalRead(10)) {
        display.setCursor(0, 0);
        display.println("*    Auto  Focus    *");
        display.display();
        while (!digitalRead(10)) {
          autoFocus();
        }
        loop();
      }
      while (!digitalRead(11)) {
        display.setCursor(0, 0);
        display.println("*   Frame Laser    *");
        display.display();
        Keyboard.press(KEY_LEFT_ALT);  // press and hold Alt
        Keyboard.press('r');          // press and hold r
        Keyboard.releaseAll();           // release both
        while (!digitalRead(11)) {
        }
        loop();
      }
      while (!digitalRead(12)) {
        display.setCursor(0, 0);
        display.println("*   START LASER    *");
        display.display();
        Keyboard.press(KEY_F2); // press and hold F2
        Keyboard.releaseAll();
        while (!digitalRead(12)) {
        }
        loop();
      }
    }
  }

  // show neopixels, incredment swirl
  pixels.show();
  j++;

  // display oled
  display.display();
}

// Input a value 0 to 255 to get a color value.
// The colours are a transition r - g - b - back to r.
uint32_t Wheel(byte WheelPos) {
  if (WheelPos < 85) {
    return pixels.Color(255 - WheelPos * 3, 0, WheelPos * 3);
  } else if (WheelPos < 170) {
    WheelPos -= 85;
    return pixels.Color(0, WheelPos * 3, 255 - WheelPos * 3);
  } else {
    WheelPos -= 170;
    return pixels.Color(WheelPos * 3, 255 - WheelPos * 3, 0);
  }
}

void moveDown() {
  pcf.digitalWrite(dirPin, LOW);
  pcf.digitalWrite(stepPin, LOW);
  delayMicroseconds(stepperDelay);
  pcf.digitalWrite(stepPin, HIGH);
  delayMicroseconds(stepperDelay);
  //decrease stepper delay value by 1 microsecond
  if (stepperDelay > map(MotorSpeed, 1, 10, 10000, 5)) {
    stepperDelay = stepperDelay - 1;
  }
}

void moveUp() {
  pcf.digitalWrite(dirPin, HIGH);
  pcf.digitalWrite(stepPin, LOW);
  delayMicroseconds(stepperDelay);
  pcf.digitalWrite(stepPin, HIGH);
  delayMicroseconds(stepperDelay);
  //decrease stepper delay value by 1 microsecond
  if (stepperDelay > map(MotorSpeed, 1, 10, 10000, 5)) {
    stepperDelay = stepperDelay - 1;
  }
}

void CalFocalLength() {
  LensFL = distance;
  if (LensNum == 1) {
    Lens1FL = LensFL;
  }
  else if (LensNum == 2) {
    Lens2FL = LensFL;
  }
  else if (LensNum == 3) {
    Lens3FL = LensFL;
  }
  else if (LensNum == 4) {
    Lens4FL = LensFL;
  }
  else if (LensNum == 5) {
    Lens5FL = LensFL;
  }
}

void CalResistor() {
  ReadADC();
  resistor = (3 * adc * (SensorMax - SensorMin)) / (64 * (SensorMax - SensorMin + 4 * LensFL - 400));
  if (LensNum == 1) {
    Resistor1 = resistor;
  }
  else if (LensNum == 2) {
    Resistor2 = resistor;
  }
  else if (LensNum == 3) {
    Resistor3 = resistor;
  }
  else if (LensNum == 4) {
    Resistor4 = resistor;
  }
  else if (LensNum == 5) {
    Resistor5 = resistor;
  }
}

void beep() {
  tone(PIN_SPEAKER, 988, 100);  // tone1 - B5
  delay(100);
  tone(PIN_SPEAKER, 1319, 200); // tone2 - E6
  delay(200);
}

void storeCal() {
  if (LensNum == 1) {
    fram.writeObject(50, LensNum);
    fram.writeObject(54, Lens1FL);
    fram.writeObject(58, Resistor1);
  }
  else if (LensNum == 2) {
    fram.writeObject(50, LensNum);
    fram.writeObject(62, Lens2FL);
    fram.writeObject(66, Resistor2);
  }
  else if (LensNum == 3) {
    fram.writeObject(50, LensNum);
    fram.writeObject(70, Lens3FL);
    fram.writeObject(74, Resistor3);
  }
  else if (LensNum == 4) {
    fram.writeObject(50, LensNum);
    fram.writeObject(78, Lens3FL);
    fram.writeObject(82, Resistor3);
  }
  else if (LensNum == 5) {
    fram.writeObject(50, LensNum);
    fram.writeObject(86, Lens3FL);
    fram.writeObject(90, Resistor3);
  }
  //  delay(300);
}

void storeDefaults() {
  if (LensNum == 1) {
    Lens1FL = 100.0000000000;
    Resistor1 = 220.0000000000;
    LensFL = Lens1FL;
    resistor = Resistor1;
    fram.writeObject(50, LensNum);
    fram.writeObject(54, Lens1FL);
    fram.writeObject(58, Resistor1);
  }
  else if (LensNum == 2) {
    Lens2FL = 163.0000000000;
    Resistor2 = 220.0000000000;
    LensFL = Lens2FL;
    resistor = Resistor2;
    fram.writeObject(50, LensNum);
    fram.writeObject(62, Lens2FL);
    fram.writeObject(66, Resistor2);
  }
  else if (LensNum == 3) {
    Lens3FL = 210.0000000000;
    Resistor3 = 220.0000000000;
    LensFL = Lens3FL;
    resistor = Resistor3;
    fram.writeObject(50, LensNum);
    fram.writeObject(70, Lens3FL);
    fram.writeObject(74, Resistor3);
  }
  else if (LensNum == 4) {
    Lens4FL = 290.0000000000;
    Resistor4 = 220.0000000000;
    LensFL = Lens4FL;
    resistor = Resistor4;
    fram.writeObject(50, LensNum);
    fram.writeObject(78, Lens4FL);
    fram.writeObject(82, Resistor4);
  }
  else if (LensNum == 5) {
    Lens5FL = 525.0000000000;
    Resistor5 = 220.0000000000;
    LensFL = Lens5FL;
    resistor = Resistor5;
    fram.writeObject(50, LensNum);
    fram.writeObject(86, Lens5FL);
    fram.writeObject(90, Resistor5);
  }
  //  delay(300);
}

int ReadADC() {
  if (!ads1115.conversionComplete()) {
    while (0);
  }
  adc = ads1115.getLastConversionResults();
  ads1115.startADCReading(ADS1X15_REG_CONFIG_MUX_SINGLE_3, /*continuous=*/false);
  return adc;
}

float ReadDISTANCE() {
  if (!ads1115.conversionComplete()) {
    while (0);
  }
  distance = ((((ads1115.computeVolts(ads1115.getLastConversionResults()) / resistor) * 1000) - 4) / 16) * (SensorMax - SensorMin) + 100;
  ads1115.startADCReading(ADS1X15_REG_CONFIG_MUX_SINGLE_3, /*continuous=*/false);
  return distance;
}

float autoFocus() {
  ads1115.setDataRate(RATE_ADS1115_8SPS);
  pcf.digitalWrite(enaPin, HIGH);
  while ( distance > SensorMin && distance < SensorMax && (distance > (LensFL + 0.02) || distance < (LensFL - 0.02))) { // loop till focused
    //  while ((distance > (LensFL) || distance < (LensFL))) { // loop till focused
    if (distance > LensFL) {
      pcf.digitalWrite(dirPin, LOW);
    }
    else {
      pcf.digitalWrite(dirPin, HIGH);
    }
    if (distance > (LensFL + 2) || distance < (LensFL - 2)) {
      ads1115.setDataRate(RATE_ADS1115_860SPS);
      pcf.digitalWrite(stepPin, LOW);
      pcf.digitalWrite(stepPin, HIGH);
      ReadDISTANCE();
    }
    else if (distance > (LensFL + 1) || distance < (LensFL - 1)) {
      ads1115.setDataRate(RATE_ADS1115_250SPS);
      pcf.digitalWrite(stepPin, LOW);
      pcf.digitalWrite(stepPin, HIGH);
      ReadDISTANCE();
    }
    else if (distance > float(LensFL + 0.5) || distance < float(LensFL - 0.5)) {
      ads1115.setDataRate(RATE_ADS1115_8SPS);
      pcf.digitalWrite(stepPin, LOW);
      pcf.digitalWrite(stepPin, HIGH);
      delayMicroseconds(1000);
      ReadDISTANCE();
    }
    else if (distance > LensFL || distance < LensFL) {
      ads1115.setDataRate(RATE_ADS1115_8SPS);
      pcf.digitalWrite(stepPin, LOW);
      pcf.digitalWrite(stepPin, HIGH);
      delayMicroseconds(10000);
      ReadDISTANCE();
    }
  }
  pcf.digitalWrite(enaPin, LOW);
  beep();
  return distance;
}

float autoFocusHold() {
  ads1115.setDataRate(RATE_ADS1115_8SPS);
  pcf.digitalWrite(enaPin, HIGH);
  while ( distance > SensorMin && distance < SensorMax && !digitalRead(7) && (distance > (LensFL + 0.02) || distance < (LensFL - 0.02))) { // loop till focused
    //  while (!digitalRead(1) && (distance > (LensFL) || distance < (LensFL))) { // loop till focused
    if (distance > LensFL) {
      pcf.digitalWrite(dirPin, LOW);
    }
    else {
      pcf.digitalWrite(dirPin, HIGH);
    }
    if (distance > (LensFL + 2) || distance < (LensFL - 2)) {
      ads1115.setDataRate(RATE_ADS1115_860SPS);
      pcf.digitalWrite(stepPin, LOW);
      pcf.digitalWrite(stepPin, HIGH);
      ReadDISTANCE();
    }
    else if (distance > (LensFL + 1) || distance < (LensFL - 1)) {
      ads1115.setDataRate(RATE_ADS1115_250SPS);
      pcf.digitalWrite(stepPin, LOW);
      pcf.digitalWrite(stepPin, HIGH);
      ReadDISTANCE();
    }
    else if (distance > (LensFL + 0.1) || distance < (LensFL - 0.1)) {
      ads1115.setDataRate(RATE_ADS1115_8SPS);
      pcf.digitalWrite(stepPin, LOW);
      pcf.digitalWrite(stepPin, HIGH);
      delayMicroseconds(1000);
      ReadDISTANCE();
    }
    else if (distance > LensFL || distance < LensFL) {
      ads1115.setDataRate(RATE_ADS1115_8SPS);
      pcf.digitalWrite(stepPin, LOW);
      pcf.digitalWrite(stepPin, HIGH);
      delayMicroseconds(10000);
      ReadDISTANCE();
    }
  }
  pcf.digitalWrite(enaPin, LOW);
  beep();
  return distance;
}

void storeDefaultsFirstRun() {
  if (LensNum < 1) {
    LensNum = 1;
    Lens1FL = 100.0000000000;
    Resistor1 = 220.0000000000;
    LensFL = Lens1FL;
    resistor = Resistor1;
    fram.writeObject(50, LensNum);
    fram.writeObject(54, Lens1FL);
    fram.writeObject(58, Resistor1);
    Lens2FL = 163.0000000000;
    Resistor2 = 220.0000000000;
    LensFL = Lens2FL;
    resistor = Resistor2;
    fram.writeObject(62, Lens2FL);
    fram.writeObject(66, Resistor2);
    Lens3FL = 210.0000000000;
    Resistor3 = 220.0000000000;
    LensFL = Lens3FL;
    resistor = Resistor3;
    fram.writeObject(70, Lens3FL);
    fram.writeObject(74, Resistor3);

    Lens4FL = 290.0000000000;
    Resistor4 = 220.0000000000;
    LensFL = Lens4FL;
    resistor = Resistor4;
    fram.writeObject(78, Lens4FL);
    fram.writeObject(82, Resistor4);

    Lens5FL = 525.0000000000;
    Resistor5 = 220.0000000000;
    LensFL = Lens5FL;
    resistor = Resistor5;
    fram.writeObject(85, Lens5FL);
    fram.writeObject(90, Resistor5);
  
  }
}
void DisplayKeys() {
  // display key grid
  int k = 1;
  display.setCursor(8, 32 + ((k - 1) / 3) * 8);
  display.print("UP");
  k++;
  display.setCursor(35, 32 + ((k - 1) / 3) * 8);
  display.print("CALIBRATE");
  k++;
  display.setCursor(98, 32 + ((k - 1) / 3) * 8);
  display.print("SetFL");
  k++;
  display.setCursor(2, 32 + ((k - 1) / 3) * 8);
  display.print("DOWN");
  k++;
  display.setCursor(42, 32 + ((k - 1) / 3) * 8);
  display.print("DEFAULT");
  k++;
  display.setCursor(100, 32 + ((k - 1) / 3) * 8);
  display.print("SAVE");
  k++;
  display.setCursor(52, 32 + ((k - 1) / 3) * 8);
  display.print("COPY");
  k++;
  display.setCursor(98, 32 + ((k - 1) / 3) * 8);
  display.print("PASTE");
  k++;
  display.setCursor(0, 32 + ((k - 1) / 3) * 8);
  display.print("FOCUS");
  k++;
  display.setCursor(3, 32 + ((k - 1) / 3) * 8);
  display.print("AUTO");
  k++;
  display.setCursor(49, 32 + ((k - 1) / 3) * 8);
  display.print("FRAME");
  k++;
  display.setCursor(97, 32 + ((k - 1) / 3) * 8);
  display.print("START");
  k++;
}
