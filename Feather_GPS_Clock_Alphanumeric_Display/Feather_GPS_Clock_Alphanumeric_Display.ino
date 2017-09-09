// This is a sketch to turn an Adafruit Feather (Arduino) with a GPS module and LED display into a clock
//
// Adafruit sample GPS code here: https://github.com/adafruit/Adafruit_GPS/tree/master/examples/GPS_HardwareSerial_Parsing
// Reference LED alphanumeric code here: https://github.com/adafruit/Adafruit_LED_Backpack/tree/master/examples/quadalphanum
//
// Big thanks to Lady Ada from Adafruit and adafruit_support_carter from the Adafruit forums for help getting this going.
//
// Tested and works great with:
// Adafruit Ultimate GPS FeatherWing ------------------------> http://amzn.to/2wpKIdT
// Adafruit 0.54" Quad Alphanumeric FeatherWing Display --> https://www.adafruit.com/product/3129
// Adafruit Feather M0 Basic Proto - ATSAMD21 Cortex M0 -----> http://amzn.to/2wpuWj5
// 
//  Written by Jay Doscher.  
//  BSD license, all text above must be included in any redistribution
// ****************************************************/
 
#include "Adafruit_LEDBackpack.h"
#include <Adafruit_GPS.h>
#include <Adafruit_GFX.h>
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

// Set a brightness value from 1-15
int displayBrightness = 5;

// This is for the alpha display. This sketch will not work with the 7 segment.
Adafruit_AlphaNum4 alpha4 = Adafruit_AlphaNum4();

// This timer is for the GPS readings.  Don't mess with it.
uint32_t timer = millis();

void setup()  
{
  //Print the source URL for the project in case you lose it
  Serial.begin(115200);
  
  alpha4.begin(0x70);  // pass in the address

  // 9600 NMEA is the default baud rate for Adafruit MTK GPS's- some use 4800
  GPS.begin(9600);
  
  // Turn on RMC (recommended minimum) and GGA (fix data) including altitude
  GPS.sendCommand(PMTK_SET_NMEA_OUTPUT_RMCGGA);
  
  // Set the update rate
  GPS.sendCommand(PMTK_SET_NMEA_UPDATE_1HZ);   // 1 Hz update rate

  // Set the display brightness
  alpha4.setBrightness(displayBrightness);
  delay(500);
  displayOnce = true;
}

