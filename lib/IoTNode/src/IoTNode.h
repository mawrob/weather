#ifndef IoTNode_h
#define IoTNode_h

#ifdef PARTICLE
  #include "Particle.h"
#else
  #include "arduino.h"
#endif //end of #ifdef PARTICLE


#include <Adafruit_MCP23017.h>
#include "MCP7941x.h"
#include "FramI2C.h"

// Globals defined here

// #define PLATFORM_SPARK_CORE                 0
// #define PLATFORM_SPARK_CORE_HD              2
// #define PLATFORM_GCC                        3
// #define PLATFORM_PHOTON_DEV                 4
// #define PLATFORM_TEACUP_PIGTAIL_DEV         5
// #define PLATFORM_PHOTON_PRODUCTION          6
// #define PLATFORM_TEACUP_PIGTAIL_PRODUCTION  7
// #define PLATFORM_P1                         8
// #define PLATFORM_ETHERNET_PROTO             9
// #define PLATFORM_ELECTRON_PRODUCTION        10
// #define PLATFORM_ARGON						           12
// #define PLATFORM_BORON						           13
// #define PLATFORM_XENON						           14
// #define PLATFORM_NEWHAL                     60000

//Photon
#if PLATFORM_ID==6 || PLATFORM_ID==8
#define N_SDA0 SDA
#define N_SCL0 SCL
#define N_D0 D2
#define GIOA D2
#define N_D1 D3
#define GIOB D3
#define N_D2 D4
#define GIOC D4
#define N_D3 D5
#define GIOD D5
#define N_D4 D6
#define GIOE D6
#define N_D5 D7
#define GIOF D7
#define N_D6 DAC
#define GIOG DAC
#define N_A2 A0
#define N_A3 A1
#define N_SCK SCK
#define N_MOSI MOSI
#define N_MISO MISO
#define N_RX0 RX
#define N_TX0 TX
#endif

//Electron
#if PLATFORM_ID==10
#define N_SDA0 SDA
#define N_SCL0 SCL
#define N_D0 D2
#define GIOA D2
#define N_D1 D3
#define GIOB D3
#define N_D2 D4
#define GIOC D4
#define N_D3 D5
#define GIOD D5
#define N_D4 D6
#define GIOE D6
#define N_D5 D7
#define GIOF D7
#define N_D6 DAC
#define GIOG DAC
#define N_A0 B4
#define N_A1 B5
#define N_A2 A0
#define N_A3 A1
#define N_SCK SCK
#define N_MOSI MOSI
#define N_MISO MISO
#define N_RX0 RX
#define N_TX0 TX
#define N_RX1 UART4_RX
#define N_TX1 UART4_TX
#define CANRX CAN1_RX
#define CANTX CAN1_TX
#endif

//Mesh
#if PLATFORM_ID==12 || PLATFORM_ID==13 || PLATFORM_ID==14
#define N_SDA0 SDA
#define N_SCL0 SCL
#define N_D0 D2
#define GIOA D2
#define N_D1 D3
#define GIOB D3
#define N_D2 D4
#define GIOC D4
#define N_D3 D5
#define GIOD D5
#define N_D4 D6
#define GIOE D6
#define N_D5 D7
#define GIOF D7
#define N_D6 D8
#define GIOG D8
#define N_A0 A0
#define N_A1 A1
#define N_A2 A2
#define N_A3 A3
#define N_A4 A4
#define N_A5 A5
#define N_SCK SCK
#define N_MOSI MOSI
#define N_MISO MISO
#define N_RX0 RX
#define N_TX0 TX
#endif

/**
 * @brief v1.1 of the Sentient Things IoT Node includes EXT3V3 and EXT5V regulators only.
 * Enable pins are set for all
 * 
 */
enum powerName {INT5V, INT12V, EXT3V3, EXT5V, EXT12V};

// These are the GPIO pins for the RJ45 connectors when solder jumpered to digital
// these connect to the MCP23018 but can be jumpered to N_A0, N_A1 and N_A2
// See IoT Node schematic
enum gioName {GIO1=11, GIO2, GIO3};

