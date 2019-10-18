/*
  WeatherSensors.h - Weather and Level adapter library for weather and light
  Copyright (c) 2018 Sentient Things, Inc.  All right reserved.
*/



// include this library's description file
#include "WeatherSensors.h"

#include <math.h>


// Constructor /////////////////////////////////////////////////////////////////
// Function that handles the creation and setup of instances

/*WeatherSensors::WeatherLevel()
{
  // initialize this instance's variables


  // do whatever is required to initialize the library

}
*/
// Public Methods //////////////////////////////////////////////////////////////
// Functions available in Wiring sketches, this library, and other libraries
void WeatherSensors::begin(void)
{
//  pinMode(AnemometerPin, INPUT_PULLUP);
  AnemoneterPeriodTotal = 0;
  AnemoneterPeriodReadingCount = 0;
  GustPeriod = UINT_MAX;  //  The shortest period (and therefore fastest gust) observed
  lastAnemoneterEvent = 0;
//  attachInterrupt(AnemometerPin, handleAnemometerEvent, FALLING);

  barom.begin();
  barom.setModeBarometer();
  barom.setOversampleRate(7);
  barom.enableEventFlags();

  am2315.begin();
}

float  WeatherSensors::getAndResetAnemometerMPH(float * gustMPH)
{
    if(AnemoneterPeriodReadingCount == 0)
    {
        *gustMPH = 0.0;
        return 0;
    }
    // Nonintuitive math:  We've collected the sum of the observed periods between pulses, and the number of observations.
    // Now, we calculate the average period (sum / number of readings), take the inverse and muliple by 1000 to give frequency, and then mulitply by our scale to get MPH.
    // The math below is transformed to maximize accuracy by doing all muliplications BEFORE dividing.
    float result = AnemometerScaleMPH * 1000.0 * float(AnemoneterPeriodReadingCount) / float(AnemoneterPeriodTotal);
    AnemoneterPeriodTotal = 0;
    AnemoneterPeriodReadingCount = 0;
    *gustMPH = AnemometerScaleMPH  * 1000.0 / float(GustPeriod);
    GustPeriod = UINT_MAX;
    return result;
}


float WeatherSensors::getAndResetRainInches()
{
    float result = RainScaleInches * float(rainEventCount);
    rainEventCount = 0;
    return result;
}
/// Wind Vane
void WeatherSensors::captureWindVane() {
    // Read the wind vane, and update the running average of the two components of the vector
    unsigned int windVaneRaw = analogRead(WindVanePin);
//Serial.println(windVaneRaw);
    float windVaneRadians = lookupRadiansFromRaw(windVaneRaw);
//Serial.println(windVaneRadians);
    if(windVaneRadians > 0 && windVaneRadians < 6.14159)
    {
        windVaneCosTotal += cos(windVaneRadians);
        windVaneSinTotal += sin(windVaneRadians);
        windVaneReadingCount++;
    }
    return;
}

float WeatherSensors::getAndResetWindVaneDegrees()
{
    if(windVaneReadingCount == 0) {
        return 0;
    }
    float avgCos = windVaneCosTotal/float(windVaneReadingCount);
    float avgSin = windVaneSinTotal/float(windVaneReadingCount);
    float result = atan(avgSin/avgCos) * 180.0 / 3.14159;
    windVaneCosTotal = 0.0;
    windVaneSinTotal = 0.0;
    windVaneReadingCount = 0;
    // atan can only tell where the angle is within 180 degrees.  Need to look at cos to tell which half of circle we're in
    if(avgCos < 0) result += 180.0;
    // atan will return negative angles in the NW quadrant -- push those into positive space.
    if(result < 0) result += 360.0;

   return result;
}