// This is our lookup table for the characters 0-9
// This is needed because the alpha display can't read from
// the GPS readings directly
// Plus binary in Arduino sketches looks cool
int getNumber(int numInput){
  if (numInput == 0) {
    return (0b0000110000111111);
  } else if (numInput ==1) {
    return (0b0000000000000110);
  } else if (numInput ==2) {
    return (0b0000000011011011);
  } else if (numInput ==3) {
    return (0b0000000010001111);
  } else if (numInput ==4) {
    return (0b0000000011100110);
  } else if (numInput ==5) {
    return (0b0010000001101001);
  } else if (numInput ==6) {
    return(0b0000000011111101);
  } else if (numInput ==7) {
    return (0b0000000000000111);
  } else if (numInput ==8) {
    return (0b0000000011111111);
  } else if (numInput ==9) {
    return (0b0000000011101111);
  }
}
void loop()                     // run over and over again
{
  // read data from the GPS in the 'main loop'
  char c = GPS.read();
    // if a sentence is received, we can check the checksum, parse it...
  if (GPS.newNMEAreceived()) {
    // a tricky thing here is if we print the NMEA sentence, or data
    // we end up not listening and catching other sentences!
    // so be very wary if using OUTPUT_ALLDATA and trytng to print out data
    if (!GPS.parse(GPS.lastNMEA())) // this also sets the newNMEAreceived() flag to false
      return; // we can fail to parse a sentence in which case we should just wait for another
  }
  // if millis() or timer wraps around, we'll just reset it
  if (timer > millis()) timer = millis();

   // approximately every 2 seconds or so, print out the current stats
  if (millis() - timer > 2000) {
    timer = millis(); // reset the timer
    // Clear the LED display
    alpha4.clear();
    
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
    // Here we apply corrected hour to 24 hour time
      // We use the getNumber() function because the alpha display
    // can't take decimals from the GPS readings directly  
    if (timeFormat == 24) {
      if (correctedHour > 19) {
        alpha4.writeDigitRaw(0, getNumber(2));
        alpha4.writeDigitRaw(1, getNumber(correctedHour-20));
      } else if ((correctedHour > 9) && (correctedHour < 20)) {
        alpha4.writeDigitRaw(0, getNumber(1));
        alpha4.writeDigitRaw(1, getNumber(correctedHour-10));
      } else if (correctedHour < 10) {
        alpha4.writeDigitRaw(1, getNumber(correctedHour));
      }
    }
  
    // Hour calculations for 12 hour time
    // Here we apply corrected hour to 12 hour time
    // We use the getNumber() function because the alpha display
    // can't take decimals from the GPS readings directly  
    if (timeFormat == 12) {
      int twelvehour;
      if (correctedHour > 12) {
        twelvehour = (correctedHour - 12);
      } else if (correctedHour < 13) {
        twelvehour = (correctedHour);
      } else if (correctedHour == 0) {
        twelvehour = 12;
      }
      if (twelvehour > 9) {
        alpha4.writeDigitRaw(0, getNumber(1));
        twelvehour = twelvehour-10;
        alpha4.writeDigitRaw(1, getNumber(twelvehour));
      } else if (twelvehour < 10) {
        alpha4.writeDigitRaw(1, getNumber(twelvehour));
      }
    }    
  
    // Minute calculations and character assignment
    // We use the getNumber() function because the alpha display
    // can't take decimals from the GPS readings directly    
    if (GPS.minute < 10) {
      alpha4.writeDigitRaw(2, getNumber(0));
      alpha4.writeDigitRaw(3, getNumber(GPS.minute));
    } else if ((GPS.minute > 9) && (GPS.minute < 20)) {
      alpha4.writeDigitRaw(2, getNumber(1));
      alpha4.writeDigitRaw(3, getNumber(GPS.minute-10));
    } else if ((GPS.minute > 19) && (GPS.minute < 30)) {
      alpha4.writeDigitRaw(2, getNumber(2));
      alpha4.writeDigitRaw(3, getNumber(GPS.minute-20));
    } else if ((GPS.minute > 29) && (GPS.minute < 40)) {
      alpha4.writeDigitRaw(2, getNumber(3));
      alpha4.writeDigitRaw(3, getNumber(GPS.minute-30));
    } else if ((GPS.minute > 39) && (GPS.minute < 50)) {
      alpha4.writeDigitRaw(2, getNumber(4));
      alpha4.writeDigitRaw(3, getNumber(GPS.minute-40));
    } else if (GPS.minute > 49) {
      alpha4.writeDigitRaw(2, getNumber(5));
      alpha4.writeDigitRaw(3, getNumber(GPS.minute-50));
    } else if (GPS.minute == 0) {
      alpha4.writeDigitRaw(2, getNumber(0));
      alpha4.writeDigitRaw(3, getNumber(0));
    }
  
    // Simple status reading if we're not getting values from the GPS
    // F242 is for a great Belgian electronic music group
    // https://en.wikipedia.org/wiki/Front_242 
    if ((GPS.hour == 0) && (GPS.minute == 0)) {
        alpha4.clear();
        alpha4.writeDigitAscii(0, 'F');
        alpha4.writeDigitAscii(1, '2');
        alpha4.writeDigitAscii(2, '4');
        alpha4.writeDigitAscii(3, '2');
    }
    // Write to the LED display
    alpha4.writeDisplay();

    if (displayOnce == true) {
    Serial.println("GPS Clock by Jay Doscher from https://github.com/jdoscher/Feather_GPS_Clock/tree/master/Feather_GPS_Clock_Alphanumeric_Display");
    displayOnce = false;  
    }
  }
}
