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