/**
 * @brief The framArray class is used to create arrays of elements in Fram.
 * The library manages the location of the array in Fram.
 * Elements may be the standard variables as well as structs and arrays
 * The number of elements and the size of each element is passed to the contructor.
 * framResult returns the success - see FramI2C.h.
 * 
 */
class framArray
{
  public:
  /**
   * @brief Construct a new fram Array object
   * 
   * @param fram is the fram instance that is being used - see the FramI2C class
   * @param numberOfElements is the number of elements in the array
   * @param sizeOfElement is the size of one element in bytes - use sizeof(element)
   * @param result is an enum defining the success:
   * enum framResult
   * {
   * framOK = 0,
   * framBadStartAddress,
   * framBadNumberOfBytes,
   * framBadFinishAddress,
   * framArrayElementTooBig,
   * framBadArrayIndex,
   * framBadArrayStartAddress,
   * framBadResponse,
   * framPartNumberMismatch,
   * framUnknownError = 99
   * };
   */
  framArray(FramI2C& fram, uint32_t numberOfElements, byte sizeOfElement, framResult& result);

  /**
   * @brief write an element to an array.
   * Not normally used directly see makeFramArray.
   * 
   * @param index is the index of the array and will fail if out of bounds
   * @param buffer is a pointer to the element - i.e. (uin8_t*)&element
   * @return true is the write was successful
   * @return false if the write was not successful
   */
  bool write(uint32_t index, byte *buffer);

  /**
   * @brief read an element to an array.
   * 
   * @param index is the index of the array and will fail if out of bounds
   * @param buffer is a pointer to the element - i.e. (uin8_t*)&element
   * @return true is the read was successful
   * @return false if the read was not successful
   */  
  bool read(uint32_t index, byte *buffer);
  
  private:
  uint32_t _numberOfElements;
  byte _sizeOfElement;
  FramI2C& myFram;
  framResult& myResult;
  FramI2CArray myArray;
};

class framRing
{
  public:

  /**
   * @brief Construct a new framRing Array object.  The framRing keeps track of the
   * ring pointers in Fram so that the ring can be used between power off cycles.
   * Not normally used directly see makeFramRing.
   * 
   * @param fram is the fram instance that is being used - see the FramI2C class
   * @param numberOfElements is the number of elements in the array
   * @param sizeOfElement is the size of one element in bytes - use sizeof(element)
   * @param result is an enum defining the success:
   * enum framResult
   * {
   * framOK = 0,
   * framBadStartAddress,
   * framBadNumberOfBytes,
   * framBadFinishAddress,
   * framArrayElementTooBig,
   * framBadArrayIndex,
   * framBadArrayStartAddress,
   * framBadResponse,
   * framPartNumberMismatch,
   * framUnknownError = 99
   * };
   */  
  framRing(FramI2C& fram, uint32_t numberOfElements, byte sizeOfElement, framResult& result);

	/**
	 * @brief pop the oldest element off the ring
	 * 
   * @param buffer is a pointer to the element - i.e. (uin8_t*)&element
   * @return true is the pop was successful
   * @return false if the pop was not successful
	 */
  bool pop(byte *buffer);

  /**
   * @brief pop the last (newest) element off the ring
   * 
   * @param buffer is a pointer to the element - i.e. (uin8_t*)&element
   * @return true is the pop was successful
   * @return false if the pop was not successful
	 */
  bool popLast(byte *buffer);

  /**
   * @brief peek (do not remove) the first (oldest) element on the ring
   * 
   * @param buffer is a pointer to the element - i.e. (uin8_t*)&element
   * @return true is the pop was successful
   * @return false if the pop was not successful
	 */
  bool peekFirst(byte *buffer);

  
    /**
   * @brief peek (do not remove) the last (newest) element on the ring
   * 
   * @param buffer is a pointer to the element - i.e. (uin8_t*)&element
   * @return true is the pop was successful
   * @return false if the pop was not successful
	 */
  bool peekLast(byte *buffer);
  
