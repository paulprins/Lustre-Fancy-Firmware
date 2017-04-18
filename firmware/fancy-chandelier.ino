/*-------------------------------------------------------------------------
  Spark Core, Particle Photon, P1, Electron and RedBear Duo library to control
  WS2811/WS2812 based RGB LED devices such as Adafruit NeoPixel strips.

  Supports:
  - 800 KHz and 400kHz bitstream WS2812, WS2812B and WS2811
  - 800 KHz bitstream SK6812RGBW (NeoPixel RGBW pixel strips)
    (use 'SK6812RGBW' as PIXEL_TYPE)

  Also supports:
  - Radio Shack Tri-Color Strip with TM1803 controller 400kHz bitstream.
  - TM1829 pixels

  PLEASE NOTE that the NeoPixels require 5V level inputs
  and the Spark Core, Particle Photon, P1, Electron and RedBear Duo only
  have 3.3V level outputs. Level shifting is necessary, but will require
  a fast device such as one of the following:

  [SN74HCT125N]
  http://www.digikey.com/product-detail/en/SN74HCT125N/296-8386-5-ND/376860

  [SN74HCT245N]
  http://www.digikey.com/product-detail/en/SN74HCT245N/296-1612-5-ND/277258

  Written by Phil Burgess / Paint Your Dragon for Adafruit Industries.
  Modified to work with Particle devices by Technobly.
  Contributions by PJRC and other members of the open source community.

  Adafruit invests time and resources providing this open source code,
  please support Adafruit and open-source hardware by purchasing products
  from Adafruit!
  --------------------------------------------------------------------*/

/* ======================= includes ================================= */

#include "application.h"
#include <neopixel.h> // use for Build IDE
// #include "neopixel.h" // use for local build
#include <vector>
#include <sstream>

/* ======================= prototypes =============================== */

void colorAll(uint32_t c, uint8_t wait);
void colorWipe(uint32_t c, uint8_t wait);
void rainbow(uint8_t wait);
void rainbowCycle(uint8_t wait);
uint32_t Wheel(byte WheelPos);
std::vector<int> SplitStringToInt( String input );

/* ======================= fancy-chandelier.cpp ======================== */

SYSTEM_MODE(AUTOMATIC);

// IMPORTANT: Set pixel COUNT, PIN and TYPE
#define PIXEL_COUNT 60
#define PIXEL_PIN D2
#define PIXEL_TYPE WS2812B

// Parameter 1 = number of pixels in strip
//               note: for some stripes like those with the TM1829, you
//                     need to count the number of segments, i.e. the
//                     number of controllers in your stripe, not the number
//                     of individual LEDs!
// Parameter 2 = pin number (most are valid)
//               note: if not specified, D2 is selected for you.
// Parameter 3 = pixel type [ WS2812, WS2812B, WS2812B2, WS2811,
//                             TM1803, TM1829, SK6812RGBW ]
//               note: if not specified, WS2812B is selected for you.
//               note: RGB order is automatically applied to WS2811,
//                     WS2812/WS2812B/WS2812B2/TM1803 is GRB order.
//
// 800 KHz bitstream 800 KHz bitstream (most NeoPixel products
//               WS2812 (6-pin part)/WS2812B (4-pin part)/SK6812RGBW (RGB+W) )
//
// 400 KHz bitstream (classic 'v1' (not v2) FLORA pixels, WS2811 drivers)
//                   (Radio Shack Tri-Color LED Strip - TM1803 driver
//                    NOTE: RS Tri-Color LED's are grouped in sets of 3)

Adafruit_NeoPixel strip = Adafruit_NeoPixel(PIXEL_COUNT, PIXEL_PIN, PIXEL_TYPE);

// IMPORTANT: To reduce NeoPixel burnout risk, add 1000 uF capacitor across
// pixel power leads, add 300 - 500 Ohm resistor on first pixel's data input
// and minimize distance between Arduino and first pixel.  Avoid connecting
// on a live circuit...if you must, connect GND first.


//
// Member Variables
int lightBrightness = 255 * .15;    // Initial Brightness between 0 and 255 (Max)
String lightOn = "false";           // Initial state of the light
String lightMode = "warm";          // Initial display type
String customColor = NULL;          // an R,G,B string for every LED to show
int luxChange = 1500;               // in milleseconds

