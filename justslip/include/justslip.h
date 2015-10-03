/*
* esp-just-slip - justslip.h
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

#ifndef JUSTSLIP_INCLUDE_JUSTSLIP_H_
#define JUSTSLIP_INCLUDE_JUSTSLIP_H_

#include "os_type.h"
#include "user_config.h"

#include "driver/uart.h"
#include "softuart.h"

//
// source https://en.wikipedia.org/wiki/Serial_Line_Internet_Protocol
//
// SLIP_END - Frame End - distinguishes datagram boundaries in the byte stream
// SLIP_ESC - Frame Escape
//
// If the END byte occurs in the data to be sent, the two byte sequence ESC, ESC_END is sent instead
// If the ESC byte occurs in the data, the two byte sequence ESC, ESC_ESC is sent.
//
// SLIP_ESC_END - Transposed Frame End
// SLIP_ESC_ESC - Transposed Frame Escape
//
#define SLIP_END 0xC0
#define SLIP_ESC 0xDB
#define SLIP_ESC_END 0xDC
#define SLIP_ESC_ESC 0xDD

#define SLIP_BUFFER_SIZE 64


uint8_t ICACHE_FLASH_ATTR slipDecodeSerial(Softuart *softuart, uint8_t *dataBuffer);
uint8_t ICACHE_FLASH_ATTR slipDecodeSerialUart0(uint8_t *dataBuffer);
void ICACHE_FLASH_ATTR slipEncodeSerial(Softuart *softuart, uint8_t *dataBuffer, uint8_t nCount);
void ICACHE_FLASH_ATTR slipEncodeSerialUart0(uint8_t *dataBuffer, uint8_t nCount);
uint8_t ICACHE_FLASH_ATTR readKeyboard(uint8_t *dataBuffer);
uint8_t ICACHE_FLASH_ATTR appendCrc16(uint8_t *dataBuffer, uint8_t nCount);
bool ICACHE_FLASH_ATTR checkCrc16(uint8_t *dataBuffer, uint8_t nCount);
void ICACHE_FLASH_ATTR printBuffer(uint8_t *dataBuffer, uint8_t nCount);

#endif /* JUSTSLIP_INCLUDE_JUSTSLIP_H_ */