	/**
	 * @brief push an element onto the ring. OVERWRITE if full.
	 * 
   * @param buffer is a pointer to the element - i.e. (uin8_t*)&element
   * @return true is the push was successful
   * @return false if the push was not successful
	 */	
  void push(byte *buffer);

  /**
   * @brief clear the ring array with 0 values and reset the pointers to the beginning
   * 
   */
	void clearArray();

  /**
   * @brief check to see if the ring array is empty
   * 
   * @return true if empty
   * @return false if not empty
   */
	bool isEmpty();

  /**
   * @brief check to see if the ring array is full
   * 
   * @return true if full
   * @return false if not full
   */
	bool isFull();

  /**
   * @brief initializes the ring by loading the saved pointers.
   * Must be run (in setup) before using the ring
   * 
   */
  void initialize();
  
  private:
  uint32_t _numberOfElements;
  byte _sizeOfElement;
  FramI2C& myFram;
  framResult& myResult;
  Ring_FramArray myRing;
};

/**
 * @brief main IoT Node class.
 * Includes functions to manage external power. Read the state of the battery charger.
 * Send a "tickle" to reset the watchdog timer and control GPIO on the IO headers. 
 * 
 */
class IoTNode
{
  public:

  /**
   * @brief Construct a new Io T Node object
   * 
   */
  IoTNode();

  /**
   * @brief start the IoT Node.
   * Sets the state of the internal MCP23018 expander.
   * Reads the node mac address stored in the MCP79412
   * real time clock. Starts (Wire) I2C if needed.
   * 
   */
  void begin();

  /**
   * @brief checks to see if the IoT Node is working correctly
   * by checking to make sure that the integrated I2C parts
   * (MCP23018 expander, MCP79412 RTCC, MB85RC256V FRAM & MCP3221 ADC)
   * respond.
   * 
   * @return true if all I2C devices respond
   * @return false if any one of the I2C devices does not respond
   */
  bool ok();

  /**
   * @brief Switch the external (or internal if added) power on or off
   * Controls the enable pins on the IoT Node - see pwrName
   * The IoT Node includes on-board regulators for external 3.3V (EXT3V3) and
   * external 5V (EXT5V)
   * 
   * @param pwrName - one off INT5V, INT12V, EXT3V3, EXT5V, EXT12V
   * @param state - true or false, HIGH or LOW
   */
  void setPowerON(powerName pwrName, bool state);

   /**
   * @brief Switch the external (or internal if added) power on or off
   * Controls the enable pins on the IoT Node - see pwrName
   * The IoT Node includes on-board regulators for external 3.3V (EXT3V3) and
   * external 5V (EXT5V)
   * 
   * @param pwrName - one off INT5V, INT12V, EXT3V3, EXT5V, EXT12V
   * @param state - true or false, HIGH or LOW
   */ 
  void setPower(powerName pwrName, bool state);

   /**
   * @brief Switch ON the external (or internal if added) power
   * Controls the enable pins on the IoT Node - see pwrName
   * The IoT Node includes on-board regulators for external 3.3V (EXT3V3) and
   * external 5V (EXT5V)
   * 
   * @param pwrName - one off INT5V, INT12V, EXT3V3, EXT5V, EXT12V
   */
  void powerON(powerName pwrName);

   /**
   * @brief Switch OFF the external (or internal if added) power
   * Controls the enable pins on the IoT Node - see pwrName
   * The IoT Node includes on-board regulators for external 3.3V (EXT3V3) and
   * external 5V (EXT5V)
   * 
   * @param pwrName - one off INT5V, INT12V, EXT3V3, EXT5V, EXT12V
   */
  void powerOFF(powerName pwrName);

   /**
   * @brief Switch ON the external (or internal if added) power
   * Controls the enable pins on the IoT Node - see pwrName
   * The IoT Node includes on-board regulators for external 3.3V (EXT3V3) and
   * external 5V (EXT5V)
   */  
  void allPowerON();

