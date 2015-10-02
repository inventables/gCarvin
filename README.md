# gCarvin
Grbl fork for the Carvey machine

## Compiling and Flashing
You can compile and flash a Carvey using the Arduino IDE. You just have to put this repo into the `libraries` folder (by default `~/Documents/Arduino/libraries`) or symlink this directory there. 

Then re-open the Arduino IDE and look under `Examples -> gCarvin -> grblUpload`. That sketch will compile and upload the firmware.

Make sure to have the "Board" type set as "Arduino Mega 2560".
