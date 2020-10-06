#ifdef PARTICLE
  #include "Particle.h"
#else
  #include "arduino.h"
#endif //end of #ifdef PARTICLE

#include "IoTNode.h"

Adafruit_MCP23017 expand;

MCP7941x rtc = MCP7941x();

File myFile;

SdFat SD;

// Create FRAM instances
#define PART_NUMBER MB85RC256V


// Constructor
IoTNode::IoTNode() : myFram(PART_NUMBER)
{

}

bool IoTNode::begin()
{
  if (!Wire.isEnabled())
  {
    Wire.begin();
  }
  delay(20);
  byte error, address;
  bool result = true;

  // The i2c_scanner uses the return value of
  // the Write.endTransmisstion to see if
  // a device did acknowledge to the address.
  address = 0x20; // MCP23017 address
  Wire.beginTransmission(address);
  error = Wire.endTransmission();

  // Try again if there is an error
  if (!error==0)
  {
    Wire.reset();
    Wire.beginTransmission(address);
    error = Wire.endTransmission();
  }

  // Return false if there is an error
  if (!error == 0)
  {
    result = false;
  }
  

  expand.begin();
  //Set pin direction 1 = out, 0 = in
  //PORT_A,0b10111111 | PORT_B,0b00001111
  expand.pinMode(0,OUTPUT);
  expand.pinMode(1,OUTPUT);
  expand.pinMode(2,OUTPUT);
  expand.pinMode(3,OUTPUT);
  expand.pinMode(4,OUTPUT);
  expand.pinMode(5,OUTPUT);
  expand.pinMode(6,INPUT);
  expand.pinMode(7,OUTPUT);
  expand.pinMode(8,INPUT);
  expand.pinMode(9,INPUT);
  expand.pinMode(10,INPUT);
  expand.pinMode(11,INPUT);
  expand.pinMode(12,INPUT);
  expand.pinMode(13,INPUT);
  expand.pinMode(14,INPUT);
  expand.pinMode(15,INPUT);

  expand.pullUp(0,HIGH);
  expand.pullUp(1,HIGH);
  expand.pullUp(2,HIGH);
  expand.pullUp(3,HIGH);
  expand.pullUp(4,HIGH);
  expand.pullUp(5,HIGH);
  expand.pullUp(6,HIGH);
  expand.pullUp(7,HIGH);
  expand.pullUp(8,HIGH);
  expand.pullUp(9,HIGH);
  expand.pullUp(10,HIGH);
  expand.pullUp(11,HIGH); 
  expand.pullUp(12,HIGH);
  expand.pullUp(13,HIGH);
  expand.pullUp(14,HIGH);
  expand.pullUp(15,HIGH);

  // Get node ID from MCP79412 EUI-64 node address
  byte nodeHex[8] = "";
  char nodeHexStr[17] = "";
  rtc.getMacAddress(nodeHex);
  array_to_string(nodeHex, 8, nodeHexStr);
  nodeID = String(nodeHexStr);
  return result;

}

// check i2c devices with i2c names at i2c address of length i2c length returned in i2cExists
bool IoTNode::ok()
{
  // "RTC MCP79412",
  // "Expander MCP23018",
  // "RTC EEPROM",
  // "ADC MCP3221",
  // "FRAM M85RC256V",
  byte i2cAd[]=
  {
      0x6F, //111
      0x20, //32
      0x57, //87
      0x4D, //77
      0x50 //80
  };
  if (!Wire.isEnabled())
  {
    Wire.begin();
  }
  byte error, address;
  bool result = true;
  for (int i=0; i<5; ++i)
  {
    // The i2c_scanner uses the return value of
    // the Write.endTransmisstion to see if
    // a device did acknowledge to the address.
    address = i2cAd[i];
    Wire.beginTransmission(address);
    error = Wire.endTransmission();

    // Try again if there is an error
    if (!error==0)
    {
      Wire.reset();
      Wire.beginTransmission(address);
      error = Wire.endTransmission();
    }
 
    // Return false if there is an error
    if (!error == 0)
    {
      result = false;
      break;
    }
  }
  return result;
}




