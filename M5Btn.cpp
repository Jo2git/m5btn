#include "M5Btn.h"

#define NumHwButtons 3 // A, B, C

// Parameter scheinen M5Stack-gerätespezifisch zu sein (wie schnell Tastendrücken erkannt wird)
#define SHORT 300   // Zeit in ms, bis zu der ein Kurzdruck erkannt wird
#define LONG  500   // ZZeit in ms, ab der ein Langdruck erkannt wird

// x-Achsen der drei Hardwarebuttons A, B, C
#define xA TFT_W/4-12
#define xB TFT_W/2
#define xC TFT_W/4*3+12

// Höhe jeder Buttonbeschriftungen
#define btnHeight 20

// Textgröße Buttonbeschriftungen
#define textSize 2

// Abstand der Buttonbeschriftungen untereinander horizontal (x) und vertikal (y)
#define btnDisty 2

// y-Line der ersten (yBtn1, Kurzclick) und zweiten (yBtn2, Langclick) Buttonbar
#define yBtn1 TFT_H-1-btnDisty-btnHeight/2
#define yBtn2 yBtn1-btnHeight-btnDisty

String M5Btn::functions[NumButtons]; // NumButtons = logische Buttons, auch Kombinationen
TFT_eSPI* M5Btn::tft;
bool M5Btn::buttonBarActive = false;


extern void buttonPressed(M5Btn::ButtonType btn);

bool wasPressed[NumHwButtons];
bool wasReleased[NumHwButtons];
bool isPressed[NumHwButtons];
bool longNotified[NumHwButtons];
long lastReleased[NumHwButtons];
long lastPressed[NumHwButtons];
bool doublePress = false;

// ----------------------------------------------------------------------------------------------------
// 

void M5Btn::begin(TFT_eSPI* tft) {
    M5Btn::tft = tft;
}

// ----------------------------------------------------------------------------------------------------
//

void M5Btn::setFunction(ButtonType button, String function) {
    if (button >= NumButtons) return;
    functions[button] = function;
}

String M5Btn::getFunction(ButtonType button) {
  if (button < NumButtons) return functions[button];
  return ""; // Drehknopf etc.
}

void M5Btn::clearFunctions() {
  for (int i=0; i<NumButtons; i++) functions[i] = "";
  // Bereich löschen
  tft->fillRect(0, yBtn2 - btnHeight/2, TFT_W, TFT_H-(yBtn2) - 1, screenBgColor);
}

// ----------------------------------------------------------------------------------------------------
//
// Alternative: via FastLED:
// http://community.m5stack.com/topic/665/unexpected-behavior-on-m5stack-fire-leds/2

void M5Btn::led(int ledIndex, int red, int green, int blue) {
	Wire.beginTransmission(Faces_Encoder_I2C_ADDR);
	Wire.write(ledIndex);
	Wire.write(red);
	Wire.write(green);
	Wire.write(blue);
	Wire.endTransmission();
}

// ----------------------------------------------------------------------------------------------------
// 

void M5Btn::ledRing(int red, int green, int blue, int delay_ms) {
	for(int ledIndex=0;ledIndex<12;ledIndex++) {
        led(ledIndex, red, green, blue);
		delay(delay_ms);
    }
}

// ----------------------------------------------------------------------------------------------------
//

void M5Btn::activateButton(ButtonType btn, int width, int x, int y) {
    if (functions[btn] == 0) return;
    tft->fillRoundRect(x - width/2, y-btnHeight/2, width, btnHeight, 4, btnBGColor);
    tft->drawString(functions[btn], x, y, textSize);
}

void M5Btn::setButtonBarActive(bool active) {

    M5Btn::buttonBarActive = active;

    bool doubleButtonsExist = true;
    for (int i=6; i<NumButtons-1; i++)
        doubleButtonsExist = doubleButtonsExist && (functions[i] != 0);

    int btnWidth = doubleButtonsExist ? TFT_W/8 : TFT_W/4;

    tft->setTextColor(btnFGColor, btnBGColor);
    tft->setTextDatum(CC_DATUM);

    // untere Reihe: kurzer Druck
    activateButton(ButtonType::A, btnWidth, xA, yBtn1);
    activateButton(ButtonType::B, btnWidth, xB, yBtn1);
    activateButton(ButtonType::C, btnWidth, xC, yBtn1);

    // obere Reihe: langer Druck
    activateButton(ButtonType::AA, btnWidth, xA, yBtn2);
    activateButton(ButtonType::BB, btnWidth, xB, yBtn2);
    activateButton(ButtonType::CC, btnWidth, xC, yBtn2);

}

// ----------------------------------------------------------------------------------------------------
// Wichtig: Enumreihenfolge wird angenommen!

bool doubleButton(int first, int second) {
    switch ((M5Btn::ButtonType)first) {
        case M5Btn::ButtonType::A: buttonPressed(static_cast<M5Btn::ButtonType>(5+second)); break;
        case M5Btn::ButtonType::B: buttonPressed(static_cast<M5Btn::ButtonType>(second == M5Btn::ButtonType::A ? M5Btn::ButtonType::BA : M5Btn::ButtonType::BC)); break;
        case M5Btn::ButtonType::C: buttonPressed(static_cast<M5Btn::ButtonType>(10+second)); break;
    }
}

// ----------------------------------------------------------------------------------------------------
// Prüft, ob andere als Button <pressedButtonIndex> (0..NumHwButtons-1) gedrückt wurden und liefert dann
// false, sonst true

