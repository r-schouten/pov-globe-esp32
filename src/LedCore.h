//Rene Schouten
//project POV-povGlobe
//finalized at 6/20/2018

//edits:

//much variables used in this file are declared in jpegCore.h
#include <Arduino.h>
#include "FastSPI.h"
//pin of the hall sensor
#define interruptPin 25
#define PARTS 256
//if defined simulate pin 26 will trigger pin 25(interupt pin) to simulate rotations
//#define SIMULATE

//time last interupt occures
uint32_t lastInteruptTime=0;
//the time of the last round
double totalRoundTime;
//the totalRoundTime/PARTS
double partTime;
//for directly starting with a new partTime
bool stopLeds=false;
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
    stopLeds=true;
  }
  portEXIT_CRITICAL_ISR(&mux);
}

void endFrame()
{
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
  if(stopLeds){
    return;
  }
  SPI.write32(0x00000000);
  SPI2.write32(0x00000000);
  uint32_t *imagePointer=ledData+((int)rotationPart%16)*16;
  uint32_t *imagePointer2=imagePointer;
  SPI.write16x32(imagePointer2);
  imagePointer2=imagePointer+16*16*4;
  SPI2.write16x32(imagePointer2);
  if(stopLeds){
    endFrame();
    return;
  }

  imagePointer2=imagePointer+16*16;
  SPI.write16x32(imagePointer2);
  imagePointer2=imagePointer+16*16*5;
  SPI2.write16x32(imagePointer2);
  if(stopLeds){
    endFrame();
    return;
  }

  imagePointer2=imagePointer+16*16*2;
  SPI.write16x32(imagePointer2);
  imagePointer2=imagePointer+16*16*6;
  SPI2.write16x32(imagePointer2);

  if(stopLeds){
    endFrame();
    return;
  }
  imagePointer2=imagePointer+16*16*3;
  SPI.write16x32(imagePointer2);
  imagePointer2=imagePointer+16*16*7;
  SPI2.write16x32(imagePointer2);

  //end frame
  endFrame();
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
void memset32(uint32_t* destination,uint32_t data,int length)
{
  for(int i=0;i<length;i++)
  {
    destination[i]=data;
  }
}
void ledControlSetup()
{
  Serial.printf("povglobe starting\n");
  SPI.begin(14,12,13,15);
  SPI2.begin(18,19,23,5);
  SPI.beginTransaction(SPISettings(20000000, MSBFIRST, SPI_MODE0));//the esp32 support 40mhz clock, but cause strange colors
  SPI2.beginTransaction(SPISettings(20000000, MSBFIRST, SPI_MODE0));
  pinMode(interruptPin, INPUT);
  attachInterrupt(digitalPinToInterrupt(interruptPin), handleInterrupt, FALLING);

  #ifdef SIMULATE
  pinMode(26, OUTPUT);
  digitalWrite(26,HIGH);
  #endif

    memset32(ledData1,0x000000FF,16*128);
    memset32(ledData2,0x000000FF,16*128);
  showLeds();

  uint32_t wifiStartTime=millis();
  Serial.printf("Connecting to %s ", ssid);
  WiFi.begin(ssid, password);

  int ledIndex=0;
  int ledDirection=-1;
  while (WiFi.status() != WL_CONNECTED){
    delay(20);
    if((ledIndex==63)||(ledIndex==0))ledDirection*=-1;;
    ledIndex+=ledDirection;
    for(int i=0;i<64;i++)
    {
      if(i==ledIndex)
      {
        ledData[(i%16)+(16*16*(i/16))]=0x000099FF;
        ledData[(i%16)+(16*16*(i/16))+16*16*4]=0x000099FF;
      }
      else
      {
        ledData[(i%16)+(16*16*(i/16))]=0x000000FF;
        ledData[(i%16)+(16*16*(i/16))+16*16*4]=0x000000FF;
      }
    }
      showLeds();
    Serial.print(".");
    if(millis()-wifiStartTime>15000)
    {
      Serial.printf("\nesp will restart because wifi is not connecting\n");
      delay(100);
      ESP.restart();
    }
  }
  Serial.printf(" connected\n");
  //static ip
  IPAddress ip(192,168,137,100);
  IPAddress gateway(192,168,137,1);
  IPAddress subnet(255, 255, 255, 0);
  if (!WiFi.config(ip, gateway, subnet)) {
    Serial.printf("STA Failed to configure\n");
  }
  if(!Udp.begin(localUdpPort))
  {
    Serial.printf("error starting udp\n");
  }

   Serial.printf("Now listening at IP %s, UDP port %d\n", WiFi.localIP().toString().c_str(), localUdpPort);

   //green animation
   memset32(ledData1,0x000000FF,16*128);
   for(int i=64;i>=-10;i--)
   {
     showLeds();
     delay(10);
     for(int i2=0;i2<64;i2++)
     {
       if((i<=i2)&&(abs(i-i2)<15))
       {
         ledData1[(i2%16)+(16*16*(i2/16))]=0x009900FF;
         ledData1[(i2%16)+(16*16*(i2/16))+16*16*4]=0x009900FF;
         ledData2[(i2%16)+(16*16*(i2/16))]=0x009900FF;
         ledData2[(i2%16)+(16*16*(i2/16))+16*16*4]=0x009900FF;
       }
       else
       {
         ledData1[(i2%16)+(16*16*(i2/16))]=0x000000FF;
         ledData1[(i2%16)+(16*16*(i2/16))+16*16*4]=0x000000FF;
         ledData2[(i2%16)+(16*16*(i2/16))]=0x000000FF;
         ledData2[(i2%16)+(16*16*(i2/16))+16*16*4]=0x000000FF;
       }
     }
   }
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
      stopLeds=false;
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
