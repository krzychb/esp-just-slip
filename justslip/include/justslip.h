/*
 * bare_slip.h
 *
 *  Created on: 27 Sep 2015
 *      Author: Krzysztof
 */

#ifndef JUSTSLIP_INCLUDE_JUSTSLIP_H_
#define JUSTSLIP_INCLUDE_JUSTSLIP_H_

#include "os_type.h"
#include "user_config.h"

#include "driver/uart.h"
#include "softuart.h"

// SLIP_END - transmission END marker - SLIP data block should be ended with this marker
// SLIP_ESC - SLIP escape character - this one will be placed just before a byte if it's value is either 0333 (0xdb) or 0300 (c0)
// SLIP_ESC_END - placed in the data stream instead of the SLIP_END byte, preceded by a SLIP_ESC byte
// SLIP_ESC_ESC - placed in the data stream instead of the SLIP_ESC byte, preceded by a SLIP_ESC byte
//
#define SLIP_END 0xC0
#define SLIP_ESC 0xDB
#define SLIP_ESC_END 0xDC
#define SLIP_ESC_ESC 0xDD

#define SLIP_BUFFER_SIZE 64


uint8_t ICACHE_FLASH_ATTR slipDecodeSerial(Softuart *softuart, uint8_t *dataBuffer);
void ICACHE_FLASH_ATTR slipEncodeSerial(Softuart *softuart, uint8_t *dataBuffer, uint8_t nCount);
uint8_t ICACHE_FLASH_ATTR readKeyboard(uint8_t *dataBuffer);
uint8_t ICACHE_FLASH_ATTR appendCrc16(uint8_t *dataBuffer, uint8_t nCount);
bool ICACHE_FLASH_ATTR checkCrc16(uint8_t *dataBuffer, uint8_t nCount);

#endif /* JUSTSLIP_INCLUDE_JUSTSLIP_H_ */