float WeatherSensors::lookupRadiansFromRaw(unsigned int analogRaw)
{
//Serial.println(analogRaw);
    // The mechanism for reading the weathervane isn't arbitrary, but effectively, we just need to look up which of the 16 positions we're in.
    if(analogRaw >= 2200 && analogRaw < 2400) return (3.14);//South
    if(analogRaw >= 2100 && analogRaw < 2200) return (3.53);//SSW
    if(analogRaw >= 3200 && analogRaw < 3299) return (3.93);//SW
    if(analogRaw >= 3100 && analogRaw < 3200) return (4.32);//WSW
    if(analogRaw >= 3890 && analogRaw < 3999) return (4.71);//West
    if(analogRaw >= 3700 && analogRaw < 3780) return (5.11);//WNW
    if(analogRaw >= 3780 && analogRaw < 3890) return (5.50);//NW
    if(analogRaw >= 3400 && analogRaw < 3500) return (5.89);//NNW
    if(analogRaw >= 3570 && analogRaw < 3700) return (0.00);//North
    if(analogRaw >= 2600 && analogRaw < 2700) return (0.39);//NNE
    if(analogRaw >= 2750 && analogRaw < 2850) return (0.79);//NE
    if(analogRaw >= 1510 && analogRaw < 1580) return (1.18);//ENE
    if(analogRaw >= 1580 && analogRaw < 1650) return (1.57);//East
    if(analogRaw >= 1470 && analogRaw < 1510) return (1.96);//ESE
    if(analogRaw >= 1900 && analogRaw < 2000) return (2.36);//SE
    if(analogRaw >= 1700 && analogRaw < 1750) return (2.74);//SSE
    if(analogRaw > 4000) return(-1); // Open circuit?  Probably means the sensor is not connected
   // Particle.publish("error", String::format("Got %d from Windvane.",analogRaw), 60 , PRIVATE);
    return -1;
}

/// end Wind vane

void WeatherSensors::captureTempHumidityPressure() {
  // Read the humidity and pressure sensors, and update the running average
  // The running (mean) average is maintained by keeping a running sum of the observations,
  // and a count of the number of observations

  // Measure Relative Humidity and temperature from the AM2315
  float humidityRH, tempC, tempF;
  bool validTH = am2315.readTemperatureAndHumidity(tempC, humidityRH);

  uint16_t tempKx10 = uint16_t(tempC*10)+2732;
  airTempKMedian.add(tempKx10);

  relativeHumidtyMedian.add(humidityRH);

if (validTH){
    //If the result is reasonable, add it to the running mean
    if(humidityRH > 0 && humidityRH < 105) // It's theoretically possible to get supersaturation humidity levels over 100%
    {
        // Add the observation to the running sum, and increment the number of observations
        humidityRHTotal += humidityRH;
        humidityRHReadingCount++;
    }


    tempF = (tempC * 9.0) / 5.0 + 32.0;
    //If the result is reasonable, add it to the running mean
    if(tempF > -50 && tempF < 150)
    {
        // Add the observation to the running sum, and increment the number of observations
        tempFTotal += tempF;
        tempFReadingCount++;
    }
  }
  //Measure Pressure from the MPL3115A2
  float pressurePascals = barom.readPressure();

  //If the result is reasonable, add it to the running mean
  // What's reasonable? http://findanswers.noaa.gov/noaa.answers/consumer/kbdetail.asp?kbid=544
  if(pressurePascals > 80000 && pressurePascals < 110000)
  {
      // Add the observation to the running sum, and increment the number of observations
      pressurePascalsTotal += pressurePascals;
      pressurePascalsReadingCount++;
  }

  //Maybe get lux and voltage here?

  return;
}

