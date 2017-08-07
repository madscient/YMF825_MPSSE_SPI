// YMF825_MPSSE.cpp : �R���\�[�� �A�v���P�[�V�����̃G���g�� �|�C���g���`���܂��B
//

#include "stdafx.h"

/*!
* \file sample-static.c
*
* \author FTDI
* \date 20110512
*
* Copyright � 2000-2014 Future Technology Devices International Limited
*
*
* THIS SOFTWARE IS PROVIDED BY FUTURE TECHNOLOGY DEVICES INTERNATIONAL LIMITED ``AS IS'' AND ANY EXPRESS
* OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
* FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL FUTURE TECHNOLOGY DEVICES INTERNATIONAL LIMITED
* BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
* BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
* INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
* (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
* THE POSSIBILITY OF SUCH DAMAGE.
*
* Project: libMPSSE
* Module: SPI Sample Application - Interfacing 94LC56B SPI EEPROM
*
* Rivision History:
* 0.1  - 20110512 - Initial version
* 0.2  - 20110801 - Changed LatencyTimer to 255
* 					 Attempt to open channel only if available
*					 Added & modified macros
*					 Included stdlib.h
* 0.3  - 20111212 - Added comments
* 0.41 - 20140903 - Fixed compilation warnings
*					 Added testing of SPI_ReadWrite()
*/

/******************************************************************************/
/* 							 Include files										   */
/******************************************************************************/
/* Standard C libraries */
#include<stdio.h>
#include<stdlib.h>
#include<string.h>
/* OS specific libraries */
#ifdef _WIN32
#include<windows.h>
#endif

/* Include D2XX header*/
#include "ftd2xx.h"

/* Include libMPSSE header */
#include "libMPSSE_spi.h"

/******************************************************************************/
/*								Macro and type defines							   */
/******************************************************************************/
/* Helper macros */

#define APP_CHECK_STATUS(exp) {if(exp!=FT_OK){printf("%s:%d:%s(): status(0x%x) \
!= FT_OK\n",__FILE__, __LINE__, __FUNCTION__,exp);exit(1);}else{;}};
#define CHECK_NULL(exp){if(exp==NULL){printf("%s:%d:%s():  NULL expression \
encountered \n",__FILE__, __LINE__, __FUNCTION__);exit(1);}else{;}};

/* Application specific macro definations */
#define SPI_DEVICE_BUFFER_SIZE		256
#define SPI_WRITE_COMPLETION_RETRY		10
#define START_ADDRESS_EEPROM 	0x00 /*read/write start address inside the EEPROM*/
#define END_ADDRESS_EEPROM		0x10
#define RETRY_COUNT_EEPROM		10	/* number of retries if read/write fails */
#define CHANNEL_TO_OPEN			0	/*0 for first available channel, 1 for next... */
#define SPI_SLAVE_0				0
#define SPI_SLAVE_1				1
#define SPI_SLAVE_2				2
#define DATA_OFFSET				4
#define USE_WRITEREAD			0

/******************************************************************************/
/*								Global variables							  	    */
/******************************************************************************/
static FT_HANDLE ftHandle;
static uint8 buffer[SPI_DEVICE_BUFFER_SIZE] = { 0 };

/******************************************************************************/
/*						Public function definitions						  		   */
/******************************************************************************/

void reset()
{
	FT_WriteGPIO(ftHandle, 0xff, 0xff);
	FT_WriteGPIO(ftHandle, 0xff, 0xfe);
	::Sleep(1);
	FT_WriteGPIO(ftHandle, 0xff, 0xff);
}

void write_reg(uint8 addr, uint8 data)
{
	uint32 sizeToTransfer = 0;
	uint32 sizeTransfered = 0;
	uint8 writeComplete = 0;
	uint32 retry = 0;
	FT_STATUS status;
	byte buf[2];

	buf[0] = addr;
	buf[1] = data;
	/* Write command EWEN(with CS_High -> CS_Low) */
	sizeToTransfer = 16;
	sizeTransfered = 0;
	status = SPI_Write(ftHandle, buf, sizeToTransfer, &sizeTransfered,
		SPI_TRANSFER_OPTIONS_SIZE_IN_BITS |
		SPI_TRANSFER_OPTIONS_CHIPSELECT_ENABLE |
		SPI_TRANSFER_OPTIONS_CHIPSELECT_DISABLE);
	APP_CHECK_STATUS(status);
}

void write_burst(uint8 addr, uint8* data, uint32 size)
{
	uint32 sizeToTransfer = size + 1;
	uint32 sizeTransfered = 0;
	uint8 writeComplete = 0;
	uint32 retry = 0;
	FT_STATUS status;
	byte* buf = new byte[size+1];

	buf[0] = addr;
	memcpy(&buf[1], data, size);
	/* Write command EWEN(with CS_High -> CS_Low) */
	sizeTransfered = 0;
	status = SPI_Write(ftHandle, buf, sizeToTransfer, &sizeTransfered,
		SPI_TRANSFER_OPTIONS_SIZE_IN_BYTES |
		SPI_TRANSFER_OPTIONS_CHIPSELECT_ENABLE |
		SPI_TRANSFER_OPTIONS_CHIPSELECT_DISABLE);
	APP_CHECK_STATUS(status);
	delete[] buf;
}

