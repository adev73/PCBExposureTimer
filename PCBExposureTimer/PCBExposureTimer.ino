/* 
 *  ------------------------------------------------------------------------------
 *  PCB Exposure Timer Software
 *  ------------------------------------------------------------------------------
 *  
 *  Target platform: Arduino Nano or Arduino Mini Pro (or plain-old Atmel
 *  '328, without all the Ardino addons.
 *  
 *  Note: This version is NOT ready for release... I'm using it to test my
 *  rotary encoder & other bits.
 *  
 *  ------------------------------------------------------------------------------
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
 *  ------------------------------------------------------------------------------
*/
#include <EEPROM.h>

//Debug flag, remove to dump serial port
//#define DEBUG
//#define RESET_EEPROM

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
STATE nextState = RESET;      // Used to prepare for next state, where required.

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

// Other inputs
#define StartPauseSW  8
#define ResetSW       9

// Other outputs
#define TopPanel      10
#define BottomPanel   11
#define Buzzer        12

bool startSW = false;         // True if START/PAUSE switch is pressed
bool resetSW = false;         // True if RESET switch is pressed

/*
 * Variables/definitions for the 4-segment LED display driver
 */
// Pins
#define DisplayData   5
#define DisplayClock  6
#define DisplayLatch  7

// Control variables
unsigned long displayTime = 0;  // Store the time we last changed digit on the display
byte displayDigit = 0;          // 0-3, selects the digit we're outputting at the time.
byte lampState = B00000000;     // Bitmap as follows:
               // ^---------------  Bit 8 = ready
               //  ^--------------  Bit 7 = run
               //   ^-------------  Bit 6 = pause
               //    ^------------  Bit 5 = Finished
               //     ^-----------  Bit 4 = Time Set
               //      ^----------  Bit 3 = TBB Set
               //       ^---------  Bit 2 = Top Panel
               //        ^--------  Bit 1 = Bottom Panel

byte control = B00000011;       // More bitmapping....
             // ^^^^^^------------  Bits 8-3 = future expansion
             //       ^-----------  Bit 2 = top panel enabled
             //        ^----------  Bit 1 = bottom panel enabled

bool paused = false;            // Indicates the system is paused, therefore don't re-init the timer when starting
bool displayOn = true;          // Indicates the display is currently switched ON.
bool colonOn = true;            // If TRUE, the colon is ON.

// Time related variables
signed int setTime = 0;         // The pre-defined duration of the timer
signed int timeLeft = 0;        // Number of seconds remaining of current timer run
unsigned long refTime = 0;      // Reference time (when we last decremented timeLeft)
unsigned long now = 0;          // Time the current loop started

void setup() {

  // Turn the display off until we're ready
  displayOn = false;
  
#ifdef DEBUG
  // Enable serial output for testing purposes
  Serial.begin(38400);
  Serial.println("Encoder Testing.");
#endif

#ifdef RESET_EEPROM
  // Clear the stored EEPROM data
  EEPROM.put(0,0);
  EEPROM.put(2,0);
#endif

  // Set up the encoder input pins
  pinMode(EncoderA,INPUT);
  pinMode(EncoderB,INPUT);
  pinMode(EncoderSW,INPUT);

  // Set up the other switch input pins
  pinMode(StartPauseSW,INPUT);
  pinMode(ResetSW,INPUT);

  // Setup the display pins & reset to zero
  pinMode(DisplayData,OUTPUT);
  pinMode(DisplayLatch,OUTPUT);
  pinMode(DisplayClock,OUTPUT);
  digitalWrite(DisplayData,false);
  digitalWrite(DisplayLatch,false);
  digitalWrite(DisplayClock,false);

  // Setup the panel control pins
  pinMode(TopPanel,OUTPUT);
  pinMode(BottomPanel,OUTPUT);
  digitalWrite(TopPanel,false);
  digitalWrite(BottomPanel,false);

  // And the buzzer pin...
  pinMode(Buzzer,OUTPUT);
  digitalWrite(Buzzer,false);

  // Read the default time from the EEPROM
  EEPROM.get(0,setTime);
#ifdef DEBUG
  Serial.print("Read ");
  Serial.print(setTime);
  Serial.println(" from EEPROM.");
#endif
  
  // If setTime is zero or greater than 60m, set to 2m30s and write that value to the EEPROM.
  if(setTime <= 0 || setTime > 3600) {
#ifdef DEBUG
    Serial.println("Setting EPROM value to 2m30s");
#endif
    setTime = 150;          //2m30s = 150s
    EEPROM.put(0,setTime);
  } else {
    String myTime = fancyTime(setTime);
#ifdef DEBUG
    Serial.print("EPROM sets time to ");
    Serial.println(myTime);
#endif
  }

  // Read the top/bottom panel settings from the EEPROM
  EEPROM.get(2,control);
  if ((control & B00000011) == 0) {
    // Control byte not set, so set top & bottom panels to active & write the control byte
    control = B00000011;
    EEPROM.put(2,control);
  }
 
#ifdef DEBUG
  Serial.println("Preparation complete. You may now putten das fingers into das springenworks und twiddle das knobs");
#endif

  // OK, we're done.
  displayOn = true;

}

