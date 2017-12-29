//
// This file contains all of the functions required to run the actual timer
// It also contains the functions used to control the buzzer, as these also
// involve timer-like functions.
//

#define BUZZER_TIME 10    // Duration the buzzer will buzz for.

void startTimer() {
  // Initialise the timer
  timeLeft = setTime;
  setDigits(fancyTime(timeLeft));  // Make sure the remaining time is displayed properly
  refTime = now;
  colonOn = true;
}

void doTimer() {
  // Is it time to knock a second off the time yet?
  if (now - refTime >= 500000) {
    // Half a second has elapsed. Is the colon currently ON?
    if (colonOn) {
      // Yes; so only switch the colon off
      colonOn = false;
      refTime = now;                    // Begin the wait for another half second
    } else {
      // Yes, yes it is.
      colonOn = true;                   // Switch the colon back on again
      timeLeft--;                       // Remove 1 second from timeLeft    
      refTime = now;                    // Begin the wait for another half second
  
      // Has the timer finished?
      if (timeLeft <= 0) {
        timeLeft = 0;
        nextState = FINISH_BUZZER;      // Beeper on.
      }
  
      // Make sure the remaining time is displayed properly
      setDigits(fancyTime(timeLeft));   
    }
  }
}

void startBuzzer() {
  // Initialise the buzzer
  timeLeft = BUZZER_TIME;     // Buzz for 5 seconds
  refTime = now;              // Starting now
  displayOn = true;           // Ensure the display is enabled to begin with
  digitalWrite(Buzzer,HIGH);  // Start buzzing.
}

void doBuzzer() {
  // Every 1/2 second, switch display & buzzer off...
  if (now - refTime > 500000) {
    // 1/2 million microseconds = 1/2 second... time to do something
    if (displayOn) {
      // Switch display & buzzer OFF
      displayOn = false;
      digitalWrite(Buzzer,LOW);
      refTime = now;                    // Begin the wait for another second      
    } else {
      //Half a second of silence/display off has passed... so switch the display back on.     
      displayOn = true;
      timeLeft--;
      if(timeLeft > 0) {
        // Still some time left on the clock, so buzz again
        digitalWrite(Buzzer,HIGH);      
        refTime = now;                    // Begin the wait for another second
      } else {
        // All done.
        timeLeft=0;
        displayOn = true;             // Just make sure it's on.
        digitalWrite(Buzzer,LOW);     // Make sure the buzzer's off, 'cos that would be REALLY irritating...
        nextState = FINISH;
      }
    }
  }
}

