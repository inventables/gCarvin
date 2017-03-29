# gCarvin 2
Grbl 1.1e fork for the Carvey machine

## Compiling and Flashing
You can compile and flash a Carvey using the Arduino IDE. You just have to put this repo into the `libraries` folder (by default `~/Documents/Arduino/libraries`) or symlink this directory there. 

Then re-open the Arduino IDE and look under `Examples -> gCarvin -> grblUpload`. That sketch will compile and upload the firmware.

Make sure to have the "Board" type set as "Arduino Mega 2560".

## Carvey specific features of grbl
The gCarvin firmware is a specialization of grbl intended for use on the Carvey 3D carving machine from Inventables. gCarvin supports the following features:
* grbl 1.1e base features
* Carvey Gen 2 Hardware
* Variable Spindle Speed
* Door Open Interlock
* Button/Status LED
* Spindle Active LED
