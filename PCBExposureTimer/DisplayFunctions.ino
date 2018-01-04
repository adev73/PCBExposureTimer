//
// This file contains all of the functions required to run the display unit
//

// doDisplay:       Called every time the main loop runs. Moves to the next digit every few milliseconds.
// refreshDisplay:  Set up the '595 contents & push the data out to them
// fancyTime:       Returns a String variable containing a time, calculated from the number of seconds supplied to it
// setDigits:       Converts a string from fancyTime & sets an array containing the bitmasks to push to the display
// getDigit:        Converts an individual char into 
// setLamps:        Configures the lamp display byte as appropriate

#define DISPLAY_REFRESH_TIME  3500

// Digit bitmaps. 1=segment ON, 0 = OFF.
// Segments:     ABCDEFG: (we're ignoring the colon, that's handled separately)
#define digit__ B00000000 //All off (blank)
#define digit_0 B11111100 //ABCDEF
#define digit_1 B01100000 // BC
#define digit_2 B11011010 //AB DE G
#define digit_3 B11110010 //ABCD  G
#define digit_4 B01100110 // BC  FG
#define digit_5 B10110110 //A CD FG
#define digit_6 B10111110 //A CDEFG
#define digit_7 B11100000 //ABC    
#define digit_8 B11111110 //ABCDEFG
#define digit_9 B11110110 //ABCD FG
#define digit_E B10011110 //A  DEFG

// Storage for the four digits we wish to display right now
byte digit[4];

void doDisplay() {
  // If it's time, move to the next display digit & display it.
  // This runs every few milliseconds
  if (now - displayTime > DISPLAY_REFRESH_TIME) {
    displayTime = micros();
    displayDigit += 1;
    if (displayDigit > 3) displayDigit = 0;
    refreshDisplay();
    displayTime = now;
  }
}

void refreshDisplay() {
  // Called every few milliseconds. Push 3 bytes out to the '595 units, as follows:
  // Byte 1: State LEDs.
  // Byte 2: Segments to display for the current time digit
  // Byte 3: Display digit (Bits 0-4 control which anode is ON, and therefore which digit is active)

  // Note that as the lamp states are common anode, as is the 7-seg
  // display, we need to invert lampState before we display it.
  // digit[n] is already inverted in the bitmask that defines it.
  // displayDigit is converted into a simple bitmask, only 4 bits are used.

  // Prepare the '595s for icoming data
  digitalWrite(DisplayLatch,LOW);

  // We need to invert lampState
  shiftOut(DisplayData,DisplayClock,LSBFIRST,lampState ^ 255);      // Send it backwards as well, fecking wiring.

  // If the colon is ON, use the byte as we find it (colon is ON in all digit bitmaps)
  byte disp = digit[displayDigit];
  if (!colonOn) {
    // If, on the other hand, the colon is OFF... we need to adjust the output
    disp = disp & B11111110;  // That ought to do it.
  } else {
    disp = disp | B00000001;  // That ought to do it.
  }
  shiftOut(DisplayData,DisplayClock,LSBFIRST,disp);  // Whatever the current display digit is.

  // Enable the appropriate digit on the 7-seg display
  if (displayOn) {
    switch(displayDigit){
      case 0:
        shiftOut(DisplayData,DisplayClock,MSBFIRST,1 ^ B11111111);
        break;
      case 1:
        shiftOut(DisplayData,DisplayClock,MSBFIRST,2 ^ B11111111);
        break;
      case 2:
        shiftOut(DisplayData,DisplayClock,MSBFIRST,4 ^ B11111111);
        break;
      case 3:
        shiftOut(DisplayData,DisplayClock,MSBFIRST,8 ^ B11111111);
        break;
    }
  } else {
    // Display is off, show nothing
    shiftOut(DisplayData,DisplayClock,MSBFIRST,255);
  }

  // Tell the '595s we're done, this will cause them to output the new patterns.
  digitalWrite(DisplayLatch,HIGH);
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
  
  return output;
}

void setDigits(String timeString) {
  // Set the four display digits according to the time value in the string
  if (timeString.charAt(0) == '0') digit[0] = getDigit(' '); else digit[0] = getDigit(timeString.charAt(0));
  digit[1] = getDigit(timeString.charAt(1));
  digit[2] = getDigit(timeString.charAt(3));
  digit[3] = getDigit(timeString.charAt(4));

#ifdef DEBUG
  Serial.println("Digits have been set.");
#endif

}

byte getDigit(char thisDigit) {
  // Return the digit bitmask corresponding to the char we've been supplied
  switch(thisDigit) {
    case ' ': return digit__; break;
    case '0': return digit_0; break;
    case '1': return digit_1; break;
    case '2': return digit_2; break;
    case '3': return digit_3; break;
    case '4': return digit_4; break;
    case '5': return digit_5; break;
    case '6': return digit_6; break;
    case '7': return digit_7; break;
    case '8': return digit_8; break;
    case '9': return digit_9; break;
    default:  return digit_E; break;    // Unrecognised character set to "E" (error)
  }
}

void setLamps(STATE systemState) {
  // Configure lamps to display the correct state
  switch(systemState) {
    case RESET:
    case READY:         lampState = B10000000; break;
    case RUN:           lampState = B01000000; break;
    case PAUSE:         lampState = B00100000; break;
    case FINISH_BUZZER: 
    case FINISH:        lampState = B00010000; break;
    case TIME_SAVE:
    case TIME_SET:      lampState = B00001000; break;
    case TBB_SAVE:
    case TBB_SET:       lampState = B00000100; break;
    default:            lampState = B00000000; break; // If no lights are on, we're in an unknown state
  }

  // Show the top/bottom panel active status too.
  byte controlMask = control & B00000011;
  lampState = lampState | controlMask;
}

void doFlashDisplay() {
    // This is EXACTLY like doBuzzer, only without the buzzer...
    // Every 1/2 second, switch display off & on
  if (now - refTime > 500000) {
    // 1/2 million microseconds = 1/2 second... time to do something
    if (displayOn) {
      // Switch display OFF
      displayOn = false;
      refTime = now;                    // Begin the wait for another second      
    } else {
      //Half a second of display off has passed... so switch the display back on.
      displayOn = true;
      refTime = now;
    }
  }
}

