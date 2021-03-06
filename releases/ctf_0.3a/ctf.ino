/**
 * DIY RF Laptimer by Andrey Voroshkov (bshep)
 * SPI driver based on fs_skyrf_58g-main.c by Simon Chambers
 * fast ADC reading code is by "jmknapp" from Arduino forum
 * fast port I/O code from http://masteringarduino.blogspot.com.by/2013/10/fastest-and-smallest-digitalread-and.html
 * CTF code by Carlos Costa (SeekNDFPV @ youtube)

The MIT License (MIT)

Copyright (c) 2016 by Andrey Voroshkov (bshep)

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

/*
TODO (from CC): Clean the code :) 
Please don't kill me once you look at it. I learned arduino in 2 days and coded this so I understand if it looks and runs terribly, but hey .. it runs!
The CTF mod was created by using Chorus RF code as base. There is now a lot of code that is unnecessary. 
I'm  positive the Chorus RF code will not work as intended after the changes that I've done to it (if you were thinking of using this code to run a Chorus RF timing system don't).
This is intended to be a standalone Capture The Flag mod only (or whatever you can think to do with it). 
Scavenger hunt also sounds pretty sweet if you ask me! Hide a few of these and see who can get the most flags before their battery ends :D
Feel free to clear up the code and post a better version if you can!
I've commented everything in the code for you to know what I've done or if you want to change anything in it.
TODO: AUTO RSSI or some way to change RSSI without editing code
TODO: ADD button to change modes

HOWTO (from CC): Once you power up the arduino board it will start scanning for all RACEBAND frequencies. It takes ~6 seconds to do a loop. 
Once it finds the first frequency it will change the LED colour depending if you are blue team (RACEBAND 1-4) or red team (RACEBAND 5-8)
Once the flag changes colour it will only scan for the other team frequencies so the scanning takes place faster (~3 seconds for a loop)
Protect your flags anyway you can!

GAME IDEA 1 (2 flags - MODE 1)
Setup both teams in each flag area. Prime your flag by putting your team's drone next to it.
Try to capture the other team's flag and defend yours. Game ends when everyone runs out of their battery (TEAM with both flags WINS or DRAW 1 FLAG EACH).
Press reset button in each arduino to restart game

GAME IDEA 2 (multiple flags - MODE 1)
Team death match. Both teams start from a staging area and try to capture and hold as many flags as you can.
Game ends when everyone runs out of their battery (TEAM with most flags WINS).
Press reset button in each arduino to restart game

GAME IDEA 3 (DEATH MATCH - MODE 2)
Up to 8 players (R1-R8) start from a staging area and try to capture as many flags as they can before everyone runs out of battery.
The one with the most flags captured wins.
- R1 BLUE
- R2 GREEN
- R3 YELLOW
- R4 RED
- R5 CYAN
- R6 VIOLET
- R7 WHITE
- R8 // not used at the moment

GAME IDEA 4 (SCAVENGER HUNT - MODE 3)
Up to 8 players (R1-R8) start from a staging area and try to find as many HIDDEN flags as they can before everyone runs out of battery.
Once a flag is captured it cannot be recaptured by another drone. The one with the most flags captured wins.
- R1 BLUE
- R2 GREEN
- R3 YELLOW
- R4 RED
- R5 CYAN
- R6 VIOLET
- R7 WHITE
- R8 // not used at the moment


*/


#include "FastLED.h" 
#define NUM_LEDS 4  // (CC) change the the number of leds you have in your strip. I wouldn't recommend any more then 10 if you are powering your unit with a power bank (2amps 5v), otherwise power the leds using external source and remember to link up all grounds together.
#define BRIGHTNESS  64  // (CC) how bright the leds are. 64 is max
#define DATA_PIN 5      // (CC) this is the data pin (D5) on your arduino where you connect the signal cable for the leds. This works only with leds that have 5v,Ground,Din (if you have one that has R,G,B connections it won't work.
CRGB leds[NUM_LEDS];


#define API_VERSION 2 // version number to be increased with each API change (int16)

// #define DEBUG

