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
#include "colorRGB.h"
#include "colorHSL.h"
#include <neopixel.h> // use for Build IDE
// #include "neopixel.h" // use for local build
#include <vector>
#include <sstream>
// #include <math.h>
#include <cmath>
#include <map>

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
#define PIXEL_COUNT 139
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
int lightBrightness = 250 * 1;    // Initial Brightness between 0 and 255 (Max)
String lightOn = "false";           // Initial state of the light
String lightMode = "custom";          // Initial display type
int luxChange = 500;               // in milleseconds
int colorHue = 23;                   // This is the H from HSL
int colorSaturation = 100;          // This is the S from HSL
int colorLightness = 60;            // This is the L from HSL
String quickSetting = "";           // Comma seperated string - Mode, Hue, Saturation, Lightness, Brightness, Light Change

// Build out the initial colors
HSL thisHSL = HSL( colorHue, colorSaturation, colorLightness );
colorRGB* thisRGB = thisHSL.toRGB();

// Cache values
bool fetchedEEPROM = false;         // delay fetching the cached values until we know they have been set (to avoid overwriting with defaults)
int cacheMaxDuration = 60 * 15;     // Second until the cached values are ignored [Defaults to 15 minutes]


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
    updateQuickSetting();


    //
    // Functions to expose to the web
    Particle.function("toggleLight",toggleLight);
    Particle.function("setHue", setHue);
    Particle.function("setSat", setSaturation);
    Particle.function("setLight", setLightness);
    Particle.function("setLux", setLux);

    Particle.function("setMode", setModeExpose);
    Particle.function("setLuxSpeed", setLuxSpeed);


    //
    // Variable to expose to the web
    Particle.variable("brightness", lightBrightness);  // Expose Variables
    Particle.variable("lightOn", lightOn);  // Expose Variables
    Particle.variable("lightMode", lightMode);
    Particle.variable("colorHue", colorHue);
    Particle.variable("colorSat", colorSaturation);
    Particle.variable("colorLight", colorLightness);
    Particle.variable("luxChange", luxChange);
    Particle.variable("cacheSec", cacheMaxDuration);
    Particle.variable("quickSetting", quickSetting );


    strip.setBrightness( 0 );   // Ensure the lights start off
    setMode( lightMode, false );
    setLux( String( lightBrightness ) );    // 0: off and 255: Max
    toggleLight("1");
}

// Need us a loop!
void loop() {
    if( fetchedEEPROM ){
        pushToEEPROM(); // Save Setting
    }
}



