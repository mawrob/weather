
/*
 * Project Sentient Things IoT Node weather station
 * Description: Weather station and light level monitor. Measures Weather
 * and light level data and sends the results to Thingspeak.com
 * Author: Robert Mawrey, Sentient Things, Inc.
 * Date: December 2019
 */
#include "Particle.h"
#include "WeatherSensors.h"
#include <thingspeak-webhooks.h>
#include <ArduinoJson.h>
#include "IoTNode.h"
#include "SdCardLogHandlerRK.h"

#define SERIAL_DEBUG

#ifdef SERIAL_DEBUG
  #define DEBUG_PRINT(...) Serial.print(__VA_ARGS__)
  #define DEBUG_PRINTLN(...) Serial.println(__VA_ARGS__)
#else
  #define DEBUG_PRINT(...)
  #define DEBUG_PRINTLN(...)
#endif

const int SD_CHIP_SELECT = N_D0;
SdFat sd;
SdCardPrintHandler printToSd(sd, SD_CHIP_SELECT, SPI_FULL_SPEED);

STARTUP(printToSd.withMaxFilesToKeep(3000));

#define SENSOR_SEND_TIME_MS 60000
#define SENSOR_POLL_TIME_MS 2000

#define IOTDEBUG

LEDStatus fadeRed(RGB_COLOR_RED, LED_PATTERN_FADE, LED_SPEED_NORMAL, LED_PRIORITY_IMPORTANT);

const int firmwareVersion = 0;

SYSTEM_THREAD(ENABLED);

// Using SEMI_AUTOMATIC mode to get the lowest possible data usage by
// registering functions and variables BEFORE connecting to the cloud.
//SYSTEM_MODE(SEMI_AUTOMATIC);

// Use structs defined in WeatherSensors.h
sensorReadings_t sensorReadings;
config_t config;

//********CHANGE BELOW AS NEEDED**************
// Set to true and enter TS channel ID and keys AND change firstRunTest to use an existing TS channel
// Set to false if you wish to create a new TS channel the first time the code runs
bool useManualTSChannel = false;
const char *manualTSWriteKey = "XXXXXXXXXXXXXXXX";
const char *manualTSReadKey = "XXXXXXXXXXXXXXXX";
const int manualTSChannel = 895141;
// Change this value to force hard reset and clearing of FRAM when Flashing
// You have to change this value (if you have flashed before) for the TS channel to change
const int firstRunTest = 1122124;
//********CHANGE ABOVE AS NEEDED**************


// ThingSpeak webhook keys. Do not edit.
char const* webhookKey[] = {
    "d", // "description=",
    "e", // "elevation=",
    "1", // "field1=",
    "2", // "field2=",
    "3", // "field3=",
    "4", // "field4=",
    "5", // "field5=",
    "6", // "field6=",
    "7", // "field7=",
    "8", // "field8=",
    "a", // "latitude=",
    "o", // "longitude=",
    "n", // "name=",
    "f", // "public_flag=",
    "t", // "tags=",
    "u", // "url=",
    "m", // "metadata=",
    // "api_key=",
    "end" //Do not remove or change
};

// ThingSpeak create Channel labels. Do not edit.
char const* chanLabels[] = {
    "description",
    "elevation",
    "field1",
    "field2",
    "field3",
    "field4",
    "field5",
    "field6",
    "field7",
    "field8",
    "latitude",
    "longitude",
    "name",
    "public_flag",
    "tags",
    "url",
    "metadata",
    "end" //Do not remove or change
};

//Sensor Names
char const* sensorNames[] = {
  "Weather station using the Sentient Things IoT Node",   // "description=",
  "",                       // "elevation=",
  "Wind Direction deg.",    // "field1=",
  "Wind Speed mph",         // "field2=",
  "Humidity %",             // "field3=",
  "Air Temperature F",      // "field4=",
  "Rainfall in.",           // "field5=",
  "Air Pressure in.",       // "field6=",
  "Battery Voltage V",      // "field7"
  "Light Intensity lux",    // "field8=",
  "",                       // "latitude=",
  "",                       // "longitude=",
  "Weather station",        // "name=",
  "false",                  // "public_flag=",
  "",                       // "tags=",
  "http://sentientthings.com",   // "url=",
  "",                       // "metadata=",
  "end"
};