#ifdef DEBUG
    #define DEBUG_CODE(x) do { x } while(0)
#else
    #define DEBUG_CODE(x) do { } while(0)
#endif

uint8_t MODULE_ID = 0;
uint8_t MODULE_ID_HEX = '0';

#define SERIAL_DATA_DELIMITER '\n'

#include <avr/pgmspace.h>

#define BAUDRATE 115200

const uint16_t musicNotes[] PROGMEM = { 523, 587, 659, 698, 784, 880, 988, 1046 };

// number of analog rssi reads to average for the current check.
// single analog read with FASTADC defined (see below) takes ~20us on 16MHz arduino
// so e.g. 10 reads will take 200 ms, which gives resolution of 5 RSSI reads per ms,
// this means that we can theoretically have 1ms timing accuracy :)
#define RSSI_READS 5 // 5 should give about 10 000 readings per second

//----- RSSI --------------------------------------
#define FILTER_ITERATIONS 5
uint16_t rssiArr[FILTER_ITERATIONS + 1];
uint16_t rssiThreshold = 210; // (CC) play around with this value to find your best RSSI threshold. (between 190 to 260 seem to be acceptable values). The higher the value the closer the quad needs to get. But if it's too high it wont detect it.
uint16_t rssi;

#define RSSI_MAX 1024
#define RSSI_MIN 0
#define MAGIC_THRESHOLD_REDUCE_CONSTANT 2
#define THRESHOLD_ARRAY_SIZE  100
uint16_t rssiThresholdArray[THRESHOLD_ARRAY_SIZE];

#define DEFAULT_RSSI_MONITOR_DELAY_CYCLES 1000 //each 100ms, if cycle takes 100us
#define MIN_RSSI_MONITOR_DELAY_CYCLES 10 //each 1ms, if cycle takes 100us, to prevent loss of communication
uint16_t rssiMonitorDelayCycles = DEFAULT_RSSI_MONITOR_DELAY_CYCLES;


//----- Voltage monitoring -------------------------
#define VOLTAGE_READS 3 //get average of VOLTAGE_READS readings

// analog readings less than VOLTAGE_ZERO_THRESHOLD value won't be sent.
// This way entire chorus device will send voltages only from devices that are attached to LiPo
// So if single Solo device has LiPo attached, then broadcast voltage request to
// entire Chorus device will produce a single voltage response.
#define VOLTAGE_ZERO_THRESHOLD 100

uint16_t voltage = 0;


//----- Calibration -------------------------------
uint8_t isCalibrated = 1;
uint32_t calibrationMilliseconds = 0;

// Usage of signed int calibration constant:
// calibratedMs = readMs + readMs/timeCalibrationConst
int32_t timeCalibrationConst = 0;

//----- other globals------------------------------
uint8_t allowEdgeGeneration = 0;
uint8_t channelIndex = 0;     // (CC) This starts the module in channel 1 ( yes, 0 means channel 1, 1 means channel 2 ... 7 means channel 8 .. you get the picture ;)  )
uint8_t bandIndex = 0;        // (CC) This starts the module in Raceband. Check Channels.h file if you want to use another band instead.
uint8_t isRaceStarted = 1;    
uint8_t isSoundEnabled = 1;
uint8_t isConfigured = 1;
uint8_t rssiMonitor = 1;
uint8_t newLapIndex = 0;
uint8_t shouldSkipFirstLap = 1;
uint8_t isSendingData = 0;
uint8_t sendStage = 0;
uint8_t sendLapTimesIndex = 0;
uint8_t sendLastLapIndex = 0;
uint8_t shouldSendSingleItem = 0;
uint8_t lastLapsNotSent = 0;
uint16_t rssiMonitorDelayExpiration = 0;
uint16_t frequency = 0;
uint16_t runloop = 0;          // (CC) because I couldn't find any other way to make the code wait after the frequency had been changed I have to run a numeric loop that runs from 0 to 10. At 10 it moves to the next frequency then resets to 0, and runs the loop again.
uint16_t bluered = 0;          // (CC) This is an numberic value to identify if the flag has been capture and if so by blue or red (0 = not captured ; 1= blue flag ; 2=red flag
uint16_t mode = 2;          // (CC) mode 1:CTF team (only blue flag and red flag) - mode 2:Free For All Deathmatch (each flag will turn to each colour of the pilot) - mode 3:Scavenger hunt (once you capture a flag it will stay in that colour, no one else can capture it)

