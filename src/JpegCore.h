#include <WiFi.h>
#include <WiFiUdp.h>
#include <SPI.h>
#include "JPEGDecoder.h"

const char* ssid = "povGlobe";
const char* password = "povGlobe";

WiFiUDP Udp;
unsigned int localUdpPort = 4210;
struct udpData
{
  uint8_t  imageData[14][1460];
  uint8_t  index=0;
  uint8_t  flowLabel=0;
  long StartTime;
  int imageLength=0;
}udpData;
uint8_t unDecodedImage[20000];
byte* incomingPacket;

int part=0;
int lastPart=10;
int rotationPart=0;
int rotation=0;
int rotationFactor=0;
uint32_t *ledData1 = new uint32_t[128*16];
uint32_t *ledData2 = new uint32_t[128*16];
//switching pointers
uint32_t *ledData = ledData1;
uint32_t *ledBuffer= ledData2;

bool needNewJPEG=true;

uint8_t jpegImagePart=0;
bool jpegInitialized=false;
bool jpegInitialized2=false;
bool needReconstruction=true;
void printArray(uint8_t* data)
{
   for(int i=0;i<146;i++)
   {
     Serial.print(data[i],HEX);
     Serial.print(" ");
   }Serial.println();
}
void reconstructImage()
{
long startTime=micros();
  Serial.println("reconstructing Image");
  udpData.imageLength=0;
  for(uint8_t packet=0;packet<=udpData.index;packet++)
  {
    for(uint8_t i=0;i<= udpData.index;i++)
    {
       if(udpData.imageData[i][3]==packet)
       {
          int length = (uint32_t) udpData.imageData[i][1] << 8 | (uint32_t) udpData.imageData[i][2];
          memcpy(unDecodedImage+udpData.imageLength,udpData.imageData[i]+10,length);
          //Serial.print(udpData.imageData[i][3]);
          //Serial.print("  ");
          //Serial.println(length);
         udpData.imageLength+=length;
         break;
       }
    }
  }
  //Serial.print("image copy time:");
  //Serial.println(micros()-startTime);

}
void initializeJPEG()
{
  JpegDec.decodeArray(unDecodedImage,udpData.imageLength);
   /*String header = "width:";
   header += JpegDec.width;
   header += ", height:";
   header += JpegDec.height;
   header += "\n MCUSPerRow:";
   header += JpegDec.MCUSPerRow;
   header += ", MCUSPerCol:";
   header += JpegDec.MCUSPerCol;
   header += "\n MCUWidth:";
   header += JpegDec.MCUWidth;
   header += ", MCUHeight:";
   header += JpegDec.MCUHeight;
   Serial.println(header);*/
   jpegImagePart=0;
}
void decodeRow()
{
     uint32_t *imagePointer=ledBuffer;
     for(int mcuRow=0;mcuRow<8;mcuRow++)
     {
       bool status = JpegDec.read(imagePointer);
       if(!status)Serial.println("end of image");
       imagePointer+=16*16;
     }
     /*imagePointer=ledBuffer;
       for(int i=0;i<16;i++)
       {
         uint32_t *imagePointer2=imagePointer;
         for(int i2=0;i2<16;i2++)
         {
           Serial.print(*imagePointer2++,HEX);
           Serial.print(" ");
         }imagePointer2=imagePointer+16*16;
         for(int i2=0;i2<16;i2++)
         {
           Serial.print(*imagePointer2++,HEX);
           Serial.print(" ");
         }imagePointer2=imagePointer+16*16*2;
         for(int i2=0;i2<16;i2++)
         {
           Serial.print(*imagePointer2++,HEX);
           Serial.print(" ");
         }imagePointer2=imagePointer+16*16*3;
         for(int i2=0;i2<16;i2++)
         {
           Serial.print(*imagePointer2++,HEX);
           Serial.print(" ");
         }
         imagePointer2=imagePointer+16*16*4;
         for(int i2=0;i2<16;i2++)
         {
           Serial.print(*imagePointer2++,HEX);
           Serial.print(" ");
         }
         imagePointer2=imagePointer+16*16*5;
         for(int i2=0;i2<16;i2++)
         {
           Serial.print(*imagePointer2++,HEX);
           Serial.print(" ");
         }
         imagePointer2=imagePointer+16*16*6;
         for(int i2=0;i2<16;i2++)
         {
           Serial.print(*imagePointer2++,HEX);
           Serial.print(" ");
         }
         imagePointer2=imagePointer+16*16*7;
         for(int i2=0;i2<16;i2++)
         {
           Serial.print(*imagePointer2++,HEX);
           Serial.print(" ");
         }
         imagePointer+=16;
         Serial.println();
     }*/
}
void receivePacket()
{
  int packetSize = Udp.myRead(incomingPacket);
  if (packetSize){//when a packet is received
    if(udpData.index==0)
    {
        udpData.StartTime=micros();
    }
    int length = (uint32_t) incomingPacket[1] << 8 | (uint32_t) incomingPacket[2];

    if(incomingPacket[5]!=udpData.flowLabel)
    {
      udpData.flowLabel=incomingPacket[5];
      udpData.index=0;
    }
    /*Serial.print(length);
    Serial.print("  ");
    Serial.print(udpData.index);
    Serial.print("  ");
    Serial.print(incomingPacket[3]);
    Serial.print("  ");
    Serial.println(incomingPacket[4]);*/
    if(incomingPacket[4]==udpData.index+1){
      rotationFactor=incomingPacket[6]-128;
      //Serial.printf("\nimage receiving time: %i\n",micros()-udpData.StartTime);
      long starttime=micros();
      needReconstruction=true;
      reconstructImage();
      udpData.index=0;
      incomingPacket=udpData.imageData[udpData.index];
      return;
    }
    udpData.index++;
    incomingPacket=udpData.imageData[udpData.index];

  }
}
void jpegSetup()
{

  Serial.printf("Connecting to %s ", ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED){
    delay(200);
    Serial.print(".");
  }
  Serial.println(" connected");
  Udp.begin(localUdpPort);
   Serial.printf("Now listening at IP %s, UDP port %d\n", WiFi.localIP().toString().c_str(), localUdpPort);
   incomingPacket=udpData.imageData[udpData.index];
   memset(ledData1,0,16*128*4);
   memset(ledData2,0,16*128*4);
}

void jpegLoop(void * pvParameters)
{
  while(true)
  {
    receivePacket();
    if(!jpegInitialized)
    {
      jpegInitialized=true;
      JpegDec.abort();
      initializeJPEG();
      //Serial.println("intialize jpeg");
      needNewJPEG=false;
      rotation+=rotationFactor;
      if(rotation>255)
      {
        rotation-=256;
      }
      if(rotation<0)
      {
        rotation+=256;
      }
    }
    if(needNewJPEG){
      needNewJPEG=false;
      //Serial.print("decode ");
      //Serial.println(jpegImagePart);
      decodeRow();
    }
    vTaskDelay(1);
  }
}
