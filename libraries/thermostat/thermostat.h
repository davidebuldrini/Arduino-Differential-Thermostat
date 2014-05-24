#ifndef thermostat_h
#define thermostat_h

#include <Arduino.h>

// how many sensors the thermostat object can handle. it's used to dimensionate the sensors array
#if !defined(MAX_SENSORS_COUNT)
	#define MAX_SENSORS_COUNT 5
#endif

/*** TempSensor ****
	It reads raw values from analog pin and convert them to temperature in degrees.
	It's possible to calibrate it with another temperature value taken with a thermometer for example
*/
class TempSensor 
{
  private:

  public:
  
    TempSensor(char*, byte);
   
    char *name; // sensor name
    byte pin; // analog pin input number
    int Vref; // analog pin sampled voltage at calibration time
    byte linkedOutputPin; // store a digital output pin number
	
    #if defined(TEMPERATURE_BYTE)
		byte value(); // returns the current temperature of rilevated by the sensor
		byte Tref; // temperature at calibration time
		void calibrate(byte); // calibrate the sensor with the given temperature
	#else 
		float value(); // returns the current temperature of rilevated by the sensor	
		float Tref; // temperature at calibration time
		void calibrate(float); // calibrate the sensor with the given temperature 		
	#endif
	
    int raw(); // returns the raw value directly read from analog pin
    
    void linkOutput(byte pin, byte defaultState); 
};




/*** Thermostat ****

It basically compares the temperature of a reference sensor (from example the one inside the boiler) 
   with the temperature of one or more sensors (for example from solar panel, wood stove, ecc.) 
   
   When one sensor overcome the specified delta t the callback function onOvercome will fire providing the sensor object as argument,
   while the callback onNormal will fire when the sensor temperature will fall again within delta t values.
*/
class Thermostat
{
  private:
  
	unsigned long _timer[MAX_SENSORS_COUNT]; // it will contains millis() values 
	byte _state; // thermostat on / off 	useful values: 0 , 1
      
  public:
    Thermostat();
    
    TempSensor *sensors[MAX_SENSORS_COUNT]; // array of pointers to sensors objects
    boolean overcome[MAX_SENSORS_COUNT]; // indicate wheter the sensor temperatures are currently over the delta t
    byte sensorsCount; // total number of sensors
    
    boolean _allNormal; // whether all sensor are under delta t
    
    TempSensor *reference; // pointer to the sensor that provides the reference temperature to be compared with other sensors
    byte triggerDeltaT; // temperature difference between reference sensor and other sensors necessary to trigger the thermostat    
    
    void (*onOvercome)(TempSensor*, Thermostat*); // pointer to callback that fires when the sensor overcome delta t
    void (*onNormal)(TempSensor*, Thermostat*); // pointer to callback that fires when the sensor temperature return to values within delta t
    void (*onAllNormal)(Thermostat*); // pointer to callback that fires when all sensors returns to values within delta t
    
    unsigned int switchDelay; // how long need wait after the termostat can switch again (this is to avoid relays intermittance
    byte addSensor(TempSensor*);
    byte addReference(TempSensor*); // reference sensor: the thermostat will act upon calculating the delta t between this sensor and the others
    boolean isReference(byte);
    boolean checkDelta(); // check the sensors temperature against the reference temperature and call appropriate callbacks 
    void setState(byte state); // activate or deactivate the thermostat
    
    #if defined(TEMPERATURE_BYTE)
		byte deltaT(byte); // return the temperature difference between the sensor argument and the reference sensor
	#else
		float deltaT(byte); // return the temperature difference between the sensor argument and the reference sensor
	#endif
};

#endif
