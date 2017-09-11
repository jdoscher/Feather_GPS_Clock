// GPS clock using test code from Adafruit's GPS and I2C 7-segment LED backpack.
// Original GPS code here: https://github.com/adafruit/Adafruit_GPS/tree/master/examples/GPS_HardwareSerial_Parsing
// Original LED Backpack code here: https://github.com/adafruit/Adafruit-LED-Backpack-Library
//
// Big thanks to Lady Ada from Adafruit and driverblock from the Adafruit forums for help getting this going.
//
// Tested and works great with:
// Adafruit Ultimate GPS FeatherWing ------------------------> http://amzn.to/2wpKIdT
// Adafruit 0.56" 4-Digit 7-Segment Display w/ FeatherWing --> https://www.adafruit.com/product/3109
// Adafruit Feather M0 Basic Proto - ATSAMD21 Cortex M0 -----> http://amzn.to/2wpuWj5

#include <Wire.h>
#include "Adafruit_LEDBackpack.h"
#include <Adafruit_GPS.h>
#define GPSSerial Serial1
Adafruit_GPS GPS(&GPSSerial);

// Display serial stuff once
boolean displayOnce;
    
// Set the time format to either 12 or 24 hour format
int timeFormat = 12;

// Set your GMT offset number here, even if it's a negative
int GMToffset = 8;

// If your GMT offset is a negative number, set the value below to true
boolean GMTnegative = true;
boolean USDST = true;

// Our variable for the changing the time based on DST and time zone
int correctedHour;
int DSToffset;

// Which our gets displayed, is an output of the 24 or 12 hour subroutines
int displayHour;

//Adafruit 7 segment display library
Adafruit_7segment matrix = Adafruit_7segment();

// timer for the loop, to make sure the GPS query doesn't try and run too quickly
uint32_t timer = millis();

void setup()  
{
  //Print the source URL for the project in case you lose it
  Serial.begin(115200);
  
  matrix.begin(0x70); 
  // 9600 NMEA is the default baud rate for Adafruit MTK GPS's- some use 4800
  GPS.begin(9600);
  
  // uncomment this line to turn on RMC (recommended minimum) and GGA (fix data) including altitude
  GPS.sendCommand(PMTK_SET_NMEA_OUTPUT_RMCGGA);
  
  // Set the update rate
  GPS.sendCommand(PMTK_SET_NMEA_UPDATE_1HZ);   // 1 Hz update rate

  delay(500);
  displayOnce = true;
}

void loop()                     // run over and over again
{
  // read data from the GPS in the 'main loop'
  char c = GPS.read();
  // if a sentence is received, we can check the checksum, parse it...
  if (GPS.newNMEAreceived()) {
    if (!GPS.parse(GPS.lastNMEA())) // this also sets the newNMEAreceived() flag to false
      return; // we can fail to parse a sentence in which case we should just wait for another
  }
  // if millis() or timer wraps around, we'll just reset it
  if (timer > millis()) timer = millis();

   // approximately every 2 seconds or so, print out the current stats
  if (millis() - timer > 2000) {
    timer = millis(); // reset the timer

    // DST offset checks. This would need overhauling to incorporate global DST
    // logic, because the dates for start & end of DST are different
    if (USDST == true) {
      DSToffset = 0;
      if ((GPS.month == 3) && (GPS.day > 12)) {
        DSToffset = 1;
      } else if ((GPS.month == 3) && (GPS.day == 12)) {
        if (GPS.hour > 2){
        DSToffset = 1;
        }
      } else if ((GPS.month > 3) && (GPS.month < 11)) {
        DSToffset = 1;
      } else if ((GPS.month == 11) && (GPS.day < 6)) { 
        DSToffset = 1; 
      } else if ((GPS.month == 11) && (GPS.day == 5)) {
        if (GPS.hour < 2){
        DSToffset = 1;
        }      
      }
    }
    // GMT offset (time zone) calculation here
    // There may be a better way with Arduino, but this
    // gets around Arduino weirdness with negative numbers
    if (GMTnegative == true) {
      correctedHour = (GPS.hour - GMToffset + DSToffset);
    } else {
      correctedHour = (GPS.hour + GMToffset);
    }
    if (correctedHour > 24) {
      correctedHour = (correctedHour -24);
    } else if (correctedHour < 0) {
      correctedHour = (correctedHour +24);
    }
    
    // Hour calculations for 24 hour time
    // No need to modify 24 hour time except to multiply it for the display 
    if (timeFormat == 24) {
      displayHour = (correctedHour * 100);
    }
  
    // Hour calculations for 12 hour time
    // Here we apply corrected hour to 12 hour time
    // We use the getNumber() function because the alpha display
    // can't take decimals from the GPS readings directly  
    if (timeFormat == 12) {
      if (correctedHour > 12) {
        displayHour = ((correctedHour - 12)*100);
      } else if (correctedHour < 13) {
         displayHour = ((correctedHour)*100);
      } else if (correctedHour == 0) {
        displayHour = 1200;
      }
    }    
        
    matrix.print((displayHour)+ GPS.minute);
    matrix.drawColon(true);
    matrix.writeDisplay();
    if (displayOnce == true) {
      Serial.println("GPS Clock by Jay Doscher from https://github.com/jdoscher/Feather_GPS_Clock/tree/master/Feather_GPS_Clock_7_Segment_Display");
      displayOnce = false;  
    }
    timer = millis(); // reset the timer
  }
}
