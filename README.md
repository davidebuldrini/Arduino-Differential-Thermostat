Arduino-Differential-Thermostat
===============================

Transform Arduino into an higly configurable differential thermostat. 

You can declare one or more sensors attached to analog pins to retrieve the temperature.
 
Each sensor can be calibrated during any time by putting the current temperature 
 
You must define a reference sensor that is used to calculate the delta t with the other sensors and to activate the thermostat accordingly.

The program provides a web interface accessible by network that shows in real time the values of each sensors and the state of all analog pins and digital pins. All data is updated by ajax without refreshing the page, you can else calibrate each sensors, set the minimum delta t necessary to activate the thermostat and save all settings to a simple text file on the SD card.

The web pages are stored into the SD card and I wrote a simple editor to change the content of the files via web without removing everytime the SD card.
You can else write new files to the SD card.
 
NECESSARY HARDWARE:

- arduino (I tested it with arduino one)
- network shield
- SD card shield


Sketch uses 31,294 bytes (97%) of program storage space. Maximum is 32,256 bytes.
Global variables use 1,255 bytes (61%) of dynamic memory, leaving 793 bytes for local variables. Maximum is 2,048 bytes.

As you can see the sketch eat almost all memory in Arduino one, any suggestion to optimize the code is welcome...


SETUP:

- put the thermostat folder located under libraries into your arduino libraries folder
- put the files under SD-CARD into the arduino sd card root 
- compile and upload to arduino the sketch located under differential-thermostat
