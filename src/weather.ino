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

// // SdFat and SdCardLogHandler does not currently work with Particle Mesh devices
// ///------
// //#include <SdFat.h> // There is currently a bug in this version of SdFat for gen 3 Particle
// #include "SdFat.h"
// #include "SdCardLogHandlerRK.h" //Reverted to local version as public uses old SdFat.h

// // Consider adding SD and logging to IoTNode library

// const int SD_CHIP_SELECT = N_D0;
// SdFat sd;
// SdCardLogHandler logHandler(sd, SD_CHIP_SELECT, SPI_FULL_SPEED);

// STARTUP(logHandler.withNoSerialLogging().withMaxFilesToKeep(3000));
// ///------

#define SENSOR_SEND_TIME_MS 60000
#define SENSOR_POLL_TIME_MS 2000

#define IOTDEBUG

LEDStatus fadeRed(RGB_COLOR_RED, LED_PATTERN_FADE, LED_SPEED_NORMAL, LED_PRIORITY_IMPORTANT);

const int firmwareVersion = 0;

//SYSTEM_THREAD(ENABLED);

// Using SEMI_AUTOMATIC mode to get the lowest possible data usage by
// registering functions and variables BEFORE connecting to the cloud.
//SYSTEM_MODE(SEMI_AUTOMATIC);

// Use structs defined in WeatherSensors.h
sensorReadings_t sensorReadings;
config_t config;

//********CHANGE BELOW AS NEEDED**************
// Set to true and enter TS channel ID and keys AND change firstRunTest to use an existing TS channel
// Set to false if you wish to create a new TS channel the first time the code runs
bool useManualTSChannel = true;
const char *manualTSWriteKey = "JEIXDXSFCGVWJZT9"; //Enter your own ThingSpeak Write key
const char *manualTSReadKey = "2BQPCFHTDEBG44I4"; //Enter your own ThingSpeak Read key
const int manualTSChannel = 895141; //Enter your ThingSpeak channel number
// Change this value to force hard reset and clearing of FRAM when Flashing
// You have to change this value (if you have flashed before) for the TS channel to change
const int firstRunTest = 1122121;
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
  Serial1.begin(9600);
  #ifdef IOTDEBUG
  delay(5000);
  #endif

  node.begin();
  node.setPowerON(EXT3V3,true);
  node.setPowerON(EXT5V,true);


  // Check for attached I2C devices. Make sure that the device is plugged into the IoT Node
  byte error, address;
  int nDevices;
  int i2cTime;

  Serial.println("Scanning...");
  i2cTime=millis();
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
      Serial.print("I2C device found at address 0x");
      if (address<16)
      {
      Serial.print("0");
      i2cDevices.concat("0");
      }
      String addr = String(address,HEX);
      Serial.print(addr);
      i2cDevices.concat(addr);
      i2cDevices.concat(" ");
      Serial.println("  !");

      nDevices++;
    }
    else if (error==4)
    {
      Serial.print("Unknown error at address 0x");
      if (address<16)
        Serial.print("0");
      Serial.println(address,HEX);
    }
  }
  if (nDevices == 0)
    Serial.println("No I2C devices found\n");
  else
  Serial.println("done\n");
  Serial.println(millis()-i2cTime);
  Serial.println(sizeof(sensorReadings));

  delay(5000);
  /// End of I2C checking

  // Check for the FRAM address
  if (i2cDevices.indexOf("50 ")==-1)
  {
    #ifdef IOTDEBUG
    Particle.publish("Unplugged","Plug the device into the IoT Node",PRIVATE);
    Serial.println("Plug the device into the IoT Node");
    #endif
    deviceStatus="Device is not plugged into the IoTNode";
    fadeRed.setActive(true);
  }
  else
  {
    
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
        Serial.println(manualTSChannel);
        Serial.println(manualTSWriteKey);
        Serial.println(manualTSReadKey);
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
          Serial.println("Trying to create ThingSpeak channel");
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
      Serial.println("Using previously created channel number"+String(config.channelId));
      #endif
    }
      // end of first run code.

      if (syncRTC())
      {
        Serial.println("RTC sync'ed with cloud");
      }
      else
      {
        Serial.println("RTC not sync'ed with cloud");
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
    // Log.info(currentCsvData);

    String csvData = currentCsvData + lastCsvData;
    #ifdef IOTDEBUG
    Serial.println();
    Serial.println(csvData);
    Serial.println(String(config.channelId));
    Serial.println(String(config.writeKey));
    #endif
    String time_format = "absolute";
    if (waitFor(Particle.connected,config.particleTimeout)){
      // Update TSBulkWriteCSV later to use chars
      thingspeak.TSBulkWriteCSV(String(config.channelId), String(config.writeKey), time_format, csvData);
    }
    else
    {
      Serial.println("Timeout");
    }
    readyToGetResetAndSendSensors = false;

    if (tickleWD)
    {
      node.tickleWatchdog();
      tickleWD = false;
    }

    readyToGetResetAndSendSensors = false;
    #ifdef IOTDEBUG
    Serial.println("readyToGetResetAndSendSensors");
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
    Serial.println("capture");
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
    Serial.println(data);
    #endif
  }
  tickleWD = true;
}

void TSCreateChannelHandler(const char *event, const char *data) {
  // Handle the TSCreateChannel response
  StaticJsonBuffer<256> jb;
  #ifdef IOTDEBUG
  Serial.println(data);
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
      Serial.println(channelId);
      Serial.println(write_key);
      Serial.println(read_key);
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
      Serial.println("Parse failed");
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
  Serial.println("Plug the device into the IoT Node");
  #endif

}