/* 
 *  -------------------------------------------------------------------------
 *  PCB Exposure Timer Software
 *  ---------------------------
 *  
 *  Target platform: Arduino Nano or Arduino Mini Pro (or plain-old Atmel
 *  '328, without all the Ardino addons.
 *  
 *  Note: This version is NOT ready for release... I'm using it to test my
 *  rotary encoder & other bits.
 *  
 *  --------------------------------------------------------------------------
 *  
 *  MIT License
 *  
 *  Copyright (c) 2018 Ade Vickers aka Solution Engineers Ltd.
 *  
 *  Permission is hereby granted, free of charge, to any person obtaining a copy
 *  of this software and associated documentation files (the "Software"), to deal
 *  in the Software without restriction, including without limitation the rights
 *  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 *  copies of the Software, and to permit persons to whom the Software is
 *  furnished to do so, subject to the following conditions:
 *  
 *  The above copyright notice and this permission notice shall be included in all
 *  copies or substantial portions of the Software.
 *  
 *  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 *  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 *  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 *  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 *  SOFTWARE.
 *  
*/
#include <EEPROM.h>

// Define state machine states
enum STATE {
  RESET,
  READY,
  RUN,
  PAUSE,
  FINISH_BUZZER,
  FINISH,
  TIME_SET,
  TIME_SAVE,
  TBB_SET,
  TBB_SAVE  
};

STATE currentState = RESET;   // Current machine state
STATE nextState = RESET;      // May need this, may not

/*
 * Variables/definitions for the rotary encoder
 */
// Pins
#define EncoderA      2
#define EncoderB      4
#define EncoderSW     3

// Control variables
bool encoderA  = false;       // Value of encoder A
bool encoderB  = false;       // Value of encoder B
bool encoderSW = false;       // Value of encoder Switch
bool _encoderA = false;       // Previous encoder A value
bool _encoderSW = false;      // Set to true when encoder button is pressed (until handled)
bool _encoderIgnore = false;  // Set to true to ignore encoder until the readings are good (if encoder initialises funny)

/*
 * Variables/definitions for the 4-segment LED display driver
 */
// Pins
#define DisplayData   5
#define DisplayLatch  6
#define DisplayClock  7

// Control variables
// Note that to save variable space, we'll use time offsets from a known marker
unsigned long displayTime = 0;  // Store the time we last did something with the display
byte displayState = 0;          // Bitmap as follows:
                                // Bits 0,1 = the digit to display (0->3)
                                // Bit 2 = flash enable
                                // Bit 3 = display on/off (when flashing; otherwise ignored)
                                // Bits 4-7 = flash delay count, 0-63.

/*
 * Other global program variables
 */
unsigned int setTime = 0;       // The pre-defined duration of the timer
unsigned int timeLeft = 0;      // Number of seconds remaining of current timer run
unsigned long refTime = 0;      // Reference time (when we last decremented timeLeft)

// Variables below this line are/were used for testing & shall be removed.
signed long pos = 0;      // Position indicator
signed long _pos = 0;     // Previous position - if different, print the new pos.

void setup() {
  // Enable serial output for testing purposes
  Serial.begin(38400);
  Serial.println("Encoder Testing.");

  // Set up the encoder pins
  pinMode(EncoderA,INPUT);
  pinMode(EncoderB,INPUT);
  pinMode(EncoderSW,INPUT);

  // Setup the display pins & reset to zero
  pinMode(DisplayData,OUTPUT);
  pinMode(DisplayLatch,OUTPUT);
  pinMode(DisplayClock,OUTPUT);

  digitalWrite(DisplayData,false);
  digitalWrite(DisplayLatch,false);
  digitalWrite(DisplayClock,false);

  // Read the default time from the EEPROM
  EEPROM.get(0,setTime);
  Serial.print("Read ");
  Serial.print(setTime);
  Serial.println(" from EEPROM.");

  // If setTime is zero or greater than 60m, set to 2m30s and write that value to the EEPROM.
  if(setTime == 0 || setTime > 3600) {
    Serial.println("Setting EPROM value to 2m30s");
    setTime = 150;          //2m30s = 150s
    EEPROM.put(0,setTime);
  } else {
    Serial.print("EPROM sets time to ");
    Serial.println(fancyTime(setTime));
  }
  
  Serial.println("Preparation complete. You may now putten das fingers into das springenworkz und twiddle de knobs");
}

void loop() {

  if(nextState != currentState) {
    switch(nextState) {
      case RESET:
        // Moving into reset state. Nothing to do here.
        currentState = nextState;
        break;
      case TIME_SET:
        // Moving into time set state. Prepare the encoder.
        encoderA = digitalRead(EncoderA);
        encoderB = digitalRead(EncoderB);
        _encoderA = encoderA;
        
        if(encoderA != encoderB) {
          _encoderIgnore = true;
        }

    }
  }

  switch(currentState) {
    case RESET:
      break;
    case RUN:
      break;
    case PAUSE:
      break;
    case FINISH_BUZZER:
      break;
    case FINISH:
      break;
    case TIME_SET:
      readEncoder();
      break;
  }
}

void readEncoder() {
  
  // Read the three inputs. EncoderA = EncoderB if the encoder is in a detent.
  encoderA = digitalRead(EncoderA);

  // Debounce by repeatedly re-reading until things are stable...
  bool done;
  do {
    done = true;
    delayMicroseconds(10);  // Doze for 10uS just to be sure to be sure
    if (digitalRead(EncoderA) != encoderA) {done = false; encoderA = digitalRead(EncoderA);}
  } while (!done);

  encoderB = digitalRead(EncoderB);
  encoderSW = digitalRead(EncoderSW);

  // If the encoder is not yet known to be stabilised at an indent, check now.
  if (_encoderIgnore) {
    if (encoderA == encoderB) {
      // OK, that's near enough.
      _encoderIgnore = false;
      _encoderA = encoderA;
    } 
  } else {
    // Has sigA changed since we last read it??
    if (encoderA != _encoderA) {
      // Change! But in which direction?
      if (encoderA == encoderB) {
        // Clockwise, add
        pos++;
      } else {
        // Anticlockwise, subtract
        pos--;
      }
      _encoderA = encoderA;
    }

    if (encoderSW != _encoderSW) {
      // Button state change. Make a note of this
      _encoderSW = encoderSW;
      if (encoderSW) {
        Serial.println("Encoder button pwessed");
      } else {
        Serial.println("Encoder button weleased");
      }
    }

    // Encoder testing... dump to serial port
    if (_pos != pos) {
      _pos = pos;
      Serial.print("New encoder value: ");
      Serial.println(pos);
    }
  }
}

String fancyTime(int numSec) {
  // Convert number of seconds to MM:SS format
  char mins[3];
  char secs[3];
  
  itoa(numSec / 60,mins,10);
  itoa(numSec % 60,secs,10);
  
  String output = mins;
  output += ":";
  if(numSec % 60 < 10) output += "0";
  output += secs;
  
  return output;
}