bool fetchedEEPROM = false;         // delay fetching the cached values until we know they have been set (to avoid overwriting with defaults)
int cacheMaxDuration = 60 * 15;     // Second until the cached values are ignored


void setup() {
    strip.begin();
    if( Particle.connected() == false) {
        Particle.connect();
        return;
    }

    // Set the Timezone
    Time.zone(0);


    //
    // Grab the variables from EEPROM (Persistent Flash storage)
    pullFromEEPROM();

    // Functions to expose to the web
    Particle.function("toggleLight",toggleLight);
    Particle.function("setColor", setColor);
    Particle.function("setLux", setLux);
    Particle.function("setMode",setMode);
    Particle.function("setLuxSpeed",setLuxSpeed);
    
    
    Particle.variable("brightness", lightBrightness);  // Expose Variables
    Particle.variable("lightOn", lightOn);  // Expose Variables
    Particle.variable("lightMode", lightMode);
    Particle.variable("customColor", customColor);
    Particle.variable("luxChange", luxChange);
    Particle.variable("cacheSec", cacheMaxDuration);


    strip.setBrightness( 0 );   // Ensure the lights start off
    setMode( lightMode );
    setLux( String( lightBrightness ) );    // 0: off and 255: Max
    toggleLight("1");


    // Turn off the status LED
    RGB.brightness(0); // all of the way off (MAX DARK)

//   strip.setPixelColor(0, strip.Color(255, 0, 255));

//   rainbowCycle(20);

//   colorAll(strip.Color(0, 255, 255), 50); // Cyan
    // colorAll(strip.Color(255, 255, 255), 50); // white
    // colorAll(strip.Color(255, 145, 120), 50); // warm white
}

void loop() {
    // strip.show();   // Not 100% sure this is needed here, but the loop seemed lonely
    if( fetchedEEPROM ){
        pushToEEPROM(); // Save Setting
    }
}



