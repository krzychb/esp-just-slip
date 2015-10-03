#include <ets_sys.h>
#include <osapi.h>
#include <os_type.h>
#include <gpio.h>
#include <user_interface.h>
#include <mem.h>

#include "driver/uart.h"
#include "softuart.h"
#include "justslip.h"


Softuart softuart;

#define SLIP_BUFFER_SIZE 64
static uint8_t inputBuffer[SLIP_BUFFER_SIZE];

#define UART_READ_CB_TIME 10
#define UART_SEND_CB_TIME 30
static os_timer_t uart_read_timer, uart_send_timer;


//
// prepare and send out a buffer with diagnostic data
// - packet number
// - some random data padding
//
void ICACHE_FLASH_ATTR sendDiagBuffer(void)
{
	// packetNumber should start with 1
	// so the other side does not divide by 0
	// when calculating % of packets lost
	// for the first packet received
	static long packetNumber = 1;
	uint8_t diagBuffer[SLIP_BUFFER_SIZE];


	char *srcAddr, *dstAddr;

	// add packet counter to buffer
	srcAddr = (char *) &packetNumber;
	dstAddr = (char *) &diagBuffer[0];
	memcpy(dstAddr, srcAddr, sizeof(packetNumber));
	packetNumber++;

	int i;
	// add some random data to the buffer
	for (i = 0; i < 8; i++)
		// http://esp8266-re.foogod.com/wiki/Random_Number_Generator
		diagBuffer[4 + i] = * (uint8_t *) 0x3FF20E44;

	uint8_t nCount = appendCrc16(diagBuffer, 4 + 8);
	slipEncodeSerial(&softuart, diagBuffer, nCount);
}


//
// print values from dataBuffer for diagnostic purposes
// the last two bytes of dataBuffer contain crc16
//
//	*dataBuffer - data buffer to print out
//  nCount - number of bytes to print out
//
void ICACHE_FLASH_ATTR  printDiagBuffer(uint8_t *dataBuffer, uint8_t nCount)
{
	static long lastPacketNumber = 0;
	static long lostPackets = 0;

	uint8_t i;
	for (i = 0; i < nCount - 2; i++)
		if(dataBuffer[i] < 0x10)
			os_printf("0%x ", dataBuffer[i]);
		else
			os_printf("%x ", dataBuffer[i]);
	os_printf(": ");
	if (checkCrc16(dataBuffer, nCount))
	{
		long packetNumber;
		char *srcAddr, *dstAddr;

		// retreive packet counter from buffer
		srcAddr = (char *) &dataBuffer[0];
		dstAddr = (char *) &packetNumber;
		memcpy(dstAddr, srcAddr, sizeof(packetNumber));
		long lost = packetNumber - lastPacketNumber - 1;
		lastPacketNumber = packetNumber;
		if (lost > 0)
		{
			// report packets that have been lost
			os_printf("%ld lost!", lost);
			lostPackets += lost;
		}
		else
		{
			os_printf("%ld%%", 100 * lostPackets / packetNumber);
		}
		os_printf(" ");
	}
	else
	{
		os_printf("Fail!");
	}
	os_printf("\r\n");
}


//
// read slip data from SoftUART
//
void ICACHE_FLASH_ATTR uart_read_cb(void *arg)
{
	uint8_t nCount;

	nCount = slipDecodeSerial(&softuart, inputBuffer);
	if (nCount > 0)
		printDiagBuffer(inputBuffer, nCount);
}


//
// send slip data through SoftUART
//
void ICACHE_FLASH_ATTR uart_send_cb(void *arg)
{
	sendDiagBuffer();
}



void ICACHE_FLASH_ATTR user_init(void)
{
	// set bit rates of UART0 and UART1
	//
	// UART0 Rx/Tx - GPIO3/GPIO1
	// UART1 Tx (no Rx) - GPIO2
	//
	// set bit rate for UART1 != 0 to direct output of os_printf() to UART1
	// set bit rate for UART1 = 0 to direct output of os_printf() to UART0
	//
	uart_init(BIT_RATE_115200, BIT_RATE_115200);

	// enable os_printf() to go to the debug output of UART0 or UART1
	system_set_os_print(1);

	os_printf("\r\n");
	os_printf("ESP8255 Slip Transmit / Receive (esp-just-slip)\r\n");

	// initialise software UART connected to GIPIO14 (Tx) and GPIO12 (Rx)
	Softuart_SetPinRx(&softuart, 14);  // LoLin D5
	Softuart_SetPinTx(&softuart, 12);  // LoLin D6
	Softuart_Init(&softuart, 57600);

	// SoftUART reading
	os_timer_disarm(&uart_read_timer);
	os_timer_setfn(&uart_read_timer, (os_timer_func_t *)uart_read_cb, (void *)0);
	os_timer_arm(&uart_read_timer, UART_READ_CB_TIME, 1);

	// SoftUART writing
	os_timer_disarm(&uart_send_timer);
	os_timer_setfn(&uart_send_timer, (os_timer_func_t *)uart_send_cb, (void *)0);
	os_timer_arm(&uart_send_timer, UART_SEND_CB_TIME, 1);
}
