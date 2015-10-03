/*
* ino-just-slip - JustSlip.c
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


//
// read SLIP encoded data from serial port
// decode data and store in dataBuffer
// retrun number of byter read until SLIP_END
//
uint8_t slipDecodeSerial(uint8_t *dataBuffer)
{
  uint8_t dataByte;
  static uint8_t previousDataByte = 0;
  static uint8_t nPos = 0;

  while (espSerial.available())
  {
    dataByte = espSerial.read();
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
        Serial.print("Orphan SLIP_END received!\n");
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
      Serial.print("Input buffer purged because of overflow!\n");
    }
  }
  return 0;
}


//
// SLIP encode values from dataBuffer
// and send them over serial port
//
void slipEncodeSerial(uint8_t *dataBuffer, uint8_t nCount)
{
  for (int i = 0; i < nCount; i++)
    switch (dataBuffer[i])
    {
      case SLIP_END:
        espSerial.write(SLIP_ESC);
        espSerial.write(SLIP_ESC_END);
        break;
      case SLIP_ESC:
        espSerial.write(SLIP_ESC);
        espSerial.write(SLIP_ESC_ESC);
        break;
      default:
        espSerial.write(dataBuffer[i]);
    }
  espSerial.write(SLIP_END);
}



//
// read data from keyboard and store them in dataBuffer
// return number of bytes in dataBufer once enter is pressed
//
uint8_t readKeyboard(uint8_t *dataBuffer)
{
  uint8_t dataByte;
  static uint8_t nPos = 0;

  while (Serial.available())
  {
    dataByte = Serial.read();
    if (dataByte == '\r' || dataByte == '\n')
    {
      // process only non empty lines
      if (nPos > 0)
      {
        dataBuffer[nPos++] = dataByte;
        uint8_t result = nPos;
        nPos = 0;
        return result;
      }
    }
    else
    {
      if (nPos == SLIP_BUFFER_SIZE - 2)
      {
        // purge buffer in case of overflow
        nPos = 0;
        Serial.print("Keyboard buffer purged because of overflow!\n");
      }
      else
      {
        // add next character to the buffer
        dataBuffer[nPos++] = dataByte;
      }
    }
  }
  return 0;
}


//
// append crc16 to dataBuffer
//
uint8_t appendCrc16(uint8_t *dataBuffer, uint8_t nCount)
{
  if (nCount + 2 > SLIP_BUFFER_SIZE)
  {
    Serial.print("Unable to add crc16 - buffer too small!\n");
    return 0;
  }
  unsigned short crc = crc16_data(dataBuffer, nCount, 0x00);
  dataBuffer[nCount] =  (uint8_t) (crc >> 8);
  dataBuffer[nCount + 1] =  (uint8_t) crc;
  return nCount + 2;
}


//
// check crc16 by comparig value received in last two bytes of dataBuffer
// against value calculated for reaminig data in the dataBuffer
//
bool checkCrc16(uint8_t *dataBuffer, uint8_t nCount)
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
// print values from dataBuffer
// for diagnostic purposes
// the last two bytes of dataBuffer contain crc16
//

void printBuffer(uint8_t *dataBuffer, uint8_t nCount)
{
  for (uint8_t i = 0; i < nCount - 2; i++)
  {
    if (dataBuffer[i] < 0x10)
    {
      Serial.print("0");
      Serial.print(dataBuffer[i], HEX);
    }
    else
    {
      Serial.print(dataBuffer[i], HEX);
    }
    Serial.print(" ");
  }
  Serial.print(": ");
  if (checkCrc16(dataBuffer, nCount))
  {
    Serial.print("OK");
  }
  else
  {
    Serial.print("Fail!");
  }
  Serial.print("\n");
}

