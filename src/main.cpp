#define LEDS_PER_SIDE 64
#define SIDES 2
#define LEDS 128
#define PARTS 256


#include <Arduino.h>
#include "JpegCore.h"
#include "LedCore.h"
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"
void setup()
{
  Serial.begin(2000000);
  //WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);
  ledControlSetup();
  jpegSetup();
  xTaskCreatePinnedToCore(
                    jpegLoop,
                    "jpeg loop",
                    40000,
                    NULL,
                    100,
                    NULL,
                    0);
  xTaskCreatePinnedToCore(
                    ledControlLoop/*ledCoreSimulation*/,
                    "led control",
                    40000,
                    NULL,
                    100,
                    NULL,
                    1);
}

void loop()
{
  delay(10000);
}
