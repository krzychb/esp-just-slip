//
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
  Serial.print("Rx ");
  for (uint8_t i = 0; i < nCount - 2; i++)
  {
    Serial.print(byteToHex(&dataBuffer[i], 1, true));
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


//
// prints 8-bit data in hex with leading zeros
// for uppercase set capslock to true
// tempCharBuffer should be 2 x length + 1
//
static char tempCharBuffer[2 * 4 + 1];
//
char* byteToHex(uint8_t *data, uint8_t length, bool capslock)
{
  byte first;
  byte caps = capslock ? (byte)7 : (byte)39;
  int j = length * 2 - 1;
  // required static char buffer
  tempCharBuffer[length * 2] = '\0';

  for (uint8_t i = 0; i < length; i++)
  {
    first = (data[i] & 0x0F) | 48;
    if (first > 57) tempCharBuffer[j] = first + caps;
    else tempCharBuffer[j] = first;
    j--;

    first = (data[i] >> 4) | 48;
    if (first > 57) tempCharBuffer[j] = first + caps;
    else tempCharBuffer[j] = first;
    j--;
  }
  return tempCharBuffer;
}