void WeatherSensors::captureAirTempHumid() {
  // Read the humidity and pressure sensors, and update the running average
  // The running (mean) average is maintained by keeping a running sum of the observations,
  // and a count of the number of observations

  // Measure Relative Humidity and temperature from the AM2315
  float humidityRH, tempC, tempF;
  bool validTH = am2315.readTemperatureAndHumidity(tempC, humidityRH);

  uint16_t tempKx10 = uint16_t(tempC*10.0+2732.0);
  airTempKMedian.add(tempKx10);

  relativeHumidtyMedian.add(humidityRH);

  if (validTH){
      //If the result is reasonable, add it to the running mean
      if(humidityRH > 0 && humidityRH < 105) // It's theoretically possible to get supersaturation humidity levels over 100%
      {
          // Add the observation to the running sum, and increment the number of observations
          humidityRHTotal += humidityRH;
          humidityRHReadingCount++;
      }


      tempF = (tempC * 9.0) / 5.0 + 32.0;
      //If the result is reasonable, add it to the running mean
      if(tempF > -50 && tempF < 150)
      {
          // Add the observation to the running sum, and increment the number of observations
          tempFTotal += tempF;
          tempFReadingCount++;
      }
    }
}

void WeatherSensors::capturePressure() {
  //Measure Pressure from the MPL3115A2
  float pressurePascals = barom.readPressure();

  //If the result is reasonable, add it to the running mean
  // What's reasonable? http://findanswers.noaa.gov/noaa.answers/consumer/kbdetail.asp?kbid=544
  if(pressurePascals > 80000 && pressurePascals < 110000)
  {
      // Add the observation to the running sum, and increment the number of observations
      pressurePascalsTotal += pressurePascals;
      pressurePascalsReadingCount++;
  }
}

float WeatherSensors::getAndResetTempF()
{
    if(tempFReadingCount == 0) {
        return 0;
    }
    float result = tempFTotal/float(tempFReadingCount);
    tempFTotal = 0.0;
    tempFReadingCount = 0;
    return result;
}

float WeatherSensors::getAndResetHumidityRH()
{
    if(humidityRHReadingCount == 0) {
        return 0;
    }
    float result = humidityRHTotal/float(humidityRHReadingCount);
    humidityRHTotal = 0.0;
    humidityRHReadingCount = 0;
    return result;
}


float WeatherSensors::getAndResetPressurePascals()
{
    if(pressurePascalsReadingCount == 0) {
        return 0;
    }
    float result = pressurePascalsTotal/float(pressurePascalsReadingCount);
    pressurePascalsTotal = 0.0;
    pressurePascalsReadingCount = 0;
    return result;
}


uint16_t WeatherSensors::getAirTempKMedian()
{
  uint16_t airKMedian = airTempKMedian.getMedian();
  return airKMedian;
}

uint16_t WeatherSensors::getRHMedian()
{
  uint16_t RHMedian = relativeHumidtyMedian.getMedian();
  return RHMedian;
}


float WeatherSensors::readPressure()
{
  return barom.readPressure();
}
/*
//Weather
    uint32_t unixTime; //system_tick_t (uint32_t)
    uint16_t windDegrees; // 1 degree resolution is plenty
    uint16_t wind_metersph; //meters per hour
    uint8_t humid; //percent
    uint16_t airTempKx10; // Temperature in deciKelvin
    uint16_t rainmmx1000; // millimetersx1000 - resolution is 0.2794mm 0.011"
    float barometerhPa; // Could fit into smaller type if needed
    uint16_t gust_metersph; //meters per hour
    // Light
    uint16_t millivolts; // voltage in mV
    uint16_t lux; //Light level in lux
*/
void WeatherSensors::getAndResetAllSensors()
{
  uint32_t timeRTC = rtc.rtcNow();
  sensorReadings.unixTime = timeRTC;
  float gustMPH;
  float windMPH = getAndResetAnemometerMPH(&gustMPH);
  sensorReadings.wind_metersph = (uint16_t) ceil(windMPH * 1609.34);
  float rainInches = getAndResetRainInches();
  sensorReadings.rainmmx1000 = (uint16_t) ceil(rainInches * 25400);
  float windDegrees = getAndResetWindVaneDegrees();
  sensorReadings.windDegrees = (uint16_t) ceil(windDegrees);
  float airTempF = getAndResetTempF();
  sensorReadings.airTempKx10 = (uint16_t) ceil((airTempF-32.0)*50.0/9.0 + 2731.5);
  uint16_t humidityRH = relativeHumidtyMedian.getMedian();
  sensorReadings.humid =(uint8_t) ceil(humidityRH);
  float pressure = getAndResetPressurePascals();
  sensorReadings.barometerhPa = pressure/10.0;
// Light and voltage needed
}

