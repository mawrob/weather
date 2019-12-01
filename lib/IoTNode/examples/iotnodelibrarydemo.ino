/*
 * Project IoTNode demo
 * Description: Demonstration of the Sentient Things IoT Node features
 * Author: Robert Mawrey, Sentient Things, Inc.
 * Date: November 2019
 */

#include "IoTNode.h"


IoTNode node;

static const int firstRunValue = 123455;

int values;

struct Status
{
  int firstRunCheck;
  int runCount;
  uint32_t lastUnixTime;
};

Status testStatus;
uint32_t unixnow;

// Create FRAM ring of 10 integers
framRing framringtest = node.makeFramRing(10, sizeof(values));
// Create an array of one struct element
framArray framarraytest = node.makeFramArray(1, sizeof(testStatus));

SYSTEM_MODE(MANUAL);

#define SERIAL1_DEBUG

#ifdef SERIAL1_DEBUG
  #define DEBUG_PRINT(...) Serial1.print(__VA_ARGS__)
  #define DEBUG_PRINTLN(...) Serial1.println(__VA_ARGS__)
#else
  #define DEBUG_PRINT(...)
  #define DEBUG_PRINTLN(...)
#endif


// setup() runs once, when the device is first turned on.
void setup() {
  Serial1.begin(115200);
  Serial.begin(115200);
  DEBUG_PRINTLN();
  DEBUG_PRINTLN();

  node.begin();
  framringtest.initialize();

  if (node.ok())
  {
    DEBUG_PRINTLN("Successfully detected all the Sentient Things IoT Node I2C parts.");
  }
  else
  {
    DEBUG_PRINTLN("Cannot detect the I2C parts in the Sentient Things IoT Node!");
    DEBUG_PRINTLN("Please make sure your device is plugged in to the node");
    DEBUG_PRINTLN("and the power is on.");
    DEBUG_PRINTLN("Resetting in 5 seconds");
    delay(5000);   
    System.reset();
  }

  // Read from element 0 (0 indexing) since there is only 1 element
  // the (uint8_t*)& is required in front of testStatus
  // as testStatus is passed by pointer - i.e.
  // this provides the pointer to testStatus
  framarraytest.read(0,(uint8_t*)&testStatus);
  if (!(testStatus.firstRunCheck==firstRunValue))
  // Then this the first time the program has run
  // so initialize the struct and save to FRAM
  {
    testStatus.firstRunCheck = firstRunValue;
    testStatus.runCount=1;
    framarraytest.write(0,(uint8_t*)&testStatus);
    DEBUG_PRINTLN("First run");
  }
  else
  {
    DEBUG_PRINT("Setup has run ");
    DEBUG_PRINT(testStatus.runCount);
    DEBUG_PRINTLN(" times.");
    ++testStatus.runCount;
    framarraytest.write(0,(uint8_t*)&testStatus);
  }

  // time is less than 01/01/2000 @ 12:00am (UTC)
  // so it has never been set
  if (testStatus.lastUnixTime<946684800)
  {
    // So lets connect to Particle and sync the time
    connect();
    DEBUG_PRINTLN("Setting the Sentient Things IoT Node real time clock with Particle cloud time");
    unixnow = Time.now();
    node.setUnixTime(unixnow);
    testStatus.lastUnixTime = unixnow;
    framarraytest.write(0,(uint8_t*)&testStatus);
  }
  else
  {
    DEBUG_PRINTLN("Time seems to be ok.  Not connecting to the cloud.");
  }
  
}

// loop() runs over and over again, as quickly as it can execute.
void loop() {
  DEBUG_PRINT("Node ID is: ");
  DEBUG_PRINTLN(node.nodeID);
  DEBUG_PRINT("Sentient Things IoT Node time is: ");
  uint32_t nodetimenow = node.unixTime();
  DEBUG_PRINTLN(Time.format(nodetimenow, TIME_FORMAT_ISO8601_FULL));
  // Save node time to Fram
  testStatus.lastUnixTime = nodetimenow;
  framarraytest.write(0,(uint8_t*)&testStatus);

  if (node.isLiPoPowered())
  {
    DEBUG_PRINTLN("LiPo powered");
  }
  else
  {
    DEBUG_PRINTLN("Not LiPo powered");
  }
  if (node.is3AAPowered())
  {
    DEBUG_PRINTLN("3AA powered");
  }
  else
  {
    DEBUG_PRINTLN("Not 3AA powered");
  }
  if (node.isLiPoCharged())
  {
    DEBUG_PRINTLN("LiPo is fully charged");
  }
  else
  {
    DEBUG_PRINTLN("LiPo is not fully charged");
  }
  if (node.isLiPoCharging())
  {
    DEBUG_PRINTLN("LiPo is charging");
  }
  else
  {
    DEBUG_PRINTLN("LiPo is not charging");
  }

  DEBUG_PRINT("Node voltage is: ");
  DEBUG_PRINT(node.voltage());
  DEBUG_PRINTLN(" V");

  if (!framringtest.isEmpty())
  {
    DEBUG_PRINTLN("Reading the previously written random numbers from FRAM:");
    for (byte i=0; i<10;++i)
    {
      int element;
      bool valid = framringtest.pop((uint8_t*)&element);
      DEBUG_PRINT(String(element) + " ");
    }
    DEBUG_PRINTLN("");
  }

  DEBUG_PRINTLN("Writing the following random numbers to FRAM:");
  for (byte i=0; i<10;++i)
  {
    int rand = random(0,10);
    framringtest.push((uint8_t*)&rand);
    DEBUG_PRINT(String(rand) + " ");
  }
  DEBUG_PRINTLN("");

  node.switchOffFor(10);
  DEBUG_PRINTLN("Set the RTC CONTROL to \"Yes\" to enable powering off the node");
  delay(10000);
}


// Adding explicit connect routine that has to work before the rest of the code runs
void connect()
{
  // Only connect to cellular if the device is cellular
  #if Wiring_Cellular
  {
    bool cellready=Cellular.ready();
    if (!cellready)
    {
      DEBUG_PRINTLN("Attempting to connect cellular...");
      Cellular.on();
      Cellular.connect();
      waitFor(Cellular.ready,180000);
      if (!Cellular.ready())
      {
      DEBUG_PRINTLN("Cellular not ready");
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
    waitFor(Particle.connected,60000);
    if (!Particle.connected())
    {
      DEBUG_PRINTLN("Particle not connected");
      delay(200);
      System.reset();
    }
      else
    {
      DEBUG_PRINTLN("Particle connected");
    }
  }
  else
  {
    DEBUG_PRINTLN("Particle connected");
  }
}