// Uses the MCP23018 expander to enable or disable the specified power regulator
// i.e. 3V3EXT_EN high for the 3.3V external switched power to pin 3V3_EXT_SW
// State is HIGH or LOW (true or false)
// valid powerNames are INT5V, INT12V, EXT3V3, EXT5V, EXT12V
// Note that v1.1 of the Sentient Things IoT Node includes onboard regulators
// for EXT3V3 and EXT5V
void IoTNode::setPowerON(powerName pwrName, bool state)
{
  expand.digitalWrite(pwrName, state);
  //i.e. expand.digitalWrite(3, state);
}

// Uses the MCP23018 expander to enable or disable the specified power regulator
// i.e. 3V3EXT_EN high for the 3.3V external switched power to pin 3V3_EXT_SW
// State is HIGH or LOW (true or false)
// valid powerNames are INT5V, INT12V, EXT3V3, EXT5V, EXT12V
// Note that v1.1 of the Sentient Things IoT Node includes onboard regulators
// for EXT3V3 and EXT5V
void IoTNode::setPower(powerName pwrName, bool state)
{
  expand.digitalWrite(pwrName, state);
}

// Uses the MCP23018 expander to enable high the specified power regulator
// i.e. 3V3EXT_EN high for the 3.3V external switched power to pin 3V3_EXT_SW
// State is HIGH (true)
// valid powerNames are INT5V, INT12V, EXT3V3, EXT5V, EXT12V
// Note that v1.1 of the Sentient Things IoT Node includes onboard regulators
// for EXT3V3 and EXT5V
void IoTNode::powerON(powerName pwrName)
{
  expand.digitalWrite(pwrName, true);
}

// Uses the MCP23018 expander to enable low the specified power regulator
// State is LOW (false)
// Note that v1.1 of the Sentient Things IoT Node includes onboard regulators
// for EXT3V3 and EXT5V
void IoTNode::powerOFF(powerName pwrName)
{
  expand.digitalWrite(pwrName, false);
}

// Uses the MCP23018 expander to enable high all the power regulator enable pins
// State is HIGH (true)
// Note that v1.1 of the Sentient Things IoT Node includes onboard regulators
// for EXT3V3 and EXT5V
void IoTNode::allPowerON()
{
  // INT5V, INT12V, EXT3V3, EXT5V, EXT12V
  expand.digitalWrite(EXT3V3, true);
  expand.digitalWrite(EXT5V, true);
  expand.digitalWrite(EXT12V, true);
  expand.digitalWrite(INT5V, true);
  expand.digitalWrite(INT12V, true);
}

// Uses the MCP23018 expander to enable low all the power regulator enable pins
// State is LOW (false)
// Note that v1.1 of the Sentient Things IoT Node includes onboard regulators
// for EXT3V3 and EXT5V
void IoTNode::allPowerOFF()
{
  // INT5V, INT12V, EXT3V3, EXT5V, EXT12V
  expand.digitalWrite(EXT3V3, false);
  expand.digitalWrite(EXT5V, false);
  expand.digitalWrite(EXT12V, false);
  expand.digitalWrite(INT5V, false);
  expand.digitalWrite(INT12V, false);
}

// Powers off the IoT Node board using the RTC
// RTC CONTROL switch must be set to Yes
void IoTNode::switchOffFor(long seconds, maskValue mask)
{
  // Set the RTC high so that the power stays on until the alarm is enabled
  rtc.outHigh();
  // Disable both alarms
  rtc.disableAlarms();
  // Set the timer mask
  rtc.maskAlarm0(mask);
  // Set the alarm polarization high - i.e. when alarm sets switch back on
  rtc.setAlarm0PolHigh();
  rtc.clearIntAlarm0();
  // Load the alarm match value (all registers)      
  int rtcnow = rtc.rtcNow();
  int alarmTime = rtcnow + seconds;
  rtc.setAlarm0UnixTime(alarmTime);
  // Enable the alarm. This will set the MFP output low
  // turning off the power until the alarm triggers
  // turning on the power again and starting the code from
  // the beginning.
  rtc.enableClock();
  // Ensure the ALMxIF flag is cleared
  rtc.enableAlarm0();
  Wire.end();
  delay(200);  
}

