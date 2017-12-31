# PCBExposureTimer
(c) 2017-2018 Ade Vickers and Solution Engineers Ltd.
Released under the MIT licence, please see the licence file.

## Summary
This project contains complete plans, schematics, PCB layouts and code to create an Arduino Nano controlled double-sided LED UV exposure box.

In addition to the ability to expose PCBs (single and double-sided), the box can be used to expose diazo silk screen patterns.

The initial version is capable of evenly exposing Eurocard sized PCBs (160mm by 100mm); version 2 will increase the maximum size to 320mm by 200mm (4x Eurocards). Larger versions simply increase the number of UV LED panels

Each 160x100 UV light card consists of 150 LEDs, connected in series strings of 6, with a 220R resistor to limit current to a shade under 20mA per LED. Total power consumption is approximately 12 watts per panel (0.5amps @ 24v), including the timer electronics (roughly 100mA @ 5v).

The unit is built into a standard aluminium tool case, not dissimilar to this: http://www.kaizenbonsai.com/bonsai-tool-case-aluminum-tool-case

Note the case for the prototype was purchased from Makro, and hence the manufacturer/product code is not known.

## Release Notes
30 Dec 2017
v0.01
* Initial code complete. 
* One UV panel complete. 
* Timer circuit working (but not finalised) on breadboard.
* Documentation started.
