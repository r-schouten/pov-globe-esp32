//Rene Schouten
//project POV-povGlobe
//finalized at 6/20/2018

//edits:


#include <WiFi.h>
#include <WiFiUdp.h>
#include <SPI.h>
#include "JPEGDecoder.h"

const char* ssid = "povGlobe";
const char* password = "povGlobe";

WiFiUDP Udp;
unsigned int localUdpPort = 4210;
#define IMAGE_BUFFER_LENGTH 15000
uint8_t unDecodedImage1[IMAGE_BUFFER_LENGTH];
uint8_t unDecodedImage2[IMAGE_BUFFER_LENGTH];

byte* incomingPacket;
uint8_t* unDecodedImageIn=unDecodedImage1;
uint8_t* unDecodedImageOut=unDecodedImage2;
int imageLength=0;
int finalImageLength=0;
bool newImageReady=false;

int part=0;//where are we now
int lastPart=10;
double rotationPart=0;//where are we now relative to rotation
double rotationPartOld=0;
double rotation=0;
double rotationFactor=0;//add to rotation each mcu
//the two led buffers
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
void initializeJPEG()
{
  //before reading the first mcu  the header of the jpeg should be initialized
  JpegDec.decodeArray(unDecodedImageOut,finalImageLength);
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
}
void decodeRow()
{
   // read a row of the jpeg(8 mcu's). it will be placed in the ledBuffer
   //note that the image is rotated 90 degrees because jpeg is encoded from left to right(we need top to bottom)
     uint32_t *imagePointer=ledBuffer;
     for(int mcuRow=0;mcuRow<8;mcuRow++)
     {
       bool status = JpegDec.read(imagePointer);
       //if(!status)Serial.println("end of image");
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
  //upd.myRead is a custom method which is much more coefficients
  //myRead places a packet at the given pointer address
  int packetSize = Udp.myRead(incomingPacket);
  if (packetSize){//when a packet is received
      //Serial.print("|");
      if(incomingPacket[packetSize-1]==2)//if type is 2 (jpeg image data)
      {
        uint8_t resetNext=incomingPacket[packetSize-2];//is this the last package?
        //Serial.print("new package: ");
        //Serial.println(packetSize);
        imageLength+=packetSize-5;//place the pointer to the next position so the next packege will be placed behind this packege. -5 to overwrite the header
        if(newImageReady)
        {
          imageLength=0;
        }
        incomingPacket=unDecodedImageIn+imageLength;
        if(imageLength>IMAGE_BUFFER_LENGTH)
        {
          imageLength=0;//prevent overflow
          //Serial.println("to big image or protocol error");
        }
        if(resetNext)
        {
          //Serial.println("reset next");
          /*for(int i=0;i<imageLength;i++)
          {
            Serial.print(unDecodedImage[i],HEX);
          }
          Serial.println();*/
          finalImageLength=imageLength;
          imageLength=0;
          incomingPacket=unDecodedImageOut;
          newImageReady=true;
        }
      }
      else if(incomingPacket[packetSize-1]==1)//if type  is 1 (adjustment setting packet)
      {
        rotationFactor=(incomingPacket[1]-128.0)/200.0;
        JpegDec.brighness=(223+incomingPacket[2]);
        if(JpegDec.gamma!=incomingPacket[3])
        {
          JpegDec.gamma=incomingPacket[3];
          JpegDec.calculateGamma();
        }
      }
  }
}
IPAddress ip(192,168,137,100);//static ip
IPAddress gateway(192,168,137,1);
IPAddress subnet(255, 255, 255, 0);
void jpegSetup()
{
  //static ip
  if (!WiFi.config(ip, gateway, subnet)) {
    Serial.println("STA Failed to configure");
  }
  Serial.printf("Connecting to %s ", ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED){
    delay(200);
    Serial.print(".");
  }
  Serial.println(" connected");
  Udp.begin(localUdpPort);
   Serial.printf("Now listening at IP %s, UDP port %d\n", WiFi.localIP().toString().c_str(), localUdpPort);
   incomingPacket=unDecodedImageIn;
   JpegDec.gamma=128;
   JpegDec.calculateGamma();
   memset(ledData1,0,16*128*4);
   memset(ledData2,0,16*128*4);
}

void jpegLoop(void * pvParameters)
{
  while(true)
  {
    receivePacket();
    if(!jpegInitialized)//if ledtask ask for intializing the jpeg
    {
      jpegInitialized=true;
      if(newImageReady)//if we received a new image
      {
        newImageReady=false;
        uint8_t *temp=unDecodedImageIn;
        unDecodedImageIn=unDecodedImageOut;
        unDecodedImageOut=temp;
      }
      JpegDec.abort();//stop reading the image, also when the image reading was not completed
      //Serial.println("intialize jpeg");
      //uint32_t startTime3=micros();
      initializeJPEG();//intitialize
      //Serial.println(micros()-startTime3);
      //needNewJPEG=false;
    }
    if(needNewJPEG){//if ledtask ask for a new mcu row
      needNewJPEG=false;
      //Serial.print("decode ");
      //Serial.println(jpegImagePart);
      //uint32_t startTime2=micros();
      decodeRow();//decode the row of mcu's
      rotation+=rotationFactor;//add a bit to the rotation
      if(rotation>255)
      {
        rotation-=256;
      }
      if(rotation<0)
      {
        rotation+=256;
      }
      //Serial.println(micros()-startTime2);

    }
    vTaskDelay(1);//prevents error messages of freertos
  }
}
