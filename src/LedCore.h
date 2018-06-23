//Rene Schouten
//project POV-povGlobe
//finalized at 6/20/2018

//edits:

//much variables used in this file are declared in jpegCore.h
#include <Arduino.h>
#include <SPI.h>
//pin of the hall sensor
#define interruptPin 25
//if defined simulate pin 26 will trigger pin 25(interupt pin) to simulate rotations
//#define SIMULATE

//time last interupt occures
uint32_t lastInteruptTime=0;
//the time of the last round
uint32_t totalRoundTime;
//the totalRoundTime/PARTS
uint32_t partTime;

//second spi class for the second ledstrip
SPIClass SPI2(HSPI);

//interupt handling
portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;
void IRAM_ATTR handleInterrupt() {
  portENTER_CRITICAL_ISR(&mux);
  if(micros()-lastInteruptTime>30000)//debounce (now the speed is limited to 33rps!)
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
  SPI.beginTransaction(SPISettings(20000000, MSBFIRST, SPI_MODE0));//the esp32 support 40mhz clock, but cause strange colors
  SPI2.beginTransaction(SPISettings(20000000, MSBFIRST, SPI_MODE0));
  pinMode(interruptPin, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(interruptPin), handleInterrupt, FALLING);

  #ifdef SIMULATE
  pinMode(26, OUTPUT);
  digitalWrite(26,HIGH);
  #endif

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
  //note that the SPI library is changed(it don't wait for all data is sended). this make it 2 times faster
  //SPI.write16x32() reads only the first 16 32bits of a memory pointer
  //startframe
  SPI.write32(0x00000000);
  SPI2.write32(0x00000000);
  uint32_t *imagePointer=ledData+((int)rotationPart%16)*16;
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

  //end frame
  SPI.write32(0xFFFFFFFF);
  SPI2.write32(0xFFFFFFFF);
  SPI.write32(0xFFFFFFFF);
  SPI2.write32(0xFFFFFFFF);
#ifdef SIMULATE
  if(rotationPart==1)
  {
      Serial.println("#");
  }
  Serial.print("[");
  imagePointer=ledData+(part%16)*16;
  for(int mcu=0;mcu<8;mcu++)
  {
    imagePointer2=imagePointer+16*16*mcu;
    for(int i2=0;i2<16;i2++)
    {
      Serial.print((*imagePointer2++)km+0x01000000,HEX);
      Serial.print(" ");
    }
  }
  Serial.println("]");
#endif

}
#ifdef SIMULATE
  long lastTimeSimulate=0;
#endif
void ledControlLoop(void * pvParameters)
{
  while(true){
    uint32_t actualRoundTime=micros()-lastInteruptTime;
    int actualPart;
    if(partTime!=0)
    {
      //on which part are we now
      actualPart= actualRoundTime/partTime;
    }
    if(actualPart>=PARTS)
    {
      //prevent overflows
      actualPart=PARTS-1;
    }
    //if we are in a new part
    if(part!=actualPart)
    {
      part=actualPart;
      //add rotation so we not get a relative orientation
      rotationPart=part+rotation;
      if(rotationPart>255)
      {
        rotationPart-=256;
      }
      if( rotationPartOld>rotationPart)
      {
        jpegImagePart=0;
      }
      rotationPartOld=rotationPart;
      //initialize a new jpeg, for optimalisation we do this directly when the last jpeg mcu is decoded
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
      //if we need to decode the next mcu row(16 pixels)
      if(rotationPart/16>=jpegImagePart)
      {
        uint32_t *switchPointer=ledBuffer;//switch the pointer to the buffer with the pointer to the active view
        ledBuffer=ledData;
        ledData=switchPointer;
        needNewJPEG=true;
        jpegImagePart++;
        //Serial.println("need new");
      }
      //Serial.println(part);
      //Serial.print(" ");
      showLeds();
    }
    #ifdef SIMULATE
      if(millis()-lastTimeSimulate>3000)
      {
        lastTimeSimulate=millis();
        digitalWrite(26,LOW);
        delayMicroseconds(10);
        digitalWrite(26,HIGH);
      }
    #endif
  }
}