//
// Web Functions
    /*
     * Set a color to use with the "custom" mode. Must be sent as a comma seperated Red,Green,Blue (0-255) value
     */
    int setColor( String colorString ){
        // String must include three comma seperated integers in order of Red, Green Blue.
        if( colorString.length() == 0 || colorString.indexOf(",") == -1 ){
            return -1;
        }

        std::vector<int> RGB = SplitStringToInt( colorString );

        if( RGB.size() != 3 ){
            Particle.publish("setColor() fail",  "Wrong Size: size " + String( RGB.size() ) + " | " + String(RGB.at(0)) + " " + String(RGB.at(1)));
            return -1;  // Must be 3 long
        }

        // Lets make sure every value is valid
        for(std::vector<int>::const_iterator i = RGB.begin(); i != RGB.end(); ++i) {
            if( *i < 0 && *i > 255 ){
                Particle.publish("setColor() fail", "Invalid Number: " + String( *i ) );
                return -1;  // Contains an invalid value
            }
        }

        // String is good to go!
        customColor = colorString;
        setMode( "custom" );
        
        return 1;  // All the things are broken.
    }

    /*
     * Define how bright the light can be using a value between 0 and 255. 0 being off, and 255 being on.
     */
    int setLux( String newLux ){
        int newLuxInt = newLux.toInt();
        int currentLux = strip.getBrightness();     // Current Value
        int luxDiff = newLuxInt - currentLux;       // Distance between our values
        if( newLuxInt < 0 || newLuxInt > 255 ){
            return -1;   // Not a valid brightness value between 0 and 255
        }else if( luxDiff == 0 ){
            return 1;   // Already running at this value
        }


        if( luxChange == 0 ){
            if( currentLux == 0 ){  setMode( lightMode );   }   // Needed to retrigger the mode after turning off
            strip.setBrightness( newLuxInt );    // 0: off and 255: Max
            strip.show();
        }else{
            // Going to smooth out the brightness change
            int luxStepSpeed =  luxChange / abs( luxDiff );  // Milleseconds per step based on the desired change
            if( luxStepSpeed == 0 ){ luxStepSpeed = 1; }
            Particle.publish("setLux() LuxStepSpeed", String(luxChange) + " / " + String( luxDiff ) + " = " + String(luxStepSpeed)  );

            // Ensure our minimum delay is 20ms to avoid overloading the process
            int stepSize = 1;
            if( luxStepSpeed < 20 ){
                // Particle.publish("setLux() fix step", String( luxStepSpeed ) );
                while( luxStepSpeed < 20 ){
                    luxStepSpeed = luxStepSpeed * 2;
                    stepSize = stepSize * 2;
                }

                Particle.publish("setLux() fixed step", String( luxStepSpeed ) );
            }

            // Loop over the lux until
            Particle.publish("setLux()", String( currentLux ) + " to " + String( newLuxInt ) + " with " + String( luxStepSpeed ) + "ms per step, size " + String( stepSize ) + " with luxDiff of " + String( luxDiff ) );
            bool runStepper = true;
            bool modeReset = false;
            int stepLux = currentLux;
            do {
                if( luxDiff > 0 ){
                    stepLux = stepLux + stepSize;  // Step up to the new value
                    if( stepLux > newLuxInt ){ runStepper = false; }
                }else{
                    stepLux = stepLux - stepSize;  // Step down to the new value
                    if( stepLux < newLuxInt ){ runStepper = false; }
                }

                strip.setBrightness( stepLux );    // 0: off and 255: Max
                if( currentLux == 0 && modeReset == false ){  setMode( lightMode );  modeReset = true;  }   // Needed to retrigger the mode after turning off
                strip.show();


                delay(luxStepSpeed);
            }while( runStepper );

            strip.setBrightness( newLuxInt );    // 0: off and 255: Max
            strip.show();
        }


        lightBrightness = newLuxInt;
        if( newLuxInt == 0 ){
            lightOn = "false";
        }else{
            lightOn = "true";
        }

        pushToEEPROM(); // Save Setting

        return 1;
    }
    
    
    /*
     * Define the duration in MS between every change in luminocity (stepped by 2)
     */
    int setLuxSpeed( String newLuxDuration ){
        int testing = round( newLuxDuration.toInt() );

        if( testing < 0 || testing > 60000 ){
            return -1;
        }

        luxChange = testing;
        pushToEEPROM(); // Save Setting

        return 1;
    }


    /*
     * Move between presets, or a custom color for the light.
     */
    int setMode( String newMode ){
        if( newMode == "bright"){
            colorAll(strip.Color(255, 255, 255), 50);
        }else if( newMode == "warm"){
            colorAll(strip.Color(255, 145, 120), 50);
        }else if( newMode == "rainbow"){
            // rainbow(20);
            rainbowForever( 10 );
        }else if( newMode == "custom"){
            // Use the value stored as customColor (from setColor) to light up the show
            if( customColor == NULL ){
                return -1;
            }

            std::vector<int> RGB =  SplitStringToInt( customColor );
            colorAll(strip.Color( RGB.at(0), RGB.at(1), RGB.at(2)), 50);
        }else{
            return -1;
        }

        strip.show();
        lightMode = newMode;
        pushToEEPROM(); // Save Setting

        return 1;
    }

    /*
     * Quickly toggles the light on and off. Use as the central place to turn on the light. Accepts On, Off, 1, 0
     */
    int toggleLight( String newState ){
        if( newState == "on" || newState == "1" ){
            Particle.publish( "toggleLight()", "On" );
            setMode( String( lightMode ) );
            setLux( String( lightBrightness ) );   // Use function to allow fading in
            lightOn = "true";
        }else if( newState == "off" || newState == "0"){
            Particle.publish( "toggleLight()", "Off" );
            int tempBright = lightBrightness;   // Temp store the current value to reset after changing it
            setLux( "0" );   // Use function to engage fading if it exists
            lightBrightness = tempBright;
            lightOn = "false";
        }else{
            return -1;
        }

        return 1;
    }
// Web Functions
//


