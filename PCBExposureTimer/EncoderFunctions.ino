//
// This file contains all of the functions required to run the rotary encoder
//

// readEncoder: Reads the new(?) position of the rotary encoder, and returns + or - 1
//              depending on whether it moved clockwise or anticlockwise.
//              When we're using the encoder, this routine is called as often as possible.

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

byte readEncoder() {
  //
  // Reads a rotary encoder & determines 1) if it's moved, and 2) if so, in which direction.
  // We could add acceleration too... but I can't be bothered for this application.
  //

  char pos = 0; // Note the new position (-1 = CCW one notch, 0 = no movement, +1 = CW one notch.
  
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

#ifdef DEBUG
    // Encoder testing... dump to serial port
    if (pos != 0) {
      Serial.print("New encoder value: ");
      Serial.println(pos);
    }
#endif
  }

  return pos;
}