// Powers off the IoT Node board using the MCP49412 real time clock MFP output
// and on-board power switching circuitry
// RTC CONTROL switch must be set to Yes
void IoTNode::switchOffFor(long seconds)
{
  // Set the RTC high so that the power stays on until the alarm is enabled
  rtc.outHigh();
  // Disable both alarms
  rtc.disableAlarms();
  // Set the timer mask
  rtc.maskAlarm0(ALL);
  // Set the alarm polarization high - i.e. when alarm sets switch back on
  rtc.setAlarm0PolHigh();
  rtc.clearIntAlarm0();
  // Load the alarm match value (all registers)      
  int rtcnow = rtc.rtcNow();
  int alarmTime = rtcnow + seconds;
  rtc.setAlarm0UnixTime(alarmTime);
  // Enable the alarm. This will set the MFP output low
  // turning off the power until the alarm triggers
  // turning on the power again and starting the code from
  // the beginning.
  rtc.enableClock();
  // Ensure the ALMxIF flag is cleared
  rtc.enableAlarm0();
  Wire.end();
  delay(200);  
}


// Sets a high or low pullup the GPIO pin on the RJ45 connectors
// Uses the MCP23018 expander
// GIO1 is the GPIO pin (labled IO) on the RJ45 I/O-1 connector
// GIO2 is the GPIO pin (labled IO) on the RJ45 I/O-2 connector
// GIO3 is the GPIO pin (labled IO) on the RJ45 I/O-3 connector
void IoTNode::setPullUp(gioName ioName, bool state)
    {
        expand.pullUp(ioName, (uint8_t)state);
    }

// Enables or disables the GPIO pin on the RJ45 connectors
// Uses the MCP23018 expander
// GIO1 is the GPIO pin (labled IO) on the RJ45 I/O-1 connector
// GIO2 is the GPIO pin (labled IO) on the RJ45 I/O-2 connector
// GIO3 is the GPIO pin (labled IO) on the RJ45 I/O-3 connector
void IoTNode::digitalWrite(gioName ioName, bool state)
{
  expand.pinMode(ioName,OUTPUT);
  expand.digitalWrite(ioName, state);
}

// Reads the GPIO pin on the RJ45 connectors
// Uses the MCP23018 expander
// GIO1 is the GPIO pin (labled IO) on the RJ45 I/O-1 connector
// GIO2 is the GPIO pin (labled IO) on the RJ45 I/O-2 connector
// GIO3 is the GPIO pin (labled IO) on the RJ45 I/O-3 connector
bool IoTNode::digitalRead(gioName ioName)
{
  expand.pinMode(ioName,INPUT);
  if(expand.digitalRead(ioName)==0)
  {
    return true;
  }
  else
  {
    return false;
  }
}

// Sends a pulse to the analog watchdog timer TPL5010
// to prevent it from resetting the board
// Note that the WATCHDOG switch must be set to On
// for the watchdog reset to be connected
// See documentation for setting up the watchdog delay
// using the dip switch on the IoT Node board
void IoTNode::tickleWatchdog()
{
  expand.digitalWrite(5,true);
  //delayMicroseconds(100);
  delay(50);
  expand.digitalWrite(5,false);
}

bool IoTNode::isLiPoPowered()
{
// uint8_t digitalRead(uint8_t p);
  if(expand.digitalRead(10)==0)
  {
    return true;
  }
  else
  {
    return false;
  }
}

bool IoTNode::is3AAPowered()
{
// uint8_t digitalRead(uint8_t p);
  if(expand.digitalRead(10)==1)
  {
    return true;
  }
  else
  {
    return false;
  }
}

