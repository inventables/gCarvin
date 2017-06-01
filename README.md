# gCarvin
Grbl 1.1e fork for the Carvey machine

## Compiling and Flashing
You can compile and flash a Carvey using the Arduino IDE. You just have to put this repo into the `libraries` folder (by default `~/Documents/Arduino/libraries`) or symlink this directory there. It is recommended that you remove previous versions of the gCarvin library before moving it into the `libraries` folder to avoid confusion.

Then re-open the Arduino IDE and look under `Examples -> gCarvin -> grblUpload`. That sketch will compile and upload the firmware.

Make sure to have the "Board" type set as "Arduino Mega 2560".

## Carvey specific features of grbl
The gCarvin firmware is a specialization of grbl intended for use on the Carvey 3D carving machine from Inventables. gCarvin supports the following features:
* grbl 1.1e base features
* Carvey Gen 2 Hardware Support
* Variable Spindle Speed
* Spindle Over-Current Detection
* Door Open Interlock
* Button/Status LED
* Spindle Active LED