   /**
   * @brief Switch OFF the external (or internal if added) power
   * Controls the enable pins on the IoT Node - see pwrName
   * The IoT Node includes on-board regulators for external 3.3V (EXT3V3) and
   * external 5V (EXT5V)
   */    
  void allPowerOFF();

  /**
   * @brief use the internal real time clock to switch off the IoT Node power
   * The IoT Node "RTC CONTROL" switch must be set to "Yes" for this to work.
   * Note that the sleep times is in seconds.  The internal clock adds the sleep
   * seconds to the current date and time and stores the new wake up
   * date and time in memory.
   * 
   * NOTE:
   * The mask defines what the clock checks to wake up (switch back on) the
   * IoT Node.  If ALL is selected then when the Seconds, Minutes, Hours,
   * Day of Week, Date, and Month match the new wake up date and time in memory
   * then the clock will wake up the node.
   * 
   * If SEC (Seconds) is selected then when the time seconds next match then
   * the node will wake up.
   * 
   * Similarly if WKDAY (day of week) is selected then the time day-of-week
   * next matches then the node will wake up.
   * 
   * @param seconds time in seconds to switch off the power
   * @param mask - enum value one of SEC, MIN, HOUR, WKDAY, DATE, ALL 
   */
  void switchOffFor(long seconds, maskValue mask);

  /**
   * @brief use the internal real time clock to switch off the IoT Node power
   * The IoT Node "RTC CONTROL" switch must be set to "Yes" for this to work.
   * Note that the sleep times is in seconds.  The internal clock adds the sleep
   * seconds to the current date and time and stores the new wake up
   * date and time in memory.
   * 
   * @param seconds time in seconds to switch off the power
   */
  void switchOffFor(long seconds);

  /**
   * @brief Set the selected I/O connector GPIO pin pullup
   * on the IoT Node.  See the IoT Node documentation
   * for pin configurations of the RJ45 connectors (not ethernet)
   * Uses the internal MCP23018 expander.
   * 
   * NOTE: Solder jumpers on the bottom of the IoT Node may be
   * changed to connect GIOx pins to analog pins
   * on the Feather footprint.  See the IoT Node schematic
   * for details.
   * 
   * @param ioName one of GIO1, GIO2, GIO3
   * @param state true or HIGH (pulled up) false or LOW (not pulled up)
   */
  void setPullUp(gioName ioName, bool state);

  /**
   * @brief Set the selected I/O connector GPIO pin high or low
   * See the IoT Node documentation for pin configurations
   * of the RJ45 connectors (not ethernet)
   * Uses the internal MCP23018 expander.
   * 
   * @param ioName one of GIO1, GIO2, GIO3
   * @param state true or HIGH equals 3.3V, false or LOW equals 0V
   */
  void digitalWrite(gioName ioName, bool state);

  /**
   * @brief read the input state of the selected connector GPIO pin
   * See the IoT Node documentation for pin configurations
   * of the RJ45 connectors (not ethernet)
   * Uses the internal MCP23018 expander.
   * 
   * @param ioName ioName one of GIO1, GIO2, GIO3
   * @return true if low or 0V
   * @return false if high or 3.3V
   */
  bool digitalRead(gioName ioName);

  /**
   * @brief send a short pulse to the analog watchdog timer to stop it resetting the node.
   * The analog watchdog timer may be set using slider and dip switches to pull the
   * reset line on the IoT Node low to reset the device that is plugged into the node. The reset period
   * between 11 seconds and 2 hours is selected by a dip switch (see documentation).  
   * The tickleWatchdog sends a short pulse to stop the reset from happening 
   * and may be used to reset a devices if the pulse does not occur.
   * 
   * 
   * Typically used to reset hung software.
   * 
   */
  void tickleWatchdog();

  /**
   * @brief checks the state of the IoT Node to see if a LiPo battery is powering the node.
   * Uses the MCP23018 expander to read the digital state pins on the TPS2113 switch.
   * The TPS2113 automatically switches between the LiPo and the 3AA/A power source.
   * The TPS2113 switches to the 3AA (if connected) when the LiPo runs flat.
   * 
   * @return true if a LiPo is powering the node
   * @return false if a LiPo is not powering the node
   */
  bool isLiPoPowered();

