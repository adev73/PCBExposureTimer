//
// This file contains all of the functions required to run the rotary encoder
//

// readEncoder: Reads the new(?) position of the rotary encoder, and returns + or - 1
//              depending on whether it moved clockwise or anticlockwise.
//              When we're using the encoder, this routine is called as often as possible.

#define FAST        25000   // Number of uSec between encoder pulses to consider accelerating
#define FASTER      10000   // As above for super-acceleration
#define BIGSTEP        10   // Number of steps to jump when accelerated
#define BIGGERSTEP     60   // As above for super-turbo mode

void prepEncoder() {
  //
  // Prepare the encoder for use
  //
  encoderA = digitalRead(EncoderA);
  encoderB = digitalRead(EncoderB);
  _encoderA = encoderA;
  
  if(encoderA != encoderB) {
    _encoderIgnore = true;
  }
}

byte readEncoder(bool withAcceleration) {
  //
  // Reads a rotary encoder & determines 1) if it's moved, and 2) if so, in which direction.
  // We could add acceleration too... but I can't be bothered for this application.
  //
  static unsigned long lastChange = micros(); // Use this to determine the speed the encoder is going
  
  char pos = 0; // Note the new position (-moveBy = CCW one notch, 0 = no movement, +moveBy = CW one notch.
  byte moveBy = 1; // Variable to handle acceleration
  
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
  //encoderSW = digitalRead(EncoderSW); // Ignore the switch here, we will handle that elsewhere.

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
      // Change! How fast? (if acceleration is enabled)
      if (withAcceleration) {
        if (now-lastChange < FASTER) { 
          // Super-Quickly!
          moveBy = BIGGERSTEP;
        } else if (now - lastChange < FAST) { 
          // Plain old quickly
          moveBy = BIGSTEP;
        }
      }
      //...amd in which direction?
      if (encoderA == encoderB) {
        // Clockwise, add
        pos = pos + moveBy;
      } else {
        // Anticlockwise, subtract
        pos = pos - moveBy;
      }
      _encoderA = encoderA;
      lastChange = micros();
    }
  
#ifdef DEBUG
    // Encoder testing... dump to serial port
    if (pos != 0) {
      Serial.print("New encoder value: ");
      Serial.println(pos,DEC);
    }
#endif
  }

  return pos;
}