// ----------------------------------------------------------------------------
#include "fastReadWrite.h"
#include "fastADC.h"
#include "pinAssignments.h"
#include "channels.h"
// #include "sendSerialHex.h"
#include "rx5808spi.h"
#include "sounds.h"

// ----------------------------------------------------------------------------
void setup() {


    // led stuff (CC) 
    delay(2000);
    FastLED.addLeds<WS2811, DATA_PIN, RGB>(leds, NUM_LEDS);
    FastLED.setBrightness(  BRIGHTNESS );
     for(int whiteLed = 0; whiteLed < NUM_LEDS; whiteLed = whiteLed + 1) 
     {
               leds[whiteLed] = CRGB::Black;
               FastLED.show();

     }
    // end of led stuff (CC) 
    
    // initialize digital pin 13 LED as an output.
    pinMode(led, OUTPUT);
    digitalHigh(led);

    // init buzzer pin
    pinMode(buzzerPin, OUTPUT);

    //init raspberrypi interrupt generator pin
    pinMode(pinRaspiInt, OUTPUT);
    digitalLow(pinRaspiInt);

    // SPI pins for RX control
    setupSPIpins();

    // set the channel as soon as we can
    // faster boot up times :)
    frequency = setModuleChannel(channelIndex, bandIndex);

    Serial.begin(BAUDRATE);

    initFastADC();

    // Setup Done - Turn Status LED off.
    digitalLow(led);

    
    // show user what mode he is on
    if ( mode  == 1 )
    {
      mode = 1;
      digitalLow(led);
      delay (1000);
      digitalHigh(led);
      delay (1000);
      digitalLow(led);
      delay (1000);
    }
    else if (mode  == 2)
    {
      digitalLow(led);
      delay (1000);
      digitalHigh(led);
      delay (1000);
      digitalLow(led);
      delay (1000);
      digitalHigh(led);
      delay (1000);
      digitalLow(led);
      delay (1000);
    }
    else if (mode  == 3)
    {
      digitalLow(led);
      delay (1000);
      digitalHigh(led);
      delay (1000);
      digitalLow(led);
      delay (1000);
      digitalHigh(led);
      delay (1000);
      digitalLow(led);
      delay (1000);
      digitalHigh(led);
      delay (1000);
      digitalLow(led);
      delay (1000);
    }

    DEBUG_CODE(
        pinMode(serialTimerPin, OUTPUT);
        pinMode(loopTimerPin, OUTPUT);
        pinMode(bufferBusyPin, OUTPUT);
        pinMode(sequencePin, OUTPUT);
    );

     allowEdgeGeneration = 0;
}
// ----------------------------------------------------------------------------
void loop() {
    DEBUG_CODE(
        digitalToggle(loopTimerPin);
    );

if (mode == 1) // (CC) CTF mode
{
 if (bluered == 0)  // (CC) checks if the flag has not been captured yet
 {
   if ( channelIndex < 7 )
   {
   if (runloop < 10 ) // (CC) running a int loop 10x to give some time to the receiver to pickup any RSSI's. I found if I didn't do this it would change the channel to quickly and not find any RSSI. If you know any other way that this would work let me know :)
   {
     runloop++;
   }
   if (runloop >= 10)  //(CC) once runloop gets to 10 it will go up one channel and reset the runloop to 0 (in order to change to the next channel and so on)
   {
     channelIndex++;
     frequency = setModuleChannel(channelIndex, bandIndex);
     runloop = 0;
     
     delay(1000);
     
   }
   }
   else if ( channelIndex == 7 )  // (CC) once it gets to the max channels it will reset back to the first channel. This is set in RACEBAND band.
   {
     channelIndex = 0;
     frequency = setModuleChannel(channelIndex, bandIndex);
     runloop = 0;
   }
 }
 else if (bluered == 2) // (CC)  the flag is currently captured by the red team (RACEBAND 5-8). The code will now only search for blue team frequencies (RACEBAND 1-4) to save time.
 {
   if ( channelIndex < 4 )
   {
   if (runloop < 10 )
   {
     runloop++;

   }
   if (runloop >= 10)
   {
     channelIndex++;
     frequency = setModuleChannel(channelIndex, bandIndex);
     runloop = 0;
     
     delay(1000);
     
   }
   }
   else if ( channelIndex == 4 )
   {
     channelIndex = 0;
     frequency = setModuleChannel(channelIndex, bandIndex);
     runloop = 0;
   }
 }
 else if (bluered == 1) // (CC)  the flag is currently captured by the blue team (RACEBAND 1-4). The code will now only search for blue team frequencies (RACEBAND 5-8) to save time.
 {

   if ( channelIndex < 7 && channelIndex > 3 )
   {
   if (runloop < 10 )
   {
     runloop++;

   }
   if (runloop >= 10)
   {
     channelIndex++;
     frequency = setModuleChannel(channelIndex, bandIndex);
     runloop = 0;
     
     digitalLow(led);
     delay(1000);
     
   }
   }
   else if ( channelIndex == 7 )
   {
     channelIndex = 4;
     frequency = setModuleChannel(channelIndex, bandIndex);
     runloop = 0;
   }
 }
} //(CC) end of channel change looping for CTF
else if ( mode == 2 )  // FFA DEATHMATCH
{
  if ( channelIndex < 7 )
   {
   if (runloop < 10 ) // (CC) running a int loop 10x to give some time to the receiver to pickup any RSSI's. I found if I didn't do this it would change the channel to quickly and not find any RSSI. If you know any other way that this would work let me know :)
   {
     runloop++;
   }
   if (runloop >= 10)  //(CC) once runloop gets to 10 it will go up one channel and reset the runloop to 0 (in order to change to the next channel and so on)
   {
     channelIndex++;
     frequency = setModuleChannel(channelIndex, bandIndex);
     runloop = 0;
     
     delay(1000);
     
   }
   }
   else if ( channelIndex == 7 )  // (CC) once it gets to the max channels it will reset back to the first channel. This is set in RACEBAND band.
   {
     channelIndex = 0;
     frequency = setModuleChannel(channelIndex, bandIndex);
     runloop = 0;
   }
 }
 else if ( mode == 3 )  // SCAVENGER HUNT
{
  if ( bluered == 0)
  {
  if ( channelIndex < 7 )
   {
   if (runloop < 10 ) // (CC) running a int loop 10x to give some time to the receiver to pickup any RSSI's. I found if I didn't do this it would change the channel to quickly and not find any RSSI. If you know any other way that this would work let me know :)
   {
     runloop++;
   }
   if (runloop >= 10)  //(CC) once runloop gets to 10 it will go up one channel and reset the runloop to 0 (in order to change to the next channel and so on)
   {
     channelIndex++;
     frequency = setModuleChannel(channelIndex, bandIndex);
     runloop = 0;
     
     delay(1000);
     
   }
   }
   else if ( channelIndex == 7 )  // (CC) once it gets to the max channels it will reset back to the first channel. This is set in RACEBAND band.
   {
     channelIndex = 0;
     frequency = setModuleChannel(channelIndex, bandIndex);
     runloop = 0;
   }
 }
}
 
   


    rssi = getFilteredRSSI();
    
    if (rssiThreshold > 0) { // threshold = 0 means that we don't check rssi values
        if(rssi > rssiThreshold) 
        { // rssi above the threshold - drone is near
            if (allowEdgeGeneration) 
            {  // we haven't fired event for this drone proximity case yet
                allowEdgeGeneration = 0;
                gen_rising_edge(pinRaspiInt);  //generate interrupt for EasyRaceLapTimer software
/*
                uint32_t now = millis();
                uint32_t diff = now - lastMilliseconds;
                if (timeCalibrationConst) {
                    diff = diff + (int32_t)diff/timeCalibrationConst;
                }*/
                        digitalLow(led);
                        
                     //   lastMilliseconds = now;
                        // during the race play tone sequence even if no more laps can be logged


                  if ( mode == 1 )
                  {
                        if (channelIndex == 0) // (CC) BLUE TEAM STARTS HERE (CC) checks to see if your drone is in channel 1 
                        {
                             for(int whiteLed = 0; whiteLed < NUM_LEDS; whiteLed = whiteLed + 1) 
                              {
                                 playLapTones(); // (CC) does a beep
                                 leds[whiteLed] = CRGB::Blue;  // (CC) changes leds to BLUE
                                 FastLED.show();
                                 delay(1000);   // (CC) takes 1 second to change leds. You can make this faster or slower if you want
                                 bluered=1;     // (CC) changes the flag to be marked as captured for team BLUE
                                 runloop=0;    // (CC) sets runloop to 0 to start changing frequencies
                                channelIndex=4;  // (CC) sets the first channel of the opposite team (team blue cannot double capture the flag since it's already theirs
                                frequency = setModuleChannel(channelIndex, bandIndex);    // (CC) this is the code to set the frequency. The band never changes, only the channel, marked above. The 3 next checks are all the same for the next frequencies

                            }

                        }
                        else if (channelIndex == 1) //  (CC) checks to see if your drone is in channel 2
                         {
                             for(int whiteLed = 0; whiteLed < NUM_LEDS; whiteLed = whiteLed + 1) 
                              {
                                 playLapTones();
                                 leds[whiteLed] = CRGB::Blue;
                                 FastLED.show();
                                 delay(1000);
                                 bluered=1;
                                 runloop=0;
                                channelIndex=4;
                                frequency = setModuleChannel(channelIndex, bandIndex);

                             }
                        }
                       
                     else  if (channelIndex == 2)  //  (CC) checks to see if your drone is in channel 3
                     {
                             for(int whiteLed = 0; whiteLed < NUM_LEDS; whiteLed = whiteLed + 1) 
                              {
                                  leds[whiteLed] = CRGB::Blue;
                                  FastLED.show();
                                  delay(1000);
                                 
                                 bluered=1;
                                 runloop=0;
                                 channelIndex=4;
                                 frequency = setModuleChannel(channelIndex, bandIndex);

                            }

                       }
                       else if (channelIndex == 3)  //  (CC) checks to see if your drone is in channel 4
                         {
                             for(int whiteLed = 0; whiteLed < NUM_LEDS; whiteLed = whiteLed + 1) 
                              {
                                  leds[whiteLed] = CRGB::Blue;
                                  FastLED.show();
                                   delay(1000);
                                 
                                 bluered=1;
                                 runloop=0;
                                channelIndex=4;
                                frequency = setModuleChannel(channelIndex, bandIndex);

                             }
                        }
                       else if (channelIndex == 4)   // (CC) RED TEAM START HERE // (CC) checks to see if your drone is in channel 5
                         {
                             for(int whiteLed = 0; whiteLed < NUM_LEDS; whiteLed = whiteLed + 1) 
                              {
                                  leds[whiteLed] = CRGB::Green;  // (CC) changes leds to RED .. yes i know it says green. I don't know why it just does, trust me! 
                                  FastLED.show();
                                  delay(1000);                                  
                                  bluered=2;     // (CC) changes the flag to be marked as captured for team RED                                  
                                  channelIndex=0;  // (CC) sets the first channel of the opposite team (team red cannot double capture the flag since it's already theirs)
                                  runloop=0;    // (CC) sets runloop to 0 to start changing frequencies
                                  frequency = setModuleChannel(channelIndex, bandIndex);    // (CC) this is the code to set the frequency. The band never changes, only the channel, marked above. The 3 next checks are all the same for the next frequencies


                             }
                        }
                        else if (channelIndex == 5)  //  (CC) checks to see if your drone is in channel 6
                         {
                             for(int whiteLed = 0; whiteLed < NUM_LEDS; whiteLed = whiteLed + 1) 
                              {
                                  leds[whiteLed] = CRGB::Green;
                                  FastLED.show();
                                   delay(1000);
                                  bluered=2;
                                 runloop=0;
                                channelIndex=0;
                                frequency = setModuleChannel(channelIndex, bandIndex);

                             }
                        }
                        else if (channelIndex == 6)  //  (CC) checks to see if your drone is in channel 7
                         {
                             for(int whiteLed = 0; whiteLed < NUM_LEDS; whiteLed = whiteLed + 1) 
                              {
                                  leds[whiteLed] = CRGB::Green;
                                  FastLED.show();
                                   delay(1000);
                                  bluered=2;
                                 runloop=0;
                                channelIndex=0;
                                frequency = setModuleChannel(channelIndex, bandIndex);

                             }
                        }
                        else if (channelIndex == 7)  //  (CC) checks to see if your drone is in channel 8
                         {
                             for(int whiteLed = 0; whiteLed < NUM_LEDS; whiteLed = whiteLed + 1) 
                              {
                                  leds[whiteLed] = CRGB::Green;
                                  FastLED.show();
                                   delay(1000);
                                  bluered=2;
                                 runloop=0;
                                channelIndex=0;
                                frequency = setModuleChannel(channelIndex, bandIndex);

                             }
                        }
                  }
                  else if ( mode == 2 )  // (CC) DEATHMATCH
                  {
                    if (channelIndex == 0) // (CC) checks to see if your drone is in channel 1 
                        {
                             for(int whiteLed = 0; whiteLed < NUM_LEDS; whiteLed = whiteLed + 1) 
                              {
                                 playLapTones(); // (CC) does a beep
                                 leds[whiteLed] = CRGB::Blue;  // (CC) changes leds to BLUE
                                 FastLED.show();
                                 delay(1000);   // (CC) takes 1 second to change leds. You can make this faster or slower if you want
                                 runloop=0;    // (CC) sets runloop to 0 to start changing frequencies
                                 
                                channelIndex=1; // (CC) changes to the next frequency so it doesn't loop the recapture
                                frequency = setModuleChannel(channelIndex, bandIndex);   // (CC) this is what changes the frequency. The band stays the same.

                            }

                        }
                        else if (channelIndex == 1) //  (CC) checks to see if your drone is in channel 2
                         {
                             for(int whiteLed = 0; whiteLed < NUM_LEDS; whiteLed = whiteLed + 1) 
                              {
                                 playLapTones();
                                 leds[whiteLed] = CRGB::Red;
                                 FastLED.show();
                                 delay(1000);
                                 runloop=0;
                                 
                                channelIndex=2;
                                frequency = setModuleChannel(channelIndex, bandIndex);

                             }
                        }
                       
                     else  if (channelIndex == 2)  //  (CC) checks to see if your drone is in channel 3
                     {
                             for(int whiteLed = 0; whiteLed < NUM_LEDS; whiteLed = whiteLed + 1) 
                              {
                                  leds[whiteLed] = CRGB::Yellow;
                                  FastLED.show();
                                  delay(1000);
                                 runloop=0;
                                 
                                channelIndex=3;
                                frequency = setModuleChannel(channelIndex, bandIndex);
                            }

                       }
                       else if (channelIndex == 3)  //  (CC) checks to see if your drone is in channel 4
                         {
                             for(int whiteLed = 0; whiteLed < NUM_LEDS; whiteLed = whiteLed + 1) 
                              {
                                  leds[whiteLed] = CRGB::Green;
                                  FastLED.show();
                                   delay(1000);
                                 
                                 runloop=0;
                                channelIndex=4;
                                frequency = setModuleChannel(channelIndex, bandIndex);

                             }
                        }
                       else if (channelIndex == 4)   // (CC) RED TEAM START HERE // (CC) checks to see if your drone is in channel 5
                         {
                             for(int whiteLed = 0; whiteLed < NUM_LEDS; whiteLed = whiteLed + 1) 
                              {
                                  leds[whiteLed] = CRGB::Purple;
                                  FastLED.show();
                                  delay(1000);                                 
                                  runloop=0;   
                                  channelIndex=5;
                                  frequency = setModuleChannel(channelIndex, bandIndex);    // (CC) this is the code to set the frequency. The band never changes, only the channel, marked above. 


                             }
                        }
                        else if (channelIndex == 5)  //  (CC) checks to see if your drone is in channel 6
                         {
                             for(int whiteLed = 0; whiteLed < NUM_LEDS; whiteLed = whiteLed + 1) 
                              {
                                  leds[whiteLed] = CRGB::Cyan;
                                  FastLED.show();
                                  delay(1000);
                                 runloop=0;
                                channelIndex=6;
                                frequency = setModuleChannel(channelIndex, bandIndex);

                             }
                        }
                        else if (channelIndex == 6)  //  (CC) checks to see if your drone is in channel 7
                         {
                                
                           
                             for(int whiteLed = 0; whiteLed < NUM_LEDS; whiteLed = whiteLed +2 ) 
                              {
                                 
                                   FastLED.delay(33);
                                  leds[whiteLed] = CRGB::Gray;
                                  FastLED.show();
                                   delay(1000);
                                 runloop=0;
                                channelIndex=7;
                                frequency = setModuleChannel(channelIndex, bandIndex);

                                

                             }
                        }
                        else if (channelIndex == 7)  //  (CC) checks to see if your drone is in channel 8
                         {
                             for(int whiteLed = 0; whiteLed < NUM_LEDS; whiteLed = whiteLed + 1) 
                              {
                                  leds[whiteLed] = CRGB::Gray; // need to find another colour
                                  FastLED.show();
                                   delay(1000);
                                 runloop=0;
                                channelIndex=0;
                                frequency = setModuleChannel(channelIndex, bandIndex);
                             }
                        }
                  }
                  else if ( mode == 3 )  // (CC) SCAVENGER HUNT
                  {
                    if (channelIndex == 0) // (CC) checks to see if your drone is in channel 1 
                        {
                             for(int whiteLed = 0; whiteLed < NUM_LEDS; whiteLed = whiteLed + 1) 
                              {
                                 playLapTones(); // (CC) does a beep
                                 leds[whiteLed] = CRGB::Blue;  // (CC) changes leds to BLUE
                                 FastLED.show();
                                 delay(1000);   // (CC) takes 1 second to change leds. You can make this faster or slower if you want
                                 runloop=0;    // (CC) sets runloop to 0 to start changing frequencies
                                 bluered=3;

                            }

                        }
                        else if (channelIndex == 1) //  (CC) checks to see if your drone is in channel 2
                         {
                             for(int whiteLed = 0; whiteLed < NUM_LEDS; whiteLed = whiteLed + 1) 
                              {
                                 playLapTones();
                                 leds[whiteLed] = CRGB::Red;
                                 FastLED.show();
                                 delay(1000);
                                 runloop=0;
                                 bluered=3;

                             }
                        }
                       
                     else  if (channelIndex == 2)  //  (CC) checks to see if your drone is in channel 3
                     {
                             for(int whiteLed = 0; whiteLed < NUM_LEDS; whiteLed = whiteLed + 1) 
                              {
                                  leds[whiteLed] = CRGB::Yellow;
                                  FastLED.show();
                                  delay(1000);
                                 runloop=0;
                                 bluered=3;
                            }

                       }
                       else if (channelIndex == 3)  //  (CC) checks to see if your drone is in channel 4
                         {
                             for(int whiteLed = 0; whiteLed < NUM_LEDS; whiteLed = whiteLed + 1) 
                              {
                                  leds[whiteLed] = CRGB::Green;
                                  FastLED.show();
                                  delay(1000);
                                  runloop=0;
                                  bluered=3;

                             }
                        }
                       else if (channelIndex == 4)   // (CC) RED TEAM START HERE // (CC) checks to see if your drone is in channel 5
                         {
                             for(int whiteLed = 0; whiteLed < NUM_LEDS; whiteLed = whiteLed + 1) 
                              {
                                  leds[whiteLed] = CRGB::Purple;
                                  FastLED.show();
                                  delay(1000);                                 
                                  runloop=0;
                                  bluered=3;


                             }
                        }
                        else if (channelIndex == 5)  //  (CC) checks to see if your drone is in channel 6
                         {
                             for(int whiteLed = 0; whiteLed < NUM_LEDS; whiteLed = whiteLed + 1) 
                              {
                                  leds[whiteLed] = CRGB::Cyan;
                                  FastLED.show();
                                  delay(1000);
                                 runloop=0;
                                 bluered=3;
                        
                             }
                        }
                        else if (channelIndex == 6)  //  (CC) checks to see if your drone is in channel 7
                         {
                             for(int whiteLed = 0; whiteLed < NUM_LEDS; whiteLed = whiteLed + 1) 
                              {
                                  leds[whiteLed] = CRGB::Gray;
                                  FastLED.show();
                                   delay(1000);
                                 runloop=0;
                                 bluered=3;

                             }
                        }
                        else if (channelIndex == 7)  //  (CC) checks to see if your drone is in channel 8
                         {
                             for(int whiteLed = 0; whiteLed < NUM_LEDS; whiteLed = whiteLed + 1) 
                              {
                                  leds[whiteLed] = CRGB::Gray; // need to find another colour
                                  FastLED.show();
                                   delay(1000);
                                 runloop=0;
                                 bluered=3;
                              }
                        }
                  }
                     
                
                
            }
        }
        else  
        {
           allowEdgeGeneration = 1; // we're below the threshold, be ready to catch another case
           //digitalHigh(led);
        }
   }
}
   