  /**
   * @brief checks the state of the IoT Node to see if a 3AA/A battery is powering the node.
   * Uses the MCP23018 expander to read the digital state pins on the TPS2113 switch.
   * The TPS2113 automatically switches between the LiPo and the 3AA/A power source.
   * The TPS2113 switches to the 3AA (if connected) when the LiPo runs flat.
   * 
   * @return true if a 3AA is powering the node
   * @return false if a 3AA is not powering the node
   */
  bool is3AAPowered();

  /**
   * @brief checks the state of the CN3065 LiPo charger DONE output pin.
   * Uses the MCP23018 expander to check the pin.
   * 
   * @return true if the LiPo is fully charged
   * @return false if the LiPo is not fully charged
   */
  bool isLiPoCharged();

  /**
   * @brief checks the state of the CN3065 LiPo charger CHRG output pin.
   * Uses the MCP23018 expander to check the pin.
   * 
   * @return true if the LiPo is charging
   * @return false if the LiPo is not charging
   */
  bool isLiPoCharging();

  /**
   * @brief measures the IoT Node input voltage.
   * The voltage is measured after the TPS2113 auto switching
   * power multiplexer.
   * 
   * @return float - the input voltage in volts
   */
  float voltage();

  /**
   * @brief returns the real time clock time in epoch or unix time.
   * 
   * @return uint32_t unix time in seconds
   */
  uint32_t unixTime();

  /**
   * @brief Set the real time clock unix time in seconds
   * 
   * @param unixtime the unix time in seconds
   */
  void setUnixTime(uint32_t unixtime);

  /**
   * @brief the IoT Node mac address as a string.
   * This is the value of the MCP79412 real time clock.
   * MCP79412 includes an EUI-64 node address pre-programmed
   * into the protected EEPROM block that may be used as a unique
   * IoT Node address.  The value is read as part of begin().
   * 
   */
  String nodeID;

  /**
   * @brief create a ring array of elements in Fram
   * The function keeps track of the ring array pointers.
   * IoT Node includes a 256 kbits MB85RC256V I2C Fram
   * Requires initialize() to be run prior to use.
   * i.e.
   * // Create FRAM ring of 10 integers
   * int values;
   * framRing framringtest = node.makeFramRing(10, sizeof(values));
   * 
   * 
   * @param numberOfElements in the ring array
   * @param sizeOfElement is the size of an element (use sizeof())
   * @return framRing a ring object in on-board fram
   */
  framRing makeFramRing(uint32_t numberOfElements, byte sizeOfElement);

  /**
   * @brief create an array of elements in Fram
   * The function keeps track of the ring array pointers.
   * IoT Node includes a 256 kbits MB85RC256V I2C Fram
   * i.e.
   * struct Status
   * {
   *  int anumber;
   *  int runCount;
   *  uint32_t lastUnixTime;
   * };
   * // Create an array of three struct elements
   * framArray framarraytest = node.makeFramArray(3, sizeof(testStatus));
   * 
   * @param numberOfElements in the ring array
   * @param sizeOfElement is the size of an element (use sizeof())
   * @return framRing a ring object in on-board fram
   */
  framArray makeFramArray(uint32_t numberOfElements, byte sizeOfElement);

  /**
   * @brief returns framResult enum
   * 
   * enum framResult
   * {
   * 	framOK = 0,
   * 	framBadStartAddress,
   * 	framBadNumberOfBytes,
   * 	framBadFinishAddress,
   * 	framArrayElementTooBig,
   * 	framBadArrayIndex,
   * 	framBadArrayStartAddress,
   * 	framBadResponse,
   * 	framPartNumberMismatch,
   * 	framUnknownError = 99
   * };
   * 
   */
  framResult myResult = framUnknownError; 

  private:
  void array_to_string(byte array[], unsigned int len, char buffer[]);
  FramI2C myFram;
};

#endif
