#pragma once

#include <Arduino.h>

namespace HardwareConfiguration
{
    // HUB75 panel geometry (single panel; chain >1 if you daisy-chain panels)
    static const uint16_t DISPLAY_PANEL_WIDTH = 128;
    static const uint16_t DISPLAY_PANEL_HEIGHT = 64;
    static const uint8_t DISPLAY_PANEL_CHAIN = 1;

    // Derived total matrix dimensions used by the renderer
    static const uint16_t DISPLAY_MATRIX_WIDTH = DISPLAY_PANEL_WIDTH * DISPLAY_PANEL_CHAIN;
    static const uint16_t DISPLAY_MATRIX_HEIGHT = DISPLAY_PANEL_HEIGHT;

    // HUB75 pin map — Seengreat "RGB Matrix Adapter Board (E)" for ESP32-DevKitC, V2.x
    // Source: https://seengreat.com/wiki/186/
    // If your board is silkscreened V1.x, use the V1.x block below instead.
    static const int8_t HUB75_R1 = 18;
    static const int8_t HUB75_G1 = 17;
    static const int8_t HUB75_B1 = 19;
    static const int8_t HUB75_R2 = 21;
    static const int8_t HUB75_G2 = 23;
    static const int8_t HUB75_B2 = 27;
    static const int8_t HUB75_A = 26;
    static const int8_t HUB75_B = 16;
    static const int8_t HUB75_C = 25;
    static const int8_t HUB75_D = 4;
    static const int8_t HUB75_E = 22; // required for 1/32 scan (64-row panels)
    static const int8_t HUB75_LAT = 2;
    static const int8_t HUB75_OE = 32;
    static const int8_t HUB75_CLK = 33;

    // V1.x alternative (Seengreat ESP32-DevKitC, board silkscreen "V1.x"):
    //   R1=18 G1=25 B1=5  R2=17 G2=33 B2=16
    //   A=4   B=3   C=0   D=21  E=32
    //   LAT=19 OE=15 CLK=2

    // Set true if the panel uses an FM6126A driver chip (some cheap P2.5 panels do).
    // If the panel stays dark or shows ghosting at boot, flip this to true.
    static const bool HUB75_USES_FM6126A = false;
}