// This is the index for the updateTSChan
int returnIndex;

byte messageSize = 1;

Timer pollSensorTimer(SENSOR_POLL_TIME_MS, capturePollSensors);

Timer sensorSendTimer(SENSOR_SEND_TIME_MS, getResetAndSendSensors);

Timer unpluggedTimer(5000,unplugged);

WeatherSensors sensors; //Interrupts for anemometer and rain bucket
// are set up here too

IoTNode node;

ThingSpeakWebhooks thingspeak;

// // Create FRAM array and ring
framArray framConfig = node.makeFramArray(1, sizeof(config));

framRing dataRing = node.makeFramRing(300, sizeof(sensorReadings));

bool readyToGetResetAndSendSensors = false;
bool readyToCapturePollSensors = false;
bool tickleWD = false;

unsigned long timeToNextSendMS;

String deviceStatus;
String i2cDevices;
bool resetDevice = false;

String i2cNames[] =
{
    "RTC",
    "Exp",
    "RTC EEPROM",
    "ADC",
    "FRAM",
    "AM2315",
    "MPL3115",
    "TSL2591"
};

byte i2cAddr[]=
{
    0x6F, //111
    0x20, //32
    0x57, //87
    0x4D, //77
    0x50, //80
    0x5C, //
    0x60,
    0x29
    
};

/* number of elements in `array` */
static const size_t i2cLength = sizeof(i2cAddr) / sizeof(i2cAddr[0]);

bool i2cExists[]=
{
  false,
  false,
  false,
  false,
  false,
  false,
  false,
  false
};

// check i2c devices with i2c names at i2c address of length i2c length returned in i2cExists
bool checkI2CDevices()
{
  byte error, address;
  bool result = true;
  for (size_t i; i<i2cLength; ++i)
  {
    // The i2c_scanner uses the return value of
    // the Write.endTransmisstion to see if
    // a device did acknowledge to the address.
    address = i2cAddr[i];

    Wire.beginTransmission(address);
    error = Wire.endTransmission();

    //Try again if !error=0
    if (!error==0)
    {
      delay(10);
      Wire.beginTransmission(address);
      error = Wire.endTransmission();
    }

    //Try reset if !error=0
    if (!error==0)
    {
      Wire.reset();
      Wire.beginTransmission(address);
      error = Wire.endTransmission();
    }
 
    if (error == 0)
    {
      DEBUG_PRINTLN(String("Device "+i2cNames[i]+ " at"+" address:0x"+String(address, HEX)));
      i2cExists[i]=true;
    }
    else
    {
      DEBUG_PRINTLN(String("Device "+i2cNames[i]+ " NOT at"+" address:0x"+String(address, HEX)));
      i2cExists[i]=false;
      result = false;
    }
  }
  return result;
}


void printI2C(int inx)
{
    for (int i=0; i<i2cLength; i++)
        {
          if (i2cAddr[i] == inx)
          {
              DEBUG_PRINTLN(String("Device "+i2cNames[i]+ " at"+" address:0x"+String(i2cAddr[i], HEX)));
          }
        }        
}

void scanI2C()
{
  byte error, address;
  int nDevices;
 
  DEBUG_PRINTLN("Scanning...");
  nDevices = 0;
  for(address = 1; address < 127; address++ )
  {
    // The i2c_scanner uses the return value of
    // the Write.endTransmisstion to see if
    // a device did acknowledge to the address.
    Wire.beginTransmission(address);
    error = Wire.endTransmission();
 
    if (error == 0)
    {
      printI2C(address);
 
      nDevices++;
    }
    else if (error==4)
    {
      DEBUG_PRINT("Unknown error at address 0x");
      if (address<16)
        DEBUG_PRINT("0");
      DEBUG_PRINTLN(address,HEX);
    }    
  }
  if (nDevices == 0)
    DEBUG_PRINTLN("No I2C devices found\n");
  else
    DEBUG_PRINTLN("done\n");
}

