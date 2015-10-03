#include <ets_sys.h>
#include <osapi.h>

#include "driver/uart.h"
#include "softuart.h"
#include "justslip.h"
#include "crc16.h"


//
// read SLIP encoded data from serial port
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
// SLIP encode values from dataBuffer
// and send them over serial port
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