// Convert sensorData to CSV String in US units
String WeatherSensors::sensorReadingsToCsvUS()
{
  String csvData =
//  String(Time.format(sensorReadings.unixTime, TIME_FORMAT_ISO8601_FULL))+
  String(sensorReadings.unixTime)+
  ","+
  String(sensorReadings.windDegrees)+
  ","+
  minimiseNumericString(String::format("%.1f",(float)sensorReadings.wind_metersph/1609.34),1)+
  ","+
  String(sensorReadings.humid)+
  ","+
  minimiseNumericString(String::format("%.1f",(((float)sensorReadings.airTempKx10-2731.5)*9.0/50.0+32.0)),1)+
  ","+
  minimiseNumericString(String::format("%.3f",((float)sensorReadings.rainmmx1000/25400)),3)+
  ","+
  minimiseNumericString(String::format("%.2f",(float)sensorReadings.barometerhPa/338.6389),2)+
// Not enough fields in thingspeak channel for gust
//  ","+
//  minimiseNumericString(String::format("%.1f",(Float)(sensorReadings.gust_metersph/1609.34)),1)+
  ","+
  minimiseNumericString(String::format("%.2f",0),2)+ // replace with voltage/lux
  ","+
  minimiseNumericString(String::format("%.1f",0),1)+
  ",,,,"+
  String(0) // this was rangeref put in something else
  ;
//add status field = range reference
  return csvData;
}


// Convert sensorData to CSV String in US units
String WeatherSensors::sensorReadingsToCsvUS(sensorReadings_t readings)
{
  String csvData =
//  String(Time.format(readings.unixTime, TIME_FORMAT_ISO8601_FULL))+
  String(readings.unixTime)+
  ","+
  String(readings.windDegrees)+
  ","+
  minimiseNumericString(String::format("%.1f",(float)readings.wind_metersph/1609.34),1)+
  ","+
  String(readings.humid)+
  ","+
  minimiseNumericString(String::format("%.1f",(((float)readings.airTempKx10-2731.5)*9.0/50.0+32.0)),1)+
  ","+
  minimiseNumericString(String::format("%.3f",((float)readings.rainmmx1000/25400)),3)+
  ","+
  minimiseNumericString(String::format("%.2f",(float)readings.barometerhPa/338.6389),2)+
// Not enough fields in thingspeak for this
//  ","+
//  minimiseNumericString(String::format("%.1f",(Float)(readings.gust_metersph/1609.34)),1)+
  ","+
  minimiseNumericString(String::format("%.2f",0),2)+
  ","+
  minimiseNumericString(String::format("%.1f",0),1)+
  ",,,,"+
  String(0)
  ;

  return csvData;
}

// Private Methods /////////////////////////////////////////////////////////////
// Functions only available to other functions in this library

//https://stackoverflow.com/questions/277772/avoid-trailing-zeroes-in-printf
String WeatherSensors::minimiseNumericString(String ss, int n) {
    int str_len = ss.length() + 1;
    char s[str_len];
    ss.toCharArray(s, str_len);

//Serial.println(s);
    char *p;
    int count;

    p = strchr (s,'.');         // Find decimal point, if any.
    if (p != NULL) {
        count = n;              // Adjust for more or less decimals.
        while (count >= 0) {    // Maximum decimals allowed.
             count--;
             if (*p == '\0')    // If there's less than desired.
                 break;
             p++;               // Next character.
        }

        *p-- = '\0';            // Truncate string.
        while (*p == '0')       // Remove trailing zeros.
            *p-- = '\0';

        if (*p == '.') {        // If all decimals were zeros, remove ".".
            *p = '\0';
        }
    }
    return String(s);
}