// Adding explicit connect routine that has to work before the rest of the code runs
void connect()
{
  #if Wiring_Cellular
  bool cellready=Cellular.ready();
  if (!cellready)
  {
    DEBUG_PRINTLN("Attempting to connect cellular...");
    Cellular.on();
    Cellular.connect();
    waitFor(Cellular.ready,180000);
    if (!Cellular.ready())
    {
    DEBUG_PRINTLN("Cellular not ready - resetting");
    delay(200);
    System.reset();
    }
  }
  else
  {
    DEBUG_PRINTLN("Cellular ready");
  }
  #endif
  
  #if Wiring_WiFi
  if (!WiFi.hasCredentials())
  {
    DEBUG_PRINTLN("Please add WiFi credentials");
    DEBUG_PRINTLN("Resetting in 60 seconds");
    delay(60000);
    System.reset();
  }
  bool wifiready=WiFi.ready();
  if (!wifiready)
  {
    DEBUG_PRINTLN("Attempting to connect to WiFi...");
    WiFi.on();
    WiFi.connect();
    waitFor(WiFi.ready,60000);
    if (!WiFi.ready())
    {
    DEBUG_PRINTLN("WiFi not ready - resetting");
    delay(200);
    System.reset();
    }
  }
  else
  {
    DEBUG_PRINTLN("Cellular ready");
  }
  #endif

  bool partconnected=Particle.connected();
  if (!partconnected)
  {
    DEBUG_PRINTLN("Attempting to connect to Particle...");
    Particle.connect();
    // Note: that conditions must be a function that takes a void argument function(void) with the () removed,
    // e.g. Particle.connected instead of Particle.connected().
    waitFor(Particle.connected,60000);
    if (!Particle.connected())
    {
      DEBUG_PRINTLN("Particle not connected - resetting");
      delay(200);
      System.reset();
    } 
  }
  else
  {
    DEBUG_PRINTLN("Particle connected");
  }
}

// setup() runs once, when the device is first turned on.
void setup() {
  // register cloudy things
  Particle.variable("version",firmwareVersion);
  Particle.variable("devicestatus",deviceStatus);

  // Subscribe to the TSBulkWriteCSV response event
  Particle.subscribe(System.deviceID() + "/hook-response/TSBulkWriteCSV", TSBulkWriteCSVHandler, MY_DEVICES);
  // Put a test in here for first run
  // Subscribe to the TSCreateChannel response event
  Particle.subscribe(System.deviceID() + "/hook-response/TSCreateChannel", TSCreateChannelHandler, MY_DEVICES);

  // etc...
  // then connect
  //Particle.connect();
  Serial.begin(115200);
  Serial1.begin(115200);

  node.begin();
  node.setPowerON(EXT3V3,true);
  node.setPowerON(EXT5V,true);

  #ifdef IOTDEBUG
  delay(5000);
 
  checkI2CDevices();
  scanI2C();
  #endif

    // Check for I2C devices again
  if (!node.ok())
  {
    #ifdef IOTDEBUG
    // Particle.publish("Unplugged","Plug the device into the IoT Node",PRIVATE);
    DEBUG_PRINTLN("Plug the device into the IoT Node");
    #endif
    deviceStatus="Device is not plugged into the IoTNode";
    fadeRed.setActive(true);
    DEBUG_PRINTLN("Resetting in 10 seconds");
    delay(10000);
    System.reset();
  }
  else
  {
    
    connect();
    framConfig.read(0, (uint8_t*)&config);

      // Code for initialization of the IoT Node
      // i.e. the first time the software runs
      // 1. A new ThingSpeak channel is created
      // 2. The channel id and keys are Saved
      // 3. a firstRunTest variable is saved in persistent memory as a flag to indicate
      // that the IoT node has been set up already.

    if (config.testCheck != firstRunTest)
    {
      // myFram.format();
      if (useManualTSChannel)
      {
        // Use the manually entered ThingSpeak channel and keys above
        config.channelId = manualTSChannel;
        strcpy(config.writeKey,manualTSWriteKey);
        strcpy(config.readKey,manualTSReadKey);
        config.testCheck = firstRunTest;
        /// Defaults
        config.particleTimeout = 20000;
        // Save to FRAM
        framConfig.write(0, (uint8_t*)&config);
        #ifdef IOTDEBUG
        Particle.publish("Updating to use channel number:",String(manualTSChannel),PRIVATE);
        DEBUG_PRINTLN(manualTSChannel);
        DEBUG_PRINTLN(manualTSWriteKey);
        DEBUG_PRINTLN(manualTSReadKey);
        #endif
      }
      else
      {
        // Create a new ThingSpeak Channel
        thingspeak.TSCreateChan(webhookKey,sensorNames, returnIndex);
        unsigned long webhookTime = millis();
        framConfig.read(0, (uint8_t*)&config);
        // Waits for TSCreateChannelHandler to run and set config.testCheck = firstRunTest
        while (config.testCheck != firstRunTest && millis()-webhookTime<60000)
        {
          #ifdef IOTDEBUG
          Particle.publish("Trying to create ThingSpeak channel",PRIVATE);
          DEBUG_PRINTLN("Trying to create ThingSpeak channel");
          #endif
          delay(5000);
        }
        System.reset();      
      }
      
    }
    else
    {
      #ifdef IOTDEBUG
      Particle.publish("Using previously created channel number",String(config.channelId),PRIVATE);
      DEBUG_PRINTLN("Using previously created channel number"+String(config.channelId));
      #endif
    }
      // end of first run code.

      if (syncRTC())
      {
        DEBUG_PRINTLN("RTC sync'ed with cloud");
      }
      else
      {
        DEBUG_PRINTLN("RTC not sync'ed with cloud");
      }
      // load pointers
      dataRing.initialize();
      sensors.begin();
      pollSensorTimer.start();
      sensorSendTimer.start();  
  }
}


