#ifndef M5BTN_H
#define M5BTN_H

#include <M5Stack.h>
#include <Arduino.h>

#define btnBGColor TFT_BLUE
#define btnFGColor TFT_WHITE
#define screenBgColor TFT_BLACK

// scheinbar verdreht
#define TFT_W TFT_HEIGHT
#define TFT_H TFT_WIDTH

// Adresse des M5Faces-Encoders
#define Faces_Encoder_I2C_ADDR     0X5E

class M5Btn {

    #define NumButtons 11 // ohne Drehregler li/re/Knopf; diese werden getrennt behandelt

    public:

        enum ButtonType {A, B, C, AA, BB, CC, AB, AC, BA, BC, CA, CB, RotaryKnob, RotaryKnobLong, RotaryLeft, RotaryRight };

        static void clearFunctions();
        static void setFunction(ButtonType button, String function);
        static String getFunction(ButtonType button);

        static void begin(TFT_eSPI* tft);
        static void loop();

        // visible = true forciert Neuanzeige
        static void setButtonBarActive(bool visible);

    private:

        static void activateButton(ButtonType btn, int width, int x, int y);

        static TFT_eSPI* tft;
        static bool buttonBarActive;
        static String functions[NumButtons];   

        static void encoderLoop();
};

#endif