void loop() {

  // Record the time this loop started. This will be used for various purposes throughout the loop.
  now = micros();

  // Other vars.
  char change = 0;  // Local variable used when reading the encoder
  
  // Determine if we're changing state
  if(nextState != currentState) {
#ifdef DEBUG
    // Debug: Note state change on serial port.
    Serial.print("State is currently "); 
    Serial.print(currentState); 
    Serial.print(", and is changing to "); 
    Serial.println(nextState);
#endif

    // Determine where we're going, and do any setting up we might want to do before we get there.
    switch(nextState) {
      case RESET:
//                                                                            ---------------RESET
        // Moving into reset state. 
        // Nothing to do here, everything's handled by the actual state code.
#ifdef DEBUG
        Serial.println("State is changing to RESET");
#endif
        currentState = nextState;
        break;
      case TIME_SET:
//                                                                            ---------------TIME SET
        // Moving into time set state. 
        // Wait for the encoder button to be released before we continue though...
        doInputs();
        if (!encoderSW) {
#ifdef DEBUG
          Serial.println("State is changing to TIME-SET");
#endif
          prepEncoder();                  // Prepare the encoder for use.
          setDigits(fancyTime(setTime));  // Make sure the display is showing 
                                          // our set time (it should be... but make sure)
          setLamps(TIME_SET);             // Indicate our new mode
          currentState = nextState;       // All done :)
        } else {
          doDisplay();  // Keep the display refreshed while we wait
        }
        break;
      case TIME_SAVE:
//                                                                            ---------------TIME-SAVE
        // Nothing to do here for the moment
#ifdef DEBUG
        Serial.println("State is changing to TIME-SAVE");
#endif
        currentState = nextState;
        break;
      case TBB_SET:
//                                                                            ---------------TBB-SET
        // Indicate our new state. 
        doInputs();
        // Wait for the encoder switch to be released before we continue though...
        if (!encoderSW) {
#ifdef DEBUG
          Serial.println("State is changing to TBB-SET");
#endif
          prepEncoder();
          setLamps(TBB_SET);
          currentState = nextState; // Move on.
        } else {
          doDisplay();  // Keep the display refreshed
        }
        break;
      case TBB_SAVE:
//                                                                            ---------------TBB-SAVE
        // Wait for encoder switch to be released before we move on
        doInputs();
        if (!encoderSW) {
#ifdef DEBUG
          Serial.println("State is changing to TBB-SAVE");
#endif
          currentState = nextState; // Move on.
        } else {
          doDisplay();  // Keep refreshing the display...
        }
        break;
      case READY:
//                                                                            ---------------READY
        // Nothing to do here for the moment
#ifdef DEBUG
        Serial.println("State is changing to READY");
#endif
        setLamps(READY);
        currentState = nextState;
        break;
      case RUN:
//                                                                            ---------------RUN
        // Wait for the RUN button to be released before we start...
        doInputs();
        if (!startSW) {
#ifdef DEBUG
          Serial.print("State is changing to RUN, setTime is: ");
          Serial.println(setTime);
#endif
          setLamps(RUN);
          switchPanels(true);
          if (!paused) {
            startTimer();  // Initialise the timer
          } else {
            paused = false;
            refTime = now;  // Continue the timer from the nearest whole second
          }
          currentState = nextState;
//        } else {
//          doDisplay();    // Keep the display up to date while we wait
        }
        break;
      case PAUSE:
//                                                                            ---------------PAUSE
        // Wait for the Start/Pause button to be released before we actually pause...
        doInputs();
        if (!startSW) {
  #ifdef DEBUG
          Serial.println("State is changing to PAUSE");
  #endif
          setLamps(PAUSE);
          switchPanels(false);
          paused = true;
          currentState = nextState;
        }
        break;
//                                                                            ---------------FINISH-BUZZER
      case FINISH_BUZZER:
        // OK, the timer finished & dumped us here. Prep the buzzer.
        setLamps(FINISH_BUZZER);
        switchPanels(false);        // Turn off the UV panels
        startBuzzer();
        currentState = nextState;   // Move us into the right state.
        break;
//                                                                            ---------------FINISH
      case FINISH:
        refTime = now;      // Ensure the display stays on for at least 1/2 sec...
        displayOn = true;   // Switch it on
        setLamps(FINISH);
        currentState = nextState;   // Go straight to finish.
        break;
    }
  }

  switch(currentState) {
    case RESET:
#ifdef DEBUG
      Serial.println("State is now RESET");
#endif
      paused = false;                 // We're not paused any more
      timeLeft = setTime;             // Set the time to whatever setTime is.
      lampState = B00000000;          // Lamp State = all off
      setDigits(fancyTime(setTime));  // Setup the display
      colonOn = true;                 // Ensure the colon is displayed
      displayOn = true;               // Ensure the display is enabled
      digitalWrite(Buzzer,LOW);       // Make sure the buzzer is off
      doDisplay();                    // Initialise the display
      displayTime = now;
      nextState = READY;              // Prepare to move to the next state.
      break;
    case READY:
      doDisplay();  // Update the display if required
      doInputs();   // Read the state of all of the buttons, this will determine what (if anything) we do next
      if (encoderSW) {
        // Encoder switch is pressed, so move to TIME_SET mode
        nextState = TIME_SET;
      } else if (startSW) {
        // Start button is pressed, so move to RUN mode
        nextState = RUN;
      }
      break;
    case RUN:
      // Check for buttons...
      doInputs();
      if (startSW) {
        nextState = PAUSE;
      }

      doTimer();    // Timer is running, if it drops to zero, it will set the next-state flag.
      doDisplay();  // Keep the display going.
      
      break;
    case PAUSE:
      // If we're paused, we stop doing the timer until we're ready to RUN again.
      doInputs();
      if (startSW) {
        // Resume
        nextState = RUN;
      } else if (resetSW) {
        // Reset
        nextState = RESET;
      }
      // Keep the display running
      doDisplay();
      break;
    case FINISH_BUZZER:
      // Check for the reset button being pressed; if yes, then reset
      doInputs();
      if (resetSW) {
        nextState = RESET;
      } else {
        // OK, reset isn't pressed... 
        doBuzzer();   // ...so check to see if we've buzzed for long enough,
        doDisplay();  // and refresh the display as required.
      }
      break;
    case FINISH:
      doInputs();
      if (resetSW) {
        nextState = RESET;
      } else {
        // OK, reset isn't pressed...
        doFlashDisplay();   // ...so keep flashing the display until we hit Reset.
        doDisplay();        // and refresh the display as required.
      }
      break;
    case TIME_SET:
      // First read & act on the decoder      
      change = readEncoder();
      if(change != 0 ) {  // If the encoder value has changed, update the display values
        setTime = setTime + change;
        // Make sure the time is within bounds (1 sec to 10 mins)
        if (setTime < 1) setTime = 1;
        if (setTime > 3600) setTime = 3600;

        // Load our new set time into the display
        setDigits(fancyTime(setTime));
      }
      
      doInputs();   // Read the state of all of the buttons, this will determine what (if anything) we do next
      if(encoderSW) {
        // User pressed the encoder, so save the current value & move to the next mode
        nextState=TIME_SAVE;
      } else if(resetSW) {
        // User cancelled changes by pressing the reset button; so re-read the stored time & return to READY
        EEPROM.get(0,setTime);
        setDigits(fancyTime(setTime));
        nextState = READY;
      }

      // Finally refresh the display, if it's time
      doDisplay();

      break;
    case TIME_SAVE:
      // Store the currently set time & move to the TBB state
      EEPROM.put(0,setTime);
      nextState = TBB_SET;
      break;
    case TBB_SET:
      // Read the encoder, change the settings as required.
      change = readEncoder();
      if (change != 0) {
        byte panels = control & B00000011;
        switch(panels) {
          case 0: // Impossible case, treat it as 1
          case 1: // BOTTOM panel active only. +ve change makes TOP panel active, negative makes BOTH panels active
            if (change > 0) panels = B00000010; else panels = B00000011;
            break;
          case 2: // TOP panel active only. +ve change makes BOTH panels active, negative makes BOTTOM panel active
            if (change > 0) panels = B00000011; else panels = B00000001;
            break;
          case 3: // BOTH panels active. +ve change makes BOTTOM panel active, negative makes TOP panel active
            if (change > 0) panels = B00000001; else panels = B00000010;
            break;
        }
        byte mask = control & B11111100;
        control = control & mask;   // Clear panel state
        control = control | panels; // Set panel state
        setLamps(TBB_SET);  // Update the active panel(s) indicators
                            // Note that the lamps will lag by up to DISPLAY_REFRESH_TIME microseconds. 
                            // I reckon we can live with this
      }

      doInputs();
      if (encoderSW) {
        // User's finished messing about, save state
        nextState = TBB_SAVE;
      } else if (resetSW) {
        // User's canceled changes, re-load control byte from EEPROM.
        EEPROM.get(2,control);
        nextState = READY;
      }

      // Finally refresh the display, if it's time
      doDisplay();
      break;
    case TBB_SAVE:
      // Store the new value of the control byte into the EPROM
      EEPROM.put(2,control);             // Store the new value
      nextState = READY;                // Return to ready state.
      break;
  }
}

void doInputs() {
  // Read the switche states into their variables
  encoderSW = digitalRead(EncoderSW);
  startSW = digitalRead(StartPauseSW);
  resetSW = digitalRead(ResetSW);
}

void switchPanels(bool On) {
  // Turn the appropriate panel(s) on or off
  byte mask = control & B00000011;

  if (On) {
    // Turn panels ON if they're configured
    if (mask & 2) digitalWrite(TopPanel,HIGH);
    if (mask & 1) digitalWrite(BottomPanel,HIGH);
  } else {
    // Turn panels OFF regardless
    digitalWrite(TopPanel,LOW);
    digitalWrite(BottomPanel,LOW);
  }

}

