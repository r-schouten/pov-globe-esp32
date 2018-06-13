#include <Arduino.h>
#include <SPI.h>
#define interruptPin 25

uint32_t lastInteruptTime=0;
uint32_t totalRoundTime;
uint32_t partTime;


SPIClass SPI2(HSPI);
portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;
void IRAM_ATTR handleInterrupt() {
  portENTER_CRITICAL_ISR(&mux);
  if(micros()-lastInteruptTime>40000)
  {
    totalRoundTime=micros()-lastInteruptTime;
    partTime=totalRoundTime/PARTS;
    lastInteruptTime=micros();
  }
  portEXIT_CRITICAL_ISR(&mux);
}
void ledControlSetup()
{
  Serial.println("povglobe starting");
  SPI.begin(14,12,13,15);
  SPI2.begin(18,19,23,5);
  SPI.beginTransaction(SPISettings(20000000, MSBFIRST, SPI_MODE0));
  SPI2.beginTransaction(SPISettings(20000000, MSBFIRST, SPI_MODE0));
  pinMode(interruptPin, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(interruptPin), handleInterrupt, FALLING);
  uint32_t ledsOff[16];
  for(int i=0;i<16;i++)
  {
    ledsOff[i]=0x000000FF;
  }
  SPI.write32(0x00000000);
  SPI2.write32(0x00000000);
  SPI.write16x32(ledsOff);
  SPI2.write16x32(ledsOff);
  SPI.write16x32(ledsOff);
  SPI2.write16x32(ledsOff);
  SPI.write16x32(ledsOff);
  SPI2.write16x32(ledsOff);
  SPI.write16x32(ledsOff);
  SPI2.write16x32(ledsOff);
  SPI.write32(0xFFFFFFFF);
  SPI2.write32(0xFFFFFFFF);
  SPI.write32(0xFFFFFFFF);
  SPI2.write32(0xFFFFFFFF);

}
void showLeds()
{
  SPI.write32(0x00000000);
  SPI2.write32(0x00000000);
  /*SPI.write16x32(ledDataDepricated[part]);
  SPI.write16x32(ledDataDepricated[part]+16);
  SPI.write16x32(ledDataDepricated[part]+32);
  SPI.write16x32(ledDataDepricated[part]+48);*/
  uint32_t *imagePointer=ledData+(rotationPart%16)*16;
  uint32_t *imagePointer2=imagePointer;
  SPI.write16x32(imagePointer2);
  imagePointer2=imagePointer+16*16*4;
  SPI2.write16x32(imagePointer2);

  imagePointer2=imagePointer+16*16;
  SPI.write16x32(imagePointer2);
  imagePointer2=imagePointer+16*16*5;
  SPI2.write16x32(imagePointer2);

  imagePointer2=imagePointer+16*16*2;
  SPI.write16x32(imagePointer2);
  imagePointer2=imagePointer+16*16*6;
  SPI2.write16x32(imagePointer2);

  imagePointer2=imagePointer+16*16*3;
  SPI.write16x32(imagePointer2);
  imagePointer2=imagePointer+16*16*7;
  SPI2.write16x32(imagePointer2);

  SPI.write32(0xFFFFFFFF);
  SPI2.write32(0xFFFFFFFF);
  SPI.write32(0xFFFFFFFF);
  SPI2.write32(0xFFFFFFFF);
  imagePointer=ledData+(part%16)*16;
  /*for(int mcu=0;mcu<8;mcu++)
  {
    imagePointer2=imagePointer+16*16*mcu;
    for(int i2=0;i2<16;i2++)
    {
      Serial.print(*imagePointer2++,HEX);
      Serial.print(" ");
    }
  }
  Serial.println();*/

}

void ledControlLoop(void * pvParameters)
{
  while(true){
    uint32_t actualRoundTime=micros()-lastInteruptTime;
    int actualPart;
    if(partTime!=0)
    {
      actualPart= actualRoundTime/partTime;
    }
    if(actualPart>=PARTS)
    {
      actualPart=PARTS-1;
    }

    if(part!=actualPart)
    {
      part=actualPart;
      rotationPart=part+rotation;
      if(rotationPart>255)
      {
        rotationPart-=256;
      }
      if(rotationPart>250)
      {
        if(jpegInitialized2)
        {
          jpegInitialized=false;
          jpegInitialized2=false;
        }
      }
      else{
        jpegInitialized2=true;
      }
      if(rotationPart/16>=jpegImagePart)
      {
        uint32_t *switchPointer=ledBuffer;//switch the pointer to the buffer with the pointer to the active view
        ledBuffer=ledData;
        ledData=switchPointer;
        needNewJPEG=true;
        jpegImagePart++;
        //Serial.println("need new");
      }
      //Serial.print(part);
      //Serial.print(" ");
      showLeds();
    }
  }
}