void loop() {
  if (readyToGetResetAndSendSensors)
  {
    sensors.getAndResetAllSensors();
    String lastCsvData = "";
    messageSize = 1;
    if (!dataRing.isEmpty())
    {
      sensorReadings_t temporaryReadings;
      dataRing.peekLast((uint8_t*)&temporaryReadings);
      lastCsvData = "|" + sensors.sensorReadingsToCsvUS(temporaryReadings);
      messageSize = 2;
    }
    dataRing.push((uint8_t*)&sensorReadings);
    String currentCsvData = sensors.sensorReadingsToCsvUS();

    // Consider putting the SD logging in the IoTNode library
    printToSd.println(currentCsvData);

    String csvData = currentCsvData + lastCsvData;
    #ifdef IOTDEBUG
    DEBUG_PRINTLN();
    DEBUG_PRINTLN(csvData);
    DEBUG_PRINTLN(String(config.channelId));
    DEBUG_PRINTLN(String(config.writeKey));
    #endif
    String time_format = "absolute";
    if (waitFor(Particle.connected,config.particleTimeout)){
      // Update TSBulkWriteCSV later to use chars
      thingspeak.TSBulkWriteCSV(String(config.channelId), String(config.writeKey), time_format, csvData);
    }
    else
    {
      DEBUG_PRINTLN("Timeout");
    }
    readyToGetResetAndSendSensors = false;

    if (tickleWD)
    {
      node.tickleWatchdog();
      tickleWD = false;
    }

    readyToGetResetAndSendSensors = false;
    #ifdef IOTDEBUG
    DEBUG_PRINTLN("readyToGetResetAndSendSensors");
    #endif
    // Update status information
    deviceStatus = 
    String(config.channelId)+"|"+
    String(config.testCheck)+"|"+
    String(config.writeKey)+"|"+
    String(config.readKey)+"|"+
    String(config.unitType)+"|"+
    String(config.firmwareVersion)+"|"+
    String(config.particleTimeout)+"|"+
    String(config.latitude)+"|"+
    String(config.longitude)+"|"+
    i2cDevices;

  }

  if (readyToCapturePollSensors)
  {
    sensors.captureTempHumidityPressure();
    sensors.captureWindVane();
    sensors.captureLightLux();
    sensors.captureBatteryVoltage();
    readyToCapturePollSensors = false;
    #ifdef IOTDEBUG
    Particle.publish("Capturing sensors",PRIVATE);
    DEBUG_PRINTLN("capture");
    #endif
  }
  // If flag set then reset here
  if (resetDevice)
  {
    System.reset();
  }

}

