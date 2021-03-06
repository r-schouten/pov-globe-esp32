/*
JPEGDecoder.h

JPEG Decoder for Arduino
Public domain, Makoto Kurauchi <http://yushakobo.jp>

Adapted by Bodmer for use with a TFT screen

Latest version here:
https://github.com/Bodmer/JPEGDecoder

*/

#ifndef JPEGDECODER_H
	#define JPEGDECODER_H

	#include "User_Config.h"

	#include "Arduino.h"



#include "picojpeg.h"

enum {
	JPEG_ARRAY = 0,
	JPEG_FS_FILE,
	JPEG_SD_FILE
};

//#define DEBUG

//------------------------------------------------------------------------------
#ifndef jpg_min
	#define jpg_min(a,b)     (((a) < (b)) ? (a) : (b))
#endif

#ifndef min
  #define min(a,b) (((a) < (b)) ? (a) : (b))
#endif

//------------------------------------------------------------------------------
typedef unsigned char uint8;
typedef unsigned int uint;
//------------------------------------------------------------------------------

class JPEGDecoder {

public:
	pjpeg_scan_type_t scan_type;
	pjpeg_image_info_t image_info;

	int is_available;
	int mcu_x;
	int mcu_y;
	uint g_nInFileSize;
	uint g_nInFileOfs;
	uint row_pitch;
	uint decoded_width, decoded_height;
	uint row_blocks_per_mcu, col_blocks_per_mcu;
	uint8 status;
	uint8 jpg_source = 0;
	uint8_t* jpg_data;
	uint8_t brighness=255;
	uint8_t gamma=128;
	uint8_t gamma8[256];
	static uint8 pjpeg_callback(unsigned char* pBuf, unsigned char buf_size, unsigned char *pBytes_actually_read, void *pCallback_data);
	uint8 pjpeg_need_bytes_callback(unsigned char* pBuf, unsigned char buf_size, unsigned char *pBytes_actually_read, void *pCallback_data);
	int decode_mcu(void);
	int decodeCommon(void);



	JPEGDecoder *thisPtr;

	int width;
	int height;
	int comps;
	int MCUSPerRow;
	int MCUSPerCol;
	pjpeg_scan_type_t scanType;
	int MCUWidth;
	int MCUHeight;
	int MCUx;
	int MCUy;

	JPEGDecoder();
	~JPEGDecoder();

	int available(void);
	int read(uint32_t *pImage);

	int decodeArray(uint8_t array[], uint32_t  array_size);
	void abort(void);

	void calculateGamma();

};

extern JPEGDecoder JpegDec;

#endif // JPEGDECODER_H
