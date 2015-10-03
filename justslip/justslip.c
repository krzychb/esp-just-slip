/*
* esp-just-slip - justslip.c
*
* Copyright (c) 2014-2015, Krzysztof Budzynski <krzychb at gazeta dot pl>
* All rights reserved.
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions are met:
*
* * Redistributions of source code must retain the above copyright notice,
* this list of conditions and the following disclaimer.
* * Redistributions in binary form must reproduce the above copyright
* notice, this list of conditions and the following disclaimer in the
* documentation and/or other materials provided with the distribution.
* * The name of Krzysztof Budzynski or krzychb may not be used
* to endorse or promote products derived from this software without
* specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
* AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
* IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
* ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
* LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
* CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
* SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
* INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
* CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
* ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
* POSSIBILITY OF SUCH DAMAGE.
*/

#include <ets_sys.h>
#include <osapi.h>

#include "driver/uart.h"
#include "softuart.h"
#include "justslip.h"
#include "crc16.h"


//
// read SLIP encoded data from software serial port
// decode data and store in dataBuffer
// return number of bytes read until SLIP_END
//
// *softuart - pointer to software UART
// *dataBuffer - pointer to data buffer to store received data
//
// returned value - number of bytes read to dataBuffer
//
uint8_t ICACHE_FLASH_ATTR slipDecodeSerial(Softuart *softuart, uint8_t *dataBuffer)
{
	uint8_t dataByte;
	static uint8_t previousDataByte = 0;
	static uint8_t nPos = 0;

	while (Softuart_Available(softuart))
	{
		dataByte = Softuart_Read(softuart);
		if (dataByte == SLIP_END)
		{
			if (nPos > 0)
			{
				previousDataByte = 0;
				uint8_t result = nPos;
				nPos = 0;
				return result;
			}
			else
			{
				os_printf("Orphan SLIP_END received!\r\n");
				return 0;
			}
		}
		else if (dataByte == SLIP_ESC)
		{
			previousDataByte = SLIP_ESC;
			return 0;
		}
		else if (dataByte == SLIP_ESC_END && previousDataByte == SLIP_ESC)
		{
			previousDataByte = 0;
			dataBuffer[nPos++] = SLIP_END;
		}
		else if (dataByte == SLIP_ESC_ESC && previousDataByte == SLIP_ESC)
		{
			previousDataByte = 0;
			dataBuffer[nPos++] = SLIP_ESC;
		}
		else
		{
			dataBuffer[nPos++] = dataByte;
		}
		if (nPos == SLIP_BUFFER_SIZE - 2)
		{
			// purge buffer in case of overflow
			nPos = 0;
			previousDataByte = 0;
			os_printf("Input buffer purged because of overflow!\r\n");
		}
	}
	return 0;
}


//
// read SLIP encoded data from UART0 serial port
// decode data and store in dataBuffer
// return number of bytes read until SLIP_END
//
// *dataBuffer - pointer to data buffer to store received data
//
// returned value - number of bytes read to dataBuffer
//
uint8_t ICACHE_FLASH_ATTR slipDecodeSerialUart0(uint8_t *dataBuffer)
{
	uint8_t dataByte;
	static uint8_t previousDataByte = 0;
	static uint8_t nPos = 0;

	int c;
	while ((c = uart0_rx_one_char()) != -1)
	{
		dataByte = (uint8_t) c;
		if (dataByte == SLIP_END)
		{
			if (nPos > 0)
			{
				previousDataByte = 0;
				uint8_t result = nPos;
				nPos = 0;
				return result;
			}
			else
			{
				os_printf("Orphan SLIP_END received!\r\n");
				return 0;
			}
		}
		else if (dataByte == SLIP_ESC)
		{
			previousDataByte = SLIP_ESC;
			return 0;
		}
		else if (dataByte == SLIP_ESC_END && previousDataByte == SLIP_ESC)
		{
			previousDataByte = 0;
			dataBuffer[nPos++] = SLIP_END;
		}
		else if (dataByte == SLIP_ESC_ESC && previousDataByte == SLIP_ESC)
		{
			previousDataByte = 0;
			dataBuffer[nPos++] = SLIP_ESC;
		}
		else
		{
			dataBuffer[nPos++] = dataByte;
		}
		if (nPos == SLIP_BUFFER_SIZE - 2)
		{
			// purge buffer in case of overflow
			nPos = 0;
			previousDataByte = 0;
			os_printf("Input buffer purged because of overflow!\r\n");
		}
	}
	return 0;
}


