//Rene Schouten
//project POV-povGlobe
//finalized at 6/20/2018

//edits:


#include <WiFi.h>
#include "FastWiFiUdp.h"
#include "FastSPI.h"
#include "JPEGDecoder.h"

const char* ssid = "povGlobe";
const char* password = "povGlobe";

FastWiFiUDP Udp;
unsigned int localUdpPort = 4210;
#define IMAGE_BUFFER_LENGTH 50000
uint8_t* unDecodedImage1;
uint8_t* unDecodedImage2;

byte* incomingPacket;
uint8_t* unDecodedImageIn=unDecodedImage1;
uint8_t* unDecodedImageOut=unDecodedImage2;
int imageLength=0;
uint8_t flowLabel=0;
int imageIndex=0;
int finalImageLength=0;
bool newImageReady=false;

int part=0;//where are we now
int lastPart=10;
double rotationPart=0;//where are we now relative to rotation
double rotationPartOld=0;
double rotation=0;
double rotationFactor=0;//add to rotation each mcu
//the two led buffers
uint32_t *ledData1;
uint32_t *ledData2;
//switching pointers
uint32_t *ledData;
uint32_t *ledBuffer;

bool needNewJPEG=true;

uint8_t jpegImagePart=0;
bool jpegInitialized=false;
bool jpegInitialized2=false;
bool needReconstruction=true;

void initializeJPEG()
{
  //before reading the first mcu  the header of the jpeg should be initialized
  JpegDec.decodeArray(unDecodedImageOut,finalImageLength);
}
void decodeRow()
{
   // read a row of the jpeg(8 mcu's). it will be placed in the ledBuffer
   //note that the image is rotated 90 degrees because jpeg is encoded from left to right(we need top to bottom)
     uint32_t *imagePointer=ledBuffer;
     for(int mcuRow=0;mcuRow<8;mcuRow++)
     {
       JpegDec.read(imagePointer);
       //if(!status)Serial.println("end of image");
       imagePointer+=16*16;
     }
}
void receivePacket()
{
  //upd.myRead is a custom method which is much more efficient
  //myRead places a packet at the given pointer address
  int packetSize = Udp.myRead(incomingPacket);
  /*int PacketSize=Udp.parsePacket();
  if(PacketSize>0)
  {
    char incomingPack[PacketSize];
    Udp.read(incomingPack,PacketSize);
    Serial.println(PacketSize);
  }*/
  if (packetSize>0){//when a packet is received
      Serial.print("|");
      if(incomingPacket[packetSize-1]==2)//if type is 2 (jpeg image data)
      {
        imageIndex++;
        if(incomingPacket[packetSize-4]!=flowLabel)
        {
          //imageIndex=1;
          //imageLength=0;
        }
        flowLabel=incomingPacket[packetSize-4];
        uint8_t imageParts=incomingPacket[packetSize-3];
        uint8_t imagePart=incomingPacket[packetSize-2];

        Serial.print("new package: ");
        Serial.println(packetSize);
        /*Serial.print(flowLabel);
        Serial.print(" ");
        Serial.print(imageIndex);
        Serial.print(" ");
        Serial.print(imagePart);
        Serial.print(" ");
        Serial.println(imageParts);*/
        imageLength+=packetSize-5;//place the pointer to the next position so the next packege will be placed behind this packege. -5 to overwrite the header
        if(newImageReady)
        {
          imageLength=IMAGE_BUFFER_LENGTH-1461;//dump the next packet on the end of the buffer
          imageIndex=0;
          Serial.println("dropped");
        }
        else if(imageLength+1460>IMAGE_BUFFER_LENGTH)
        {
          imageLength=0;//prevent overflow
          imageIndex=0;
          Serial.println("to big image or protocol error");
        }
        else if(imageIndex!=imagePart+1)
        {
          imageLength=0;
          imageIndex=0;
        Serial.println("not in the right order");
        }
        incomingPacket=unDecodedImageIn+imageLength;
        if(imageIndex>=imageParts)
        {
          Serial.println("image complete");
          finalImageLength=imageLength;
          incomingPacket=unDecodedImageOut;
          newImageReady=true;
          imageLength=IMAGE_BUFFER_LENGTH-1461;
          imageIndex=0;
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
      else if(incomingPacket[packetSize-1]==3)//if type  is 3 (new gamma settings)
      {
        memcpy(JpegDec.gamma8,incomingPacket,256);
      }
  }
}
void allocateMemory()
{
  //allocate the memory for all the different buffers
  Serial.printf("free heap: ");
  Serial.println(ESP.getFreeHeap());
  Serial.println();

  //allocate memory for receiving jpeg packets
  unDecodedImage1=(uint8_t*)heap_caps_malloc(IMAGE_BUFFER_LENGTH, MALLOC_CAP_8BIT);
  unDecodedImage2=(uint8_t*)heap_caps_malloc(IMAGE_BUFFER_LENGTH, MALLOC_CAP_8BIT);
  unDecodedImageIn=unDecodedImage1;
  unDecodedImageOut=unDecodedImage2;
  incomingPacket=unDecodedImageIn;

  Serial.printf("free heap: ");
  Serial.println(ESP.getFreeHeap());
  Serial.println();

 ledData1=(uint32_t*)heap_caps_malloc(128*16*4, MALLOC_CAP_32BIT);
 ledData2=(uint32_t*)heap_caps_malloc(128*16*4, MALLOC_CAP_32BIT);
 ledData = ledData1;
 ledBuffer= ledData2;

 Serial.printf("free heap: ");
 Serial.println(ESP.getFreeHeap());
}

void jpegSetup()
{
   JpegDec.gamma=128;
   JpegDec.calculateGamma();
}
uint32_t udpDebugTimer=0;
void jpegLoop(void * pvParameters)
{
  while(true)
  {
    if(millis()-udpDebugTimer>100)
    {
      Serial.println(millis());
      udpDebugTimer=millis();
      if(newImageReady)//if we received a new image
      {
        newImageReady=false;

        uint8_t *temp=unDecodedImageIn;
        unDecodedImageIn=unDecodedImageOut;
        unDecodedImageOut=temp;
        imageLength=0;
        incomingPacket=unDecodedImageIn+imageLength;
        Serial.println();
      }
      else{
        Serial.println("      I have no new image");
      }
    }
    receivePacket();
    if(!jpegInitialized)//if ledtask ask for intializing the jpeg
    {
      jpegInitialized=true;
      if(newImageReady)//if we received a new image
      {
        newImageReady=false;
        //switch the buffers
        uint8_t *temp=unDecodedImageIn;
        unDecodedImageIn=unDecodedImageOut;
        unDecodedImageOut=temp;
        imageLength=0;
        incomingPacket=unDecodedImageIn+imageLength;
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
    vTaskDelay(1);//prevents watchdog error messages of freertos
  }
}
