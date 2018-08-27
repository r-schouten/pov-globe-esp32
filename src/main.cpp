//Rene Schouten
//project POV-povGlobe
//finalized at 6/20/2018
//for more information see software reference manual
//before editing this software read the manuel, the esp32 reference manual, the apa102 datasheet and the jpeg standaard

//edits:

// todo:
// wat doen die
#include <Arduino.h>
#include "JpegCore.h"
#include "LedCore.h"
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"

#define LEDS_PER_SIDE 64
#define SIDES 2
#define LEDS 128
#define PARTS 256

void setup()
{
  Serial.begin(2000000);
  //the esp32 has 2 cores, core 0 for decoding and receiving jpeg and core 1 is for controlling the leds and system processes
  allocateMemory();
  ledControlSetup();
  jpegSetup();
  xTaskCreatePinnedToCore(
                    jpegLoop,
                    "jpeg loop",
                    5000,
                    NULL,
                    100,
                    NULL,
                    0);
  xTaskCreatePinnedToCore(
                    ledControlLoop,
                    "led control",
                    5000,
                    NULL,
                    100,
                    NULL,
                    1);
}

void loop()
{
  delay(10000);
}