// ----------------------------------------------------------------------------
uint16_t setRssiMonitorDelay(uint16_t delay) {
    return delay < MIN_RSSI_MONITOR_DELAY_CYCLES ? MIN_RSSI_MONITOR_DELAY_CYCLES : delay;
}
// ----------------------------------------------------------------------------
uint16_t getFilteredRSSI() {
    rssiArr[0] = readRSSI();

    // several-pass filter (need several passes because of integer artithmetics)
    // it reduces possible max value by 1 with each iteration.
    // e.g. if max rssi is 300, then after 5 filter stages it won't be greater than 295
    for(uint8_t i=1; i<=FILTER_ITERATIONS; i++) {
        rssiArr[i] = (rssiArr[i-1] + rssiArr[i]) >> 1;
    }

    return rssiArr[FILTER_ITERATIONS];
}


/* 
// ----------------------------------------------------------------------------
void sortArray(uint16_t a[], uint16_t size) {
    for(uint16_t i=0; i<(size-1); i++) {
        for(uint16_t j=0; j<(size-(i+1)); j++) {
                if(a[j] > a[j+1]) {
                    uint16_t t = a[j];
                    a[j] = a[j+1];
                    a[j+1] = t;
                }
        }
    }
}*/
// ----------------------------------------------------------------------------
uint16_t getMedian(uint16_t a[], uint16_t size) {
    return a[size/2];
}
// ----------------------------------------------------------------------------
void gen_rising_edge(int pin) {
    digitalHigh(pin); //this will open mosfet and pull the RasPi pin to GND
    delayMicroseconds(10);
    digitalLow(pin); // this will close mosfet and pull the RasPi pin to 3v3 -> Rising Edge
}
// ----------------------------------------------------------------------------
uint16_t readRSSI() {
    int rssiA = 0;

    for (uint8_t i = 0; i < RSSI_READS; i++) {
        rssiA += analogRead(rssiPinA);
    }

    rssiA = rssiA/RSSI_READS; // average of RSSI_READS readings
    return rssiA;
}
// ----------------------------------------------------------------------------
uint16_t readVoltage() {
    int voltageA = 0;

    for (uint8_t i = 0; i < VOLTAGE_READS; i++) {
        voltageA += analogRead(voltagePinA);
    }

    voltageA = voltageA/VOLTAGE_READS; // average of RSSI_READS readings
    return voltageA;
}
