/*
JPEGDecoder.cpp

JPEG Decoder for Arduino
https://github.com/MakotoKurauchi/JPEGDecoder
Public domain, Makoto Kurauchi <http://yushakobo.jp>

Latest version here:
https://github.com/Bodmer/JPEGDecoder

Bodmer (21/6/15): Adapted by Bodmer to display JPEGs on TFT (works with Mega and Due) but there
is a memory leak somewhere, crashes after decoding 1 file :-)

Bodmer (29/1/16): Now in a state with sufficient Mega and Due testing to release in the wild

Bodmer (various): Various updates and latent bugs fixed

Bodmer (14/1/17): Tried to merge ESP8266 and SPIFFS support from Frederic Plante's broken branch,
				worked on ESP8266, but broke the array handling :-(

Bodmer (14/1/17): Scrapped all FP's updates, extended the built-in approach to using different
				data sources (currently array, SD files and/or SPIFFS files)

Bodmer (14/1/17): Added ESP8266 support and SPIFFS as a source, added configuration option to
				swap bytes to support fast image transfer to TFT using ESP8266 SPI writePattern().

Bodmer (15/1/17): Now supports ad hoc use of SPIFFS, SD and arrays without manual configuration.

Bodmer (19/1/17): Add support for filename being String type

Bodmer (20/1/17): Correct last mcu block corruption (thanks stevstrong for tracking that bug down!)

Bodmer (20/1/17): Prevent deleting the pImage pointer twice (causes an exception on ESP8266),
				tidy up code.

Bodmer (24/1/17): Correct greyscale images, update examples
*/

#include "JPEGDecoder.h"
#include "picojpeg.h"

JPEGDecoder JpegDec;

JPEGDecoder::JPEGDecoder(){
	mcu_x = 0 ;
	mcu_y = 0 ;
	is_available = 0;
	thisPtr = this;
}


JPEGDecoder::~JPEGDecoder(){
	//if (pImage) delete pImage;
	//pImage = NULL;
}


uint8_t JPEGDecoder::pjpeg_callback(uint8_t* pBuf, uint8_t buf_size, uint8_t *pBytes_actually_read, void *pCallback_data) {
	JPEGDecoder *thisPtr = JpegDec.thisPtr;
	thisPtr->pjpeg_need_bytes_callback(pBuf, buf_size, pBytes_actually_read, pCallback_data);
	return 0;
}


uint8_t JPEGDecoder::pjpeg_need_bytes_callback(uint8_t* pBuf, uint8_t buf_size, uint8_t *pBytes_actually_read, void *pCallback_data) {
	uint n;

	pCallback_data;
	n = jpg_min(g_nInFileSize - g_nInFileOfs, buf_size);
	//	for (int i = 0; i < n; i++) {
				//pBuf[i] = *jpg_data++;
		//}
		memcpy(pBuf,jpg_data,n);
		jpg_data+=n;


	*pBytes_actually_read = (uint8_t)(n);
	g_nInFileOfs += n;
	return 0;
}

int JPEGDecoder::decode_mcu(void) {

	status = pjpeg_decode_mcu();

	if (status) {
		is_available = 0 ;

		if (status != PJPG_NO_MORE_BLOCKS) {
			#ifdef DEBUG
			Serial.print("pjpeg_decode_mcu() failed with status ");
			Serial.println(status);
			#endif

			return -1;
		}
	}
	return 1;
}

uint8_t gamma8[256];
void JPEGDecoder::calculateGamma()
{
  for(float i=0.0; i<=255.0; i++) {
      gamma8[(int)i]=(int)(pow(i / 255.0,(gamma/50.0)) * 255.0 + 0.5);
  }
}
int JPEGDecoder::read(uint32_t *pImage) {

	int y, x;
	uint32_t *pDst_row;

	if(is_available == 0 || mcu_y >= image_info.m_MCUSPerCol) {
		abort();
		return 0;
	}
	/*int mcuXCoord = MCUx;
	int mcuYCoord = MCUy;
	// Get the number of pixels in the current MCU
	uint32_t mcuPixels = MCUWidth * MCUHeight;
	Serial.print(mcuXCoord);
	Serial.print("  ");
	Serial.print(mcuYCoord);
	Serial.print("  ");
	Serial.print(MCUWidth);
	Serial.print("  ");
	Serial.print(MCUHeight);
	Serial.print("  ");
	Serial.println(mcuPixels);*/

	// Copy MCU's pixel blocks into the destination bitmap.
	pDst_row = pImage;

	for (y = 0; y < image_info.m_MCUHeight; y += 8) {

		for (x = 0; x < image_info.m_MCUWidth; x += 8) {
			uint32_t *pDst_block = pDst_row + x;
			// Compute source byte offset of the block in the decoder's MCU buffer.
			uint src_ofs = (x * 8U) + (y * 16U);
			const uint8_t *pSrcR = image_info.m_pMCUBufR + src_ofs;
			const uint8_t *pSrcG = image_info.m_pMCUBufG + src_ofs;
			const uint8_t *pSrcB = image_info.m_pMCUBufB + src_ofs;

				uint8_t bx, by;
				for (by = 0; by < 8; by++) {
					uint32_t *pDst = pDst_block;

					for (bx = 0; bx < 8; bx++) {
						*pDst++ =  gamma8[*pSrcR++] << 24 | gamma8[*pSrcG++] << 16 | gamma8[*pSrcB++] << 8 | brighness;
					}
					pDst_block += row_pitch;
				}
			}
		pDst_row += (row_pitch * 8);
	}

	MCUx = mcu_x;
	MCUy = mcu_y;

	mcu_x++;
	if (mcu_x == image_info.m_MCUSPerRow) {
		mcu_x = 0;
		mcu_y++;
	}

	if(decode_mcu()==-1) is_available = 0 ;

	return 1;
}



int JPEGDecoder::decodeArray(uint8_t array[], uint32_t  array_size) {

	jpg_source = JPEG_ARRAY; // We are not processing a file, use arrays

	g_nInFileOfs = 0;

	jpg_data = array;

	g_nInFileSize = array_size;

	return decodeCommon();
}


int JPEGDecoder::decodeCommon(void) {

	status = pjpeg_decode_init(&image_info, pjpeg_callback, NULL, 0);

	if (status) {
		#ifdef DEBUG
		Serial.print("pjpeg_decode_init() failed with status ");
		Serial.println(status);

		if (status == PJPG_UNSUPPORTED_MODE) {
			Serial.println("Progressive JPEG files are not supported.");
		}
		#endif

		return -1;
	}

	decoded_width =  image_info.m_width;
	decoded_height =  image_info.m_height;

	row_pitch = image_info.m_MCUWidth;


//	memset(pImage , 0 , sizeof(pImage));

	row_blocks_per_mcu = image_info.m_MCUWidth >> 3;
	col_blocks_per_mcu = image_info.m_MCUHeight >> 3;

	is_available = 1 ;

	width = decoded_width;
	height = decoded_height;
	comps = 1;
	MCUSPerRow = image_info.m_MCUSPerRow;
	MCUSPerCol = image_info.m_MCUSPerCol;
	scanType = image_info.m_scanType;
	MCUWidth = image_info.m_MCUWidth;
	MCUHeight = image_info.m_MCUHeight;

	return decode_mcu();
}

void JPEGDecoder::abort(void) {

	mcu_x = 0 ;
	mcu_y = 0 ;
	is_available = 0;
	//if(pImage) delete pImage;
	//pImage = NULL;

}
