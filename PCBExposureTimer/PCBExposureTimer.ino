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
#define DisplayClock  6
#define DisplayLatch  7

// Control variables
// Note that to save variable space, we'll use time offsets from a known marker
unsigned long displayTime = 0;  // Store the time we last did something with the display
byte displayDigit = 0;          // 0-3, selects the digit we're outputting at the time.
byte lampState = 0;             // Bitmap as follows:
                                //  Bit 0 = ready
                                //  Bit 1 = run
                                //  Bit 2 = pause
                                //  Bit 3 = Finished
                                //  Bit 4 = Time Set
                                //  Bit 5 = TBB Set
                                //  Bit 6 = Top Panel
                                //  Bit 7 = Bottom Panel

byte control = B00000011;       // More bitmapping....
                                //  Bits 0-5 = future expansion
                                //  Bit 6 = top panel enabled
                                //  Bit 7 = bottom panel enabled

// A bumch of constants representing the bit patterns of each digit
// Segments are ABCDEFG: This may need to be reversed if I've wired it wrong. Which I probably have...
// Colon is ignored (kinda). It will be set at runtime by the colon condition.
// Also... 1=OFF, 0=ON.
//               ABCDEFG:
#define digit_0 B00000010 //ABCDEF
#define digit_1 B10011110 // BC
#define digit_2 B00100100 //AB DE G
#define digit_3 B00001100 //ABCD  G
#define digit_4 B10011000 // BC  FG
#define digit_5 B01001000 //A CD FG
#define digit_6 B01000000 //A CDEFG
#define digit_7 B00011110 //ABC    
#define digit_8 B00000000 //ABCDEFG
#define digit_9 B00001000 //ABCD FG
#define digit_E B01100000 //A  DEFG

// The four actual time digits
byte digit[4];

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

  // Temporarily stuff some test data into the EEPROM
  EEPROM.put(0,3417);

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
    String myTime = fancyTime(setTime);
    Serial.print("EPROM sets time to ");
    Serial.println(myTime);
  }

  refTime=micros();
  
  Serial.println("Preparation complete. You may now putten das fingers into das springenworkz und twiddle de knobs");
}

void loop() {

  if(nextState != currentState) {
    Serial.print("State is currently ");
    Serial.print(currentState);
    Serial.print(", and is changing to ");
    Serial.println(nextState);
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
        break;
      case READY:
        // Nothing to do here for the moment
        currentState = nextState;
        break;
    }
  }

  unsigned long now = micros();

  switch(currentState) {
    case RESET:
      Serial.println("State is now RESET");
      timeLeft = setTime;     // Set the time to whatever setTime is.
      lampState = B10000000;  // Lamp State = all off(?)
      refreshDisplay();       // Initialise the display
      displayTime = now;
      nextState = READY;      // Prepare to move to the next state.
      break;
    case READY:
      if (now - displayTime > 3500) {
        displayTime = micros();
        displayDigit += 1;
        if (displayDigit > 3) displayDigit = 0;
        refreshDisplay();
        displayTime = now;
      }
      if (now - refTime > 5000000) {
        refTime = now;
        Serial.println("5 seconds elapsed.");
      }
      break;
    case RUN:
      Serial.println("State is now RUN");
      break;
    case PAUSE:
      Serial.println("State is now PAUSE");
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

void refreshDisplay() {
  // Called every 1000 uSec. Push 3 bytes out to the '595 units, as follows:
  // Byte 1: State LEDs.
  // Byte 2: Segments to display for the current digit
  // Byte 3: Display digit (Bits 0-4 control which anode is ON)

  // Note that as the lamp states are common anode, as is the 7-seg
  // display, we need to invert lampState before we display it.
  // digit[n] is already "inverted" in the bitmask.

  digitalWrite(DisplayLatch,LOW);

  // We need to invert lampState
  shiftOut(DisplayData,DisplayClock,LSBFIRST,lampState ^ 255);    // Send it backwards as well, fecking wiring.
  shiftOut(DisplayData,DisplayClock,LSBFIRST,digit[displayDigit]);    // A pattern
  
  switch(displayDigit){
    case 0:
      shiftOut(DisplayData,DisplayClock,MSBFIRST,1);
      break;
    case 1:
      shiftOut(DisplayData,DisplayClock,MSBFIRST,2);
      break;
    case 2:
      shiftOut(DisplayData,DisplayClock,MSBFIRST,4);
      break;
    case 3:
      shiftOut(DisplayData,DisplayClock,MSBFIRST,8);
      break;
  }
  
  digitalWrite(DisplayLatch,HIGH);

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
  
  String output = "";
  if(numSec / 60 < 10) output += "0";
  output += mins;
  output += ":";
  if(numSec % 60 < 10) output += "0";
  output += secs;

  setDigit(0,0,output);
  setDigit(1,1,output);
  setDigit(2,3,output);
  setDigit(3,4,output);
  
  return output;
}

void setDigit(byte digitNo, byte strIndex, String textTime) {
  // Pick the specified digit out of the string, convert it to an integer, and 
  // set the digit bitmask...
  // Wow, this is properly messy.
  
  Serial.print("Digit ");Serial.print(digitNo);
  switch(textTime.charAt(strIndex)) {
    case '0': digit[digitNo] = digit_0; Serial.print(" set to ");Serial.println(digit_0); break;
    case '1': digit[digitNo] = digit_1; Serial.print(" set to ");Serial.println(digit_1); break;
    case '2': digit[digitNo] = digit_2; Serial.print(" set to ");Serial.println(digit_2); break;
    case '3': digit[digitNo] = digit_3; Serial.print(" set to ");Serial.println(digit_3); break;
    case '4': digit[digitNo] = digit_4; Serial.print(" set to ");Serial.println(digit_4); break;
    case '5': digit[digitNo] = digit_5; Serial.print(" set to ");Serial.println(digit_5); break;
    case '6': digit[digitNo] = digit_6; Serial.print(" set to ");Serial.println(digit_6); break;
    case '7': digit[digitNo] = digit_7; Serial.print(" set to ");Serial.println(digit_7); break;
    case '8': digit[digitNo] = digit_8; Serial.print(" set to ");Serial.println(digit_8); break;
    case '9': digit[digitNo] = digit_9; Serial.print(" set to ");Serial.println(digit_9); break;
    default:  digit[digitNo] = digit_E; Serial.print(" set to ");Serial.println(digit_E); break;
  }
}