uint8 read_reg(uint8 addr)
{
	uint8 ret = 0xff;
	uint32 sizeToTransfer = 1;
	uint32 sizeTransfered = 0;
	uint8 writeComplete = 0;
	uint32 retry = 0;
	FT_STATUS status;
	status = SPI_Read(ftHandle, &ret, sizeToTransfer, &sizeTransfered,
		SPI_TRANSFER_OPTIONS_SIZE_IN_BYTES |
		SPI_TRANSFER_OPTIONS_CHIPSELECT_ENABLE |
		SPI_TRANSFER_OPTIONS_CHIPSELECT_DISABLE);
	APP_CHECK_STATUS(status);
	return ret;
}

/*!
* \brief Main function / Entry point to the sample application
*
* This function is the entry point to the sample application. It opens the channel, writes to the
* EEPROM and reads back.
*
* \param[in] none
* \return Returns 0 for success
* \sa
* \note
* \warning
*/
#define OUTPUT_power 0

void init825()
{
	reset();
	write_reg(0x1D, OUTPUT_power);
	write_reg(0x02, 0x0E);
	::Sleep(1);
	write_reg(0x00, 0x01);//CLKEN
	write_reg(0x01, 0x00); //AKRST
	write_reg(0x1A, 0xA3);
	::Sleep(1);
	write_reg(0x1A, 0x00);
	::Sleep(30);
	write_reg(0x02, 0x04);//AP1,AP3
	::Sleep(1);
	write_reg(0x02, 0x00);
	//add
	write_reg(0x19, 0x20);//MASTER VOL
	write_reg(0x1B, 0x3F);//interpolation
	write_reg(0x14, 0x00);//interpolation
	write_reg(0x03, 0x01);//Analog Gain

	write_reg(0x08, 0xF6);
	::Sleep(21);
	write_reg(0x08, 0x00);
	write_reg(0x09, 0xF8);
	write_reg(0x0A, 0x00);

	write_reg(0x17, 0x40);//MS_S
	write_reg(0x18, 0x00);
}

void set_tone(void) {
	unsigned char tone_data[35] = {
		0x81,//header
			 //T_ADR 0
			 0x01,0x85,
			 0x00,0x7F,0xF4,0xBB,0x00,0x10,0x40,
			 0x00,0xAF,0xA0,0x0E,0x03,0x10,0x40,
			 0x00,0x2F,0xF3,0x9B,0x00,0x20,0x41,
			 0x00,0xAF,0xA0,0x0E,0x01,0x10,0x40,
			 0x80,0x03,0x81,0x80,
	};

	write_reg(0x08, 0xF6);
	::Sleep(1);
	write_reg(0x08, 0x00);

	write_burst(0x07, &tone_data[0], 35);//write to FIFO
}

void set_ch(void) {
	write_reg(0x0F, 0x30);// keyon = 0
	write_reg(0x10, 0x71);// chvol
	write_reg(0x11, 0x00);// XVB
	write_reg(0x12, 0x08);// FRAC
	write_reg(0x13, 0x00);// FRAC  
}

void keyon(unsigned char fnumh, unsigned char fnuml) {
	write_reg(0x0B, 0x00);//voice num
	write_reg(0x0C, 0x54);//vovol
	write_reg(0x0D, fnumh);//fnum
	write_reg(0x0E, fnuml);//fnum
	write_reg(0x0F, 0x40);//keyon = 1  
}

void keyoff(void) {
	write_reg(0x0F, 0x00);//keyon = 0
}

int main()
{
	FT_STATUS status = FT_OK;
	FT_DEVICE_LIST_INFO_NODE devList = { 0 };
	ChannelConfig channelConf = { 0 };
	uint8 address = 0;
	uint32 channels = 0;
	uint16 data = 0;
	uint8 i = 0;
	uint8 latency = 1;

	channelConf.ClockRate = 10000000;
	channelConf.LatencyTimer = latency;
	channelConf.configOptions = SPI_CONFIG_OPTION_MODE0 | SPI_CONFIG_OPTION_CS_ACTIVELOW;// | SPI_CONFIG_OPTION_CS_ACTIVELOW;
	channelConf.Pin = 0x00000000;/*FinalVal-FinalDir-InitVal-InitDir (for dir 0=in, 1=out)*/

								 /* init library */
#ifdef _MSC_VER
	Init_libMPSSE();
#endif
	status = SPI_GetNumChannels(&channels);
	APP_CHECK_STATUS(status);
	printf("Number of available SPI channels = %d\n", (int)channels);

	if (channels>0)
	{
		for (i = 0; i<channels; i++)
		{
			status = SPI_GetChannelInfo(i, &devList);
			APP_CHECK_STATUS(status);
			printf("Information on channel number %d:\n", i);
			/* print the dev info */
			printf("		Flags=0x%x\n", devList.Flags);
			printf("		Type=0x%x\n", devList.Type);
			printf("		ID=0x%x\n", devList.ID);
			printf("		LocId=0x%x\n", devList.LocId);
			printf("		SerialNumber=%s\n", devList.SerialNumber);
			printf("		Description=%s\n", devList.Description);
			printf("		ftHandle=0x%x\n", (unsigned int)devList.ftHandle);/*is 0 unless open*/
		}

		/* Open the first available channel */
		status = SPI_OpenChannel(CHANNEL_TO_OPEN, &ftHandle);
		APP_CHECK_STATUS(status);
		printf("\nhandle=0x%x status=0x%x\n", (unsigned int)ftHandle, status);
		status = SPI_InitChannel(ftHandle, &channelConf);
		APP_CHECK_STATUS(status);

		init825();

		status = SPI_CloseChannel(ftHandle);
	}

#ifdef _MSC_VER
	Cleanup_libMPSSE();
#endif

#ifndef __linux__
	system("pause");
#endif
	return 0;
}