bool IoTNode::isLiPoCharged()
{
// uint8_t digitalRead(uint8_t p);
  if(expand.digitalRead(8)==0)
  {
    return true;
  }
  else
  {
    return false;
  }
}

bool IoTNode::isLiPoCharging()
{
// uint8_t digitalRead(uint8_t p);
  if(expand.digitalRead(9)==0)
  {
    return true;
  }
  else
  {
    return false;
  }
}

float IoTNode::voltage()
{
    unsigned int rawVoltage = 0;
    float voltage = 0.0;
    Wire.requestFrom(0x4D, 2);
    if (Wire.available() == 2)
    {
      rawVoltage = (Wire.read() << 8) | (Wire.read());
      voltage = (float)(rawVoltage)/4096.0*13.64; // 3.3*(4.7+1.5)/1.5
    }
    return voltage;
}


uint32_t IoTNode::unixTime()
{
  return rtc.rtcNow();
}

void IoTNode::setUnixTime(uint32_t unixtime)
{
  rtc.setUnixTime(unixtime);
}


bool IoTNode::backupFRAMtoSD(String filename)
{
  if (!SD.begin(N_D0)) {
    //Serial.println("initialization failed!");
    return false;
  }

  framResult res = myFram.begin();

	if ( res == framBadResponse)
	{
		//Serial.println(F("FRAM response not OK"));
    return false;
	}
	else
	{
		//Serial.println(F("FRAM response OK"));
		//Serial.println();
    
		framResult myResult = framUnknownError;
		unsigned int myBufferSize = myFram.getMaxBufferSize();
		unsigned long myBottom = myFram.getBottomAddress();
		unsigned long myTop = myFram.getTopAddress();

		byte framBuffer[myBufferSize];

    // Write FRAM to file
    myFile = SD.open(filename, FILE_WRITE);

    // if the file opened okay, write to it:
    if (myFile) {
      //Serial.print("Writing to " + filename);
      myFile.seek(0);
      for (unsigned long i = 0; i < myTop; i += myBufferSize)
        {
          readFRAM(i, myBufferSize, framBuffer);
          myFile.write(framBuffer,myBufferSize);
        }
      myFile.close();
      //Serial.println(" done.");
      return true;
    } else {
      // if the file didn't open, print an error:
      //Serial.println("error opening " + filename);
      return false;
    }

  }
}

bool IoTNode::restoreFRAMfromSD(String filename)
{
  if (!SD.begin(N_D0)) {
  //Serial.println("SD initialization failed!");
  return false;
  }

  framResult res = myFram.begin();

	if ( res == framBadResponse)
	{
		//Serial.println(F("FRAM response not OK"));
    return false;
	}
	else
	{
		//Serial.println(F("FRAM response OK"));
		//Serial.println();
    
		framResult myResult = framUnknownError;
		unsigned int myBufferSize = myFram.getMaxBufferSize();
		unsigned long myBottom = myFram.getBottomAddress();
		unsigned long myTop = myFram.getTopAddress();

		byte framBuffer[myBufferSize];

    // Write file to FRAM
    myFile = SD.open(filename, FILE_WRITE);
    
    // if the file opened okay, read from it:
    if (myFile) {
      //Serial.print("Reading from " + filename);
      myFile.seek(0);
      for (unsigned long i = 0; i < myTop; i += myBufferSize)
        {
          myFile.read(framBuffer,myBufferSize);
          writeFRAM(i, myBufferSize, framBuffer);          
        }
      myFile.close();
      //Serial.println(" done.");
      return true;
    } else {
      // if the file didn't open, print an error:
      //Serial.println("error opening " + filename);
      return false;
    }

  }
}


// Private

void IoTNode::array_to_string(byte array[], unsigned int len, char buffer[])
{
    for (unsigned int i = 0; i < len; i++)
    {
        byte nib1 = (array[i] >> 4) & 0x0F;
        byte nib2 = (array[i] >> 0) & 0x0F;
        buffer[i*2+0] = nib1  < 0xA ? '0' + nib1  : 'A' + nib1  - 0xA;
        buffer[i*2+1] = nib2  < 0xA ? '0' + nib2  : 'A' + nib2  - 0xA;
    }
    buffer[len*2] = '\0';
}

