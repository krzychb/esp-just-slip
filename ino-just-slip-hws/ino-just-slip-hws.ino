/*
* ino-just-slip - ino-just-slip-hws.c
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

#define DIAG_DIODE 13

#include <SoftwareSerial.h>
SoftwareSerial DiagUART(10, 11); // RX, TX

#define SLIP_BUFFER_SIZE 64
uint8_t inputBuffer[SLIP_BUFFER_SIZE];

//
// prepare and send out a buffer with diagnostic data
// - packet number
// - some random data padding
//
void sendDiagBuffer(void)
{
  // packetNumber should start with 1
  // so the other side does not divide by 0
  // when calculating % of packets lost
  // for the first packet received
  static long packetNumber = 1;
  uint8_t diagBuffer[SLIP_BUFFER_SIZE];

  // Toggle diagnostic diode
  digitalWrite(DIAG_DIODE, !digitalRead(DIAG_DIODE));

  byte *srcAddr, *dstAddr;

  // add packet counter to buffer
  srcAddr = (byte *) &packetNumber;
  dstAddr = (byte *) &diagBuffer[0];
  memcpy(dstAddr, srcAddr, sizeof(packetNumber));
  packetNumber++;

  for (int i = 0; i < 8; i++)
    diagBuffer[4 + i] = random(256);

  uint8_t nCount = appendCrc16(diagBuffer, 4 + 8);
  slipEncodeSerial(diagBuffer, nCount);
}


//
// print values from dataBuffer
// for diagnostic purposes
// the last two bytes of dataBuffer contain crc16
//

void printDiagBuffer(uint8_t *dataBuffer, uint8_t nCount)
{
  static long lastPacketNumber = 0;
  static long lostPackets = 0;

  for (uint8_t i = 0; i < nCount - 2; i++)
  {
    if (dataBuffer[i] < 0x10)
    {
      DiagUART.print("0");
      DiagUART.print(dataBuffer[i], HEX);
    }
    else
    {
      DiagUART.print(dataBuffer[i], HEX);
    }
    DiagUART.print(" ");
  }
  DiagUART.print(": ");
  if (checkCrc16(dataBuffer, nCount))
  {
    long packetNumber;
    byte *srcAddr, *dstAddr;

    // retreive packet counter from buffer
    srcAddr = (byte *) &dataBuffer[0];
    dstAddr = (byte *) &packetNumber;
    memcpy(dstAddr, srcAddr, sizeof(packetNumber));
    long lost = packetNumber - lastPacketNumber - 1;
    lastPacketNumber = packetNumber;
    if (lost > 0)
    {
      // some packets have been lost
      DiagUART.print(lost);
      DiagUART.print(" lost!");
      lostPackets += lost;
    }
    else
    {
      DiagUART.print(" ");
      DiagUART.print(100 * lostPackets / packetNumber);
      DiagUART.print("%");
    }
    DiagUART.print(" ");
  }
  else
  {
    DiagUART.print("Fail! ");
  }
  DiagUART.print("\n");
}


void setup(void)
{
  pinMode(DIAG_DIODE, OUTPUT);
  randomSeed(analogRead(0));

  DiagUART.begin(57600);
  DiagUART.print("Arduino Slip Transmit / Receive (ino-bare-slip)\n");

  // SoftwareSerial Initialization
  DiagUART.print("SoftwareSerial initialization...");
  Serial.begin(57600);
  DiagUART.print("done\n");

  DiagUART.println("Starting main loop...\n");
}


void loop(void)
{
  uint8_t nCount;

  if (millis() % 30 == 0)
    sendDiagBuffer();

  if (millis() % 10 == 0)
  {
    nCount = slipDecodeSerial(inputBuffer);
    if (nCount > 0)
      printDiagBuffer(inputBuffer, nCount);
  }
  delay(1);
}

