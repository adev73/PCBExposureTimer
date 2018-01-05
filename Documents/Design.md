# Arduino PCB Exposure Timer
## State Machine States
* READY
* RUN
* PAUSE
* FINISHED-BUZZER
* FINISHED
* RESET
* TIME-SET
* TIME-SAVE
* TPB-SET
* TPB-SAVE

## Valid State Changes
* RESET
* * READY

* READY
* * RUN
* * TIME-SET

* RUN
* * PAUSE
* * FINISHED-BUZZER

* PAUSE
* * RUN
* * RESET

* TIME-SET
* * TIME-SAVE
* * RESET

* TIME-SAVE
* * TBB-SET

* TBB-SET
* * TBB-SAVE
* * RESET

* TBB-SAVE
* * RESET

* FINISHED-BUZZER
* * FINISHED
* * RESET

* FINISHED
* * RESET

## State Functions

### State: RESET
* Clear all variables & return to read values.
* Go to state -> READY

### State: READY
* Display configured time on 4-dig LED
* Extinguish all lamps, display READY lamp
* Start Button -> RUN
* Encoder Button -> TIME-SET

### State: RUN
* Decrease time by 1s every 1000ms
* Display remaining time on 4-dig LED
* Start Button -> PAUSE
* Time = 0 -> FINISHED-BUZZER

### State: PAUSE
* Display remaining time on 4-dig LED
* Extinguish all lamps, light PAUSE lamp
* Start Button -> RUN
* Reset Button -> RESET

### State: FINISHED-BUZZER
* Extinguish all lamps, light FINISHED lamp
* Display 00:00 on 4-digit LED, flash every 0.5s
* Buzz for 1/2sec every second
* After ~5 seconds -> FINISHED

### State: FINISHED
* Display 00:00 on 4-dig LED, flash every 0.5s
* Reset Button -> READY
* Encoder Button -> TIME-SET

### State: TIME-SET
* Extinguish all lamps, light TIME SET lamp
* Adjust time on encoder input, min 1 second, max 1 hour
* Reset Button -> RESET
* Encoder Button -> TIME-SAVE

### State: TIME-SAVE
* Store new time in EEPROM
* Goto -> TBB-SET

### State: TBB-SET
* Extinguish all lights, light TBB SET lamp
* Cycle between Top, Both, Bottom, lighting appropriate lamps, using encoder
* Reset Button -> RESET
* Encoder Button -> TBB-SAVE

### State: TBB-SAVE
* Store top/bottom panel state in EEPROM
* Goto -> RESET

# Hardware
## Front panel controls

* Mode button (on the encoder)
* Set Time Up/Down, Set panel Top/Bottom/Both (encoder)
* Start/Pause button
* Reset button
* Time display (4x7seg unit)

* Mode indicator LEDs:
* * Ready
* * Run
* * Pause
* * Set Time
* * Set Panels
* * Top/Bottom/Both

## BOM (timer controller)
* 1x Arduino Nano or Mini Pro (to taste)
* 1x EC12D1524406 encoder with integral push button
* 1x ATA3491BW 4-digit 7-segment common anode display
* 2x SPST momentary push-buttons
* 1x Piezo buzzer
* 3x SN74HC595 serial/parallel converters
* 2x FQP20N06 MOSFETs (topside, bottomside)
* 9x KSP2222A NPNs (segment drive, buzzer drive)
* 4x KSP2906A PNPs (digit drive)
* 4x 10k resistors (PNP pullups)
* 13x 1k resistors (base current limiters)
* 8x 100R resistors (segment current limit
* 8x 220R resistors (LED current limiters) [subject to LED requirements]
* 2x 1k resistors (MOSFET gate protection)
* 3x 4k7 resistors (encoder current limiters) <-- maybe change to 1.4k, or...
* 2x 1k4 resistors (switch current limiters)  <-- ...change these to 4k7. Experiment...
* 24v PSU
* 24v->5v buck converter
* UK mains plug
* 250vac SPST rocker switch
* UK mains lead
* Suitable box
* Various mounting hardware

## BOM (LED panel)
* 150x UV LEDs
* 25x 220R current limiting resistors
* 1x 2-pin connector (male) (for later expansion)
* 1x 2-pin connector (female)

 