void IoTNode::writeFRAM(uint32_t startaddress, uint8_t numberOfBytes, uint8_t *buffer)
{
  	// Write in 32 byte blocks due to wire limit
	  const uint8_t blockSize = 30;
	  byte* buf = buffer;
	  uint32_t address = startaddress;

	  while (numberOfBytes >= blockSize)
	  {
			myFram._writeMemory(address, blockSize, buf);
		  address += blockSize;
			buf += blockSize;
		  numberOfBytes -= blockSize;
	  }
	  if (numberOfBytes > 0)
	  {
	    myFram._writeMemory(address, numberOfBytes, buf);
	  }
}

void IoTNode::readFRAM(uint32_t startaddress, uint8_t numberOfBytes, uint8_t *buffer)
{
  // Read in 30 byte blocks due to wire requestFrom() limit
  const uint8_t blockSize = 30;
  byte* buf = buffer;
  uint32_t address = startaddress;

  while (numberOfBytes >= blockSize)
  {
	myFram._readMemory(address, blockSize, buf);
	  address += blockSize;
		buf += blockSize;
	  numberOfBytes -= blockSize;
  }
  if (numberOfBytes > 0)
  {
    myFram._readMemory(address, numberOfBytes, buf);
  }
}

//////////////////

// Fram Array Constructor
framArray::framArray(FramI2C& fram, uint32_t numberOfElements, byte sizeOfElement, framResult& result):
  _numberOfElements(numberOfElements), _sizeOfElement(sizeOfElement), myFram(fram), myResult(result),
  myArray(fram, _numberOfElements, _sizeOfElement, result)
{

}

framArray IoTNode::makeFramArray(uint32_t numberOfElements, byte sizeOfElement)
{
  return framArray(myFram, numberOfElements,sizeOfElement, myResult);
}

bool framArray::write(uint32_t index, byte *buffer)
{
  framResult checkResult = framUnknownError;
  myArray.writeElement(index, buffer, checkResult);
  if (checkResult==framOK)
  {
    return true;
  }
  else
  {
    return false;
  }
  
}

bool framArray::read(uint32_t index, byte *buffer)
{
  framResult checkResult = framUnknownError;
  myArray.readElement(index, buffer, checkResult);
  if (checkResult==framOK)
  {
    return true;
  }
  else
  {
    return false;
  }
  
}


//////////////////

// Fram Ring Array Constructor
framRing::framRing(FramI2C& fram, uint32_t numberOfElements, byte sizeOfElement, framResult& result):
  _numberOfElements(numberOfElements), _sizeOfElement(sizeOfElement), myFram(fram), myResult(result),
  myRing(fram, _numberOfElements, _sizeOfElement, result)
{

}

framRing IoTNode::makeFramRing(uint32_t numberOfElements, byte sizeOfElement)
{
  return framRing(myFram, numberOfElements,sizeOfElement, myResult);
}

void framRing::initialize()
{
  myRing.initialize();
}


// Pop first element by default
bool framRing::pop(byte *buffer)
{
  return myRing.pop(buffer);
}

bool framRing::popLast(byte *buffer)
{
  return myRing.popLastElement(buffer);
}

bool framRing::peekFirst(byte *buffer)
{
  return myRing.peekFirstElement(buffer);
}

bool framRing::peekLast(byte *buffer)
{
  return myRing.peekLastElement(buffer);
}

// Circular buffer overwrites when full!
void framRing::push(byte *buffer)
{
  myRing.push(buffer);
}

void framRing::clearArray()
{
  myRing.clearArray();
}

bool framRing::isEmpty()
{
  return myRing.isEmpty();
}

bool framRing::isFull()
{
  return myRing.isFull();
}

 