bool checkOthers(int pressedButtonIndex) {
    int prev = (pressedButtonIndex -1 ) % NumHwButtons; if (prev < 0) prev = NumHwButtons - 1;
    int next = (pressedButtonIndex + 1) % NumHwButtons;

    if (!isPressed[prev] && !isPressed[next]) return true;

    if (isPressed[prev] && isPressed[next]) return false; // ignoriere gleichzeitiges ABC

    if (isPressed[prev]) { 
        doubleButton(prev, pressedButtonIndex);
        longNotified[prev] = true; 
        doublePress = true;
        return false; 
    }
    if (isPressed[next]) { 
        doubleButton(next, pressedButtonIndex);
        longNotified[next] = true;
        doublePress = true;
        return false; 
    }

}

// ----------------------------------------------------------------------------------------------------
//

void M5Btn::loop() {

    M5.update(); // Buttonzustand einlesen

    // Zustände speichern (weil z.B. M5.BtnA nur einmal wirkt und das Event dann verbraucht ist)
    wasPressed [0] = M5.BtnA.wasPressed();  wasPressed [1] = M5.BtnB.wasPressed();  wasPressed [2] = M5.BtnC.wasPressed();
    wasReleased[0] = M5.BtnA.wasReleased(); wasReleased[1] = M5.BtnB.wasReleased(); wasReleased[2] = M5.BtnC.wasReleased();
    isPressed  [0] = M5.BtnA.isPressed();   isPressed  [1] = M5.BtnB.isPressed();   isPressed  [2] = M5.BtnC.isPressed();

    for (int i=0; i<NumHwButtons; i++) {
    
        if (wasReleased[i]) {
            lastReleased[i] = millis();
        }

        if (wasPressed[i]) {
            lastPressed[i] = millis();
            longNotified[i] = false;
        }

        /* SHORT?
        <-----> kleiner SHORT
        |~~~~~|
     ---|     |-----
              |
              v Event: short pressed
        */
        if (wasReleased[i]) {
            if (millis() - lastPressed[i] <= SHORT)
                if (checkOthers(i)) {
                    if (!doublePress) buttonPressed((ButtonType)i); else doublePress = false;
                }
            wasReleased[i] = false;
        }

        /* LONG?
        <------------> größer SHORT 
        |                  | < gleich LONG
        |~~~~~~~~~~~~~~~~~~|
     ---|             |    |------- 
                        v Event: long pressed
        */
        if (isPressed[i] && millis() - lastPressed[i] >= LONG && !longNotified[i]) {
            buttonPressed((ButtonType)(3 + i));
            longNotified[i] = true;
        }
    }

    encoderLoop();

}

// ----------------------------------------------------------------------------------------------------
//

// Drehreglerknopf
int buttonState = HIGH;
int oldButtonState = buttonState;
boolean _buttonDown = false;
long rotaryKnobLastPressed;
bool rotaryKnobLongNotified = false;

// Drehreglerstellung
int _min = 0;
int _max = 0;
int _incr = 1;
int _value = 0;
int _oldValue = _value;
int _direction = 0;

// Drehgeschwindigkeit
long lastTurned = 0;
int _fast;
int _veryFast;

void M5Btn::encoderLoop() {
    
  int tmp_increment, increment;
  boolean turnedFast, turnedVeryFast;

  Wire.requestFrom(Faces_Encoder_I2C_ADDR, 3);
  if (Wire.available()) {
    tmp_increment = Wire.read();
    buttonState = Wire.read();
  } else {
      return;
  }

  if (tmp_increment > 127) { // nach links gedreht
    _direction = 1;
    increment = -_incr;
    buttonPressed(M5Btn::RotaryLeft);
  } else {
    _direction = 0;
    increment = tmp_increment * _incr; // 0: nicht gedreht, 1: rechts gedreht
    if (increment) buttonPressed(M5Btn::RotaryRight);
  }

  turnedFast = turnedVeryFast = false;
  if (increment != 0) { // wurde gedreht
    if (millis() - lastTurned < _veryFast) {
      turnedVeryFast = true;
    } else if (millis() - lastTurned < _fast) {
      turnedFast = true;
    }
    lastTurned = millis();
  }

  if (turnedVeryFast) increment = increment * 10;
  if (turnedFast) increment = increment * 5;

  _value += increment;

  if (turnedVeryFast) _value = _value - _value % 10;
  if (turnedFast) _value = _value - _value % 5;

  if (_value < _min) _value = _min;
  if (_value > _max) _value = _max;

  // Button gedrückt?
  bool wasReleased = buttonState == HIGH && oldButtonState == LOW; 
  bool wasPressed =  buttonState == LOW  && oldButtonState == HIGH;
  bool isPressed =   buttonState == LOW;
  oldButtonState =   buttonState;

  if (wasPressed) {
    rotaryKnobLastPressed = millis();
    rotaryKnobLongNotified = false;
  }
  
  // Drehknopf kurz gedrückt?
  if (wasReleased) {
    if (millis() - rotaryKnobLastPressed <= SHORT) {
        buttonPressed(ButtonType::RotaryKnob);  
    }
  }

  // Drehknopf lang gedrückt?
  if (isPressed && (millis() - rotaryKnobLastPressed) >= LONG && !rotaryKnobLongNotified) {
    buttonPressed(ButtonType::RotaryKnobLong);
    rotaryKnobLongNotified = true;
  }

}