//
// Persist Variables
    /*
     *  
     */
    struct MyEEPROM {
        int epoch; // LoggedTime;
        int lumens; // lightBrightness;
        // String lightOn;  // Do not include this value
        char mode[100]; // lightMode;
        char color[12]; // customColor;
        int luxChange;
    };


    /*
     * Push the current configuration values into EEPROM 
     */
    void pushToEEPROM(){
        // char charMode[100];
        // lightMode.toCharArray(charMode, 100);

        // char charColor[12];
        // customColor.toCharArray(charColor, 12);
        
        MyEEPROM configVals = {};
            configVals.epoch = Time.now();
            configVals.lumens = lightBrightness;
            lightMode.toCharArray(configVals.mode, 100);
            customColor.toCharArray(configVals.color, 12);
            configVals.luxChange = luxChange;
        EEPROM.put( 20, configVals );
        
        return;
    }

    /*
     * Grab the configuration data from EEPROM 
     */
    void pullFromEEPROM(){
        MyEEPROM configVals;
        EEPROM.get( 20, configVals );
        fetchedEEPROM = true;

        if( configVals.epoch != 0 && ( Time.now() - configVals.epoch ) < cacheMaxDuration ){
            lightBrightness = configVals.lumens;
            lightMode = String( configVals.mode );
            customColor = String( configVals.color );
            luxChange  = configVals.luxChange;
        }else{
            EEPROM.clear();
        }

        return;
    }
// Persist Variables
//


//
// Utility Functions
    // Set all pixels in the strip to a solid color, then wait (ms)
    void colorAll(uint32_t c, uint8_t wait) {
      uint16_t i;

      for(i=0; i<strip.numPixels(); i++) {
        strip.setPixelColor(i, c);
      }
      strip.show();
      delay(wait);
    }

    // Fill the dots one after the other with a color, wait (ms) after each one
    void colorWipe(uint32_t c, uint8_t wait) {
      for(uint16_t i=0; i<strip.numPixels(); i++) {
        strip.setPixelColor(i, c);
        strip.show();
        delay(wait);
      }
    }

    void rainbow(uint8_t wait) {
      uint16_t i, j;

      for(j=0; j<256; j++) {
        for(i=0; i<strip.numPixels(); i++) {
          strip.setPixelColor(i, Wheel((i+j) & 255));
        }
        strip.show();
        delay(wait);
      }
    }


    void rainbowForever(uint8_t wait) {
        uint16_t i, j;
        
        for(j=0; j<256; j++) {
            if( lightMode != "rainbow"){ break; }
          
            for(i=0; i<strip.numPixels(); i++) {
                strip.setPixelColor(i, Wheel((i+j) & 255));
            }
            strip.show();
            delay(wait);
            if( j == 255 ){ j = 0; }
        }
    }

    // Slightly different, this makes the rainbow equally distributed throughout, then wait (ms)
    void rainbowCycle(uint8_t wait) {
      uint16_t i, j;

      for(j=0; j<256; j++) { // 1 cycle of all colors on wheel
        for(i=0; i< strip.numPixels(); i++) {
          strip.setPixelColor(i, Wheel(((i * 256 / strip.numPixels()) + j) & 255));
        }
        strip.show();
        delay(wait);
      }
    }

    // Input a value 0 to 255 to get a color value.
    // The colours are a transition r - g - b - back to r.
    uint32_t Wheel(byte WheelPos) {
      if(WheelPos < 85) {
      return strip.Color(WheelPos * 3, 255 - WheelPos * 3, 0);
      } else if(WheelPos < 170) {
      WheelPos -= 85;
      return strip.Color(255 - WheelPos * 3, 0, WheelPos * 3);
      } else {
      WheelPos -= 170;
      return strip.Color(0, WheelPos * 3, 255 - WheelPos * 3);
      }
    }

    std::vector<int> SplitStringToInt( String input ){
        String split = ",";
        std::vector<int> Return;

        if( input.length() == 0 || input.indexOf( split ) == -1 ){
            if( input.length() != 0 ){
                Return.push_back( input.toInt() );  // Kick back out what your momma gave ya
            }
            return Return;
        }

        int start = 0;
        bool KeepGoing = true;
        while( KeepGoing ){
            if( input.indexOf( split, start ) != -1 ){
                Return.push_back( input.substring( start, input.indexOf( split, start ) ).toInt() );
            }else{
                Return.push_back( input.substring( start ).toInt() );   // Ensure we grab the last value
                KeepGoing = false;
                break;
            }

            start = input.indexOf( split, start ) + 1;
        }

        return Return;
    }
// Utility Functions
//