//
// Web Functions

    /*
     * Set a hue to use with the "custom" mode
     */
    int setHue( String hueString ){
        int hueValue = hueString.toInt();

        // Lets make sure every value is valid
        if( hueValue < 1 || hueValue > 360 ){
            Particle.publish("setHue() fail", "Invalid Number: " + String( hueValue ) + " | Must be between 0 and 360");
            return -1;  // Contains an invalid value
        }

        // String is good to go!
        colorHue = hueValue;
        updateHSL();

        setMode( "custom", true );

        return 1;  // All the things are broken.
    }
    
    

    /*
     * Set a saturation to use with the "custom" mode
     */
    int setSaturation( String satString ){
        int satValue = satString.toInt();
        
        // Lets make sure every value is valid
        if( satValue < 0 || satValue > 100 ){
            Particle.publish("setSaturation() fail", "Invalid Number: " + String( satValue ) + " | Must be between 0 and 100");
            return -1;  // Contains an invalid value
        }

        // String is good to go!
        colorSaturation = satValue;
        updateHSL();

        setMode( "custom", true );

        return 1;  // All the things are broken.
    }
    
    

    /*
     * Set a saturation to use with the "custom" mode
     */
    int setLightness( String lightString ){
        int lightValue = lightString.toInt();
        
        // Lets make sure every value is valid
        if( lightValue < 0 || lightValue > 100 ){
            Particle.publish("setLightness() fail", "Invalid Number: " + String( lightValue ) + " | Must be between 0 and 100");
            return -1;  // Contains an invalid value
        }

        // String is good to go!
        colorLightness = lightValue;
        updateHSL();

        setMode( "custom", true );
        return 1;  // All the things are broken.
    }


    bool updateHSL(){
        thisHSL = HSL( colorHue, colorSaturation, colorLightness );
        thisRGB = thisHSL.toRGB();

        Particle.publish("updateHSL()", "HSL H:" +  String( thisHSL.H ) + " S:" +  String( thisHSL.S ) + " L:" +  String( thisHSL.L ) + " | RGB R:" +  String( thisRGB->R ) + " G:" +  String( thisRGB->G ) + " B:" +  String( thisRGB->B ) );

        return true;
    }


    /*
     * Define how bright the light can be using a value between 0 and 255. 0 being off, and 255 being on.
     */
    int setLux( String newLux ){
        int newLuxInt = newLux.toInt();
        int currentLux = strip.getBrightness();     // Current Value
        int luxDiff = newLuxInt - currentLux;       // Distance between our values
        if( newLuxInt < 0 || newLuxInt > 250 ){
            Particle.publish("setLux() fail", "Must have value between 0 and 250" );
            return -1;   // Not a valid brightness value between 0 and 250
        }else if( luxDiff == 0 ){
            return 1;   // Already running at this value
        }


        if( luxChange == 0 ){
            if( currentLux == 0 ){  setMode( lightMode, false );   }   // Needed to retrigger the mode after turning off
            strip.setBrightness( newLuxInt );    // 0: off and 255: Max
            setMode( lightMode, false );   // Ensure the color doesn't shift
            strip.show();
        }else{
            // Going to smooth out the brightness change
            int luxStepSpeed =  luxChange / abs( luxDiff );  // Milleseconds per step based on the desired change
            if( luxStepSpeed == 0 ){ luxStepSpeed = 1; }
            // Particle.publish("setLux() LuxStepSpeed", String(luxChange) + " / " + String( luxDiff ) + " = " + String(luxStepSpeed)  );

            // Ensure our minimum delay is 20ms to avoid overloading the process
            int stepSize = 1;
            if( luxStepSpeed < 5 ){
                // Particle.publish("setLux() fix step", String( luxStepSpeed ) );
                while( luxStepSpeed < 5 ){
                    luxStepSpeed = luxStepSpeed * 2;
                    stepSize = stepSize * 2;
                }

                // Particle.publish("setLux() fixed step", String( luxStepSpeed ) );
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

                setMode( lightMode, false );   // Ensure the color doesn't shift
                strip.setBrightness( stepLux );    // 0: off and 255: Max
                strip.show();


                delay(luxStepSpeed);
            }while( runStepper );

            setMode( lightMode, false );   // Ensure the color doesn't shift
            strip.setBrightness( newLuxInt );    // 0: off and 255: Max
            strip.show();
        }


        lightBrightness = newLuxInt;
        updateQuickSetting();
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
        updateQuickSetting();
        pushToEEPROM(); // Save Setting

        return 1;
    }


    int setModeExpose( String newMode ){
        return setMode( newMode, true );
    }

    /*
     * Move between presets, or a custom color for the light.
     */
    int setMode( String newMode, bool includeShow ){
        // Particle.publish( "setMode()", newMode + " | RGB R:" +  String( thisRGB->R ) + " G:" +  String( thisRGB->G ) + " B:" +  String( thisRGB->B ) );
        if( newMode == "bright"){
            colorAll(strip.Color(255, 255, 255), 50);   // HSL: X, 100, 100
        }else if( newMode == "clear"){
            colorAll(strip.Color(255,180,130), 50);   // HSL: X, 100, 100
        }else if( newMode == "warm"){
            colorAll(strip.Color(255,150,60), 50);      // HSL: 25, 90, 66
        }else if( newMode == "rainbow"){
            // rainbow(20);
            rainbowForever( 10 );
        }else if( newMode == "custom"){
            // Use the value stored as customColor (from setColor) to light up the show
            if( colorHue == NULL ){
                return -1;
            }

            colorAll(strip.Color( thisRGB->R, thisRGB->G, thisRGB->B), 50);
        }else{
            return -1;
        }


        if( includeShow == false ){
            return 1;
        }

        strip.show();
        lightMode = newMode;

        updateQuickSetting();
        pushToEEPROM(); // Save Setting

        return 1;
    }

    /*
     * Quickly toggles the light on and off. Use as the central place to turn on the light. Accepts On, Off, 1, 0
     */
    int toggleLight( String newState ){
        if( newState == "on" || newState == "1" ){
            Particle.publish( "toggleLight()", "On" );
            setMode( String( lightMode ), false );
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

        updateQuickSetting();
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
        int hue; // colorHue;
        int saturation; // colorSaturation;
        int lightness; // colorLightness;
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
            configVals.hue = colorHue;
            configVals.saturation = colorSaturation;
            configVals.lightness = colorLightness;
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
            colorHue = configVals.hue;
            colorSaturation = configVals.saturation;
            colorLightness = configVals.lightness;
            luxChange  = configVals.luxChange;
        }else{
            EEPROM.clear();
        }

        return;
    }
// Persist Variables
//


//
// Manage the quickSetting variable
void updateQuickSetting(){
    quickSetting = lightMode + "," + colorHue + "," + colorSaturation + "," + colorLightness + "," + lightBrightness + "," + luxChange;           // Comma seperated string - Mode, Hue, Saturation, Lightness, Brightness, Light Change
    return;
}




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