void capturePollSensors()
{
  // Set the flag to poll the sensors
  readyToCapturePollSensors = true;
}

void getResetAndSendSensors()
{
  // Set the flag to read and send data.
  // Has to be done out of this Timer thread
  timeToNextSendMS = millis();
  readyToGetResetAndSendSensors = true;
}

void TSBulkWriteCSVHandler(const char *event, const char *data) {
  timeToNextSendMS = SENSOR_SEND_TIME_MS - (millis() - timeToNextSendMS);
  String resp = "true";
  if (resp.equals(String(data)))
  {
    sensorReadings_t temporaryReadings;
    if (messageSize == 2)
    {
      dataRing.popLast((uint8_t*)&temporaryReadings);
      dataRing.popLast((uint8_t*)&temporaryReadings);
    }
    else
    {
      dataRing.popLast((uint8_t*)&temporaryReadings);
    }
    #ifdef IOTDEBUG
    DEBUG_PRINTLN(data);
    #endif
  }
  tickleWD = true;
}

void TSCreateChannelHandler(const char *event, const char *data) {
  // Handle the TSCreateChannel response
  StaticJsonBuffer<256> jb;
  #ifdef IOTDEBUG
  DEBUG_PRINTLN(data);
  #endif
  //JsonObject& obj = jb.parseObject((char*)data);
  JsonObject& obj = jb.parseObject(data);
  if (obj.success()) {
      int channelId = obj["i"];
      const char* write_key = obj["w"];
      const char* read_key = obj["r"];
      // Copy to config
      config.channelId = channelId;
      strcpy(config.writeKey,write_key);
      strcpy(config.readKey,read_key);
      config.testCheck = firstRunTest;
      /// Defaults
      config.particleTimeout = 20000;
      // Save to FRAM
      framConfig.write(0, (uint8_t*)&config);
      #ifdef IOTDEBUG
      DEBUG_PRINTLN(channelId);
      DEBUG_PRINTLN(write_key);
      DEBUG_PRINTLN(read_key);
      #endif
      int len = sizeof(channelId)*8+1;
      char buf[len];
      char const* chan = itoa(channelId,buf,10);
      if (returnIndex!=-1)
      {
        thingspeak.updateTSChan(chan,sensorNames,chanLabels,returnIndex);
      }
      String chanTags = "weather, light, wind, temperature, humidity, pressure, Sentient Things," + System.deviceID();
      String lab = "tags";
      delay(1001);
      thingspeak.TSWriteOneSetting(channelId, chanTags, lab);
 
      ///
  } else {
      DEBUG_PRINTLN("Parse failed");
      Particle.publish("Parse failed",data,PRIVATE);
  }

}

bool syncRTC()
{
    uint32_t syncNow;
    bool sync = false;
    unsigned long syncTimer = millis();

    do
    {
      Particle.process();
      delay(100);
    } while (Time.now() < 1465823822 && millis()-syncTimer<500);

    if (Time.now() > 1465823822)
    {
        syncNow = Time.now();//put time into memory
        node.setUnixTime(syncNow);
        sync = true;
    }

    if (!sync)
    {
        #ifdef DEBUG
        Particle.publish("Time NOT synced",String(Time.format(syncNow, TIME_FORMAT_ISO8601_FULL)+"  "+Time.format(node.unixTime(), TIME_FORMAT_ISO8601_FULL)),PRIVATE);
        #endif
    }
    return sync;
}

void unplugged()
{
  #ifdef IOTDEBUG
  Particle.publish("Unplugged","Plug the device into the IoT Node",PRIVATE);
  DEBUG_PRINTLN("Plug the device into the IoT Node");
  #endif

}