//
// SLIP encode values from dataBuffer
// and send them over software serial port
//
// *softuart - pointer to software UART
// *dataBuffer - pointer to data buffer to read data from
// nCount - number of bytes to read form dataBuffer
//
void ICACHE_FLASH_ATTR slipEncodeSerial(Softuart *softuart, uint8_t *dataBuffer, uint8_t nCount)
{
	int i;
	for (i = 0; i < nCount; i++)
		switch (dataBuffer[i])
		{
			case SLIP_END:
				Softuart_Putchar(softuart, (char) SLIP_ESC);
				Softuart_Putchar(softuart, (char) SLIP_ESC_END);
			break;
			case SLIP_ESC:
				Softuart_Putchar(softuart, (char) SLIP_ESC);
				Softuart_Putchar(softuart, (char) SLIP_ESC_ESC);
			break;
			default:
				Softuart_Putchar(softuart, (char) dataBuffer[i]);
		}
	Softuart_Putchar(softuart, (char) SLIP_END);
}


//
// SLIP encode values from dataBuffer
// and send them over software UART0 serial port
//
// *dataBuffer - pointer to data buffer to read data from
// nCount - number of bytes to read form dataBuffer
//
void ICACHE_FLASH_ATTR slipEncodeSerialUart0(uint8_t *dataBuffer, uint8_t nCount)
{
	int i;
	for (i = 0; i < nCount; i++)
		switch (dataBuffer[i])
		{
			case SLIP_END:
				uart0_tx_one_char((uint8) SLIP_ESC);
				uart0_tx_one_char((uint8) SLIP_ESC_END);
			break;
			case SLIP_ESC:
				uart0_tx_one_char((uint8) SLIP_ESC);
				uart0_tx_one_char((uint8) SLIP_ESC_ESC);
			break;
			default:
				uart0_tx_one_char((uint8) dataBuffer[i]);
		}
	uart0_tx_one_char((uint8) SLIP_END);
}


//
// calculate and append crc16 to dataBuffer
// crc16 is calculated for nCount data and appended at the end of the data
//
// *dataBuffer - pointer to data buffer to calculate and append crc16
// nCount - number of data bytes in dataBuffer
//
// returned value - number of bytes in data buffer including crc16
//
uint8_t ICACHE_FLASH_ATTR appendCrc16(uint8_t *dataBuffer, uint8_t nCount)
{
	if (nCount + 2 > SLIP_BUFFER_SIZE)
	{
		os_printf("Unable to add crc16 - buffer too small!\r\n");
	return 0;
	}
	unsigned short crc = crc16_data(dataBuffer, nCount, 0x00);
	dataBuffer[nCount] =  (uint8_t) (crc >> 8);
	dataBuffer[nCount + 1] =  (uint8_t) crc;
	return nCount + 2;
}


//
// check crc16 by comparing value received in last two bytes of dataBuffer
// against value calculated for remaining data in the dataBuffer
//
// *dataBuffer - pointer to data buffer
// nCount - number of data bytes in dataBuffer including crc16
//
// returned value - true of false depending on result of crc16 check
//
bool ICACHE_FLASH_ATTR checkCrc16(uint8_t *dataBuffer, uint8_t nCount)
{
	// crc received in last two bytes of dataBuffer
	unsigned short crc_rec = (dataBuffer[nCount - 2] << 8) | dataBuffer[nCount - 1];
	//
	// crc calculated basing on data in dataBuffer
	unsigned short crc_chk = crc16_data(dataBuffer, nCount - 2, 0x00);
	//
	return (crc_chk == crc_rec) ? true : false;
}


//
// print values from dataBuffer for diagnostic purposes
// the last two bytes of dataBuffer contain crc16
//
// *dataBuffer - pointer to data buffer
// nCount - number of data bytes in dataBuffer including crc16
//
void ICACHE_FLASH_ATTR printBuffer(uint8_t *dataBuffer, uint8_t nCount)
{
	uint8_t i;
	for (i = 0; i < nCount - 2; i++)
		if(dataBuffer[i] < 0x10)
			os_printf("0%x ", dataBuffer[i]);
		else
			os_printf("%x ", dataBuffer[i]);
	os_printf(": ");
	os_printf((checkCrc16(dataBuffer, nCount)) ? "OK" : "Fail!");
	os_printf("\r\n");
}

